# SPDX-License-Identifier: GPL-2.0-or-later
"""Use PlatformIO's installed esptool with ESP32-S3 USB reset backports.

Based on Espressif esptool v4.9.0 targets/esp32s3.py and commit 0215786283.
https://github.com/espressif/esptool/blob/v4.9.0/esptool/targets/esp32s3.py
Keeps this compatibility fix in the project, without changing the SDK install.
"""
import sys
import time


def disable_watchdogs_esp32s3(esp):
    # 4.4 predates uses_usb_jtag_serial() and its watchdog hook entirely.
    native_usb = (esp.uses_usb_jtag_serial() if hasattr(esp, "uses_usb_jtag_serial") else
                  not esp.secure_download_mode and (esp.read_reg(0x3FCEF14C) & 0xFF) == 4)
    if not native_usb:
        return
    # Older targets omitted this step or used +0x90; WDTCONFIG0 is +0x98.
    # A watchdog left armed by esp_restart can reset ROM during flash writes.
    esp.write_reg(0x600080B0, 0x50D83AA1)
    esp.write_reg(0x60008098, 0)
    esp.write_reg(0x600080B0, 0)
    # Match the later official target's super-WDT auto-feed policy as well.
    esp.write_reg(0x600080B8, 0x8F1D312A)
    esp.write_reg(0x600080B4, esp.read_reg(0x600080B4) | (1 << 31))
    esp.write_reg(0x600080B8, 0)
    print("[MeowKit USB] ROM RTC watchdog disabled; super-WDT auto-feed enabled")


def post_connect_esp32s3(esp):
    # Preserve 4.4's OTG RAM transfer limit and actually install the missing
    # watchdog call, rather than only defining a method 4.4 never invokes.
    if esp.uses_usb_otg():
        esp.ESP_RAM_BLOCK = esp.USB_RAM_BLOCK
    disable_watchdogs_esp32s3(esp)


def reset_esp32s3(esp, sleep=time.sleep):
    # Old esptool 4.4 leaves FORCE_DOWNLOAD_BOOT/strapping state latched.
    # Clear the software request, then reset the entire chip with the RTC WDT,
    # including strapping re-sampling, rather than only pulsing virtual RTS.
    print("[MeowKit USB] Clearing forced download; RTC watchdog chip reset")
    esp.write_reg(0x6000812C, 0, 1)
    esp.write_reg(0x600080B0, 0x50D83AA1)
    esp.write_reg(0x6000809C, 2000)
    esp.write_reg(0x60008098, (1 << 31) | (5 << 28) | (1 << 8) | 2)
    esp.write_reg(0x600080B0, 0)
    sleep(0.5)


def main():
    if len(sys.argv) < 3 or sys.argv[1] != "--esptool-dir":
        raise SystemExit("Use --esptool-dir <PlatformIO tool-esptoolpy directory> before esptool arguments")
    sys.path.insert(0, sys.argv[2])
    del sys.argv[1:3]
    import esptool
    from esptool.targets.esp32s3 import ESP32S3ROM
    ESP32S3ROM.RTC_CNTL_WDTCONFIG0_REG = 0x60008098
    ESP32S3ROM._post_connect = post_connect_esp32s3
    ESP32S3ROM.disable_rtc_watchdog = disable_watchdogs_esp32s3
    ESP32S3ROM.hard_reset = reset_esp32s3
    esptool._main()


if __name__ == "__main__":
    main()
