#include "infrared.h"
#include <IRutils.h>

namespace MOONCAKE::APPS {
void App09::_updateLed() {
    const uint32_t now = millis();
    // Keep the same continuous strobe while learning or scanning, including TX gaps.
    const bool scanning = _scene == Scene::Scan && _scan.running && !_scan.paused;
    const bool active = _rxEnabled || scanning || (_txBusy.load() && !_txCancel.load()) || (_ledUntil && int32_t(_ledUntil - now) > 0);
    if (active && !_ledActive) _ledPhaseStart = now;
    const bool on = active && ((now - _ledPhaseStart) / 60) % 2 == 0;
    if (on != _ledOn) {
        if (on) _device->led.setColor(WS2812B_Class::WHITE);
        else _device->led.off();
    }
    _ledActive = active; _ledOn = on;
}
void App09::_startRx() {
    if (_txBusy || !_receiver) return;
    infrared_reset_decoder(_decoder);
    _receiver->enableIRIn();
    _rxEnabled = true;
}

void App09::_stopRx() {
    if (_receiver && _rxEnabled) _receiver->disableIRIn();
    _rxEnabled = false;
}

void App09::_receive() {
    if (!_rxEnabled) return;
    decode_results results;
    if (!_receiver->decode(&results)) return;
    if (results.overflow || results.rawlen < 3) { _receiver->resume(); return; }
    meow_ir::Signal received;
    received.name = "Signal";
    received.raw = true;
    received.timings.reserve(results.rawlen - 1);
    infrared_reset_decoder(_decoder);
    InfraredMessage decoded{};
    bool found = false;
    infrared_decode(_decoder, false, INFRARED_RAW_RX_TIMING_DELAY_US);
    for (size_t i = 1; i < results.rawlen; ++i) {
        const uint32_t duration = uint32_t(results.rawbuf[i]) * kRawTick;
        received.timings.push_back(duration);
        const auto* message = infrared_decode(_decoder, i % 2 != 0, duration);
        if (message && !message->repeat && !found) { decoded = *message; found = true; }
    }
    const auto* tail = infrared_check_decoder_ready(_decoder);
    if (!found && tail && !tail->repeat) { decoded = *tail; found = true; }
    if (_decodeMode == 1 && !found) { _receiver->resume(); return; }
    if (_decodeMode != 2 && found) {
        received.raw = false;
        received.protocol = decoded.protocol;
        received.address = decoded.address;
        received.command = decoded.command;
        received.timings.clear();
    }
    if (!meow_ir::validSignal(received)) { _receiver->resume(); return; }
    _learned = std::move(received);
    _stopRx();
    _push(Scene::Result);
    _ledUntil = millis() + 200;
}

bool App09::_startTx(const meow_ir::Signal& signal, bool repeat) {
    if (!_txTask || _txBusy || !meow_ir::validSignal(signal)) return false;
    _stopRx();
    _txSignal = signal;
    _txRepeat = repeat;
    _txCancel.store(false);
    _txSucceeded.store(true);
    _txBusy.store(true);
    _ledUntil = millis() + 200; // Make even a short frame visibly acknowledge input.
    xTaskNotifyGive(_txTask);
    return true;
}

void App09::_stopTx() {
    _txCancel.store(true);
    _ledUntil = 0;
    _holdLatched = false;
    _heldSource = nullptr;
}

void App09::_runHeld(const meow_ir::Signal& signal) {
    if (!_txBusy && !_txSucceeded.exchange(true)) {
        _message("Send Failed", "Unsupported legacy signal"); return;
    }
    if (!_txTask && _input.a) { _message("TX Unavailable", "Cannot start infrared worker"); return; }
    if (_input.b) { _stopTx(); return; }
    // Releasing A stops repetition after the current frame, so a short tap
    // still emits a complete usable signal. Back/scene exit cancels promptly.
    if (!_input.held) { _holdLatched = false; _heldSource = nullptr; return; }
    if (_heldSource != &signal) { _stopTx(); _heldSource = &signal; }
    if (_txBusy || (_holdLatched && millis() - _lastTx < 50)) return;
    if (_startTx(signal, _holdLatched)) {
        _holdLatched = true;
        _lastTx = millis();
    }
}

void App09::_txEntry(void* context) { static_cast<App09*>(context)->_txWorker(); }

void App09::_txWorker() {
    auto* encoder = infrared_alloc_encoder();
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (_txShutdown.load()) break;
        const auto& signal = _txSignal;
        bool ok = true;
        if (signal.raw) {
            _sender->enableIROut(signal.frequency, uint8_t(signal.duty * 100));
            for (size_t i = 0; i < signal.timings.size() && !_txCancel.load(); ++i) {
                uint32_t duration = signal.timings[i];
                if (i % 2 == 0) {
                    // IRsend::mark takes uint16_t. Split long marks, preserving polarity.
                    while (duration && !_txCancel.load()) {
                        uint16_t part = uint16_t(std::min(uint32_t(60000), duration));
                        _sender->mark(part); duration -= part;
                    }
                } else {
                    while (duration && !_txCancel.load()) {
                        uint32_t part = std::min(uint32_t(20000), duration);
                        _sender->space(part); duration -= part;
                    }
                }
            }
        } else if (signal.protocol != InfraredProtocolUnknown) {
            InfraredMessage message{signal.protocol, signal.address, signal.command, false};
            if (!_txRepeat) infrared_reset_encoder(encoder, &message);
            _sender->enableIROut(infrared_get_protocol_frequency(signal.protocol),
                uint8_t(infrared_get_protocol_duty_cycle(signal.protocol) * 100));
            size_t frames = 0;
            const size_t minimum = _txRepeat ? 1 : infrared_get_protocol_min_repeat_count(signal.protocol);
            for (size_t pulses = 0; pulses < 50000 && !_txCancel.load(); ++pulses) {
                uint32_t duration = 0; bool level = false;
                auto status = infrared_encode(encoder, &duration, &level);
                if (status == InfraredStatusError) { ok = false; break; }
                // A fresh transmission starts immediately. The encoder's
                // initial silence is only needed between subsequent frames.
                if (pulses == 0 && !_txRepeat && !level) continue;
                if (level) _sender->mark(uint16_t(duration));
                else {
                    while (duration && !_txCancel.load()) {
                        uint32_t part = std::min(uint32_t(20000), duration);
                        _sender->space(part); duration -= part;
                    }
                }
                if (status == InfraredStatusDone && ++frames >= minimum) break;
            }
            if (frames < minimum && !_txCancel.load()) ok = false;
        } else {
            // Old MeowKit files with explicit value/bits retain their native sender.
            const auto protocol = strToDecodeType(signal.legacyProtocol.c_str());
            ok = protocol != UNKNOWN && _sender->send(protocol, signal.legacyValue, signal.legacyBits);
        }
        _sender->space(0);
        _txSucceeded.store(ok);
        _txBusy.store(false);
    }
    infrared_free_encoder(encoder);
    _sender->space(0);
    _txBusy.store(false);
    _txStopped.store(true);
    vTaskDelete(nullptr);
}
} // namespace MOONCAKE::APPS
