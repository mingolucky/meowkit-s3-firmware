# Air Mouse

Enable Bluetooth in Settings and open Air Mouse. Pair the advertised MeowKit
mouse from the computer's Bluetooth settings and accept its pairing prompt.
Keep the device still during calibration. Move it to steer the cursor.

- A presses/releases the left mouse button, including dragging.
- A short B press performs a right click; hold B to return to the launcher.
- Left/Right changes sensitivity; the selected speed is saved.
- Up/Down scrolls. Drag vertically in the Touch Scroll panel to scroll by touch.
- Tap the speed control to cycle sensitivity; tap the calibration control to retry.

The Bluetooth link is ready only after the host enables the required mouse
report notifications. Windows may report a saved connection before that happens.
The app clears stale subscriptions on disconnect and handles bonded reconnects
without requiring the user to pair again. Leaving/reopening releases input state
and reacquires the Bluetooth manager's exclusive lease.

Calibration is nonblocking. A valid prior bias remains usable during a retry;
an unsuccessful recalibration does not disable an already calibrated mouse.
A new device without a valid calibration still needs one successful still capture.
Cursor and scroll accumulators reset across link changes, and buttons must be
released before a fresh click is accepted after reconnecting.

If it stays at pairing/waiting, ensure the computer is actually connected to the
mouse service, Bluetooth is enabled, and another MeowKit Bluetooth app does not
own the radio. If an old host bond is incompatible, remove the device in the
computer's Bluetooth settings and pair again. Do this only as a troubleshooting
step, not on every app launch. USB storage is independent of Bluetooth pairing.
