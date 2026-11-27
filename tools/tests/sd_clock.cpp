#include "../../src/system/sd_clock.h"
#include <array>
#include <cassert>
#include <cstdio>
int main(){
 alignas(4) std::array<uint8_t,512> dma{},baseline{};baseline.fill(0x5a);
 unsigned selected=0,reads=0;bool failSet=false,failRead=false,corrupt=false;
 auto set=[&](uint32_t f){selected=f;return failSet?-1:0;};
 auto read=[&](uint8_t* p){++reads;memcpy(p,baseline.data(),512);if(corrupt)p[511]^=1;return failRead?-1:0;};
 auto verify=[&](uint32_t f){return meow_storage::verifySDClock(f,dma.data(),baseline.data(),read,set);};
 assert(verify(10000) && selected==10000 && reads==1);
 failSet=true;assert(!verify(5000) && reads==1);failSet=false;
 failRead=true;assert(!verify(5000));failRead=false;
 corrupt=true;assert(!verify(5000));corrupt=false;
 assert(verify(1000) && selected==1000);
 selected=0;assert(!verify(0) && selected==0);
 assert(!meow_storage::verifySDClock(1000,nullptr,baseline.data(),read,set));
 puts("SD clock checks passed (selection, I/O verification, failure and corruption gating)");
}
