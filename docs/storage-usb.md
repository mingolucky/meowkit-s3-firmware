# SD, USB storage and power

## Everyday file copying

Use a readable FAT SD card with a supported, valid partition layout. Open the USB
icon in Settings (above the information icon) to expose the whole SD card to the
computer as USB mass storage. The firmware's normal filesystem access is paused
while the raw card belongs to USB storage. Wait for the operating system to mount
it before copying. A rising transfer counter can include operating-system reads;
it is not by itself proof that a file was saved.

Copy personal remote files under `/infrared/remote/`. Preserve the existing
`/infrared/.meowkit-settings` file and any other application data. Universal
libraries have firmware fallbacks and need not be copied onto the card.

After copying, wait for completion and use the operating system's safe-eject
command. Then press B on MeowKit to leave USB storage. The firmware drains active
transfers, releases the raw card and remounts the normal filesystem. Reopen the
USB icon for another session; a new Bluetooth pairing or reboot is not required.
Do not pull the card or power down while a write is in progress.

## What the fixes change

All applications share a checked SD mount path. Directory creation and writes
report failures; file completion includes a filesystem sync. The raw USB driver
handles nonzero offsets and partial sectors through internal DMA-compatible
scratch memory. Teardown retains ownership until in-flight I/O has drained.
Host eject/load requests hide or reveal prepared media without freeing callback
state or disconnecting the composite USB device. Unsupported SCSI commands fail;
cache synchronization is acknowledged only because sector writes are synchronous.
CDC writes and control events are bounded so a host that is not draining serial
output cannot indefinitely block the USB task. SD clock requests are explicitly
selected and verified against the pinned SDK's frequency ladder.

Mounting prefers a validated FAT MBR partition and supports a genuine unpartitioned
FAT card. It rejects ambiguous/invalid geometry. A conflicting legacy filesystem
is handled by the [backup-gated recovery procedure](sd-recovery.md), never by an
automatic format or boot-sector rewrite.

## Screen timeout versus power off

The display can still go dark after its configured timeout; touch or a physical
button wakes it. Normal idle shutdown is suppressed whenever the PMU detects USB
power, even if the battery is full and reports `charging=no`. On battery, the idle
shutdown threshold remains 2.5 times the display timeout, with a five-minute
minimum (five minutes when the display timeout is disabled). Critical battery
protection remains active and is separate from this idle policy. Active USB
storage also prevents ordinary idle shutdown.

## Troubleshooting

If Windows sees a USB device but no mounted drive, allow its filesystem checks to
finish, inspect the device transfer counter, and check the OS disk view. Use a
USB data cable and inspect the actual serial/partition identity before attempting
any repair. A descriptor error is different from an SD write error. Do not format
or restore onto a drive merely because it has the same letter as an earlier
session. Capture serial logs from boot for SD partition, clock and I/O errors.

A FAT VFS error may be a failed open/write/sync rather than an unreadable card.
Keep an off-card backup before repair. See [build and verification](build-and-test.md)
for deterministic host checks; those checks do not replace physical storage tests.
