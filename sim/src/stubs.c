/* Bouchons de bureau pour tout ce que src/ui attend du matériel.
 *
 * Ils rendent des valeurs plausibles plutôt que des zéros : une UI nourrie de
 * données réalistes révèle les vrais défauts de mise en page (débordements de
 * libellés, troncatures) qu'un écran vide masquerait. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "nvs.h"
#include "esp_system.h"
#include "ui_rtc_bridge.h"
#include "ui_sd_bridge.h"
#include "ui_wifi_bridge.h"

/* ── Arduino ────────────────────────────────────────────────────────── */
uint32_t millis(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}

void delay(uint32_t ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

/* ── ESP-IDF ────────────────────────────────────────────────────────── */
void esp_restart(void)
{
    printf("[stub] esp_restart() — le firmware redémarrerait ici.\n");
    exit(0);
}

esp_err_t nvs_get_stats(const char *part_name, nvs_stats_t *stats)
{
    (void)part_name;
    if (!stats) return -1;
    stats->used_entries    = 312;
    stats->free_entries    = 3784;
    stats->total_entries   = 4096;
    stats->namespace_count = 4;
    return ESP_OK;
}

/* ── RTC (PCF8563) → horloge système ────────────────────────────────── */
void ui_rtc_bridge_register(void *pcf8563_ptr) { (void)pcf8563_ptr; }

bool ui_rtc_bridge_get(uint16_t *year, uint8_t *month, uint8_t *day,
                       uint8_t *weekday,
                       uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    time_t t = time(NULL);
    struct tm tm_now;
    localtime_r(&t, &tm_now);
    if (year)    *year    = (uint16_t)(tm_now.tm_year + 1900);
    if (month)   *month   = (uint8_t)(tm_now.tm_mon + 1);
    if (day)     *day     = (uint8_t)tm_now.tm_mday;
    if (weekday) *weekday = (uint8_t)tm_now.tm_wday;
    if (hour)    *hour    = (uint8_t)tm_now.tm_hour;
    if (minute)  *minute  = (uint8_t)tm_now.tm_min;
    if (second)  *second  = (uint8_t)tm_now.tm_sec;
    return true;
}

bool ui_rtc_bridge_set_date(uint16_t y, uint8_t m, uint8_t d) { (void)y;(void)m;(void)d; return true; }
bool ui_rtc_bridge_set_time(uint8_t h, uint8_t m, uint8_t s)  { (void)h;(void)m;(void)s; return true; }
bool ui_rtc_bridge_set_weekday(uint8_t w)                     { (void)w; return true; }

/* ── Carte SD ───────────────────────────────────────────────────────── */
int ui_sd_present(void) { return 1; }

int ui_sd_list_dir(const char *path, ui_sd_visit_cb cb, void *user)
{
    (void)path;
    if (!cb) return 0;
    /* Reprend l'arborescence de « sd files » du dépôt. */
    cb("badusb",   1, user);
    cb("infrared", 1, user);
    cb("music",    1, user);
    cb("vu_meter", 1, user);
    cb("assets",   1, user);
    cb("README.txt", 0, user);
    return 6;
}

uint64_t ui_sd_total_bytes(void) { return 32ULL * 1000 * 1000 * 1000; }
uint64_t ui_sd_used_bytes(void)  { return  7ULL * 1000 * 1000 * 1000; }

/* ── WiFi ───────────────────────────────────────────────────────────── */
static const char *AP_SSID[] = {
    "Freebox-8A2C1D", "Livebox-4021", "SFR_1A2B",
    "un-ssid-volontairement-tres-long-pour-tester", "iPhone de Kevin",
};
static const int AP_PCT[] = { 92, 71, 55, 38, 24 };
#define AP_COUNT ((int)(sizeof(AP_SSID) / sizeof(AP_SSID[0])))

static wifi_bridge_status_t g_status = WIFI_BRIDGE_CONNECTED;

void ui_wifi_bridge_init(void) {}
void ui_wifi_bridge_scan(void) {}
int  ui_wifi_bridge_get_scan_count(void) { return AP_COUNT; }

void ui_wifi_bridge_get_ssid(int idx, char *buf, int len)
{
    if (!buf || len <= 0) return;
    snprintf(buf, (size_t)len, "%s", (idx >= 0 && idx < AP_COUNT) ? AP_SSID[idx] : "");
}

int  ui_wifi_bridge_get_rssi(int idx)      { return (idx >= 0 && idx < AP_COUNT) ? -40 - idx * 12 : -99; }
bool ui_wifi_bridge_is_encrypted(int idx)  { return idx != 4; }
void ui_wifi_bridge_start_scan(void) {}
void ui_wifi_bridge_stop_scan(void)  {}
void ui_wifi_bridge_connect(const char *ssid, const char *pass) { (void)ssid; (void)pass; g_status = WIFI_BRIDGE_CONNECTED; }
void ui_wifi_bridge_connect_saved(const char *ssid)             { (void)ssid; g_status = WIFI_BRIDGE_CONNECTED; }
void ui_wifi_bridge_set_pending_ssid(const char *ssid)          { (void)ssid; }
void ui_wifi_bridge_connect_pending(const char *password)       { (void)password; g_status = WIFI_BRIDGE_CONNECTED; }
void ui_wifi_bridge_disconnect(void)                            { g_status = WIFI_BRIDGE_IDLE; }

wifi_bridge_status_t ui_wifi_bridge_get_status(void) { return g_status; }
bool ui_wifi_bridge_is_scanning(void)     { return false; }
bool ui_wifi_bridge_scan_complete(void)   { return true; }

bool ui_wifi_bridge_get_ap(int i, char *ssid, int ssid_len, int *pct_out, bool *saved_out)
{
    if (i < 0 || i >= AP_COUNT) return false;
    if (ssid && ssid_len > 0) snprintf(ssid, (size_t)ssid_len, "%s", AP_SSID[i]);
    if (pct_out)   *pct_out   = AP_PCT[i];
    if (saved_out) *saved_out = (i == 0);
    return true;
}

void ui_wifi_bridge_get_status_text(char *buf, int len)
{
    if (!buf || len <= 0) return;
    snprintf(buf, (size_t)len, "connect: %s", AP_SSID[0]);
}

bool ui_wifi_bridge_is_connected(void) { return g_status == WIFI_BRIDGE_CONNECTED; }
void ui_wifi_bridge_get_ip(char *buf, int len) { if (buf && len > 0) snprintf(buf, (size_t)len, "192.168.1.42"); }
const char *ui_wifi_bridge_get_current_ssid(void) { return AP_SSID[0]; }
void ui_wifi_bridge_forget(const char *ssid) { (void)ssid; }
void ui_wifi_bridge_get_saved_pass(char *buf, int len) { if (buf && len > 0) snprintf(buf, (size_t)len, "motdepasse"); }

/* ── Réglages, alimentation, USB ─────────────────────────────────────
 * On inclut les vrais en-têtes du firmware : si l'amont change une
 * signature, la compilation casse ici au lieu de dériver en silence. */
#include "system/settings_bridge.h"
#include "system/power_mgmt.h"
#include "system/persist.h"
#include "system/recovery.h"
#include "system/usb_msc.h"
#include "system/usb_manager.h"

static int  g_volume     = 60;
static int  g_brightness = 80;
static int  g_led        = 50;
static bool g_key_sound  = true;
static bool g_ble_en     = false;
static bool g_wifi_en    = true;

int  sys_get_volume(void)     { return g_volume; }
int  sys_get_brightness(void) { return g_brightness; }
int  sys_get_led(void)        { return g_led; }
bool sys_get_key_sound(void)  { return g_key_sound; }

bool settings_get_ble_en(void)  { return g_ble_en; }
bool settings_get_wifi_en(void) { return g_wifi_en; }

void settings_set_ble_en(int en)       { g_ble_en    = en != 0; }
void settings_set_wifi_en(int en)      { g_wifi_en   = en != 0; }
void settings_set_brightness(int pct)  { g_brightness = pct; }
void settings_set_volume(int pct)      { g_volume     = pct; }
void settings_set_led_bright(int pct)  { g_led        = pct; }
void settings_set_key_sound(int on)    { g_key_sound  = on != 0; }

int  power_battery_pct(void) { return 76; }
bool power_is_charging(void) { return true; }

bool persist_get_str(const char *key, char *buf, size_t len, const char *default_val)
{
    (void)key;
    if (!buf || len == 0) return false;
    snprintf(buf, len, "%s", default_val ? default_val : "");
    return true;
}

void recovery_factory_reset(void)
{
    printf("[stub] recovery_factory_reset() — réinitialisation d'usine simulée.\n");
}

int usb_msc_is_active(void) { return 0; }
int usb_manager_request(usb_mode_t mode) { (void)mode; return 0; }
uint64_t usb_msc_bytes_transferred(void) { return 0; }
