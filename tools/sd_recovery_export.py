"""Verify a firmware serial recovery export and save an off-card backup.

This utility never opens a serial port, disk, SD card or mounted volume.
Usage: python tools/sd_recovery_export.py boot.log NEW_BACKUP_DIRECTORY
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import zlib


def decode(text):
    if len(text.encode("utf-8")) > 16 * 1024 * 1024:
        raise ValueError("Log exceeds 16 MiB")
    def one(pattern):
        found = re.findall(pattern, text)
        if len(found) != 1:
            raise ValueError("Need exactly one complete boot export: " + pattern)
        return found[0]
    partition, start, sectors = map(int, one(
        r"\[SD PARTITION\] selected=(\d+) start=(\d+) sectors=(\d+) legacy=1"))
    if not 1 <= partition <= 4 or not start or not sectors:
        raise ValueError("Invalid canonical partition identity")
    one(r"\[SD INVENTORY\] begin")
    count, directories, total = map(int, one(
        r"\[SD INVENTORY\] end ok=1 files=(\d+) directories=(\d+) bytes=(\d+) unknown_nonempty=0"))
    inventory = {}
    directory_paths = []
    def path_from_hex(value):
        path = bytes.fromhex(value).decode("utf-8")
        parts = path.split("/")
        if not path.startswith("/") or "\\" in path or "\0" in path or any(p in (".", "..", "") for p in parts[1:]):
            raise ValueError("Unsafe exported path")
        return str(PurePosixPath(path).relative_to("/"))
    records = re.findall(r"\[SD INVENTORY\] path_hex=([0-9A-Fa-f]+) kind=(file|directory) size=(\d+) known=([01])", text)
    if sum(kind == "file" for _, kind, _, _ in records) != count or sum(kind == "directory" for _, kind, _, _ in records) != directories:
        raise ValueError("Incomplete inventory")
    if sum(int(size) for _, kind, size, _ in records if kind == "file") != total:
        raise ValueError("Inventory byte count mismatch")
    seen = set()
    for path_hex, kind, size, known in records:
        path = path_from_hex(path_hex)
        if path.casefold() in seen:
            raise ValueError("Duplicate or case-conflicting inventory path")
        seen.add(path.casefold())
        if known == "1":
            if path.split("/")[0] not in ("infrared", "handshakes", "nfc"):
                raise ValueError("Unexpected recovery root")
            if kind == "file":
                inventory[path] = int(size)
            else:
                directory_paths.append(path)
        elif kind == "file" and int(size):
            raise ValueError("Nonempty data outside the recovery roots")
    expected = int(one(r"\[SD BACKUP\] begin files=(\d+) memory_bytes=\d+"))
    ended = int(one(r"\[SD BACKUP\] end files=(\d+)"))
    if expected != ended or expected != len(inventory) or expected > 4096:
        raise ValueError("Export file count mismatch")
    if sum(inventory.values()) > 2 * 1024 * 1024:
        raise ValueError("Export exceeds firmware snapshot bound")
    one(r"\[SD RECOVERY\] NEEDS_HOST_RESTORE files=" + str(expected) + r";")
    files, active, payload = [], None, bytearray()
    for line in text.splitlines():
        if "[SD BACKUP] file=" not in line:
            continue
        record = line[line.index("[SD BACKUP] file="):]
        header = re.fullmatch(r"\[SD BACKUP\] file=(\d+) path_hex=([0-9A-Fa-f]+) size=(\d+) crc32=([0-9A-Fa-f]{8})", record)
        chunk = re.fullmatch(r"\[SD BACKUP\] file=(\d+) offset=(\d+) hex=([0-9A-Fa-f]+)", record)
        end = re.fullmatch(r"\[SD BACKUP\] file=(\d+) end", record)
        if header:
            number, path_hex, size, crc = header.groups()
            path = path_from_hex(path_hex)
            if active is not None or int(number) != len(files) or inventory.get(path) != int(size) or any(f["path"] == path for f in files):
                raise ValueError("Unexpected file header")
            active = dict(id=int(number), path=path, bytes=int(size), crc32=crc.lower())
            payload = bytearray()
        elif chunk and active is not None:
            number, offset, data = chunk.groups()
            data = bytes.fromhex(data)
            if int(number) != active["id"] or int(offset) != len(payload) or not 1 <= len(data) <= 128 or len(payload) + len(data) > active["bytes"]:
                raise ValueError("Missing, duplicate or out-of-order chunk")
            payload.extend(data)
        elif end and active is not None:
            if int(end[1]) != active["id"] or len(payload) != active["bytes"] or f"{zlib.crc32(payload):08x}" != active["crc32"]:
                raise ValueError("Incomplete file or CRC mismatch")
            active["sha256"] = hashlib.sha256(payload).hexdigest()
            active["payload"] = bytes(payload)
            files.append(active)
            active = None
        else:
            raise ValueError("Malformed backup record")
    if active is not None or len(files) != expected or {f["path"] for f in files} != set(inventory):
        raise ValueError("Incomplete backup")
    identity = f"MEOWKIT SD RECOVERY v1\npartition={partition} start={start} sectors={sectors}\n"
    return identity, directory_paths, files


def export(log, destination):
    if log.stat().st_size > 16 * 1024 * 1024:
        raise ValueError("Log exceeds 16 MiB")
    identity, directories, files = decode(log.read_text(encoding="utf-8"))
    destination.mkdir(parents=True, exist_ok=False)
    def save(target, data):
        with target.open("xb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
    for file in files:
        data = file.pop("payload")
        name = f"{file['id']:04d}.bin"
        target = destination / name
        save(target, data)
        if hashlib.sha256(target.read_bytes()).hexdigest() != file["sha256"]:
            raise OSError("Backup read-back failed")
        file["backup_file"] = name
    save(destination / "identity.txt", identity.encode("ascii"))
    save(destination / "manifest.json", (json.dumps(dict(
        verified=True, identity=identity, directories=directories, files=files), indent=2) + "\n").encode("utf-8"))
    if os.name == "posix":
        fd = os.open(destination, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    export(args.log, args.destination)
    print("CRC/count/path checks and backup read-back passed. No SD card was modified.")
