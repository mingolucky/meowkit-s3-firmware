#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace meow_nfc {
constexpr size_t MaxMemory = 1024, MaxNdef = 2048, MaxFile = 128 * 1024;
enum class Tech : uint8_t { Unknown, A, B, F, V };
enum class NdefState : uint8_t { NotRead, Absent, Read, TooLarge, Failed };
enum class MemoryState : uint8_t { NotRead, Read, Partial, Limited, Failed };
struct Tag {
    Tech tech = Tech::Unknown;
    uint8_t uid[10]{}, uidSize = 0;
    // ATQA in on-air (LSB first) order. Flipper v4 files store MSB first.
    uint8_t atqa[2]{}, sak = 0;
    uint8_t aux[32]{}, auxSize = 0;
    uint8_t memory[MaxMemory]{}, ndef[MaxNdef]{};
    uint16_t memorySize = 0, memoryExpected = 0, ndefSize = 0;
    uint32_t ndefExpected = 0;
    NdefState ndefState = NdefState::NotRead;
    MemoryState memoryState = MemoryState::NotRead;
    bool manual = false, imported = false;
    char protocol[32]{};
};
const char* techName(Tech tech);
const char* tagName(const Tag& tag);
const char* memoryStateName(MemoryState state);
const char* ndefStateName(NdefState state);
bool validUid(Tech tech, size_t size);
bool validName(const std::string& name);
std::string hex(const uint8_t* data, size_t length, bool spaced = true);
bool parseHex(const std::string& value, uint8_t* out, size_t capacity, size_t& count);
bool parseCapture(const std::string& text, Tag& tag, std::string& error);
std::string serializeCapture(const Tag& tag);
// Bounded NDEF parser: returns display lines, handles Text/URI and other records.
bool describeNdef(const uint8_t* data, size_t length, std::string& description);
} // namespace meow_nfc
