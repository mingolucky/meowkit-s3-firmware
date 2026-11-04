#include "nfc_reader.h"
#include "nfc_hardware.h"
#include "nfc_memory.h"
#include "../../bsp/i2c/I2C_Class.hpp"
#include <rfal_rfst25r3916.h>
#include <ndef_class.h>
#include <algorithm>
#include <cstring>
#include <new>

namespace meow_nfc {
struct Reader::Impl {
    explicit Impl(Reader* owner) : bus(Reader::transfer, owner), rf(&bus, HAL_NFC_IRQ_PIN), nfc(&rf), ndef(&nfc) {}
    MeowNfcBus bus;
    RfalRfST25R3916Class rf;
    RfalNfcClass nfc;
    NdefClass ndef;
    Tag tag;
    uint8_t chipId = 0;
    uint16_t supply = 0;
    uint32_t cycles = 0, activeGeneration = 0;
    bool initialized = false, cleanup = true, touched = false, fieldOn = false, externalField = false;
};
Reader::Reader() = default;
Reader::~Reader() { end(); }
bool Reader::transfer(void* context, uint8_t address, const uint8_t* tx,
                      size_t txLength, uint8_t* rx, size_t rxLength) {
    auto* self = static_cast<Reader*>(context);
    if (!self->_impl) return false;
    auto& impl = *self->_impl;
    if (!impl.cleanup && (self->_stop.load() || self->_generation.load() != impl.activeGeneration)) return false;
    return In_I2C.transfer(address, tx, txLength, rx, rxLength);
}
bool Reader::begin() {
    end();
    _mutex = xSemaphoreCreateMutex();
    _snapshot.reset(new (std::nothrow) ReaderSnapshot);
    _impl.reset(new (std::nothrow) Impl(this));
    if (!_mutex || !_snapshot || !_impl) { end(); return false; }
    _mode.store(0); _filter.store(0); _generation.store(0);
    _stop.store(false); _running.store(true);
    if (xTaskCreate(entry, "MeowNFC", 8192, this, 1, &_task) != pdPASS) {
        _task = nullptr; _running.store(false); end(); return false;
    }
    return true;
}
void Reader::end() {
    _stop.store(true);
    while (_running.load()) delay(1);
    _task = nullptr;
    _impl.reset(); _snapshot.reset();
    if (_mutex) { vSemaphoreDelete(_mutex); _mutex = nullptr; }
}
void Reader::request(ReaderMode mode, uint8_t filter) {
    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    _filter.store(filter <= 4 ? filter : 0); _mode.store(uint8_t(mode));
    _generation.fetch_add(1);
    if (_snapshot) {
        _snapshot->state = mode == ReaderMode::Idle ? ReaderState::Idle : ReaderState::Initializing;
        _snapshot->errorCode = 0;
        std::snprintf(_snapshot->message, sizeof(_snapshot->message), "%s", "Waiting for NFC worker");
    }
    if (_mutex) xSemaphoreGive(_mutex);
}
bool Reader::snapshot(ReaderSnapshot& out) const {
    if (!_mutex || !_snapshot || xSemaphoreTake(_mutex, 0) != pdTRUE) return false;
    out = *_snapshot; xSemaphoreGive(_mutex); return true;
}
void Reader::publish(ReaderState state, const char* text, uint16_t code, const Tag* tag) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_generation.load() != _impl->activeGeneration) { xSemaphoreGive(_mutex); return; }
    auto& s = *_snapshot;
    s.state = state; s.errorCode = code;
    std::snprintf(s.message, sizeof(s.message), "%s", text);
    s.chipId = _impl->chipId; s.supplyMv = _impl->supply;
    s.cycles = _impl->cycles; s.ioErrors = _impl->bus.errors();
    s.fieldOn = _impl->fieldOn;
    s.externalField = _impl->externalField;
    if (tag) s.tag = *tag;
    xSemaphoreGive(_mutex);
}
void Reader::entry(void* context) {
    auto* reader = static_cast<Reader*>(context); reader->worker();
    reader->_running.store(false); vTaskDelete(nullptr);
}
bool Reader::capture(uint32_t gen) {
    auto& i = *_impl;
    rfalNfcDevice* dev = nullptr;
    if (i.nfc.rfalNfcGetActiveDevice(&dev) != ERR_NONE || !dev || !dev->nfcid) return false;
    auto& t = i.tag; t = {};
    switch (dev->type) {
        case RFAL_NFC_LISTEN_TYPE_NFCA: t.tech = Tech::A;
            t.atqa[0] = dev->dev.nfca.sensRes.anticollisionInfo;
            t.atqa[1] = dev->dev.nfca.sensRes.platformInfo;
            t.sak = dev->dev.nfca.selRes.sak; break;
        case RFAL_NFC_LISTEN_TYPE_NFCB: t.tech = Tech::B;
            t.auxSize = std::min(sizeof(t.aux), sizeof(dev->dev.nfcb.sensbRes));
            memcpy(t.aux, &dev->dev.nfcb.sensbRes, t.auxSize); break;
        case RFAL_NFC_LISTEN_TYPE_NFCF: t.tech = Tech::F;
            t.auxSize = std::min(sizeof(t.aux), std::min(sizeof(dev->dev.nfcf.sensfRes), size_t(dev->dev.nfcf.sensfResLen)));
            memcpy(t.aux, &dev->dev.nfcf.sensfRes, t.auxSize); break;
        case RFAL_NFC_LISTEN_TYPE_NFCV: t.tech = Tech::V; break;
        default: return false;
    }
    if (!validUid(t.tech, dev->nfcidLen)) return false;
    t.uidSize = dev->nfcidLen;
    for (size_t p = 0; p < t.uidSize; ++p) t.uid[p] = dev->nfcid[t.tech == Tech::V ? t.uidSize - p - 1 : p];
    publish(ReaderState::Reading, "Reading public data; keep card still");
    auto cancelled = [&]() { return _stop.load() || _generation.load() != gen; };
    // Read NDEF using ST's tag-type dispatch (T2/T3/T4/T5), never assume every
    // ISO14443-4 card exposes the NFC Forum NDEF application.
    ReturnCode ndefResult = i.ndef.ndefPollerContextInitializationWrapper(dev);
    ndefInfo info{};
    const bool ndefSupported = ndefResult == ERR_NONE;
    if (ndefSupported && !cancelled()) ndefResult = i.ndef.ndefPollerNdefDetectWrapper(&info);
    if (ndefSupported && ndefResult == ERR_NONE) {
        t.ndefExpected = info.messageLen;
        if (info.messageLen > MaxNdef) t.ndefState = NdefState::TooLarge;
        else if (!cancelled()) {
            uint32_t received = 0;
            ndefResult = i.ndef.ndefPollerReadRawMessageWrapper(t.ndef, MaxNdef, &received, false);
            if (ndefResult == ERR_NONE && received <= MaxNdef && received == info.messageLen) {
                t.ndefSize = received; t.ndefState = NdefState::Read;
            } else t.ndefState = NdefState::Failed;
        }
    } else if (ndefSupported) {
        // A missing NDEF application is different from removal, CRC failure
        // or transport loss while detecting it.
        t.ndefState = ndefResult == ERR_REQUEST || ndefResult == ERR_NOTFOUND || ndefResult == ERR_NOTSUPP ?
            NdefState::Absent : NdefState::Failed;
    }
    if (cancelled()) return false;
    struct MemoryContext {
        Reader* reader;
        Impl* impl;
        rfalNfcDevice* device;
        uint32_t generation;
    } context{this, &i, dev, gen};
    const MemoryIo memoryIo{
        &context,
        [](void* ptr, MemoryCommand command, uint16_t address, uint8_t* bytes, size_t size, uint16_t& received) {
            auto& c = *static_cast<MemoryContext*>(ptr);
            switch (command) {
                case MemoryCommand::Type2Read:
                    return c.impl->nfc.rfalT2TPollerRead(address, bytes, size, &received) == ERR_NONE;
                case MemoryCommand::VSystemInfo:
                    return c.impl->nfc.rfalNfcvPollerGetSystemInformation(RFAL_NFCV_REQ_FLAG_DEFAULT,
                        c.device->nfcid, bytes, size, &received) == ERR_NONE;
                case MemoryCommand::VReadBlock:
                    return c.impl->nfc.rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT,
                        c.device->nfcid, address, bytes, size, &received) == ERR_NONE;
            }
            return false;
        },
        [](void* ptr) {
            auto& c = *static_cast<MemoryContext*>(ptr);
            return c.reader->_stop.load() || c.reader->_generation.load() != c.generation || !c.impl->bus.ok();
        },
        [](void*) { delay(1); }
    };
    // Only T2T with SAK=0 gets READ (0x30). Classic and other NFC-A cards stay
    // identity-only rather than treating encrypted sectors as Ultralight pages.
    if (t.tech == Tech::A && dev->dev.nfca.type == RFAL_NFCA_T2T && t.sak == 0) {
        captureType2Memory(t, memoryIo);
    }
    if (t.tech == Tech::V && !cancelled()) {
        captureIso15693Memory(t, memoryIo);
    }
    if (cancelled()) return false;
    return true;
}
void Reader::worker() {
    auto& i = *_impl;
    uint32_t seen = UINT32_MAX, lastUpdate = 0;
    ReaderMode active = ReaderMode::Idle;
    auto shutdownField = [&]() {
        i.cleanup = true;
        if (i.initialized) i.rf.rfalFieldOff();
        bool stopped = true;
        if (i.touched) {
            // Also clean up a partially initialized chip. The board exposes no
            // reset/power/IRQ GPIO, so use independent best-effort bus commands.
            const auto stop = i.rf.st25r3916ExecuteCommand(ST25R3916_CMD_STOP);
            const auto off = i.rf.st25r3916ClrRegisterBits(ST25R3916_REG_OP_CONTROL,
                ST25R3916_REG_OP_CONTROL_tx_en | ST25R3916_REG_OP_CONTROL_rx_en |
                ST25R3916_REG_OP_CONTROL_wu | ST25R3916_REG_OP_CONTROL_en);
            stopped = stop == ERR_NONE && off == ERR_NONE;
            if (stopped) { i.touched = false; i.fieldOn = false; }
        }
        i.initialized = false;
        return stopped;
    };
    while (!_stop.load()) {
        const uint32_t gen = _generation.load();
        if (gen != seen) {
            seen = gen; const bool stopped = shutdownField();
            active = ReaderMode(_mode.load()); i.activeGeneration = gen;
            if (active == ReaderMode::Idle) {
                publish(stopped ? ReaderState::Idle : ReaderState::Error,
                    stopped ? "RF field stopped" : "Could not confirm RF off; check module", stopped ? 0 : ERR_IO);
                delay(1); continue;
            }
            i.cleanup = false; i.cycles = 0; i.chipId = 0; i.supply = 0; i.externalField = false;
            publish(ReaderState::Initializing, "ST25R3916 at I2C 0x50");
            // M5Stack's warm-boot strategy: settle, retry identity, stop leftover
            // TX/RX before RFAL resets and calibrates the original 3916.
            auto cancelled = [&]() { return _stop.load() || _generation.load() != gen; };
            for (unsigned n = 0; n < 10 && !cancelled(); ++n) delay(5);
            ReturnCode rc = ERR_IO;
            for (unsigned attempt = 0; attempt < 5 && !cancelled(); ++attempt) {
                i.cleanup = false;
                rc = i.rf.st25r3916ReadRegister(ST25R3916_REG_IC_IDENTITY, &i.chipId);
                if (rc == ERR_NONE && (i.chipId & ST25R3916_REG_IC_IDENTITY_ic_type_mask) !=
                    ST25R3916_REG_IC_IDENTITY_ic_type_st25r3916) { rc = ERR_HW_MISMATCH; break; }
                const uint32_t errors = i.bus.errors();
                if (rc == ERR_NONE) { i.touched = true; rc = prepareSt25r3916(i.rf); }
                if (rc == ERR_NONE) { i.fieldOn = false; rc = i.nfc.rfalNfcInitialize(); }
                i.initialized = rc == ERR_NONE;
                if (rc == ERR_NONE) rc = configureSt25r3916Board(i.rf);
                if (rc == ERR_NONE) i.supply = i.rf.st25r3916MeasureVoltage(ST25R3916_REG_REGULATOR_CONTROL_mpsv_vdd);
                if (i.bus.errors() != errors) rc = ERR_IO;
                if (rc == ERR_NONE) break;
                shutdownField();
                for (unsigned n = 0; n < 4 && !cancelled(); ++n) delay(5);
            }
            if (cancelled()) { shutdownField(); continue; }
            if (rc != ERR_NONE) {
                shutdownField(); active = ReaderMode::Idle;
                char message[96];
                std::snprintf(message, sizeof(message), "Check 3V3/GND, SDA=%d, SCL=%d", HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL);
                publish(ReaderState::Error, rc == ERR_HW_MISMATCH ? "Expected original ST25R3916, not 3916B" : message, rc);
                continue;
            }
            if (active == ReaderMode::Scan) {
                rfalNfcDiscoverParam params; rfalNfcDefaultDiscParams(&params);
                params.devLimit = 1; params.totalDuration = 1000; params.maxBR = RFAL_BR_106;
                const uint8_t filter = _filter.load();
                params.techs2Find = filter == 1 ? RFAL_NFC_POLL_TECH_A : filter == 2 ? RFAL_NFC_POLL_TECH_B :
                    filter == 3 ? RFAL_NFC_POLL_TECH_F : filter == 4 ? RFAL_NFC_POLL_TECH_V :
                    (RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_F | RFAL_NFC_POLL_TECH_V);
                rc = i.nfc.rfalNfcDiscover(&params);
                if (rc == ERR_NONE) publish(ReaderState::Scanning, "Hold a card near the antenna");
            } else if (active == ReaderMode::Field) {
                rc = i.rf.rfalSetMode(RFAL_MODE_POLL_NFCA, RFAL_BR_106, RFAL_BR_106);
                if (rc == ERR_NONE) { i.rf.rfalSetGT(5000); rc = i.rf.rfalFieldOnAndStartGT(); }
                if (rc == ERR_NONE) { i.fieldOn = true; publish(ReaderState::Field, "13.56 MHz field enabled"); }
            } else publish(ReaderState::Debug, "I2C polling / IRQ not connected");
            if (rc != ERR_NONE) { shutdownField(); active = ReaderMode::Idle;
                publish(ReaderState::Error, "NFC initialization failed", rc); }
        }
        if (active == ReaderMode::Scan) {
            const uint32_t errors = i.bus.errors();
            i.nfc.rfalNfcWorker(); ++i.cycles;
            const auto state = i.nfc.rfalNfcGetState();
            if (state == RFAL_NFC_STATE_POLL_SELECT) i.nfc.rfalNfcSelect(0);
            if (state == RFAL_NFC_STATE_ACTIVATED) {
                const bool captured = capture(seen);
                const bool ioFailed = i.bus.errors() != errors;
                const bool stopped = shutdownField(); active = ReaderMode::Idle;
                if (captured && !ioFailed && stopped && _generation.load() == seen && !_stop.load())
                    publish(ReaderState::Found, "Card captured; RF field off", 0, &i.tag);
                else if (_generation.load() == seen && !_stop.load())
                    publish(ReaderState::Error, ioFailed || !stopped ? "NFC communication lost; retry" : "Card identification failed",
                        ioFailed || !stopped ? ERR_IO : ERR_PROTO);
            } else if (i.bus.errors() != errors) {
                shutdownField(); active = ReaderMode::Idle;
                if (_generation.load() == seen) publish(ReaderState::Error, "NFC module communication lost", ERR_IO);
            } else if (millis() - lastUpdate >= 400) {
                lastUpdate = millis(); publish(ReaderState::Scanning, "Hold a card near the antenna");
            }
        } else if ((active == ReaderMode::Debug || active == ReaderMode::Field) && millis() - lastUpdate >= 500) {
            lastUpdate = millis(); const uint32_t errors = i.bus.errors();
            const bool external = i.rf.rfalIsExtFieldOn();
            uint8_t id = 0; i.rf.st25r3916ReadRegister(ST25R3916_REG_IC_IDENTITY, &id);
            if (i.bus.errors() != errors || id != i.chipId) {
                shutdownField(); active = ReaderMode::Idle;
                publish(ReaderState::Error, "NFC module communication lost", ERR_IO);
            } else {
                i.externalField = external;
                publish(active == ReaderMode::Debug ? ReaderState::Debug : ReaderState::Field,
                    active == ReaderMode::Debug ? "I2C polling / IRQ not connected" : "13.56 MHz field enabled");
            }
        }
        delay(active == ReaderMode::Scan ? 1 : 5);
    }
    shutdownField();
}
} // namespace meow_nfc
