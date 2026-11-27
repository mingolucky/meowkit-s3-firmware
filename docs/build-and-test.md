# Build and verification

Install PlatformIO Core and run from the repository root:

```sh
pio run -e esp32s3box
```

`platformio.ini` pins Espressif32 6.0.1 (Arduino 2.0.6 / ESP-IDF 4.4.3), targeting
ESP32-S3 with 16 MB flash and 8 MB OPI PSRAM. Keep the matching partition table.
The pre-build hooks embed IR dictionaries and apply checked build-local CDC/HID/
MSC framework patches; they do not edit the globally installed SDK. Unexpected
SDK source shapes fail the patch checks rather than silently producing a build.

Flash through PlatformIO with the connected MeowKit selected:

```sh
pio run -e esp32s3box -t upload
```

The native USB upload hook requests the application's 1200-baud reboot and follows
the same board's ROM port. A successful build is not an installation. Keep a
verified application rollback and SD backup before deploying development builds;
verify the flash digest and normal boot afterward. Do not erase NVS or format the
SD card as part of a routine firmware update.

## Linux host regressions

Install GCC/G++, Python 3, zlib development headers and binutils. Run on Linux or
WSL; the tests use temporary directories, the real filesystem, ASan and UBSan:

```sh
python3 -m unittest discover -s tools/tests -p test_sd_recovery_export.py -v
bash tools/tests/run_ble_scan.sh
bash tools/tests/run_idle_power_policy.sh
bash tools/tests/run_ir_coolix.sh
bash tools/tests/run_ir_result_flow.sh
bash tools/tests/run_ir_save.sh
bash tools/tests/run_ir_saved_matches.sh
bash tools/tests/run_ir_sender.sh
bash tools/tests/run_sd_partition.sh
bash tools/tests/run_storage.sh
bash tools/tests/run_usb_manager.sh
bash tools/tests/run_usb_msc.sh
python3 tools/tests/run_usb_scsi.py --sdk-source "$HOME/.platformio/packages/framework-arduinoespressif32/cores/esp32/USBMSC.cpp"
```

Use the actual PlatformIO package location if `PLATFORMIO_CORE_DIR` is customized.
The SCSI test extracts callbacks from that pinned SDK and applies the production
patch hook before compiling. The BLE test uses a synthetic target ID; production
builds have no target configured by default. Tests exercise actual helper/app
code with simulated hardware boundaries. They cover malformed data, bounded
scans, cancellation, transmission/save failures, partition and recovery gating,
concurrent storage teardown, partial-sector I/O and absent-media behavior.

## Physical checks

Check Air Mouse after initial pairing, app close/reopen and bonded reconnect;
verify cursor, both buttons, touch scrolling and recalibration. Test IR against
actual appliances: save a working universal command, reopen it without scanning,
learn native/RAW signals, and exercise both sides of a paired Power button.
Check learned-command matches, partial checks and cancellation.

Back up current SD files and hashes. Open USB storage, copy and read back a file,
safely eject, close with B and reopen. Repeat copy/read-back and confirm personal
remotes/settings are unchanged. Leave the normal screen idle beyond five minutes
on USB with a fully charged battery; confirm the device stays powered, then wakes.
Check battery-only idle behavior separately. Verify BLE discovery near known
advertisers and app pause/exit/radio ownership. Host tests and compilation alone
do not establish optical/RF reception, appliance compatibility or device stability.

Development device flashing for these fixes used esptool 5.7.0 with an exact
application rollback and digest verification. The PR build is separately compiled;
a successful host test or build is not a claim that this exact PR image was flashed.
