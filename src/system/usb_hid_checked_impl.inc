// Included once inside the patched SDK USBHID.cpp: shares its actual TX mutex,
// completion semaphore and TinyUSB callback. No duplicate USB endpoint owner.
#include "usb_hid_status.h"
#include <atomic>
#include <cstring>

static std::atomic<bool> meow_hid_inflight{false};
static uint8_t meow_hid_expected[CFG_TUD_HID_EP_BUFSIZE] = {};
static size_t meow_hid_expected_size = 0;

static void meow_hid_tx_complete(const uint8_t* report, size_t size)
{
    if (meow_hid_inflight.load() && report && size==meow_hid_expected_size &&
        !memcmp(report,meow_hid_expected,size)) meow_hid_inflight.store(false);
}

extern "C" void meow_usb_hid_reset_tx(void)
{
    // Caller has explicitly detached USB and allowed pending callbacks to drain.
    meow_hid_inflight=false;
    if (tinyusb_hid_device_input_sem) xSemaphoreTake(tinyusb_hid_device_input_sem,0);
}

extern "C" usb_hid_tx_status_t meow_usb_hid_send_report(uint8_t id,const void* data,size_t size,
                                                       uint32_t timeout_ms,usb_hid_tx_info_t* output)
{
    const uint32_t started=millis();
    usb_hid_tx_info_t info={USB_HID_TX_OK,false,id,0,0};
    auto finish=[&](usb_hid_tx_status_t status) {
        info.status=status;info.waited_ms=uint32_t(millis()-started);
        if(output) *output=info;
        return status;
    };
    const size_t prefix=id?1:0;
    if (!data || !size || size>sizeof(meow_hid_expected)-prefix) return finish(USB_HID_TX_INVALID_REPORT);
    if (!tinyusb_hid_device_input_sem || !tinyusb_hid_device_input_mutex) return finish(USB_HID_TX_NOT_INITIALIZED);
    if (xSemaphoreTake(tinyusb_hid_device_input_mutex,pdMS_TO_TICKS(timeout_ms))!=pdTRUE)
        return finish(USB_HID_TX_LOCK_TIMEOUT);

    usb_hid_tx_status_t result=USB_HID_TX_OK;
    bool submitted=false;
    while (!submitted) {
        if (!tud_mounted()) { result=USB_HID_TX_UNMOUNTED;break; }
        const bool pending=meow_hid_inflight.load();
        const bool asleep=tud_suspended();
        if (!pending && !asleep && tud_hid_n_ready(0)) {
            // Clear stale signals BEFORE submitting. Completion may happen
            // inside tud_hid_n_report, before that call returns.
            xSemaphoreTake(tinyusb_hid_device_input_sem,0);
            if(id) meow_hid_expected[0]=id;
            memcpy(meow_hid_expected+prefix,data,size);
            meow_hid_expected_size=size+prefix;
            meow_hid_inflight=true;
            ++info.attempts;
            if (tud_hid_n_report(0,id,data,size)) {
                info.accepted=true;submitted=true;break;
            }
            meow_hid_inflight=false; // Not accepted: retrying cannot duplicate a key.
        }
        if (uint32_t(millis()-started)>=timeout_ms) {
            result=pending ? USB_HID_TX_PREVIOUS_PENDING : asleep ? USB_HID_TX_SUSPENDED :
                info.attempts ? USB_HID_TX_QUEUE_REJECTED : USB_HID_TX_ENDPOINT_BUSY;
            break;
        }
        delay(1);
    }
    while (submitted && meow_hid_inflight.load()) {
        if (!tud_mounted()) { result=USB_HID_TX_UNMOUNTED;break; }
        if (uint32_t(millis()-started)>=timeout_ms) { result=USB_HID_TX_COMPLETE_TIMEOUT;break; }
        // A semaphore signal alone is insufficient: it may belong to the
        // previous transfer. Only a matching report callback completes this one.
        xSemaphoreTake(tinyusb_hid_device_input_sem,pdMS_TO_TICKS(1));
        // Also yield if an extra/stale semaphore signal returned immediately.
        if (meow_hid_inflight.load()) delay(1);
    }
    xSemaphoreGive(tinyusb_hid_device_input_mutex);
    return finish(result);
}
