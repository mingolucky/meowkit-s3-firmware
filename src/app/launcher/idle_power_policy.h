#pragma once
#include <cstdint>

namespace meowkit {
inline bool shouldIdlePowerOff(uint32_t idleSeconds,int displayTimeoutSeconds,
                               bool charging,bool vbusPresent,bool storageActive) {
    uint32_t threshold=displayTimeoutSeconds>0?uint32_t(displayTimeoutSeconds)*5/2:300u;
    if(threshold<300u) threshold=300u;
    return idleSeconds>=threshold && !charging && !vbusPresent && !storageActive;
}
} // namespace meowkit
