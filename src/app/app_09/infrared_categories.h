#pragma once
// Action names and order follow Momentum universal scenes (GPL-3.0).
namespace meow_ir {
struct Category { const char* title; const char* file; const char* const* actions; int count; };
static const char* const Actions0[] = {"Power", "Mute", "Vol_up", "Ch_next", "Vol_dn", "Ch_prev"};
static const char* const Actions1[] = {"Power", "Mute", "Vol_up", "Vol_dn", "Play", "Pause"};
static const char* const Actions2[] = {"Power", "Mute", "Play", "Pause", "Prev", "Next", "Vol_dn", "Vol_up"};
static const char* const Actions3[] = {"Off", "Dh", "Cool_hi", "Heat_hi", "Cool_lo", "Heat_lo"};
static const char* const Actions4[] = {"Power_on", "Power_off", "Brightness_up", "Brightness_dn", "Red", "Green", "Blue", "White"};
static const char* const Actions5[] = {"Power", "Mode", "Speed_up", "Speed_dn", "Rotate", "Timer"};
static const char* const Actions6[] = {"Power", "Eject", "Play", "Pause", "Fast_ba", "Fast_fo", "Ok", "Subtitle"};
static const char* const Actions7[] = {"POWER", "SOURCE", "MENU", "EXIT"};
static const char* const Actions8[] = {"POWER", "SOURCE", "PLAY", "STOP"};
static const Category Categories[] = {
    {"TVs", "tv", Actions0, int(sizeof(Actions0) / sizeof(const char*))},
    {"Projectors", "projectors", Actions1, int(sizeof(Actions1) / sizeof(const char*))},
    {"Audio", "audio", Actions2, int(sizeof(Actions2) / sizeof(const char*))},
    {"ACs", "ac", Actions3, int(sizeof(Actions3) / sizeof(const char*))},
    {"LEDs", "leds", Actions4, int(sizeof(Actions4) / sizeof(const char*))},
    {"Fans", "fans", Actions5, int(sizeof(Actions5) / sizeof(const char*))},
    {"Blu-ray/DVDs", "bluray_dvd", Actions6, int(sizeof(Actions6) / sizeof(const char*))},
    {"Monitors", "monitor", Actions7, int(sizeof(Actions7) / sizeof(const char*))},
    {"Digital Signs", "digital_sign", Actions8, int(sizeof(Actions8) / sizeof(const char*))},
};
} // namespace meow_ir
