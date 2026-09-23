# MeowKit-S3 firmware v1.0.1

Small details make the device feel dependable. Version 1.0.1 focuses on clearer
feedback, safer transitions, and fewer surprises during everyday use.

## What?s new

### Sound that belongs to the system

MeowKit now plays a short sound with the startup animation and gives immediate
feedback when you use the joystick or A/B buttons. Speaker Volume and Key Tone
settings are remembered across restarts.

The Settings, MSC, and Info buttons now respond visually when pressed. Open Info
to see the installed firmware version and author.

### More reliable navigation

Rapid joystick input no longer starts overlapping screen transitions. This
prevents the launcher reboot reported during quick navigation and makes movement
between Home, Settings, Wi-Fi, Clock, SD Files, and Apps more predictable.

### Cleaner App Menu

Empty placeholders `app_10` through `app_15` have been removed. The App Menu now
shows only the nine applications that are ready to use. New entries can be added
when their applications are complete.

### Better audio ownership

The VU Meter and system sounds share physical audio-clock pins. Version 1.0.1
adds an explicit handoff between the ES7210 microphone input and ES8311 speaker
output. Long-pressing B to leave VU Meter no longer leaves system audio broken.

Startup and button sounds use different sample rates. The firmware now rebuilds
the I2S clock when switching between them, keeping the codec and MCLK synchronized.

### BLE Spam interaction update

BLE Spam keeps its TUI style, removes the external attacking background, and
uses clearer controls: A starts or stops, short B returns, and long B exits
through the launcher. Bluetooth resources are released during close.

### Complete source checkout

Required libraries are stored directly under `lib/`. Contributors can clone the
repository and build without initializing Git submodules. Generated build and
`output/` files remain excluded.

## Upgrade notes

- Target: MeowKit-S3 / ESP32-S3-WROOM-1-N16R8.
- Existing settings remain in NVS and load before the startup animation.
- No user-data migration is required.
- After updating, verify Speaker Volume and Key Tone in Settings.

## Validation status

The `esp32s3box` release configuration builds successfully:

```text
RAM:   126,124 / 327,680 bytes (38.5%)
Flash: 7,260,073 / 13,107,200 bytes (55.4%)
```

Firmware compilation, device download, and hardware functional testing have been
completed for this release, including rapid navigation, VU Meter exit, system
audio recovery, Settings controls, and application entry/exit.

## Credits

Thanks to [@warengonzaga](https://github.com/warengonzaga) for
[PR #29](https://github.com/mingolucky/meowkit-s3-firmware/pull/29), which prevents
reboots during overlapping screen transitions. The transition guard retains
upstream credit to [@janud](https://github.com/janud).

Firmware and product integration by Mingo.
