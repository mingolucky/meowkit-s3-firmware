#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    USB_HID_TX_OK=0,
    USB_HID_TX_DISABLED,
    USB_HID_TX_UNMOUNTED,
    USB_HID_TX_SUSPENDED,
    USB_HID_TX_LOCK_TIMEOUT,
    USB_HID_TX_ENDPOINT_BUSY,
    USB_HID_TX_QUEUE_REJECTED,
    USB_HID_TX_COMPLETE_TIMEOUT,
    USB_HID_TX_PREVIOUS_PENDING,
    USB_HID_TX_NOT_INITIALIZED,
    USB_HID_TX_INVALID_REPORT
} usb_hid_tx_status_t;

typedef struct {
    usb_hid_tx_status_t status;
    bool accepted;
    uint8_t report_id;
    uint32_t waited_ms;
    uint32_t attempts;
} usb_hid_tx_info_t;

// Implemented inside the SDK HID translation unit by the project build patch.
usb_hid_tx_status_t meow_usb_hid_send_report(uint8_t id, const void* data, size_t size,
                                          uint32_t timeout_ms, usb_hid_tx_info_t* info);
void meow_usb_hid_reset_tx(void);
#ifdef __cplusplus
}
#endif
