#pragma once
#include <Arduino.h>
namespace meow_nfc::ui {
static const char* const Keys[] = {
    "A","B","C","D","E","F","G","H","I","J",
    "K","L","M","N","O","P","Q","R","S","T",
    "U","V","W","X","Y","Z","0","1","2","3",
    "4","5","6","7","8","9","_","-","SP","aA","DEL","OK"
};
static const char* const HexKeys[] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F","DEL","OK"};
constexpr int KeyCount = sizeof(Keys) / sizeof(Keys[0]);
constexpr int HexKeyCount = sizeof(HexKeys) / sizeof(HexKeys[0]);
inline String clipped(const String& s, unsigned max = 33) { return s.length() > max ? s.substring(0, max - 1) + "~" : s; }
} // namespace meow_nfc::ui
