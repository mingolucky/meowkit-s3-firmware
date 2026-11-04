"""MeowKit native USB upload handoff. Importing/building never touches a port.

PlatformIO calls before_upload only for its explicit upload target. Standalone:
  <PlatformIO Python> build_scripts/native_usb_upload.py --list
  <PlatformIO Python> build_scripts/native_usb_upload.py --enter-bootloader --port COM7
Neither standalone command writes flash. Requires PlatformIO's pyserial.
"""
import argparse
from dataclasses import dataclass
import fnmatch
import re
import time

APP_USB_ID = (0x303A, 0x0002)  # Arduino TinyUSB defaults, explicit in platformio.ini
BOOT_USB_ID = (0x303A, 0x1001)  # ESP32-S3 hardware USB Serial/JTAG ROM downloader


class UploadError(RuntimeError):
    pass


@dataclass(frozen=True)
class Port:
    device: str
    vid: int = None
    pid: int = None
    serial_number: str = None
    location: str = None
    description: str = ""

    @property
    def usb_id(self):
        return (self.vid, self.pid)


def scan_ports():
    from serial.tools import list_ports
    return [Port(p.device, p.vid, p.pid, p.serial_number, p.location, p.description)
            for p in list_ports.comports()]


def touch_1200(port):
    import serial
    # Set line states before open: avoid pyserial's default DTR/RTS pulse.
    connection = serial.Serial(port=None, baudrate=115200, timeout=0.2, write_timeout=0.2)
    try:
        connection.dtr = False
        connection.rts = False
        connection.port = port.device
        connection.open()
        time.sleep(0.15)
        connection.baudrate = 1200
        time.sleep(0.25)
    finally:
        connection.close()


def _serial(value):
    return re.sub(r"[^a-z0-9]", "", (value or "").lower())


def _location(value):
    # Linux/macOS interface suffixes can change between composite and ROM CDC.
    return (value or "").split(":", 1)[0].lower()


def _physical_match(first, second):
    same_serial = _serial(first.serial_number) and _serial(first.serial_number) == _serial(second.serial_number)
    same_location = _location(first.location) and _location(first.location) == _location(second.location)
    return bool(same_serial or same_location)


def _physical_conflict(first, second):
    # A shared hub location is authoritative even if the ROM formats its serial
    # differently; an equal MAC serial also survives moving between USB paths.
    if _physical_match(first, second):
        return False
    return bool((first.location and second.location) or
                (first.serial_number and second.serial_number))


def _port_identity(port):
    # Friendly names and interface strings can change during Windows driver
    # startup; that does not make an unrelated, pre-existing COM port new.
    return (port.device.lower(), port.vid, port.pid)


def select_target(ports, requested=None, app_id=APP_USB_ID):
    supported = [p for p in ports if p.usb_id in (app_id, BOOT_USB_ID)]
    if requested:
        supported = [p for p in supported if fnmatch.fnmatchcase(p.device.lower(), requested.lower())]
    else:
        # An application is the intended target even if an unrelated board is
        # already in ROM mode. Never prefer an arbitrary Espressif VID alone.
        applications = [p for p in supported if p.usb_id == app_id]
        if applications:
            supported = applications
    if len(supported) == 1:
        return supported[0]
    if len(supported) > 1:
        raise UploadError("Multiple matching boards: " + ", ".join(p.device for p in supported) +
                          ". Select one with --upload-port COMx.")
    raise UploadError("No matching MeowKit CDC/download port" +
                      (" for " + requested if requested else "") +
                      ". Check the USB data cable/driver. Use BOOT only for recovery if firmware cannot run.")


