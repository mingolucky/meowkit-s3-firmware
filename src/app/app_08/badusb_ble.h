/* BLE keyboard transport. Static GATT resources and a shared lifecycle lease;
 * USB keyboard types remain isolated from this translation unit. */
#pragma once
#include <stdint.h>

bool bu_ble_begin();
bool bu_ble_failed();
void bu_ble_end();
bool bu_ble_connected();
bool bu_ble_keyboard_report(const uint8_t report[8]);
bool bu_ble_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel);
bool bu_ble_consumer_report(uint16_t usage);
uint8_t bu_ble_leds();
const char* bu_ble_error();
bool bu_ble_clear_error();
enum class BuBlePairing : uint8_t { None, Compare, Display };
struct BuBlePin { BuBlePairing mode; uint32_t value; };
BuBlePin bu_ble_pairing();
void bu_ble_pairing_reply(bool accept);
bool bu_ble_wait_idle();
using BuBleWaitCallback = bool (*)(void*);
void bu_ble_set_wait_callback(BuBleWaitCallback callback, void* context);
bool bu_ble_recovering();
bool bu_ble_requires_reconnect();
bool bu_ble_reconnect();
bool bu_ble_session_alive();
struct BuBleDiagnostics {
    bool linked, encrypted, keyboardSubscribed, consumerSubscribed, mouseSubscribed, congested;
    uint8_t disconnectReason;
    uint16_t intervalMs;
    int lastSendResult;
    uint32_t recoveredCongestions;
    uint32_t rejectedRetries;
};
BuBleDiagnostics bu_ble_diagnostics();
