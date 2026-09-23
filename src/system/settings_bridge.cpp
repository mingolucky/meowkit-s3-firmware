/**
 * @file settings_bridge.cpp
 * @brief Hardware bridge: apply settings to drivers + coalesced NVS persistence.
 *
 * Write coalescing strategy:
 *   Each settings_set_*() call marks a per-setting dirty bit and records the
 *   timestamp.  settings_tick() (called every main-loop iteration) flushes all
 *   dirty bits to NVS only after SETTINGS_FLUSH_DEBOUNCE_MS of quiet, batching
 *   rapid slider drags into a single NVS write cycle and protecting flash wear.
 */
#include "settings_bridge.h"
#include "persist.h"
#include "system_sound.h"
#include <Arduino.h>

/* ── Hardware handle ─────────────────────────────────────────── */

static DEVICES* s_dev = nullptr;

void sys_settings_bridge_attach(DEVICES* dev)
{
    s_dev = dev;
}

/* ── Cached settings state ───────────────────────────────────── */

static int      s_brightness   = 50;
static int      s_disp_timeout = 60;
static int      s_volume       = 50;
static bool     s_key_sound    = true;
static int      s_led          = 30;
static uint32_t s_led_color    = 0xFFFFFF;  /* white */
static int      s_led_effect   = 1;         /* SOLID */
static bool     s_wifi_en      = true;
static char     s_ble_name[21] = "MeowKit";
static bool     s_ble_en       = false;

/* ── Dirty-flag write coalescing ─────────────────────────────── */

enum DirtyBit : uint32_t {
    D_BRIGHT     = 1u <<  0,
    D_DISP_TO    = 1u <<  1,
    D_VOL        = 1u <<  2,
    D_KEY_SND    = 1u <<  3,
    D_LED_BRIGHT = 1u <<  4,
    D_LED_COLOR  = 1u <<  5,
    D_LED_FX     = 1u <<  6,
    D_WIFI_EN    = 1u <<  7,
    D_BLE_NAME   = 1u <<  8,
    D_BLE_EN     = 1u <<  9,
};

static uint32_t s_dirty       = 0;
static uint32_t s_last_change = 0;

#define MARK_DIRTY(bit)  do { s_dirty |= (bit); s_last_change = millis(); } while(0)

/* ── Internal: apply LED color+effect to hardware ────────────── */

static void _apply_led_fx(void)
{
    if (!s_dev) return;
    uint8_t r = (uint8_t)((s_led_color >> 16) & 0xFF);
    uint8_t g = (uint8_t)((s_led_color >>  8) & 0xFF);
    uint8_t b = (uint8_t)( s_led_color        & 0xFF);
    s_dev->led.setEffect((WS2812B_Class::Effect)s_led_effect, r, g, b);
}

/* ════════════════════════════════════════════════════════════════
 * Lifecycle
 * ════════════════════════════════════════════════════════════════ */

void settings_init(void)
{
    persist_init();
}

void settings_load_all(void)
{
    /* Load + apply display */
    sys_apply_brightness(persist_get_int(PKEY_BRIGHTNESS,   50));
    s_disp_timeout = persist_get_int(PKEY_DISP_TIMEOUT,     60);

    /* Load + apply audio */
    sys_apply_volume    (persist_get_int(PKEY_VOLUME,        50));
    sys_apply_key_sound (persist_get_int(PKEY_KEY_SOUND,      1));

    /* Load + apply LED */
    sys_apply_led       (persist_get_int(PKEY_LED_BRIGHT,    30));
    s_led_color  = persist_get_u32(PKEY_LED_COLOR,  0xFFFFFF);
    s_led_effect = persist_get_int(PKEY_LED_EFFECT, 1 /* SOLID */);
    _apply_led_fx();

    /* Load WiFi / BLE flags (hardware action deferred to respective bridges) */
    s_wifi_en = persist_get_int(PKEY_WIFI_EN, 1) != 0;
    persist_get_str(PKEY_BLE_NAME, s_ble_name, sizeof(s_ble_name), "MeowKit");
    s_ble_en  = persist_get_int(PKEY_BLE_EN,  0) != 0;

    s_dirty = 0;   /* nothing to flush yet */
    Serial.printf("[settings] Loaded — bright=%d vol=%d led=%d fx=%d wifi=%d ble=%d\n",
                  s_brightness, s_volume, s_led, s_led_effect,
                  (int)s_wifi_en, (int)s_ble_en);
}

