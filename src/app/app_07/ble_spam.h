/**
 * @file app_08.h
 * @author Mingo
 * @brief App07 — BLE Spam (ported from Momentum-Apps / Flipper Zero)
 *        Supports: Apple Continuity, Google FastPair, Samsung EasySetup,
 *                  Microsoft SwiftPair, LoveSpouse, NameFlood, Kitchen Sink
 * @version 4.0
 * @date 2026-05-12
 * @copyright Copyright (c) 2025
 *
 * Original app: https://github.com/Next-Flip/Momentum-Apps
 * Credits: @WillyJL, @ECTO-1A, @Spooks4576, @mandomat
 *
 * v4.0: Migrated UI from LvAppUI (LVGL) to direct LovyanGFX TUI drawing
 *       in the same green/white style used by app_10 (Bad USB).
 */
#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_arduino_version.h>
#include <LovyanGFX.hpp>

/* ── Max TX power per chip variant ── */
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define APP08_MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define APP08_MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define APP08_MAX_TX_POWER ESP_PWR_LVL_P9
#endif

using namespace mooncake;

namespace MOONCAKE::APPS
{

/* ── Page IDs ── */
enum class BsPage : uint8_t { MainMenu, Running };

/* ── TUI colors (green + white, same palette as app_10) ── */
static constexpr uint16_t BS_BG        = TFT_BLACK;
static constexpr uint16_t BS_FG        = 0x07E0;   /* pure green */
static constexpr uint16_t BS_FG_DIM    = 0x03E0;   /* dark green */
static constexpr uint16_t BS_ACCENT    = TFT_WHITE;
static constexpr uint16_t BS_HIGHLIGHT = 0x2E65;   /* dark green fill */

class App07 : public AppAbility {
public:
    App07(DEVICES* device);
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    DEVICES* _device = nullptr;

    /* ── Page / scene ── */
    BsPage _page = BsPage::MainMenu;
    bool   _sceneDirty = true;
    void   _switchPage(BsPage p);

    /* ── TUI helpers (app_10 style) ── */
    void _drawHeader(const char* title);
    void _drawMenuItem(int y, int index, const char* text, const char* sub, bool selected);
    void _drawFooter(const char* left, const char* right);
    /* Evenly-distributed 3-segment footer: [Dir] left, [A] centre, [B] right. */
    void _drawFooter3(const char* dirHint, const char* aHint, const char* bHint);

    /* ── Menu state ── */
    int _menuSel      = 0;
    int _menuCount    = 0;
    int _scrollOffset = 0;

    /* ── Page handlers ── */
    void _enterMainMenu();
    void _runMainMenu();
    void _enterRunning();
    void _drawRunningStatic();
    void _drawRunningStatus();
    void _updateRunning();

    /* B stops immediately; release returns, hold exits via Launcher. */
    bool _bHeld = false;
    bool _bBackPending = false;

    /* ── LED feedback (white blink while broadcasting) ── */
    bool _ledActive = false;
    void _setLedActive(bool on);

    /* ── BLE state ── */
    bool     _advertising = false;
    bool     _bleInited   = false;
    bool     _advRunning  = false;
    int      _attackIdx   = 0;
    uint16_t _delayMs     = 20;

    /* ── Classic BLE stack objects (unused in v5 raw path) ── */
    BLEServer*       _pServer       = nullptr;
    BLEAdvertising*  _pAdvertising  = nullptr;

    /* ── Timing ── */
    unsigned long _lastAdvTime       = 0;
    unsigned long _lastDrawTime      = 0;
    unsigned long _lastMacChangeTime = 0;
    uint32_t      _packetCount       = 0;

    /* ── Packet generation ── */
    void _makePacketAppleCrash(uint8_t* buf, uint8_t& len);
    void _makePacketAppleAction(uint8_t* buf, uint8_t& len);
    void _makePacketAppleDevice(uint8_t* buf, uint8_t& len);
    void _makePacketFastPair(uint8_t* buf, uint8_t& len);
    void _makePacketEasySetupBuds(uint8_t* buf, uint8_t& len);
    void _makePacketEasySetupWatch(uint8_t* buf, uint8_t& len);
    void _makePacketSwiftPair(uint8_t* buf, uint8_t& len);
    void _makePacketNameFlood(uint8_t* buf, uint8_t& len);
    void _makePacketLoveSpousePlay(uint8_t* buf, uint8_t& len);
    void _makePacketLoveSpouseStop(uint8_t* buf, uint8_t& len);
    void _makePacketKitchenSink(uint8_t* buf, uint8_t& len);

    /* ── BLE helpers ── */
    void _initBLE();
    void _deinitBLE();
    void _sendPacket(const uint8_t* data, uint8_t len);
    void _startSpam();
    void _stopSpam();
};

} // namespace MOONCAKE::APPS
