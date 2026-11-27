#include "infrared.h"
#include <IRutils.h>
#include "infrared_native_learn.h"

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
    if (results.overflow || results.rawlen < 3) {
        Serial.printf("[Infrared RX] rejected: overflow=%u rawlen=%u\n",
            unsigned(results.overflow),unsigned(results.rawlen));
        _receiver->resume(); return;
    }
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
    static_assert(kStateSizeMax<=meow_ir::MaxNativeStateBytes,"Increase native state limit for this library");
    const bool stateProtocol=hasACState(results.decode_type);
    const size_t stateBytes=stateProtocol?(size_t(results.bits)+7)/8:0;
    const bool nativeFound = _decodeMode != 2 && results.decode_type != UNKNOWN &&
        results.decode_type != UNUSED && !results.repeat && stateBytes<=kStateSizeMax &&
        meow_ir::learnNative(received,typeToString(results.decode_type).c_str(),results.bits,
            stateProtocol?0:results.value,stateProtocol?results.state:nullptr,stateBytes);
    if (_decodeMode == 1 && !found && !nativeFound) { _receiver->resume(); return; }
    if (_decodeMode != 2 && found && !nativeFound) {
        received.raw = false;
        received.protocol = decoded.protocol;
        received.address = decoded.address;
        received.command = decoded.command;
        received.timings.clear();
    }
    if (!meow_ir::validSignal(received)) { _receiver->resume(); return; }
    _learned = std::move(received);
    _stopRx();
    Serial.printf("[Infrared RX] native=%s bits=%u rawlen=%u mode=%u\n",
        typeToString(results.decode_type).c_str(),unsigned(results.bits),
        unsigned(results.rawlen),unsigned(_decodeMode));
    // Preserve the actual capture in USB logs for diagnosing multipart AC
    // frames, even before the user chooses a name or writes a remote file.
    Serial.print(meow_ir::serialize(_learned).c_str());
    _push(Scene::Result);
    _startSavedMatchSearch();
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
        _message("Send Failed", "Unsupported native signal"); return;
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
    // Keep toggle continuity across closing/reopening Infrared. Only this
    // worker uses these states; repeats retain the current key-press bit.
    static meow_ir::NativeToggleState rc5Toggle,rc6Toggle,rc6LongToggle;
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (_txShutdown.load()) break;
        const auto& signal = _txSignal;
        _sender->resetFrameStatus();
        Serial.printf("[Infrared TX] name=%s raw=%u timings=%u repeat=%u\n",
            signal.name.c_str(),unsigned(signal.raw),unsigned(signal.timings.size()),unsigned(_txRepeat));
        if(!signal.raw) Serial.printf("[Infrared TX] protocol=%s bits=%u value=0x%llX state_bytes=%u\n",
            signal.protocol==InfraredProtocolUnknown?signal.legacyProtocol.c_str():infrared_get_protocol_name(signal.protocol),
            unsigned(signal.legacyBits),static_cast<unsigned long long>(signal.legacyValue),unsigned(signal.legacyState.size()));
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
            // Native integer codes and multi-byte AC states retain their
            // dedicated sender, including protocol-specific framing/repeats.
            const auto protocol = strToDecodeType(signal.legacyProtocol.c_str());
            if (!signal.legacyState.empty()) {
                uint8_t state[kStateSizeMax]{};
                ok=protocol!=UNKNOWN && hasACState(protocol) && signal.legacyState.size()<=sizeof(state);
                if(ok) {
                    std::copy(signal.legacyState.begin(),signal.legacyState.end(),state);
                    ok=_sender->send(protocol,state,uint16_t(signal.legacyState.size()));
                }
            } else {
                uint64_t value=signal.legacyValue;
                if(protocol==RC5 || protocol==RC5X)
                    value=rc5Toggle.apply(value,_sender->toggleRC5(0),_txRepeat);
                else if(protocol==RC6) {
                    auto& toggle=signal.legacyBits==kRC6_36Bits?rc6LongToggle:rc6Toggle;
                    value=toggle.apply(value,_sender->toggleRC6(0,signal.legacyBits),_txRepeat);
                }
                ok=protocol!=UNKNOWN && !hasACState(protocol) &&
                    _sender->send(protocol,value,signal.legacyBits);
            }
        }
        _sender->space(0);
        ok=ok && _sender->carrierReady();
        Serial.printf("[Infrared TX] hardware carrier=%.1f Hz ready=%u\n",
            _sender->carrierFrequency(),unsigned(_sender->carrierReady()));
        Serial.printf("[Infrared TX] complete: ok=%u cancelled=%u\n",
            unsigned(ok),unsigned(_txCancel.load()));
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