void settings_flush(void)
{
    if (!s_dirty) return;

    if (s_dirty & D_BRIGHT)     persist_set_int(PKEY_BRIGHTNESS,   s_brightness);
    if (s_dirty & D_DISP_TO)    persist_set_int(PKEY_DISP_TIMEOUT, s_disp_timeout);
    if (s_dirty & D_VOL)        persist_set_int(PKEY_VOLUME,       s_volume);
    if (s_dirty & D_KEY_SND)    persist_set_int(PKEY_KEY_SOUND,    s_key_sound ? 1 : 0);
    if (s_dirty & D_LED_BRIGHT) persist_set_int(PKEY_LED_BRIGHT,   s_led);
    if (s_dirty & D_LED_COLOR)  persist_set_u32(PKEY_LED_COLOR,    s_led_color);
    if (s_dirty & D_LED_FX)     persist_set_int(PKEY_LED_EFFECT,   s_led_effect);
    if (s_dirty & D_WIFI_EN)    persist_set_int(PKEY_WIFI_EN,      s_wifi_en ? 1 : 0);
    if (s_dirty & D_BLE_NAME)   persist_set_str(PKEY_BLE_NAME,     s_ble_name);
    if (s_dirty & D_BLE_EN)     persist_set_int(PKEY_BLE_EN,       s_ble_en ? 1 : 0);

    Serial.printf("[settings] Flushed dirty=0x%03lX\n", (unsigned long)s_dirty);
    s_dirty = 0;
}

void settings_tick(void)
{
    if (s_dirty && (millis() - s_last_change >= SETTINGS_FLUSH_DEBOUNCE_MS)) {
        settings_flush();
    }
}

/* ════════════════════════════════════════════════════════════════
 * Display
 * ════════════════════════════════════════════════════════════════ */

#define BRIGHTNESS_MIN  20   /* 防止完全黑屏：最低亮度 20% */

void sys_apply_brightness(int pct)
{
    if (pct < BRIGHTNESS_MIN) pct = BRIGHTNESS_MIN;
    if (pct > 100)            pct = 100;
    s_brightness = pct;
    if (!s_dev) return;
    s_dev->Lcd.setBrightness((uint8_t)(pct * 255 / 100));
}

void settings_set_brightness(int pct)
{
    sys_apply_brightness(pct);
    MARK_DIRTY(D_BRIGHT);
}

int sys_get_brightness(void) { return s_brightness; }

void settings_set_disp_timeout(int secs)
{
    if (secs < 0) secs = 0;
    s_disp_timeout = secs;
    MARK_DIRTY(D_DISP_TO);
}

int settings_get_disp_timeout(void) { return s_disp_timeout; }

/* ════════════════════════════════════════════════════════════════
 * Audio
 * ════════════════════════════════════════════════════════════════ */

void sys_apply_volume(int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    s_volume = pct;
    if (!s_dev) return;
    /* UI 0-100% → hardware 0-SPK_VOLUME_MAX，滑块全程可用且永不超出扬声器额定功率 */
    system_sound_set_volume(pct);
}

void settings_set_volume(int pct)
{
    sys_apply_volume(pct);
    MARK_DIRTY(D_VOL);
}

int sys_get_volume(void) { return s_volume; }

void sys_apply_key_sound(int on)
{
    s_key_sound = (on != 0);
    system_sound_set_key_enabled(s_key_sound);
    /* No hardware toggle — callers check sys_get_key_sound() before playing */
}

void settings_set_key_sound(int on)
{
    sys_apply_key_sound(on);
    MARK_DIRTY(D_KEY_SND);
}

bool sys_get_key_sound(void) { return s_key_sound; }

/* ════════════════════════════════════════════════════════════════
 * LED
 * ════════════════════════════════════════════════════════════════ */

void sys_apply_led(int pct)
{
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    s_led = pct;
    if (!s_dev) return;
    s_dev->led.setBrightness((uint8_t)(pct * 255 / 100));
}

void settings_set_led_bright(int pct)
{
    sys_apply_led(pct);
    MARK_DIRTY(D_LED_BRIGHT);
}

int sys_get_led(void) { return s_led; }

void settings_set_led_color(uint32_t rgb)
{
    s_led_color = rgb;
    _apply_led_fx();
    MARK_DIRTY(D_LED_COLOR);
}

uint32_t settings_get_led_color(void) { return s_led_color; }

void settings_set_led_effect(int effect)
{
    s_led_effect = effect;
    _apply_led_fx();
    MARK_DIRTY(D_LED_FX);
}

int settings_get_led_effect(void) { return s_led_effect; }

/* ════════════════════════════════════════════════════════════════
 * WiFi
 * ════════════════════════════════════════════════════════════════ */

void settings_set_wifi_en(int en)
{
    s_wifi_en = (en != 0);
    MARK_DIRTY(D_WIFI_EN);
}

bool settings_get_wifi_en(void) { return s_wifi_en; }

/* ════════════════════════════════════════════════════════════════
 * BLE
 * ════════════════════════════════════════════════════════════════ */

void settings_set_ble_name(const char* name)
{
    if (!name) return;
    strncpy(s_ble_name, name, sizeof(s_ble_name) - 1);
    s_ble_name[sizeof(s_ble_name) - 1] = '\0';
    MARK_DIRTY(D_BLE_NAME);
}

void settings_get_ble_name(char* buf, int len)
{
    if (!buf || len <= 0) return;
    strncpy(buf, s_ble_name, (size_t)(len - 1));
    buf[len - 1] = '\0';
}

void settings_set_ble_en(int en)
{
    s_ble_en = (en != 0);
    MARK_DIRTY(D_BLE_EN);
}

bool settings_get_ble_en(void) { return s_ble_en; }
