# Changelog

## [1.0.4] - 2026-11-04

- Rework Infrared learning, saved remotes, and universal-library scanning with decoded and RAW signals.
- Use a consistent, faster white LED strobe during infrared learning and universal scanning.
- Fix Infrared resets when returning from learning or exiting with B.
- Return to the app menu when an app requests its own exit.
- Add the NFC application with ST25R3916 and RFAL integration.
- Improve BadUSB USB/BLE transport reliability and keyboard handling.
- Improve native USB download-mode entry and bound USB CDC/HID callbacks.
- Consolidate build scripts and SD-card resources while retaining bundled library dependencies.

## [1.0.3] - 2026-10-22

- Improve Dino sound playback and audio resource handling.
- Refine VU meter rendering and Retro TV stream handling.
- Improve HID script parsing and input cleanup.
- Refine BLE task handling, settings display, and SD file list layout.
- Consolidate embedded system assets under src/system/assets.
