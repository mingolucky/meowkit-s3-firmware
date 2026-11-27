#pragma once
#include <zlib.h>
#include <cstddef>
constexpr int TINFL_FLAG_PARSE_ZLIB_HEADER=1;
constexpr int TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF=4,TINFL_STATUS_DONE=0;
struct tinfl_decompressor {};
inline void tinfl_init(tinfl_decompressor*) {}
inline int tinfl_decompress(tinfl_decompressor*,const void* data,size_t* input,void*,void* out,size_t* output,int) {
    uLongf n=*output; uLong consumed=*input;
    const int result=uncompress2(static_cast<Bytef*>(out),&n,static_cast<const Bytef*>(data),&consumed);
    *output=n; *input=consumed;
    return result==Z_OK?TINFL_STATUS_DONE:-1;
}
