# Changelog

This file records user-visible firmware changes and their verification status.

## Unreleased

### Fixed

- Prevented launcher reboots caused by overlapping LVGL screen transitions during rapid joystick navigation.
- Physical navigation input is now dispatched immediately; new navigation is ignored while a screen transition is active.
- The shared screen-change helper now rejects a second transition until the current animation finishes.
- Wi-Fi status marquee text is clipped inside its card.
- The Wi-Fi **Connect** label is centered on its key background.
- App exit now activates the persistent Apps screen before an app-owned LVGL root screen is deleted.

### Changed

- Added screen-transition diagnostics with source screen, destination screen, input event, and free-heap information.
- External queued navigation events are processed one at a time after the display becomes idle.

### Credits

- Navigation transition fix contributed by [@warengonzaga](https://github.com/warengonzaga) in [PR #29](https://github.com/mingolucky/meowkit-s3-firmware/pull/29).
- Shared LVGL transition guard adapted from the original work by [@janud](https://github.com/janud).

### Verification

- PlatformIO release build for `esp32s3box`: **Passed**.
- Patch whitespace and merge-conflict checks: **Passed**.
- On-device rapid joystick reversal test: **Pending**.
- On-device app open/long-B exit regression test: **Pending**.

Hardware verification must be completed before this change is included in a production firmware release.
