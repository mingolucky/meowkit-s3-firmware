#pragma once
#include "infrared_signal.h"

namespace meow_ir {
struct NativeToggleState {
    bool initialized=false,bit=false;
    uint64_t apply(uint64_t value,uint64_t mask,bool repeat) {
        if(!initialized) { bit=(value&mask)!=0; initialized=true; }
        if(!repeat) bit=!bit;
        return bit?(value|mask):(value&~mask);
    }
};
// Native protocols include integer codes and full AC state arrays. Keep
// their encoding explicit so names shared with Momentum reload correctly.
inline bool learnNative(Signal& signal,const std::string& protocol,uint16_t bits,uint64_t value,
                        const uint8_t* state=nullptr,size_t stateBytes=0) {
    if(protocol.empty() || protocol=="UNKNOWN" || !bits ||
        (stateBytes && (!state || stateBytes>MaxNativeStateBytes || stateBytes!=(size_t(bits)+7)/8)) ||
        (!stateBytes && (bits>64 || (bits<64 && (value>>bits))))) return false;
    signal.raw=false;
    signal.protocol=InfraredProtocolUnknown;
    signal.address=signal.command=0;
    signal.legacyProtocol=protocol;
    signal.legacyBits=bits;
    signal.legacyValue=value;
    signal.legacyState.clear();
    if(stateBytes) signal.legacyState.assign(state,state+stateBytes);
    signal.timings.clear();
    return true;
}
}
