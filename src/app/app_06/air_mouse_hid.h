#pragma once
#include <cstdint>

// App-owned, static GATT attributes. No permanently suspended server task or
// Arduino BLEServer object tree survives an Air Mouse session.
namespace air_mouse_hid {
bool begin();
void end();
bool connected();
bool ready();
bool scrollReady();
bool failed();
void buttons(uint8_t mask);
void clickRight();
void tick();
bool move(int8_t x, int8_t y, int8_t wheel = 0);
}
