#pragma once
#include <cstdint>
#include <cstring>
namespace meow_storage {
// IDF 4.4's frequency ladder omits clocks below 20 MHz and leaves a 10 MHz
// request at its 400 kHz probe clock. Explicitly set and verify a conservative
// clock before advertising media or allowing filesystem writes.
template<class Read, class SetClock>
bool verifySDClock(uint32_t frequency, uint8_t* dma, const uint8_t* baseline,
                   Read read, SetClock setClock) {
    if (!frequency || !dma || !baseline || uintptr_t(dma)%4) return false;
    if (setClock(frequency)!=0 || read(dma)!=0) return false;
    return std::memcmp(dma, baseline, 512)==0;
}
}
