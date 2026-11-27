# Legacy SD recovery: backup first, completion marker last

## When this applies

A card can contain an old FAT superfloppy at sector zero and a newer FAT MBR
partition. Different drivers can then expose different files on the same card.
The firmware validates the MBR/BPB geometry and selects the canonical FAT
partition. If it also finds an overlapping legacy view, it inventories that view
and exports a bounded backup over serial. It writes neither view and disables
normal storage for that boot. The USB icon remains available for explicit raw
access. This is deliberate data-preservation behavior, not an invitation to format.

A normal FAT MBR card or genuine superfloppy needs no recovery marker. Do not add
markers to suppress an unexplained mount error or use another card's identity.

## 1. Preserve evidence and both filesystems

Capture the complete UTF-8 serial log from a single boot at 115200 baud, including
`[SD PARTITION]`, `[SD INVENTORY] begin/end`, `[SD BACKUP] begin/end` and
`[SD RECOVERY] NEEDS_HOST_RESTORE`. If the log starts midway through an export,
capture another full boot. Keep the unedited log on the PC. Close any serial
monitor before a firmware upload. Do not change either filesystem meanwhile.

Before partition/filesystem repair, also make a full byte image of the identified
card using a card reader and a disk-imaging tool. Verify its size against the
physical device and compute a SHA-256 of the saved image. Keep it off the card.
The image preserves canonical files, unknown data and boot metadata that are not
part of the serial export. Record the physical device serial/reader identity,
capacity, partition index, start sector and sector count; drive letters are not
identities. Back up all readable canonical files separately as well.

The serial snapshot covers `/infrared`, `/handshakes` and `/nfc`, including empty
directories. Its arena is bounded to 2 MiB including metadata, and inventory is
bounded to 4,096 entries and depth eight. A failed traversal/snapshot, unknown
nonempty file, missing export or `REFUSED` message is a stop condition. Preserve
the whole-card image and use manual filesystem recovery; do not write COMPLETE.

## 2. Decode and verify the off-card serial backup

Run the included read-only host utility with a new PC directory:

```sh
python tools/sd_recovery_export.py boot.log sd-backup
```

It accepts exactly one complete export, validates inventory/file/byte counts,
contiguous chunk offsets, safe paths and each firmware CRC32, then saves numbered
payloads, original paths, directory names, SHA-256 values and `identity.txt`.
Payload files are flushed, fsynced and read back before the verified manifest is
written last. The utility never opens a device, serial port or SD filesystem.
Use an off-card destination and retain this directory and the full image after
recovery. An incomplete directory without a verified manifest is not a backup.

Each firmware file header specifies `path_hex`, `size` and CRC32. Hex paths are
UTF-8 bytes; data chunks are up to 128 bytes and CRC32 is standard IEEE CRC32
(the same as Python `zlib.crc32`). Zero-length files have a header/end pair with
no chunks. This describes the format for independent verification.

## 3. Mount the canonical partition, not the legacy view

Exit/eject any host mount before moving the card to a reader. Prefer a Linux
machine with direct access to the reader when Windows keeps selecting the
sector-zero view. Inspect `lsblk -o NAME,SERIAL,SIZE,FSTYPE,START,MOUNTPOINTS` and
the partition table with a disk utility. Select the **partition device**, not the
whole-disk device. Its start and sector count must match `[SD PARTITION]` and the
backup identity; capacity, valid FAT BPB and physical device identity must also
match. Mount it read-only first and inspect its contents. A Windows drive letter
or a WSL `/mnt/<letter>` path alone does not establish which FAT view is mounted.

If the canonical partition is unreadable, stop and work from the verified image
with filesystem recovery tools. This procedure does not require formatting,
changing partition geometry or editing sector zero. Never run a repair or format
against a guessed disk. Recreating a filesystem is a separate destructive
operation requiring a complete verified image and a reviewed recovery plan.

After backing up its existing files, remount the confirmed canonical partition
read/write. Copy `sd-backup/identity.txt` to its root as
`.meowkit-sd-recovery-pending-v1`, preserving its exact bytes. If a different
PENDING or COMPLETE marker already exists, stop and compare its card geometry
and the corresponding backup; do not delete it just to bypass the check.

## 4. Restore without overwriting newer personal data

For every `manifest.json` directory, create that directory beneath the canonical
root if absent. For every file, copy the numbered `backup_file` to the original
relative `path`. Include dotfiles such as `/infrared/.meowkit-settings` and
zero-length files. Preserve directory structure, binary bytes and case.

If a destination already exists, compare its full bytes/hash first. If identical,
keep it. If different, preserve the canonical version and copy the exported
version beside it as `name.legacy-1.ir` (or the corresponding original extension),
choosing the next unused suffix. Record the chosen destination for each conflict
in the PC backup manifest/notes. Do not replace a working remote or its settings
with an older export merely to make the paths match.

Flush/close every restored file. Independently read every destination back and
verify its length and SHA-256 against its numbered PC payload. Check all entries,
including empty files and directories, and all conflict destinations. Save the
per-file result and mapping on the PC. Use the OS sync/safe-eject flow, then
remount and repeat read-back if in doubt. No copy success dialog or aggregate
transfer counter substitutes for these checks.

## 5. Complete only after verified restoration

The exact marker contents come from the selected canonical partition:

```text
MEOWKIT SD RECOVERY v1
partition=<index> start=<start-sector> sectors=<partition-sector-count>
```

There is an LF after **both** lines, with no BOM, CR characters or extra spaces.
For example, index 1/start 16/100000 sectors gives exactly
`b'MEOWKIT SD RECOVERY v1\npartition=1 start=16 sectors=100000\n'`.
Use the backup's `identity.txt`, which already has these bytes; do not recreate
it in an editor that changes newlines. All numbers describe the partition, not
the entire card. These markers are explicit host verification records, not
cryptographic proof that a backup exists.

Only after every restoration check passes, copy `identity.txt` to the canonical
root as `.meowkit-sd-recovery-complete-v1`. Flush it, read it back byte for byte,
and safely eject. Matching PENDING may remain. Firmware requires exact COMPLETE
identity before allowing normal writes; unknown/interrupted markers remain
blocked. Reinsert/restart the device after safe eject/B-exit so the new boot can
clear its previous blocked-for-this-boot state. The serial log should say
`Previously verified recovery; legacy view stays closed`, followed by a successful
SD write/sync/readback probe. Test saved remotes and keep both PC backups.
