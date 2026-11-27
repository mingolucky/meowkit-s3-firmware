#pragma once
#include <cstdint>
using esp_event_base_t=int;
constexpr int ARDUINO_USB_CDC_LINE_CODING_EVENT=1;
struct arduino_usb_cdc_event_data_t { struct {uint32_t bit_rate=0;} line_coding; };
struct ManagerSerial {
    using Callback=void(*)(void*,esp_event_base_t,int32_t,void*);
    Callback callback=nullptr;
    void enableReboot(bool) {} void setTxTimeoutMs(int) {}
    void onEvent(int,Callback cb) {callback=cb;}
    void flush() {} void printf(const char*,...) {} void println(const char*) {}
};
extern ManagerSerial Serial;
inline void delay(uint32_t) {}
