#pragma once
#include "infrared_signal.h"
#include <Arduino.h>

namespace meow_ir::ui {
static const char* const EasyNames[] = {
    "Power", "Vol_up", "Vol_dn", "Mute", "Ch_up", "Ch_dn", "Ok", "Up",
    "Down", "Left", "Right", "Menu", "Back", "Play", "Pause", "Stop",
    "Next", "Prev", "FF", "Rew", "Input", "Exit", "Eject", "Subtitle"
};
// 40 character cells plus four full-width control keys. The symbol page
// contains every printable ASCII punctuation character and a space.
static constexpr char LetterKeys[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-. ";
static constexpr char SymbolKeys[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ ";
constexpr int CharacterCells = 40, KeyCase = 40, KeySymbols = 41, KeyDelete = 42, KeySave = 43;
constexpr int KeyCount = 44;
inline int characterCount(bool symbols) { return int(symbols ? sizeof(SymbolKeys) - 1 : sizeof(LetterKeys) - 1); }
inline char keyCharacter(int key, bool symbols, bool lowercase) {
    if (key < 0 || key >= characterCount(symbols)) return 0;
    char c = symbols ? SymbolKeys[key] : LetterKeys[key];
    return lowercase && c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c;
}
constexpr size_t EasyCount = sizeof(EasyNames) / sizeof(EasyNames[0]);
inline const char* decodeName(uint8_t mode) { return mode == 1 ? "Decode" : mode == 2 ? "RAW" : "Auto"; }
inline String clipped(const String& value, size_t length = 33) {
    return value.length() > length ? value.substring(0, length - 1) + "~" : value;
}
inline const char* protocolName(const Signal& signal) {
    if (signal.raw) return "RAW";
    return signal.protocol == InfraredProtocolUnknown ? signal.legacyProtocol.c_str() :
        infrared_get_protocol_name(signal.protocol);
}
}
