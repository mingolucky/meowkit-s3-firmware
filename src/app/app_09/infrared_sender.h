#pragma once
#include <Arduino.h>
#include <IRsend.h>
#include <algorithm>

namespace meow_ir {
class PwmIRsend : public IRsend {
    // Channel 0 / timer 0 is dedicated to IR. Backlight uses 7 / timer 3;
    // the buzzer uses 4 / timer 2. Do not share their frequency generators.
    static constexpr uint8_t Channel=0;
    uint8_t _pin,_bits=8,_duty=50;
    double _frequency=38000;
    bool _attached=false,_ready=false,_fault=false;
public:
    explicit PwmIRsend(uint16_t pin):IRsend(pin),_pin(uint8_t(pin)) {}
    PwmIRsend(const PwmIRsend&)=delete;
    PwmIRsend& operator=(const PwmIRsend&)=delete;
    ~PwmIRsend() {
        if(_attached) { ledcWrite(Channel,0); ledcDetachPin(_pin); }
        pinMode(_pin,OUTPUT); digitalWrite(_pin,LOW);
    }
    void begin() override {
        IRsend::begin(); _fault=false;
        enableIROut(38000,50);
        if(_ready) { ledcWrite(Channel,0); ledcAttachPin(_pin,Channel); _attached=true; }
    }
    void resetFrameStatus() { _fault=false; }
    void enableIROut(uint32_t frequency,uint8_t duty=kDutyDefault) override {
        if(frequency<1000) frequency*=1000; // IRremote's kHz shorthand
        _duty=std::min(duty,uint8_t(100));
        _bits=frequency>600000?5:frequency>150000?6:8;
        if(_attached || _ready) ledcWrite(Channel,0);
        _frequency=frequency?ledcSetup(Channel,frequency,_bits):0;
        _ready=_frequency>0;
        if(!_ready) _fault=true; // latched until the next complete message
    }
    uint16_t mark(uint16_t usec) override {
        if(!usec || !carrierReady()) return 0;
        const uint32_t maximum=(1U<<_bits)-1;
        ledcWrite(Channel,(maximum*_duty+50)/100);
        _delayMicroseconds(usec);
        ledcWrite(Channel,0);
        return uint16_t(std::min(65535.0,(double(usec)*_frequency+999999)/1000000));
    }
    void space(uint32_t usec) override {
        if(_attached || _ready) ledcWrite(Channel,0);
        if(usec) _delayMicroseconds(usec);
    }
    bool carrierReady() const { return _attached && _ready && !_fault; }
    double carrierFrequency() const { return _frequency; }
};
}
