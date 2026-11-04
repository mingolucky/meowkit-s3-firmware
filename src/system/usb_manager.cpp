/**
 * @file usb_manager.cpp
 * @brief USB mode manager — serialises HID / MSC lifecycle on the single OTG port.
 */
#include "usb_manager.h"
#include "usb_msc.h"
#include "usb_hid.h"
#include "usb_download.h"
#include <Arduino.h>
#include <atomic>
#if ARDUINO_USB_MODE == 0 && ARDUINO_USB_CDC_ON_BOOT
#include <USB.h>
extern "C" void settings_flush(void);
#endif

static usb_mode_t s_active = USB_MODE_NONE;
static std::atomic<bool> s_download_requested{false};

void usb_manager_init(void)
{
    static bool initialized = false;
    if (initialized) return;
#if ARDUINO_USB_MODE == 0 && ARDUINO_USB_CDC_ON_BOOT
    // The core's automatic path reconfigures the USB PHY from a USB control
    // callback, before that control transfer can finish on Windows. Let it
    // complete, then perform the framework restart from the application loop.
    Serial.enableReboot(false);
    Serial.setTxTimeoutMs(10); // An open but unread CDC port is never a HID dependency.
    Serial.onEvent(ARDUINO_USB_CDC_LINE_CODING_EVENT,
        [](void*, esp_event_base_t, int32_t, void* data) {
            const auto* event = static_cast<arduino_usb_cdc_event_data_t*>(data);
            if (event && event->line_coding.bit_rate == 1200)
                s_download_requested.store(true);
        });
#endif
    initialized = true;
}

void usb_manager_poll(void)
{
#if ARDUINO_USB_MODE == 0 && ARDUINO_USB_CDC_ON_BOOT
    if (!s_download_requested.exchange(false)) return;
    usb_manager_request(USB_MODE_NONE);
    settings_flush();
    delay(150); // USB SET_LINE_CODING must finish before removing the PHY.
    usb_download_restart();
#endif
}

static void _teardown(usb_mode_t mode)
{
    switch (mode) {
        case USB_MODE_MSC: usb_msc_disable(); break;
        case USB_MODE_HID: usb_hid_disable(); break;
        default: break;
    }
}

static int _bring_up(usb_mode_t mode)
{
    switch (mode) {
        case USB_MODE_MSC:  return usb_msc_enable();
        case USB_MODE_HID:  return usb_hid_enable();
        case USB_MODE_NONE: return 1;
    }
    return 0;
}

int usb_manager_request(usb_mode_t mode)
{
    usb_manager_init();
    if (s_active == mode) return 1;

    /* Drain CDC TX before touching the USB stack — prevents data loss and
     * avoids the host seeing a mid-packet disconnect. */
    Serial.flush();
    delay(20);

    usb_mode_t prev = s_active;
    _teardown(prev);
    s_active = USB_MODE_NONE;

    int ok = _bring_up(mode);
    if (ok) s_active = mode;

    Serial.printf("[USB_MGR] %d → %d (%s)\n", (int)prev, (int)mode, ok ? "ok" : "fail");
    return ok;
}

void usb_manager_release(usb_mode_t mode)
{
    if (s_active != mode) return;
    usb_manager_request(USB_MODE_NONE);
}

usb_mode_t usb_manager_get_active(void)
{
    return s_active;
}
