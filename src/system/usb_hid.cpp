/**
 * @file usb_hid.cpp
 * @brief USB HID system service implementation.
 *
 * File-scope HID statics register TinyUSB descriptors at static-init time
 * (before USB.begin()).  enable/disable only starts/stops the interface;
 * the descriptor is always present in the composite device.
 */
#include "usb_hid.h"
#include <Arduino.h>

#if ARDUINO_USB_MODE == 0
#include <USB.h>

extern "C" bool tud_mounted(void) __attribute__((weak));
extern "C" bool tud_disconnect(void) __attribute__((weak));
extern "C" bool tud_connect(void) __attribute__((weak));

static USBHIDKeyboard        s_kbd;
static USBHIDMouse           s_mouse;
static USBHIDConsumerControl s_consumer;
static volatile uint8_t      s_leds   = 0;
static bool                  s_began  = false;
static bool                  s_active = false;
static usb_hid_tx_info_t     s_tx = {USB_HID_TX_OK,false,0,0,0};
static bool                  s_ambiguous = false;

int usb_hid_enable(void)
{
    if (s_active) return 1;
    if (!s_began) {
        s_kbd.begin();
        s_mouse.begin();
        s_consumer.begin();
        s_kbd.onEvent(ARDUINO_USB_HID_KEYBOARD_LED_EVENT,
            [](void*, esp_event_base_t, int32_t, void* data) {
                s_leds = ((arduino_usb_hid_keyboard_event_data_t*)data)->leds;
            });
        s_began = true;
    }
    s_active = true;
    Serial.println("[USB_HID] enabled");
    return 1;
}

void usb_hid_disable(void)
{
    if (!s_active) return;
    s_kbd.releaseAll();
    s_mouse.release(0x07);   /* LEFT | RIGHT | MIDDLE */
    s_consumer.release();
    s_active = false;
    Serial.println("[USB_HID] disabled");
}

int  usb_hid_is_active(void)  { return s_active ? 1 : 0; }
int  usb_hid_connected(void)  { return (tud_mounted ? tud_mounted() : false) ? 1 : 0; }
uint8_t usb_hid_leds(void)    { return s_leds; }

static bool send_checked(uint8_t id, const void* report, size_t size)
{
    if (s_ambiguous) return false; // An accepted timed-out report needs explicit recovery.
    s_tx={USB_HID_TX_OK,false,id,0,0};
    if (!s_active) { s_tx.status=USB_HID_TX_DISABLED;return false; }
    if (!usb_hid_connected()) { s_tx.status=USB_HID_TX_UNMOUNTED;return false; }
    const auto result=meow_usb_hid_send_report(id,report,size,750,&s_tx);
    s_ambiguous=(result!=USB_HID_TX_OK && s_tx.accepted) || result==USB_HID_TX_PREVIOUS_PENDING;
    return result==USB_HID_TX_OK;
}

bool usb_hid_send_keyboard(const uint8_t report[8]) { return send_checked(HID_REPORT_ID_KEYBOARD,report,8); }
bool usb_hid_send_mouse(uint8_t buttons,int8_t x,int8_t y,int8_t wheel) {
    const uint8_t report[5] = {buttons,uint8_t(x),uint8_t(y),uint8_t(wheel),0};
    return send_checked(HID_REPORT_ID_MOUSE,report,sizeof(report));
}
bool usb_hid_send_consumer(uint16_t usage) {
    const uint8_t report[2]={uint8_t(usage),uint8_t(usage>>8)};
    return send_checked(HID_REPORT_ID_CONSUMER_CONTROL,report,sizeof(report));
}

bool usb_hid_reconnect(void)
{
    if (!s_active || !tud_disconnect || !tud_connect) return false;
    // Only the launcher task calls this after the user has stopped a script.
    // Leave TinyUSB, CDC/HID descriptors and the shared USB PHY initialized.
    if (!tud_disconnect()) return false;
    s_leds=0;
    delay(250);
    meow_usb_hid_reset_tx();
    s_ambiguous=false;s_tx={USB_HID_TX_OK,false,0,0,0};
    return tud_connect();
}

usb_hid_tx_info_t usb_hid_last_tx(void) { return s_tx; }

USBHIDKeyboard        & usb_hid_kbd(void)      { return s_kbd; }
USBHIDMouse           & usb_hid_mouse(void)    { return s_mouse; }
USBHIDConsumerControl & usb_hid_consumer(void) { return s_consumer; }

#else  /* ARDUINO_USB_MODE != 0 — stub out for CDC-only builds */

int     usb_hid_enable(void)    { return 0; }
void    usb_hid_disable(void)   {}
int     usb_hid_is_active(void) { return 0; }
int     usb_hid_connected(void) { return 0; }
uint8_t usb_hid_leds(void)      { return 0; }
bool usb_hid_send_keyboard(const uint8_t[8]) { return false; }
bool usb_hid_send_mouse(uint8_t,int8_t,int8_t,int8_t) { return false; }
bool usb_hid_send_consumer(uint16_t) { return false; }
bool usb_hid_reconnect(void) { return false; }
usb_hid_tx_info_t usb_hid_last_tx(void) { return {USB_HID_TX_DISABLED,false,0,0,0}; }

#endif

const char* usb_hid_tx_message(usb_hid_tx_status_t status)
{
    switch(status) {
    case USB_HID_TX_DISABLED: return "USB HID inactive";
    case USB_HID_TX_UNMOUNTED: return "USB disconnected. A Retry.";
    case USB_HID_TX_SUSPENDED: return "USB host suspended. Wake PC.";
    case USB_HID_TX_LOCK_TIMEOUT: return "USB HID lock timeout";
    case USB_HID_TX_ENDPOINT_BUSY: return "USB HID endpoint busy";
    case USB_HID_TX_QUEUE_REJECTED: return "USB report not queued";
    case USB_HID_TX_COMPLETE_TIMEOUT: return "USB completion timeout. A Retry.";
    case USB_HID_TX_PREVIOUS_PENDING: return "USB previous report pending";
    case USB_HID_TX_NOT_INITIALIZED: return "USB HID not initialized";
    case USB_HID_TX_INVALID_REPORT: return "USB report invalid";
    default: return "USB transfer failed";
    }
}
