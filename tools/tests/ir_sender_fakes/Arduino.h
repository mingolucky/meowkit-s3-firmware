#pragma once
#include <cstdint>
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef LOW
#define LOW 0
#define HIGH 1
#endif
void pinMode(uint16_t,uint8_t);
void digitalWrite(uint16_t,uint8_t);
double ledcSetup(uint8_t,double,uint8_t);
void ledcWrite(uint8_t,uint32_t);
void ledcAttachPin(uint8_t,uint8_t);
void ledcDetachPin(uint8_t);
