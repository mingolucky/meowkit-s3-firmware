#include "infrared_builtins.h"
#include <rom/miniz.h>
#include <new>
#define IR_ASSETS(X) X(tv) X(projectors) X(audio) X(ac) X(leds) X(fans) X(bluray_dvd) X(monitor) X(digital_sign)
extern "C" {
#define DECLARE(name) extern const uint8_t ir_##name##_start[], ir_##name##_end[];
IR_ASSETS(DECLARE)
#undef DECLARE
}
namespace meow_ir {
namespace {
size_t decodedLength(const uint8_t* data) {
    return size_t(data[0])|(size_t(data[1])<<8)|(size_t(data[2])<<16)|(size_t(data[3])<<24);
}
const BuiltinAsset* cachedAsset=nullptr;
std::shared_ptr<const BuiltinBuffer> cached;
}
const BuiltinAsset* builtinAsset(const char* name) {
#define ENTRY(name) {#name,ir_##name##_start+4,decodedLength(ir_##name##_start),size_t(ir_##name##_end-ir_##name##_start)-4},
    static const BuiltinAsset assets[]={IR_ASSETS(ENTRY)};
#undef ENTRY
    for (const auto& asset:assets) if (!strcmp(name,asset.name)) return &asset;
    return nullptr;
}
std::shared_ptr<const BuiltinBuffer> openBuiltin(const char* name) {
    const auto* asset=builtinAsset(name);
    if (!asset || !asset->length || asset->length>512*1024) return {};
    if (asset==cachedAsset && cached) return cached;
    auto buffer=std::make_shared<BuiltinBuffer>();
    buffer->data=std::shared_ptr<uint8_t>(new (std::nothrow) uint8_t[asset->length],std::default_delete<uint8_t[]>());
    if (!buffer->data) return {};
    // ROM's convenience inflater puts an 11-KiB decoder on the loop stack.
    // Allocate it here so opening a library cannot exhaust the 16-KiB task.
    std::unique_ptr<tinfl_decompressor> decoder(new (std::nothrow) tinfl_decompressor);
    if (!decoder) return {};
    tinfl_init(decoder.get());
    size_t input=asset->compressedLength,output=asset->length;
    const auto status=tinfl_decompress(decoder.get(),asset->data,&input,buffer->data.get(),
        buffer->data.get(),&output,TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (status!=TINFL_STATUS_DONE || output!=asset->length || input!=asset->compressedLength) return {};
    buffer->length=output; cached=buffer; cachedAsset=asset; return buffer;
}
void releaseBuiltinCache() { cached.reset(); cachedAsset=nullptr; }
}
