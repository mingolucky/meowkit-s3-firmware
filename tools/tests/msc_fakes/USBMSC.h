#pragma once
#include "fake_state.h"
#include <cassert>
class USBMSC {
public:
 inline static USBMSC* instance=nullptr;
 int32_t (*read)(uint32_t,uint32_t,void*,uint32_t)=nullptr;
 int32_t (*write)(uint32_t,uint32_t,uint8_t*,uint32_t)=nullptr;
 bool (*startStop)(uint8_t,bool,bool)=nullptr;
 USBMSC(){instance=this;}
 void vendorID(const char*){}void productID(const char*){}void productRevision(const char*){}
 void onRead(decltype(read) f){read=f;}void onWrite(decltype(write) f){write=f;}void onStartStop(decltype(startStop) f){startStop=f;}
 void mediaPresent(bool value){if(value)assert(msc_fake::begun);msc_fake::present=value;}
 bool begin(uint32_t blocks,uint16_t size){assert(blocks==16000000 && size==512 && read && write && !msc_fake::present);msc_fake::begun=!msc_fake::failBegin;return msc_fake::begun;}
 void end(){++msc_fake::ends;msc_fake::present=false;msc_fake::begun=false;read=nullptr;write=nullptr;}
};
