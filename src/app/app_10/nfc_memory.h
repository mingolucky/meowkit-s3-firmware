#pragma once
#include "nfc_data.h"

namespace meow_nfc {
enum class MemoryCommand : uint8_t { Type2Read, VSystemInfo, VReadBlock };
// Protocol-independent policy; the worker supplies the RFAL exchanges and
// cancellation. Host tests exercise exactly the same read boundaries.
struct MemoryIo {
    void* context;
    bool (*exchange)(void*, MemoryCommand, uint16_t, uint8_t*, size_t, uint16_t&);
    bool (*cancelled)(void*) = nullptr;
    void (*yield)(void*) = nullptr;
};
void captureType2Memory(Tag& tag, const MemoryIo& io);
void captureIso15693Memory(Tag& tag, const MemoryIo& io);
} // namespace meow_nfc
