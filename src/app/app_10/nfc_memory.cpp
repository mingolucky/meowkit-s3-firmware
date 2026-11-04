#include "nfc_memory.h"
#include <algorithm>
#include <cstring>

namespace meow_nfc {
static bool stopped(const MemoryIo& io) { return io.cancelled && io.cancelled(io.context); }
static void pause(const MemoryIo& io) { if (io.yield) io.yield(io.context); }
static void resetMemory(Tag& tag) {
    tag.memorySize = tag.memoryExpected = 0;
    tag.memoryState = MemoryState::Failed;
}
void captureType2Memory(Tag& tag, const MemoryIo& io) {
    if (tag.tech != Tech::A || tag.sak != 0) return;
    resetMemory(tag);
    if (!io.exchange || stopped(io)) return;
    uint8_t bytes[16]{}; uint16_t received = 0;
    if (!io.exchange(io.context, MemoryCommand::Type2Read, 0, bytes, sizeof(bytes), received) || received != 16) return;
    std::memcpy(tag.memory, bytes, 16); tag.memorySize = tag.memoryExpected = 16;
    tag.memoryState = MemoryState::Partial;
    // Without a valid, readable NFC Forum v1 CC, retain the identity pages.
    // Do not guess a capacity and probe configuration/password pages.
    if (bytes[12] != 0xe1 || (bytes[13] >> 4) != 1 || !bytes[14] || (bytes[15] >> 4) != 0) return;
    tag.memoryExpected = 16 + unsigned(bytes[14]) * 8;
    const unsigned limit = std::min<unsigned>(tag.memoryExpected, MaxMemory);
    while (tag.memorySize < limit && !stopped(io)) {
        const unsigned at = tag.memorySize;
        // READ always fetches four pages. Overlap the previous window when
        // the CC ends on an eight-byte boundary, so no byte beyond it is read.
        const unsigned window = std::min(at, limit - 16);
        const unsigned offset = at - window;
        received = 0;
        if (!io.exchange(io.context, MemoryCommand::Type2Read, window / 4,
                         bytes, sizeof(bytes), received) || received != 16) return;
        const size_t count = std::min<unsigned>(16 - offset, limit - at);
        std::memcpy(tag.memory + at, bytes + offset, count); tag.memorySize += count;
        pause(io);
    }
    if (tag.memorySize == limit)
        tag.memoryState = limit == tag.memoryExpected ? MemoryState::Read : MemoryState::Limited;
}
void captureIso15693Memory(Tag& tag, const MemoryIo& io) {
    if (tag.tech != Tech::V || tag.uidSize != 8) return;
    resetMemory(tag);
    if (!io.exchange || stopped(io)) return;
    uint8_t bytes[48]{}; uint16_t received = 0;
    if (!io.exchange(io.context, MemoryCommand::VSystemInfo, 0, bytes, sizeof(bytes), received) ||
        received < 10 || received > sizeof(bytes) || (bytes[0] & 1)) return;
    // Get System Information echoes the UID in on-air (LSB first) order.
    for (size_t n = 0; n < 8; ++n) if (bytes[n + 2] != tag.uid[7 - n]) return;
    const uint8_t flags = bytes[1];
    const size_t memoryAt = 10 + bool(flags & 1) + bool(flags & 2);
    const size_t required = memoryAt + ((flags & 4) ? 2 : 0) + bool(flags & 8);
    if (received < required) return;
    tag.auxSize = std::min<size_t>(received, sizeof(tag.aux));
    std::memcpy(tag.aux, bytes, tag.auxSize);
    if (!(flags & 4)) { tag.memoryState = MemoryState::NotRead; return; }
    const unsigned blocks = unsigned(bytes[memoryAt]) + 1;
    const unsigned blockSize = (bytes[memoryAt + 1] & 31) + 1;
    tag.memoryExpected = blocks * blockSize;
    for (unsigned block = 0; block < blocks && !stopped(io); ++block) {
        if (tag.memorySize + blockSize > MaxMemory) { tag.memoryState = MemoryState::Limited; return; }
        received = 0;
        if (!io.exchange(io.context, MemoryCommand::VReadBlock, block, bytes, sizeof(bytes), received) ||
            received != blockSize + 1 || (bytes[0] & 1)) {
            tag.memoryState = tag.memorySize ? MemoryState::Partial : MemoryState::Failed; return;
        }
        std::memcpy(tag.memory + tag.memorySize, bytes + 1, blockSize); tag.memorySize += blockSize;
        pause(io);
    }
    tag.memoryState = tag.memorySize == tag.memoryExpected ? MemoryState::Read :
                      tag.memorySize ? MemoryState::Partial : MemoryState::Failed;
}
} // namespace meow_nfc
