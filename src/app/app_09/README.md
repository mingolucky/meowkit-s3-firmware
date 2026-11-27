# Save a working universal signal

1. Open Universal Remotes, choose the device category and action, and scan.
2. When the device responds, press **A** to pause. The paused screen selects
   the last transmitted code; if a transmission is in flight, it selects that
   current code. **Left/Right** selects nearby codes and **A** tests one again.
3. Press **Down** while paused to open **Save Working Signal**.
4. Choose **Create new remote**, name it (for example `TV`), and select Save.
   Or choose **Add to saved remote**, browse to an existing `.ir` file, and
   name the new button. Duplicate button names are rejected; existing buttons
   and existing files are preserved.
5. A successful save opens the remote immediately. Later, open
   **Saved Remotes → remote/ → TV.ir**, select the button and press A to send.

New remotes go to `/infrared/remote/<name>.ir` on the SD card, using the same
`IR signals file` / `Version: 1` format as learned remotes. The file contains
the full decoded or raw signal, so using it does not depend on rescanning or
on a universal library retaining the same record numbers. **Up** resumes a
paused scan and **B** goes back. The final library signal remains paused so
it can also be tested and saved.

Universal libraries can run from bundled firmware without SD, but saving
requires a writable SD card. A failed save stays in the naming flow for retry
and uses the existing checked, transactional remote writer.

Validation: `bash tools/tests/run_ir_save.sh` checks pause selection, final-code
handling, exact signal copying, append preservation, duplicate/invalid/full
rejection, raw timings and normal saved-file reload using the actual codecs.
`bash tools/tests/run_storage.sh` checks storage and bundled libraries.

## Learning recognized protocols

In **Auto** or **Decode only** mode, every recognized IRremoteESP8266 protocol
keeps its integer code or complete AC state array and uses that protocol's
native transmitter, including its carrier, duty, canonical timings and repeats.
Momentum decoding remains available when the native decoder cannot identify
the signal. Unknown signals fall back to measured pulses in Auto mode;
Decode only waits for a recognized signal. **RAW** mode explicitly records
pulses. Relearn older RAW captures in Auto mode to use protocol transmission.
For a COOLIX-compatible AC, a correctly decoded capture can show **COOLIX / 24 bits**.
Press A to test, then Right to save and name the button.

AC power messages may describe an explicit on/off state. Learn separate
On and Off buttons by recording the original remote in each state.
Native saved records use `encoding: native` to distinguish protocols sharing
names with Momentum. They retain `value`/`bits` for integer codes, or `state`
hex bytes and `bits` for AC states. Existing Momentum and legacy native
remotes continue to load. States and raw buffers are owned copies.
`bash tools/tests/run_ir_coolix.sh` decodes a real device capture and checks
COOLIX timing/repeats plus NEC, Sony and full-state Daikin transmission and
decode roundtrips. `run_ir_save.sh` checks native/Momentum name collisions,
scalar/state saved-file reload and malformed/truncated state rejection.

## One Power button with learned On and Off commands

Choose **Learn Power Toggle** from Infrared, or **+ Power Toggle** inside an
existing saved remote. With the original remote Off, press its power button
to capture On. Press Right to keep it. With the original remote On, press
power to capture Off, then Right to name and save the paired button.
The two captures may have the same protocol, but their command/state data
must differ. Identical data is rejected and the Off capture can be retried.

The saved button displays **Next: On** or **Next: Off**. A fresh A press
sends that command once, including the protocol's required repeat frames;
holding A does not alternate power. A successful transmission advances the
next command and saves it through the existing transactional writer. A
failed/cancelled transmission leaves it unchanged. Right changes and saves
the next command without transmitting, for matching a device operated with
another remote. IR is one-way: the next command is a local estimate, not a
measurement of the appliance's state.

Both complete payloads live in one normal remote record. `power_toggle`,
`power_next` and `off_` fields retain the pairing and next command across
reopening the app. Existing unpaired records continue to load. Truncated,
nested and identical pairs are rejected before rewriting an existing file.

## Transmitter and SD integrity

MeowKit IR now uses LEDC hardware carrier modulation on dedicated channel 0
(timer 0). Native protocol encoders, Momentum encoders and RAW replay share
the same PWM sender; the transmit task runs above display work on the Arduino
core. Carrier setup failures are reported as transmission failures and do
not advance a paired Power button. `run_ir_sender.sh` checks the native
On/Off/On envelopes, frequency conversion, channel isolation and cleanup.

SD mounting prefers the validated MBR FAT partition and supports genuine
superfloppies. A conflicting old sector-zero filesystem triggers read-only
inventory and CRC backup export. Normal filesystem writes stay disabled
until a host verifies the backup, restores the canonical partition and writes
the matching complete marker. Settings' USB icon remains available for raw
recovery access. Firmware never formats or rewrites the boot sectors.

## Identify a learned signal in saved remotes

Each newly learned signal is checked against personal `.ir` files beneath
`/infrared`, including legacy files, nested folders and both sides of paired
Power buttons. The capture screen shows the first matching remote and button.
Press **Down** for all matches; paired matches are labelled **[On]** or
**[Off]**. Select a match and press A to see its file path, which distinguishes
remotes with the same filename in different folders. B returns to the capture.
The universal/assets library folders are excluded.
If an identical On/Off pair is rejected and its capture also matches a saved
command, the retry message includes the first matching remote and button.

This compares exact command data, including native codes, complete AC states
and raw pulse recordings. Sharing a protocol is insufficient; noisy RAW
recaptures can have different timings and therefore no exact match. Matches
are informational: saving the same command again is allowed.

Checking is cooperative and pauses during transmission. If Right is pressed
before checking finishes, the app waits for the bounded check. Press Right
again to continue immediately with the captured command and a partial check.
Otherwise any matches or partial check are displayed before continuing.
Back returns to the capture, where
Right can continue learning/saving. Opening a matched file's information
temporarily pauses checking. Leaving the learning flow closes its SD handles.
The search is limited to 4,096 entries, 128 directories and 4 MiB of reads,
with a three-second deadline checked between filesystem operations.
Unreadable/malformed files and scan limits produce an explicit partial-check
status, rather than claiming every saved command was checked. Negative results
say "No match in files checked": this filesystem API cannot distinguish every
directory read failure from reaching the end of a directory.

See [SD/USB usage](../../../docs/storage-usb.md) and the complete
[legacy recovery procedure](../../../docs/sd-recovery.md) before repairing a card.
