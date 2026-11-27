#pragma once
#include "sdk.h"
#include "LovyanGFX.hpp"
class Button_Class {
public:
    static constexpr bool RELEASED=true;
    bool event=false;
    bool pressed() { bool result=event; event=false; return result; }
    bool state() const { return RELEASED; }
};
class DEVICES {
public:
    LGFX_Class Lcd;
    struct {
        Button_Class A,B,Up,Down,Left,Right;
        void update() {}
        void tick() {}
    } button;
    struct {
        bool touched=false;
        int x=0,y=0;
        bool readPoint(int& a,int& b) { a=x; b=y; return touched; }
    } ctp;
};
