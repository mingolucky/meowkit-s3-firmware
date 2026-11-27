#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace meow_ir {
struct BuiltinAsset { const char* name; const uint8_t* data; size_t length,compressedLength; };
struct BuiltinBuffer { std::shared_ptr<uint8_t> data; size_t length; };
const BuiltinAsset* builtinAsset(const char* name);
std::shared_ptr<const BuiltinBuffer> openBuiltin(const char* name);
void releaseBuiltinCache();
}
