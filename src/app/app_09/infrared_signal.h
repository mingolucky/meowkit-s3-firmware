#pragma once
#include <momentum_ir.h>
#include "infrared_file_utils.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace meow_ir {
constexpr size_t MaxTimings = 4096;
constexpr size_t MaxButtons = 256;
constexpr size_t MaxName = 23;

struct Signal {
    std::string name;
    bool raw = false;
    InfraredProtocol protocol = InfraredProtocolUnknown;
    uint32_t address = 0, command = 0, frequency = 38000;
    float duty = 0.33f;
    std::vector<uint32_t> timings;
    // Preserve files saved by the previous MeowKit implementation.
    std::string legacyProtocol;
    uint64_t legacyValue = 0;
    uint16_t legacyBits = 0;
};

inline bool unsignedNumber(const char* text, uint64_t& value, int base = 10) {
    if (!text || !*text || *text == '-' || *text == '+') return false;
    errno = 0;
    char* end = nullptr;
    const auto result = strtoull(text, &end, base);
    if (errno || end == text || *end) return false;
    value = result;
    return true;
}

inline bool validSignal(const Signal& signal) {
    if (signal.name.empty() || signal.name.size() > MaxName ||
        signal.name.find_first_of("\r\n") != std::string::npos) return false;
    if (signal.raw) {
        if (signal.timings.empty() || signal.timings.size() > MaxTimings ||
            signal.frequency < 10000 || signal.frequency > 1000000 ||
            !std::isfinite(signal.duty) || signal.duty <= 0 || signal.duty > 1) return false;
        for (auto timing : signal.timings) if (!timing || timing > 10000000) return false;
        return true;
    }
    if (signal.protocol == InfraredProtocolUnknown)
        return !signal.legacyProtocol.empty() && signal.legacyBits > 0 && signal.legacyBits <= 64;
    if (!infrared_is_protocol_valid(signal.protocol)) return false;
    const auto a = infrared_get_protocol_address_length(signal.protocol);
    const auto c = infrared_get_protocol_command_length(signal.protocol);
    return (a == 32 || (signal.address >> a) == 0) &&
           (c == 32 || (signal.command >> c) == 0);
}

// Feed one key/value at a time. A malformed record is never silently dropped:
// editing a remote must preserve every button, or fail without replacing it.
struct RecordParser {
    Signal signal;
    bool ok = true, hasType = false, hasAddress = false, hasCommand = false;
    bool hasFrequency = false, hasDuty = false, hasData = false;

    void field(const std::string& key, const std::string& value) {
        uint64_t number = 0;
        if (key == "name") signal.name = value;
        else if (key == "type") {
            hasType = true;
            signal.raw = value == "raw";
            ok &= signal.raw || value == "parsed";
        } else if (key == "protocol") {
            signal.protocol = infrared_get_protocol_by_name(value.c_str());
            if (signal.protocol == InfraredProtocolUnknown) signal.legacyProtocol = value;
        } else if (key == "address") {
            hasAddress = true;
            ok &= infrared_file::hexBytes(value.c_str(), signal.address);
        } else if (key == "command") {
            hasCommand = true;
            ok &= infrared_file::hexBytes(value.c_str(), signal.command);
        } else if (key == "frequency") {
            hasFrequency = true;
            if (unsignedNumber(value.c_str(), number) && number <= UINT32_MAX)
                signal.frequency = static_cast<uint32_t>(number);
            else ok = false;
        } else if (key == "duty_cycle") {
            hasDuty = true;
            char* end = nullptr;
            signal.duty = strtof(value.c_str(), &end);
            ok &= end != value.c_str() && *end == 0;
        } else if (key == "data") {
            hasData = true;
            const char* p = value.c_str();
            while (*p && ok) {
                while (*p == ' ' || *p == '\t') ++p;
                if (!*p) break;
                if (*p < '0' || *p > '9') { ok = false; break; }
                errno = 0;
                char* end = nullptr;
                const auto duration = strtoul(p, &end, 10);
                if (errno || p == end || (*end && *end != ' ' && *end != '\t') ||
                    !duration || duration > 10000000 || signal.timings.size() == MaxTimings) {
                    ok = false;
                    break;
                }
                signal.timings.push_back(static_cast<uint32_t>(duration));
                p = end;
            }
        } else if (key == "value") {
            if (unsignedNumber(value.c_str(), number, 0)) signal.legacyValue = number;
            else ok = false;
        } else if (key == "bits") {
            if (unsignedNumber(value.c_str(), number) && number <= 64)
                signal.legacyBits = static_cast<uint16_t>(number);
            else ok = false;
        }
    }

    bool valid() const {
        return ok && hasType && (signal.raw ? hasFrequency && hasDuty && hasData :
            hasAddress && hasCommand) && validSignal(signal);
    }
};

inline std::string serialize(const Signal& signal) {
    std::string out = "#\nname: " + signal.name + "\ntype: " + (signal.raw ? "raw\n" : "parsed\n");
    char buffer[128];
    if (signal.raw) {
        snprintf(buffer, sizeof(buffer), "frequency: %lu\nduty_cycle: %.6f\ndata:",
                 static_cast<unsigned long>(signal.frequency), static_cast<double>(signal.duty));
        out += buffer;
        for (auto timing : signal.timings) {
            snprintf(buffer, sizeof(buffer), " %lu", static_cast<unsigned long>(timing));
            out += buffer;
        }
        return out + "\n";
    }
    out += "protocol: ";
    out += signal.protocol == InfraredProtocolUnknown ? signal.legacyProtocol :
        infrared_get_protocol_name(signal.protocol);
    out += '\n';
    for (int field = 0; field < 2; ++field) {
        uint32_t v = field ? signal.command : signal.address;
        snprintf(buffer, sizeof(buffer), "%s: %02X %02X %02X %02X\n",
                 field ? "command" : "address", unsigned(v & 255), unsigned((v >> 8) & 255),
                 unsigned((v >> 16) & 255), unsigned(v >> 24));
        out += buffer;
    }
    if (signal.protocol == InfraredProtocolUnknown) {
        snprintf(buffer, sizeof(buffer), "value: 0x%llX\nbits: %u\n",
                 static_cast<unsigned long long>(signal.legacyValue), unsigned(signal.legacyBits));
        out += buffer;
    }
    return out;
}

inline void listMove(int delta, int count, int visible, int& selected, int& top) {
    if (count <= 0) { selected = top = 0; return; }
    selected = std::max(0, std::min(count - 1, selected + delta));
    top = std::max(0, std::min(top, std::max(0, count - visible)));
    if (selected < top) top = selected;
    if (selected >= top + visible) top = selected - visible + 1;
}

struct Scan {
    size_t index = 0, total = 0;
    bool running = false, paused = false;
    void start(size_t count) { index = 0; total = count; running = count > 0; paused = false; }
    void pause() { if (running) { paused = true; if (index) --index; } }
    void resume() { if (running) paused = false; }
    void move(int delta) {
        if (paused && total) index = size_t(std::max(0, std::min(int(total) - 1, int(index) + delta)));
    }
    void sent() { if (running && !paused && ++index >= total) running = false; }
};

struct DirectionRepeat {
    bool held = false;
    uint32_t next = 0;
    bool poll(bool down, bool edge, uint32_t now) {
        if (!down) { held = false; return false; }
        if (edge || !held) { held = true; next = now + 350; return edge; }
        if (int32_t(now - next) >= 0) { next = now + 110; return true; }
        return false;
    }
};
} // namespace meow_ir
