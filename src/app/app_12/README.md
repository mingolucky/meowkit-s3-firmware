# BLE Scanner

Offline Bluetooth Low Energy discovery for MeowKit-S3. Launch **BLE Scanner**
from the last row of Apps. Enable Bluetooth in Settings first if prompted.

- **Up/Down:** select a device. Devices with PAL evidence appear first, then
  other devices sorted by current signal strength.
- **A / tap a row:** device details. A or Left/Right switches between details
  and raw advertisement/scan-response bytes.
- **B:** pause/resume on the list, or return from details.
- **Left/Right / tap the line below the header:** All/PAL filter.
- **Hold B:** return to the launcher.

The list includes address, RSSI in dBm, last-seen age and a PAL marker. More
negative RSSI means a weaker received signal; it is not a distance measurement.
Details show the name, address type, received report count, observed controller
ID, PAL service evidence, and raw bytes. Cached observations remain visible
when paused; their age continues increasing.

`TARGET ID MATCH` is enabled only when a controller ID is configured with the
optional `MEOW_BLE_TARGET_ID` build definition. The default is empty. A PAL
service UUID, a correctly shaped controller ID and a PAL-like name are separate
clues. `PAL name hint only` is the weakest clue. Advertisements are unauthenticated
and may belong to a different receiver in the area; none of these labels proves
ownership or the ability to operate a gate.

At the gate, leave the scanner running for 30–60 seconds. Compare the signal as
you move closer and farther away, and inspect PAL candidates and unnamed devices.
Keep PalGate disconnected while checking: some receivers stop advertising while
a phone is connected. **No result does not prove there is no Bluetooth radio.**
The scanner detects legacy BLE advertisements supported by the current ESP32-S3
stack; it does not discover Bluetooth Classic devices or every BLE 5 extended
advertising mode.

Discovery uses active scan requests to obtain names/scan responses. There are no
connections, pairing, GATT operations, credentials, phone integration, internet
requests or gate-opening commands. The scanner uses the existing Bluetooth
manager's exclusive lease and respects its master setting. It releases the
lease on pause/exit and reacquires it on resume without discarding observations.

The BLE callback copies reports into a 32-entry nonblocking queue. Parsing and
rendering happen on the launcher task. Reports are bounded to 31 advertisement
plus 31 scan-response bytes and 96 unique address/type pairs. The drop counter
indicates callback queue overload. A five-row frame snapshot preserves the
identity of a device tapped while RSSI updates or table eviction occur.

PAL identifiers were investigated using public app releases; see
`com.bluegate.shared.SharedConstants` and `PalBluetoothScanItemData` in the
PAL-signed Android 1.6.0079 APK. Only the interoperable advertisement identifiers
are implemented here. The app deliberately has no gate authentication code.

Validation: `bash tools/tests/run_ble_scan.sh` runs parser/table, lifecycle,
UI/selection and launcher checks with ASan/UBSan. Build with
`pio run -e esp32s3box`. Host tests and compilation do not establish RF reception
or confirm anything about a particular gate; test on the device near the gate.