class NativeUsbUploader:
    def __init__(self, scan=scan_ports, touch=touch_1200, sleep=time.sleep,
                 clock=time.monotonic, log=print, app_id=APP_USB_ID):
        self.scan, self.touch = scan, touch
        self.sleep, self.clock, self.log = sleep, clock, log
        self.app_id = app_id

    def prepare(self, requested=None, timeout=15, legacy_cdc=False):
        before = self.scan()
        target = select_target(before, requested, self.app_id)
        if target.usb_id == BOOT_USB_ID and not legacy_cdc:
            self.log("[USB upload] Downloader already present: " + target.device)
            return target
        self.log("[USB upload] Application CDC: " + target.device + "; requesting 1200bps reboot")
        prior_boot = {_port_identity(p) for p in before if p.usb_id == BOOT_USB_ID and p != target}
        for attempt in range(3):
            try:
                self.touch(target)
                break
            except OSError as error:
                # Windows may report disconnection while the baud-rate request
                # itself successfully resets TinyUSB. Look for enumeration first.
                self.sleep(0.25)
                if not any(p.device == target.device and p.usb_id == target.usb_id for p in self.scan()):
                    break
                if attempt == 2:
                    raise UploadError("Cannot open " + target.device +
                                      ". Close PC Monitor/serial monitors before uploading. " + str(error)) from error
        if legacy_cdc:
            # The old BOX variant advertised 303A:1001 for TinyUSB too. This
            # explicit migration option also handles Windows retaining the COM
            # number across that same-ID restart; let ROM finish first.
            self.sleep(1.5)
        deadline = self.clock() + timeout
        self.log("[USB upload] Waiting for the same board's ROM USB port...")
        last_ports = None
        departed = False
        while self.clock() < deadline:
            current = self.scan()
            if current != last_ports:
                for p in current:
                    identity = "%04X:%04X" % p.usb_id if None not in p.usb_id else "not USB"
                    self.log("[USB upload] Observed " + p.device + " " + identity +
                             " serial=" + (p.serial_number or "?") + " location=" + (p.location or "?"))
                if not current: self.log("[USB upload] Application detached; no serial port enumerated")
                last_ports = current
            present = any(p.device == target.device and p.usb_id == target.usb_id for p in current)
            departed = departed or not present
            candidates = [p for p in current if p.usb_id == BOOT_USB_ID]
            matching = [p for p in candidates if _physical_match(target, p)]
            if not matching:
                matching = [p for p in candidates if _port_identity(p) not in prior_boot and not _physical_conflict(target, p)]
            if len(matching) > 1:
                raise UploadError("Ambiguous ROM USB ports after reboot; disconnect other boards and retry.")
            if matching:
                self.log("[USB upload] ROM downloader: " + matching[0].device)
                self.sleep(0.3)  # allow the Windows usbser device to finish starting
                return matching[0]
            self.sleep(0.2)
        if not last_ports:
            detail = "Application detached, but Windows did not enumerate a ROM serial port (firmware reset/USB handoff or driver failure)."
        elif any(p.device == target.device and p.usb_id == target.usb_id for p in last_ports):
            detail = "Device returned to application mode." if departed else "Application CDC did not leave after the 1200bps request."
        else:
            detail = "Serial ports were observed but none matched the selected board; see identities above."
        raise UploadError("Timed out waiting for the selected board's ROM port. " + detail +
                          " A hung device needs BOOT/RESET recovery before installing the fixed firmware.")


def define_usb_id(defines):
    values = dict(APP_USB_VID=APP_USB_ID[0], APP_USB_PID=APP_USB_ID[1])
    for definition in defines:
        if isinstance(definition, (tuple, list)) and len(definition) == 2:
            key, value = definition
            if key in ("USB_VID", "USB_PID", "MEOW_USB_VID", "MEOW_USB_PID"):
                name = key.removeprefix("MEOW_")
                values["APP_" + name] = int(str(value).strip().rstrip("uUlL"), 0)
    result = (values["APP_USB_VID"], values["APP_USB_PID"])
    if result == BOOT_USB_ID:
        raise UploadError("Application USB VID/PID must differ from the ROM downloader.")
    return result


def configure_esptool_flags(flags):
    result = list(flags)
    for key, value in (("--before", "no_reset"), ("--after", "hard_reset")):
        positions = [n for n, item in enumerate(result) if item == key]
        if len(positions) != 1 or positions[0] + 1 == len(result):
            raise UploadError("Unexpected PlatformIO esptool flags: " + key)
        result[positions[0] + 1] = value
    return result


def before_upload(source, target, env):
    if env.subst("$UPLOAD_PROTOCOL") != "esptool":
        return
    try:
        # Fail before touching hardware if the upload command is incompatible.
        flags = configure_esptool_flags(env.get("UPLOADERFLAGS", []))
        app_id = define_usb_id(env.get("CPPDEFINES", []))
        selected = NativeUsbUploader(app_id=app_id).prepare(env.subst("$UPLOAD_PORT") or None)
        import os
        tool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
        wrapper = os.path.join(env.subst("$PROJECT_DIR"), "build_scripts", "esptool_meowkit.py")
        env.Replace(UPLOAD_PORT=selected.device, UPLOADER=wrapper,
                    UPLOADERFLAGS=["--esptool-dir", tool_dir] + flags)
    except (UploadError, ValueError) as error:
        print("[USB upload] " + str(error))
        env.Exit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--list", action="store_true", help="list ports without opening or resetting them")
    operation.add_argument("--enter-bootloader", action="store_true", help="reboot to download mode; does not flash")
    parser.add_argument("--port", help="application or ROM serial port")
    parser.add_argument("--legacy-cdc", action="store_true",
                        help="touch an old application that shares the ROM's 303A:1001 ID")
    args = parser.parse_args()
    try:
        if args.list:
            for port in scan_ports():
                identity = "%04X:%04X" % port.usb_id if None not in port.usb_id else "not USB"
                print(port.device, identity, port.description, "serial=" + (port.serial_number or ""))
        else:
            NativeUsbUploader().prepare(args.port, legacy_cdc=args.legacy_cdc)
    except (UploadError, ImportError, OSError) as error:
        parser.exit(1, str(error) + "\n")


if "Import" in globals():
    Import("env")  # noqa: F821 - provided by SCons, never runs during a normal import
    # C++ only: do not inject Arduino's pin aliases into C libraries or assembly.
    env.Append(CXXFLAGS=["-include", "src/bsp/usb_config.h"])  # noqa: F821
    env.AddPreAction("upload", before_upload)  # noqa: F821
elif __name__ == "__main__":
    main()
