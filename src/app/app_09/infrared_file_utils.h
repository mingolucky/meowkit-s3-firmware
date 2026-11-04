#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdlib>

namespace infrared_file {
// Button names are signal labels, not filesystem components. Preserve every
// printable ASCII symbol; leading/trailing spaces would be trimmed on reload.
inline bool validButtonName(const char* name) {
    if (!name || !*name || name[0] == ' ' || name[std::strlen(name) - 1] == ' ') return false;
    for (const char* p = name; *p; ++p)
        if (static_cast<unsigned char>(*p) < 32 || static_cast<unsigned char>(*p) > 126) return false;
    return true;
}
// Device-entered names are single FAT path components, never paths.
inline bool validName(const char* name) {
    if (!name || !*name || !std::strcmp(name, ".") || !std::strcmp(name, "..")) return false;
    const size_t n = std::strlen(name);
    if (name[n - 1] == '.' || name[n - 1] == ' ') return false;
    for (const char* p = name; *p; ++p)
        if (static_cast<unsigned char>(*p) < 32 || std::strchr("/\\:*?\"<>|", *p)) return false;
    return true;
}

inline int listWindow(int count, int rows, int& selected, int& offset) {
    if (count <= 0 || rows <= 0) { selected = offset = 0; return 0; }
    offset = std::max(0, std::min(offset, std::max(0, count - rows)));
    const int visible = std::min(rows, count - offset);
    selected = std::max(0, std::min(selected, visible - 1));
    return visible;
}

inline bool hexBytes(const char* p, uint32_t& value) {
    value = 0;
    unsigned bytes = 0;
    while (p && *p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        if (bytes == 4 || !((*p >= '0' && *p <= '9') ||
            (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) return false;
        char* end = nullptr;
        const unsigned long byte = std::strtoul(p, &end, 16);
        if (end == p || end - p > 2 || byte > 255 ||
            (*end && *end != ' ' && *end != '\t')) return false;
        value |= static_cast<uint32_t>(byte) << (8 * bytes++);
        p = end;
    }
    return bytes != 0;
}
} // namespace infrared_file
