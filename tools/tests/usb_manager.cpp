#include "../../src/system/usb_manager.h"
#include <Arduino.h>
#include <cassert>
#include <cstdio>
ManagerSerial Serial;
static bool msc=false,retain=false,hid=false,enableFails=false;
static int restarts=0,flushes=0;
extern "C" int usb_msc_enable(){msc=true;return enableFails?0:1;}
extern "C" void usb_msc_disable(){if(!retain)msc=false;}
extern "C" int usb_msc_is_active(){return msc;}
extern "C" int usb_hid_enable(){hid=true;return 1;}
extern "C" void usb_hid_disable(){hid=false;}
extern "C" void settings_flush(){++flushes;}
void usb_download_restart(){++restarts;}
int main() {
    assert(usb_manager_request(USB_MODE_MSC));
    retain=true;
    assert(!usb_manager_request(USB_MODE_HID));
    assert(msc && !hid && usb_manager_get_active()==USB_MODE_MSC);
    retain=false;
    assert(usb_manager_request(USB_MODE_HID) && !msc && hid);
    assert(usb_manager_request(USB_MODE_MSC) && msc && !hid);
    arduino_usb_cdc_event_data_t event;event.line_coding.bit_rate=1200;
    Serial.callback(nullptr,0,ARDUINO_USB_CDC_LINE_CODING_EVENT,&event);
    retain=true;usb_manager_poll();
    assert(restarts==0 && flushes==0 && usb_manager_get_active()==USB_MODE_MSC);
    retain=false;usb_manager_poll();
    assert(restarts==1 && flushes==1 && !msc && usb_manager_get_active()==USB_MODE_NONE);
    enableFails=true;
    assert(!usb_manager_request(USB_MODE_MSC) && msc && usb_manager_get_active()==USB_MODE_MSC);
    enableFails=false;usb_manager_release(USB_MODE_MSC);
    assert(!msc && usb_manager_get_active()==USB_MODE_NONE);
    puts("USB mode checks passed (drain failure retains ownership, reboot waits, failed enable cleanup retries)");
}
