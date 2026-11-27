#include "../../src/app/app_09/infrared_sender.h"
#include "IRsend_test.h"
#include <cassert>
#include <vector>

static double frequency=0; static uint32_t duty=0;
static uint8_t channel=255,resolution=0,attached=255;
static bool failSetup=false,detached=false,configured=false;
void pinMode(uint16_t,uint8_t) {}
void digitalWrite(uint16_t,uint8_t level) { assert(level==LOW); }
double ledcSetup(uint8_t ch,double hz,uint8_t bits) { channel=ch; frequency=hz; resolution=bits; if(!failSetup) configured=true; return failSetup?0:hz; }
void ledcWrite(uint8_t ch,uint32_t value) { assert(ch==0 && configured); duty=value; }
void ledcAttachPin(uint8_t pin,uint8_t ch) { assert(pin==7 && ch==0); attached=pin; }
void ledcDetachPin(uint8_t pin) { assert(pin==7); detached=true; }

class Sender:public meow_ir::PwmIRsend {
public:
    using PwmIRsend::PwmIRsend;
    std::vector<uint32_t> envelope;
    void _delayMicroseconds(uint32_t us) override {
        if(!us) return;
        const bool on=duty!=0;
        if(envelope.empty()) { assert(on); envelope.push_back(us); }
        else if(on==(envelope.size()%2==1)) envelope.back()+=us;
        else envelope.push_back(us);
    }
};
int main() {
    {
        Sender sender(7); sender.begin();
        assert(sender.carrierReady() && attached==7 && channel==0 && duty==0);
        for(const uint64_t code:{uint64_t(0xB23F30),uint64_t(0xB27BE0),uint64_t(0xB23F30)}) {
            sender.envelope.clear();
            IRsendTest reference(7); assert(reference.send(COOLIX,code,24));
            assert(sender.send(COOLIX,code,24)); // tests protocol -> virtual PWM dispatch
            assert(frequency==38000 && resolution==8 && duty==0);
            assert(sender.envelope.size()==size_t(reference.last)+1);
            for(size_t i=0;i<sender.envelope.size();++i) assert(sender.envelope[i]==reference.output[i]);
        }
        sender.enableIROut(1000000,33); assert(sender.carrierReady() && resolution<=6);
        failSetup=true; sender.enableIROut(38000,50); assert(!sender.carrierReady() && duty==0);
        failSetup=false; sender.enableIROut(38000,50); assert(!sender.carrierReady()); // failure remains latched
        sender.resetFrameStatus(); sender.enableIROut(38000,50); assert(sender.carrierReady());
    }
    assert(detached && duty==0);
    {
        configured=false; failSetup=true; attached=255;
        Sender failed(7); failed.begin();
        assert(!failed.carrierReady() && attached==255);
        assert(failed.mark(552)==0); failed.space(0);
    }
    puts("IR hardware carrier checks passed (native routing, On/Off/On envelopes, dedicated channel, failure cleanup)");
}
