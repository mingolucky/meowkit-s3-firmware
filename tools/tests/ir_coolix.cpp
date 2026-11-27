#include "IRsend_test.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstring>
#include "ir_Daikin.h"

int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream file(argv[1]); assert(file);
    std::vector<uint16_t> pulses{0}; unsigned duration;
    while(file>>duration) pulses.push_back(duration/kRawTick);
    assert(pulses.size()==200);
    IRrecv receiver(0);
    decode_results learned{};
    learned.rawbuf=pulses.data(); learned.rawlen=pulses.size();
    assert(receiver.decodeCOOLIX(&learned,kStartOffset,24,true));
    assert(learned.decode_type==COOLIX && learned.bits==24);
    for(const auto value:{learned.value,uint64_t(0xB27BE0)}) {
        IRsendTest sender(0);
        assert(sender.send(COOLIX,value,24));
        // Dedicated sender restores the protocol's carrier, duty, timing,
        // inverse bytes, and required second frame.
        assert(sender.freq[0]==38000 && sender.duty[0]==50);
        assert(sender.output[0]==4692 && sender.output[1]==4416);
        assert(sender.output[100]==4692);
        sender.makeDecodeResult();
        assert(receiver.decodeCOOLIX(&sender.capture,kStartOffset,24,true));
        assert(sender.capture.value==value);
    }
    {
        IRsendTest sender(0);
        assert(sender.send(NEC,uint64_t(0x20DF10EF),32)); sender.makeDecodeResult();
        assert(receiver.decodeNEC(&sender.capture,kStartOffset,32,true));
        assert(sender.capture.value==0x20DF10EF);
    }
    {
        IRsendTest sender(0);
        assert(sender.send(SONY,uint64_t(0xa90),12)); sender.makeDecodeResult();
        assert(receiver.decodeSony(&sender.capture,kStartOffset,12,true));
        assert(sender.capture.value==0xa90);
    }
    {
        IRDaikin216 ac(0); ac.setPower(true); ac.setTemp(25);
        uint8_t state[kDaikin216StateLength]; memcpy(state,ac.getRaw(),sizeof(state));
        IRsendTest sender(0);
        assert(sender.send(DAIKIN216,state,sizeof(state))); sender.makeDecodeResult();
        assert(receiver.decodeDaikin216(&sender.capture,kStartOffset,kDaikin216Bits,true));
        assert(!memcmp(sender.capture.state,state,sizeof(state)));
    }
    puts("Native IR checks passed (device COOLIX capture, exact waveform/repeats, NEC, Sony and full-state Daikin roundtrip)");
}
