# System audio

MeowKit plays a short sound with the boot animation and provides consistent
feedback for the joystick and A/B buttons. Both use the existing Sound settings.

## User behavior

- **Boot sound** starts with `boot_animation.gif`. The build reads the GIF frame
  delays and embeds only complete MP3 frames needed for that duration. The
  current animation is 3150 ms, so 74.1 KB of the original boot MP3 is stored in
  firmware.
- **Button sound** plays once when a physical input is pressed. Holding a button
  does not repeat the sound. Long-press B behavior is unchanged.
- **Speaker Volume** controls both sounds. A value of 0 keeps the amplifier off.
- **Key Tone** controls button feedback only. The boot sound follows volume but
  is independent of Key Tone.
- Volume and Key Tone are restored from NVS before the boot animation starts.

## Audio path

`ESP32-S3 I2S0 TX -> ES8311 DAC -> OUTP/OUTN -> NS4150B -> speaker`

The BSP outputs BCLK, WS, DATA and the schematic's dedicated MCLK. MCLK is
configured at 128 times the sample rate to match MeowKit's hardware-validated
Audio/ES8311 path and uses APLL for stable 44.1 kHz audio.
The NS4150B `PA_EN` signal is driven through PCA9557 IO3.

The system sound service keeps the codec muted while idle. Playback order is:

1. Keep ES8311 muted.
2. Enable `PA_EN` and wait for the amplifier to settle.
3. Unmute and stream decoded PCM.
4. Mute, clear the I2S DMA buffer, then disable `PA_EN`.

This sequence limits turn-on and turn-off pops. UI volume is mapped to the BSP's
validated `SPK_VOLUME_MAX` ceiling.

## Assets and build

- `boot_sound effects.mp3` - source for the startup sound.
- `button_sound effects.mp3` - source for physical input feedback.
- `tools/embed_gif.py` - embeds both assets and calculates the boot prefix.
- `src/system/system_sound.*` - asynchronous MP3 decode and playback service.
- `src/bsp/audio/Speaker_Class.*` - ES8311 and I2S output driver.

The build does not require FFmpeg. MP3 trimming is performed at valid frame
boundaries and the original source file is never modified.

## Hardware acceptance test

1. Set Speaker Volume to 50% and enable Key Tone, then restart the device.
2. Confirm audio starts with the animation and stops when the animation ends.
3. Press each joystick direction and A/B once; confirm one sound per press.
4. Hold B; confirm there is no repeated sound and long-press exit still works.
5. Disable Key Tone; confirm button sounds stop immediately.
6. Set volume to 0; confirm boot and button sounds remain silent after restart.
7. Test 20%, 50%, and 100% volume for clean output without clicks or distortion.
