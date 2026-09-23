#pragma once
#include <stdbool.h>
#include <stdint.h>
class DEVICES;
bool system_sound_init(DEVICES* dev);
void system_sound_set_volume(int percent);
void system_sound_set_key_enabled(bool enabled);
void system_sound_play_boot(uint32_t max_duration_ms);
void system_sound_play_button(void);
void system_sound_stop(void);
void system_sound_suspend(void);
bool system_sound_resume(void);
