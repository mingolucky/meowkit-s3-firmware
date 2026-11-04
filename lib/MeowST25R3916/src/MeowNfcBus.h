// MeowKit shared-bus adapter. No Arduino Wire instance or driver reinstallation.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

class MeowNfcBus {
public:
    using Transfer = bool (*)(void*, uint8_t, const uint8_t*, size_t, uint8_t*, size_t);
    MeowNfcBus(Transfer fn, void* context) : _fn(fn), _context(context) {}
    void beginTransmission(uint8_t address) {
        _address = address; _txLen = 0; _pending = false;
        _ok = address == 0x50; _rxLen = _rxPos = 0;
    }
    size_t write(uint8_t value) {
        if (!_ok || _txLen == sizeof(_tx)) { _ok = false; return 0; }
        _tx[_txLen++] = value; return 1;
    }
    uint8_t endTransmission(bool stop = true) {
        if (!_ok || !_txLen) { fail(); return 4; }
        if (!stop) { _pending = true; return 0; }
        _ok = _fn && _fn(_context, _address, _tx, _txLen, nullptr, 0);
        _pending = false;
        if (!_ok) fail();
        return _ok ? 0 : 4;
    }
    size_t requestFrom(uint8_t address, size_t length) {
        _rxLen = _rxPos = 0;
        if (!_ok || address != 0x50 || address != _address || !_pending ||
            !length || length > sizeof(_rx)) { fail(); return 0; }
        memset(_rx, 0, length);
        _ok = _fn && _fn(_context, address, _tx, _txLen, _rx, length);
        _pending = false;
        if (!_ok) { fail(); return 0; }
        _rxLen = length; return length;
    }
    size_t available() const { return _rxLen - _rxPos; }
    int read() { return available() ? _rx[_rxPos++] : -1; }
    bool ok() const { return _ok; }
    uint32_t errors() const { return _errors; }
private:
    void fail() { _ok = false; _pending = false; _rxLen = _rxPos = 0; ++_errors; }
    Transfer _fn;
    void* _context;
    uint8_t _tx[514]{}, _rx[512]{};
    size_t _txLen = 0, _rxLen = 0, _rxPos = 0;
    uint8_t _address = 0;
    bool _ok = true, _pending = false;
    uint32_t _errors = 0;
};
