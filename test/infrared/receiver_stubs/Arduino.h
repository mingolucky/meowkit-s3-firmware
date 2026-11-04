#pragma once

// Hardware boundary for compiling the real IRrecv.cpp ESP32 branch on a host.
// Do not define UNIT_TEST: it removes the timer/interrupt cleanup under test.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

using String = std::string;
using gpio_num_t = int;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(mux) ((void)(mux))
#define portEXIT_CRITICAL(mux) ((void)(mux))
#define IRAM_ATTR
#define INPUT 0
#define INPUT_PULLUP 1
#define CHANGE 2
#define SOC_TIMER_GROUP_TOTAL_TIMERS 4

struct hw_timer_t { bool active = false; void (*callback)() = nullptr; };
struct HostESP { void restart(); };
extern HostESP ESP;

void pinMode(uint8_t pin, int mode);
void attachInterrupt(uint8_t pin, void (*callback)(), int mode);
void detachInterrupt(uint8_t pin);
void gpio_intr_enable(gpio_num_t pin);
void gpio_intr_disable(gpio_num_t pin);
uint32_t micros();
hw_timer_t* timerBegin(uint8_t number, uint16_t divider, bool countUp);
void timerAlarmWrite(hw_timer_t* timer, uint64_t value, bool autoreload);
void timerAttachInterrupt(hw_timer_t* timer, void (*callback)(), bool edge);
void timerAlarmDisable(hw_timer_t* timer);
void timerAlarmEnable(hw_timer_t* timer);
void timerDetachInterrupt(hw_timer_t* timer);
void timerWrite(hw_timer_t* timer, uint64_t value);
void timerEnd(hw_timer_t* timer);
