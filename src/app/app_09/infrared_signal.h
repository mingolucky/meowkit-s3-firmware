#pragma once
#include <momentum_ir.h>
#include "infrared_file_utils.h"
#include "infrared_scan.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>

namespace meow_ir {
constexpr size_t MaxTimings = 4096;
constexpr size_t MaxButtons = 256;
constexpr size_t MaxName = 23;
constexpr size_t MaxNativeStateBytes = 128;

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
    std::vector<uint8_t> legacyState;
    std::shared_ptr<const Signal> powerOff;
    bool powerNextOn=true;
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

inline bool samePayload(const Signal& a,const Signal& b) {
    if(a.raw!=b.raw) return false;
    if(a.raw) return a.frequency==b.frequency && a.duty==b.duty && a.timings==b.timings;
    return a.protocol==b.protocol && a.address==b.address && a.command==b.command &&
        a.legacyProtocol==b.legacyProtocol && a.legacyValue==b.legacyValue &&
        a.legacyBits==b.legacyBits && a.legacyState==b.legacyState;
}
inline bool validSignal(const Signal& signal) {
    if(signal.powerOff && (signal.powerOff->powerOff ||
        !validSignal(*signal.powerOff) || samePayload(signal,*signal.powerOff))) return false;
    if (signal.name.empty() || signal.name.size() > MaxName ||
        signal.name.find_first_of("\r\n") != std::string::npos) return false;
    if (signal.raw) {
        if (signal.timings.empty() || signal.timings.size() > MaxTimings ||
            signal.frequency < 10000 || signal.frequency > 1000000 ||
            !std::isfinite(signal.duty) || signal.duty <= 0 || signal.duty > 1) return false;
        for (auto timing : signal.timings) if (!timing || timing > 10000000) return false;
        return true;
    }
    if (signal.protocol == InfraredProtocolUnknown) {
        if (signal.legacyProtocol.empty() || signal.legacyProtocol.size()>64 ||
            signal.legacyProtocol.find_first_of("\r\n")!=std::string::npos || !signal.legacyBits) return false;
        return signal.legacyState.empty() ? signal.legacyBits<=64 :
            signal.legacyState.size()<=MaxNativeStateBytes &&
            signal.legacyState.size()==(size_t(signal.legacyBits)+7)/8;
    }
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
    bool nativeEncoding = false;
    bool hasValue = false, hasState = false;
    std::string protocolText;
    bool hasPowerToggle=false,hasPowerNext=false,isPowerOff=false;
    std::shared_ptr<RecordParser> offParser;

    void field(const std::string& key, const std::string& value) {
        if(key.rfind("off_",0)==0) {
            if(isPowerOff) { ok=false; return; }
            if(!offParser) { offParser=std::make_shared<RecordParser>(); offParser->isPowerOff=true; }
            offParser->field(key.substr(4),value);
            signal.powerOff=std::make_shared<const Signal>(offParser->signal);
            return;
        }
        uint64_t number = 0;
        if (key=="power_toggle") {
            ok &= !isPowerOff && !hasPowerToggle && value=="1"; hasPowerToggle=true;
        } else if(key=="power_next") {
            ok &= !isPowerOff && !hasPowerNext && (value=="on" || value=="off"); hasPowerNext=true;
            signal.powerNextOn=value=="on";
        } else if (key == "name") signal.name = value;
        else if (key == "type") {
            hasType = true;
            signal.raw = value == "raw";
            ok &= signal.raw || value == "parsed";
        } else if (key == "encoding") {
            nativeEncoding = value == "native";
            ok &= nativeEncoding;
            if (nativeEncoding) { signal.protocol=InfraredProtocolUnknown; signal.legacyProtocol=protocolText; }
        } else if (key == "protocol") {
            protocolText=value;
            signal.protocol = nativeEncoding?InfraredProtocolUnknown:infrared_get_protocol_by_name(value.c_str());
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
        } else if (key == "state") {
            ok &= !hasState; hasState=true;
            const char* p=value.c_str();
            while (*p && ok) {
                while (*p==' ' || *p=='\t') ++p;
                if (!*p) break;
                const auto hex=[](char c)->int {
                    if(c>='0'&&c<='9') return c-'0';
                    if(c>='A'&&c<='F') return c-'A'+10;
                    if(c>='a'&&c<='f') return c-'a'+10;
                    return -1;
                };
                const int high=hex(p[0]),low=p[1]?hex(p[1]):-1;
                if(high<0 || low<0 || (p[2] && p[2]!=' ' && p[2]!='\t') ||
                    signal.legacyState.size()>=MaxNativeStateBytes) { ok=false; break; }
                signal.legacyState.push_back(uint8_t((high<<4)|low)); p+=2;
            }
            ok &= !signal.legacyState.empty();
        } else if (key == "value") {
            ok &= !hasValue; hasValue=true;
            if (unsignedNumber(value.c_str(), number, 0)) signal.legacyValue = number;
            else ok = false;
        } else if (key == "bits") {
            if (unsignedNumber(value.c_str(), number) && number <= MaxNativeStateBytes*8)
                signal.legacyBits = static_cast<uint16_t>(number);
            else ok = false;
        }
    }

    bool valid() const {
        if(hasPowerToggle || hasPowerNext || offParser) {
            if(!hasPowerToggle || !hasPowerNext || !offParser || !offParser->valid()) return false;
        }
        if (!signal.raw && signal.protocol==InfraredProtocolUnknown && hasValue==hasState) return false;
        return ok && hasType && !(nativeEncoding && signal.raw) && (signal.raw ? hasFrequency && hasDuty && hasData :
            hasAddress && hasCommand) && validSignal(signal);
    }
};

inline std::string serializeSingle(const Signal& signal) {
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
    if (signal.protocol==InfraredProtocolUnknown) out += "encoding: native\n";
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
        snprintf(buffer,sizeof(buffer),"bits: %u\n",unsigned(signal.legacyBits)); out+=buffer;
        if (signal.legacyState.empty()) {
            snprintf(buffer,sizeof(buffer),"value: 0x%llX\n",static_cast<unsigned long long>(signal.legacyValue)); out+=buffer;
        } else {
            out+="state:";
            for (auto byte:signal.legacyState) { snprintf(buffer,sizeof(buffer)," %02X",unsigned(byte)); out+=buffer; }
            out+='\n';
        }
    }
    return out;
}

inline std::string serialize(const Signal& signal) {
    std::string out=serializeSingle(signal);
    if(!signal.powerOff) return out;
    out+="power_toggle: 1\npower_next: "; out+=signal.powerNextOn?"on\n":"off\n";
    const auto alternate=serializeSingle(*signal.powerOff);
    for(size_t begin=0;begin<alternate.size();) {
        const auto end=alternate.find('\n',begin);
        if(end==std::string::npos) break;
        const auto line=alternate.substr(begin,end-begin);
        if(!line.empty() && line[0]!='#') out+="off_"+line+"\n";
        begin=end+1;
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
