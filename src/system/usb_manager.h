/**
 * @file usb_manager.h
 * @brief USB mode manager — serialises access to the single OTG peripheral.
 *
 * ESP32-S3 has one USB OTG port.  CDC is always-on (Serial).
 * HID and MSC are mutually exclusive runtime modes: requesting one
 * automatically tears down the other before bringing the new one up.
 *
 * Usage:
 *   usb_manager_request(USB_MODE_MSC);   // enter U-disk mode
 *   usb_manager_release(USB_MODE_MSC);   // leave U-disk mode
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    USB_MODE_NONE = 0,
    USB_MODE_HID  = 1,   /* BadUSB keyboard / mouse / consumer-control */
    USB_MODE_MSC  = 2,   /* U-disk (SD card exposed to host) */
} usb_mode_t;

/** Initialize the existing CDC and defer 1200bps reboot requests to poll(). */
void usb_manager_init(void);
/** Call from the main loop, outside TinyUSB/CDC event handlers. */
void usb_manager_poll(void);

/** Request a USB mode.
 *  If another mode is active it is torn down first.
 *  Returns 1 on success, 0 on failure (previous mode is also stopped). */
int  usb_manager_request(usb_mode_t mode);

/** Release ownership of a mode (no-op if mode is not the active one).
 *  Equivalent to usb_manager_request(USB_MODE_NONE). */
void usb_manager_release(usb_mode_t mode);

/** Return the currently active mode. */
usb_mode_t usb_manager_get_active(void);

#ifdef __cplusplus
}
#endif
