#pragma once
#include "infrared_signal.h"
namespace meow_ir {
inline bool makePowerPair(const Signal& on,const Signal& off,Signal& result,std::string& error) {
    if(on.powerOff || off.powerOff || !validSignal(on) || !validSignal(off)) {
        error="Invalid power signal"; return false;
    }
    if(samePayload(on,off)) { error="Same code: switch original remote OFF and retry"; return false; }
    Signal pair=on;
    pair.powerOff=std::make_shared<const Signal>(off);
    pair.powerNextOn=true;
    result=std::move(pair); return true;
}
struct PowerPress {
    bool pending=false,sentOn=true;
    const Signal* begin(const Signal& signal,bool edge,bool busy) {
        if(!edge || busy || pending || !signal.powerOff) return nullptr;
        pending=true; sentOn=signal.powerNextOn;
        return sentOn?&signal:signal.powerOff.get();
    }
    bool finish(Signal& signal,bool success) {
        if(!pending) return false;
        pending=false;
        if(success) signal.powerNextOn=!sentOn;
        return success;
    }
};
}
