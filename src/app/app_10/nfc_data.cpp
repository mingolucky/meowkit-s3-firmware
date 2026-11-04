#include "nfc_data.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <limits>

namespace meow_nfc {
const char* techName(Tech tech) {
    switch (tech) { case Tech::A: return "NFC-A / ISO14443A";
        case Tech::B: return "NFC-B / ISO14443B";
        case Tech::F: return "NFC-F / FeliCa";
        case Tech::V: return "NFC-V / ISO15693"; default: return "Unknown"; }
}
const char* tagName(const Tag& t) {
    if (t.protocol[0]) return t.protocol;
    if (t.tech != Tech::A) return techName(t.tech);
    if (t.sak == 0x08 || t.sak == 0x18 || t.sak == 0x09) return "Mifare Classic (UID only)";
    if (t.sak & 0x20) return "ISO14443-4A";
    if (t.memorySize) return "Ultralight / NTAG";
    return "ISO14443-3A";
}
const char* memoryStateName(MemoryState state) {
    switch (state) {
        case MemoryState::Read: return "Complete public area";
        case MemoryState::Partial: return "Partial / capacity not verified";
        case MemoryState::Limited: return "Capture limit reached";
        case MemoryState::Failed: return "Memory read failed";
        default: return "Memory not read";
    }
}
const char* ndefStateName(NdefState state) {
    switch (state) {
        case NdefState::Read: return "NDEF captured";
        case NdefState::Absent: return "No readable NDEF";
        case NdefState::TooLarge: return "Exceeds capture limit";
        case NdefState::Failed: return "Read failed; retry scan";
        default: return "NDEF not read";
    }
}
bool validUid(Tech tech, size_t n) {
    return tech == Tech::A ? (n == 4 || n == 7 || n == 10) :
           tech == Tech::B ? n == 4 : (tech == Tech::F || tech == Tech::V) && n == 8;
}
bool validName(const std::string& s) {
    if (s.empty() || s.size() > 23 || s.front() == ' ' || s.back() == ' ') return false;
    for (unsigned char c : s) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ')) return false;
    return true;
}
std::string hex(const uint8_t* p, size_t n, bool spaced) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out; out.reserve(n * (spaced ? 3 : 2));
    for (size_t i = 0; i < n; ++i) { if (i && spaced) out += ' ';
        out += digits[p[i] >> 4]; out += digits[p[i] & 15]; }
    return out;
}
bool parseHex(const std::string& s, uint8_t* out, size_t capacity, size_t& n) {
    n = 0; int hi = -1;
    for (unsigned char c : s) {
        if (c == ' ' || c == '\t' || c == '\r') { if (hi >= 0) return false; continue; }
        int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (v < 0) return false;
        if (hi < 0) hi = v;
        else { if (n >= capacity) return false; out[n++] = uint8_t((hi << 4) | v); hi = -1; }
    }
    return hi < 0;
}
static std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t\r") - b + 1);
}
static bool number(const std::string& s, size_t max, size_t& value) {
    if (s.empty()) return false;
    value = 0;
    for (char c : s) { if (c < '0' || c > '9') return false;
        if (size_t(c - '0') > max || value > max / 10 || value * 10 > max - size_t(c - '0')) return false;
        value = value * 10 + c - '0'; }
    return true;
}
static Tech protocolTech(const std::string& p) {
    if (p == "NFC-B" || p.find("14443-3B") != std::string::npos || p.find("14443-4B") != std::string::npos) return Tech::B;
    if (p == "NFC-F" || p == "FeliCa") return Tech::F;
    if (p == "NFC-V" || p.find("15693") != std::string::npos || p == "SLIX") return Tech::V;
    if (p == "UID" || p == "NFC-A" || p.find("14443-3A") != std::string::npos ||
        p.find("14443-4A") != std::string::npos || p.find("Mifare") != std::string::npos ||
        p.find("NTAG") != std::string::npos) return Tech::A;
    return Tech::Unknown;
}
bool parseCapture(const std::string& text, Tag& result, std::string& error) {
    // Parse into a temporary object: a malformed file cannot corrupt the current card.
    if (text.size() > MaxFile) { error = "File exceeds 128 KiB"; return false; }
    Tag tag; bool header = false, custom = false, uid = false, type = false, version = false;
    bool atqa = false, sak = false, data = false, ndef = false, hasPages = false;
    bool ndefStateSeen = false, manualSeen = false, memoryStateSeen = false;
    bool expectedSeen = false, ndefExpectedSeen = false, responseSeen = false, pagesTotalSeen = false;
    size_t savedNdefState = 0, savedMemoryState = 0, manual = 0;
    unsigned formatVersion = 0;
    bool pages[MaxMemory / 4]{};
    size_t expected = 0, ndefExpected = 0;
    for (size_t pos = 0; pos < text.size();) {
        size_t end = text.find('\n', pos); if (end == std::string::npos) end = text.size();
        if (end - pos > 8192) { error = "Line too long"; return false; }
        std::string line = trim(text.substr(pos, end - pos)); pos = end + 1;
        if (line.empty() || line[0] == '#') continue;
        const auto colon = line.find(':'); if (colon == std::string::npos) continue;
        auto key = trim(line.substr(0, colon)), value = trim(line.substr(colon + 1));
        size_t count = 0;
        auto invalid = [&]() { error = "Invalid or duplicate " + key; return false; };
        if (key == "Filetype") {
            if (header || (value != "Flipper NFC device" && value != "MeowKit NFC capture")) return invalid();
            header = true; custom = value == "MeowKit NFC capture";
        } else if (key == "Version") {
            if (version || !number(value, 4, count)) return invalid();
            version = true; formatVersion = unsigned(count);
        } else if (key == "Device type") {
            if (type || value.size() >= sizeof(tag.protocol)) return invalid();
            type = true; tag.tech = protocolTech(value); std::strcpy(tag.protocol, value.c_str());
        } else if (key == "UID") {
            if (uid || !parseHex(value, tag.uid, sizeof(tag.uid), count)) return invalid();
            uid = true; tag.uidSize = count;
        } else if (key == "ATQA") {
            if (atqa || !parseHex(value, tag.atqa, 2, count) || count != 2) return invalid();
            atqa = true;
        } else if (key == "SAK") {
            if (sak || !parseHex(value, &tag.sak, 1, count) || count != 1) return invalid();
            sak = true;
        } else if (key == "Meow memory") {
            if (data || hasPages || !parseHex(value, tag.memory, MaxMemory, count)) return invalid();
            data = true; tag.memorySize = count;
        } else if (key == "Meow expected bytes") {
            if (expectedSeen || pagesTotalSeen || !number(value, 65535, expected)) return invalid();
            expectedSeen = true;
        } else if (key == "Meow memory state") {
            if (memoryStateSeen || !number(value, size_t(MemoryState::Failed), savedMemoryState)) return invalid();
            memoryStateSeen = true;
        } else if (key == "Meow NDEF") {
            if (ndef || !parseHex(value, tag.ndef, MaxNdef, count)) return invalid();
            ndef = true; tag.ndefSize = count; tag.ndefState = NdefState::Read;
        } else if (key == "Meow NDEF expected") {
            if (ndefExpectedSeen || !number(value, UINT32_MAX, ndefExpected)) return invalid();
            ndefExpectedSeen = true;
        } else if (key == "Meow NDEF state") {
            if (ndefStateSeen || !number(value, size_t(NdefState::Failed), savedNdefState)) return invalid();
            ndefStateSeen = true;
        } else if (key == "Meow manual") {
            if (manualSeen || !number(value, 1, manual)) return invalid();
            manualSeen = true;
        } else if (key == "Meow response") {
            if (responseSeen || !parseHex(value, tag.aux, sizeof(tag.aux), count)) return invalid();
            responseSeen = true; tag.auxSize = count;
        } else if (key == "Pages total") {
            if (pagesTotalSeen || expectedSeen || !number(value, 16383, count)) return invalid();
            pagesTotalSeen = true; expected = count * 4;
        } else if (key.size() > 5 && key.compare(0, 5, "Page ") == 0) {
            if (!number(key.substr(5), 65535, count)) return invalid();
            uint8_t bytes[4]; size_t n;
            if (!parseHex(value, bytes, 4, n) || n != 4 || data) return invalid();
            hasPages = true;
            if (count < MaxMemory / 4) {
                if (pages[count]) return invalid();
                pages[count] = true;
                std::memcpy(tag.memory + count * 4, bytes, 4);
            }
        }
    }
    if (!header || !version || !type || !uid ||
        (custom ? formatVersion != 1 : (formatVersion < 2 || formatVersion > 4)) ||
        !validUid(tag.tech, tag.uidSize) || (tag.tech == Tech::A && (!atqa || !sak))) {
        error = "Unsupported or incomplete NFC file"; return false;
    }
    if (!custom && formatVersion > 2) std::swap(tag.atqa[0], tag.atqa[1]);
    if (!data) { size_t n = 0; while (n < MaxMemory / 4 && pages[n]) ++n; tag.memorySize = n * 4; }
    if (expected && tag.memorySize > expected) { error = "Memory length mismatch"; return false; }
    tag.memoryExpected = std::max<size_t>(expected, tag.memorySize);
    if (memoryStateSeen) {
        tag.memoryState = MemoryState(savedMemoryState);
        if ((tag.memoryState == MemoryState::Read && (!tag.memorySize || tag.memorySize != tag.memoryExpected)) ||
            (tag.memoryState == MemoryState::Limited && tag.memorySize >= tag.memoryExpected)) {
            error = "Memory state mismatch"; return false;
        }
    } else if (tag.memorySize) {
        tag.memoryState = expected && expected == tag.memorySize ? MemoryState::Read : MemoryState::Partial;
    }
    if (ndef && ndefExpectedSeen && ndefExpected != tag.ndefSize) { error = "NDEF length mismatch"; return false; }
    tag.ndefExpected = std::max<size_t>(ndefExpected, tag.ndefSize);
    if (ndefStateSeen) {
        if ((savedNdefState == size_t(NdefState::Read)) != ndef) { error = "NDEF state mismatch"; return false; }
        tag.ndefState = NdefState(savedNdefState);
    }
    if (!ndefStateSeen && tag.ndefExpected > MaxNdef && !ndef) tag.ndefState = NdefState::TooLarge;
    if (tag.ndefState == NdefState::TooLarge && tag.ndefExpected <= MaxNdef) { error = "NDEF limit mismatch"; return false; }
    tag.manual = manual && tag.tech == Tech::A;
    tag.imported = true; result = tag; error.clear(); return true;
}
std::string serializeCapture(const Tag& t) {
    if (!validUid(t.tech, t.uidSize) || t.memorySize > MaxMemory || t.ndefSize > MaxNdef ||
        t.auxSize > sizeof(t.aux) || unsigned(t.ndefState) > unsigned(NdefState::Failed) ||
        unsigned(t.memoryState) > unsigned(MemoryState::Failed)) return {};
    // Only export a Flipper protocol whose mandatory fields were actually read.
    // Additional capture fields are ignored by Flipper's ISO14443-3A loader.
    const bool a = t.tech == Tech::A;
    std::string out = a ? "Filetype: Flipper NFC device\nVersion: 4\nDevice type: ISO14443-3A\n" :
                         "Filetype: MeowKit NFC capture\nVersion: 1\nDevice type: ";
    if (!a) { out += t.tech == Tech::B ? "NFC-B\n" : t.tech == Tech::F ? "NFC-F\n" : "NFC-V\n"; }
    out += "UID: " + hex(t.uid, t.uidSize) + "\n";
    if (a) { const uint8_t msb[] = {t.atqa[1], t.atqa[0]};
        out += "ATQA: " + hex(msb, 2) + "\nSAK: " + hex(&t.sak, 1) + "\n"; }
    out += "# MeowKit capture: identity and publicly readable data, no authentication keys.\n";
    out += "Meow manual: " + std::to_string(t.manual ? 1 : 0) + "\n";
    if (t.auxSize) out += "Meow response: " + hex(t.aux, t.auxSize) + "\n";
    out += "Meow expected bytes: " + std::to_string(t.memoryExpected) + "\n";
    out += "Meow memory state: " + std::to_string(unsigned(t.memoryState)) + "\n";
    if (t.memorySize) out += "Meow memory: " + hex(t.memory, t.memorySize) + "\n";
    out += "Meow NDEF expected: " + std::to_string(t.ndefExpected) + "\n";
    out += "Meow NDEF state: " + std::to_string(unsigned(t.ndefState)) + "\n";
    if (t.ndefState == NdefState::Read) out += "Meow NDEF: " + hex(t.ndef, t.ndefSize) + "\n";
    return out;
}
static std::string printable(const uint8_t* p, size_t n) {
    std::string s; const size_t limit = std::min<size_t>(n, 512);
    for (size_t i = 0; i < limit; ++i) s += p[i] >= 32 && p[i] != 127 ? char(p[i]) : ' ';
    if (n > limit) s += "...";
    return s;
}
bool describeNdef(const uint8_t* p, size_t n, std::string& result) {
    result.clear(); if (!n) { result = "Empty NDEF message"; return true; }
    if (!p || n > MaxNdef) return false;
    size_t at = 0, records = 0; bool ended = false;
    static const char* uri[] = {"", "http://www.", "https://www.", "http://", "https://", "tel:", "mailto:", "ftp://anonymous:anonymous@", "ftp://ftp.", "ftps://", "sftp://", "smb://", "nfs://", "ftp://", "dav://", "news:", "telnet://", "imap:", "rtsp://", "urn:", "pop:", "sip:", "sips:", "tftp:", "btspp://", "btl2cap://", "btgoep://", "tcpobex://", "irdaobex://", "file://", "urn:epc:id:", "urn:epc:tag:", "urn:epc:pat:", "urn:epc:raw:", "urn:epc:", "urn:nfc:"};
    while (at < n && !ended) {
        if (++records > 64 || n - at < 3) return false;
        uint8_t flags = p[at++], typeLen = p[at++]; uint32_t len = 0;
        const uint8_t tnf = flags & 7;
        if ((records == 1) != bool(flags & 0x80) || (flags & 0x20) || tnf >= 6) return false;
        if (flags & 0x10) len = p[at++];
        else { if (n - at < 4) return false; for (int i = 0; i < 4; ++i) len = (len << 8) | p[at++]; }
        uint8_t idLen = 0; if (flags & 8) { if (at == n) return false; idLen = p[at++]; }
        if ((tnf == 0 && (typeLen || len || idLen)) || (tnf == 5 && typeLen)) return false;
        if (size_t(typeLen) + idLen > n - at) return false;
        const uint8_t* type = p + at; at += size_t(typeLen) + idLen;
        if (len > n - at) return false;
        const uint8_t* payload = p + at; at += len;
        if (records > 1) result += '\n';
        if ((flags & 7) == 1 && typeLen == 1 && type[0] == 'T') {
            if (!len || (payload[0] & 0x3f) > len - 1) return false;
            const size_t start = 1 + (payload[0] & 0x3f);
            result += payload[0] & 0x80 ? "UTF-16 Text (see raw NDEF)" : "Text: " + printable(payload + start, len - start);
        } else if ((flags & 7) == 1 && typeLen == 1 && type[0] == 'U') {
            if (!len || payload[0] >= sizeof(uri) / sizeof(uri[0])) return false;
            result += std::string("URI: ") + uri[payload[0]] + printable(payload + 1, len - 1);
        } else result += "Record " + std::to_string(records) + ": " + printable(type, typeLen) + " (" + std::to_string(len) + " bytes)";
        ended = flags & 0x40;
    }
    return ended && at == n;
}
} // namespace meow_nfc
