NFC app 10 stores .nfc captures in this directory (/nfc on the SD card).

Controls: joystick selects/pages; A opens/confirms; B returns/stops.
Touch: tap a row, keyboard key, or footer action. Swipe lists to scroll.

Hardware confirmed: ST25R3916-AQWT, 3.3 V, GND, SDA GPIO1, SCL GPIO2.
No IRQ wire. The application shares the existing 100 kHz I2C bus at address 0x50.

NFC-A saves a Flipper v4 ISO14443-3A identity file, plus Meow capture extensions.
NFC-B/F/V use the MeowKit NFC capture format. These are not full authenticated
card dumps. Classic key extraction, emulation and card writing are unavailable.
Public memory capture limit: 1024 bytes. NDEF limit: 2048 bytes.
Captures preserve complete/partial/limited/failed memory-read status. Type 2
reads respect a readable v1 Capability Container; without it, only the first
16 bytes are kept. NDEF absence and communication failure are shown separately.

A failed/power-interrupted save can leave a .nfc.tmp file. The app preserves it
and asks for a different filename; inspect a recovery file before removing it.

See src/app/app_10/PORTING.md for exact features, file formats and test coverage.
