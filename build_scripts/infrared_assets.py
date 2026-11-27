"""Embed universal dictionaries in read-only flash; no SD copying required."""
from pathlib import Path
import hashlib
import struct
import zlib

NAMES = ("tv", "projectors", "audio", "ac", "leds", "fans", "bluray_dvd", "monitor", "digital_sign")


def assembly(asset_dir, output):
    output.mkdir(parents=True, exist_ok=True)
    chunks = [".section .rodata\n.balign 4\n"]
    for name in NAMES:
        path = asset_dir / (name+".ir")
        if not path.is_file():
            raise RuntimeError(f"Missing universal IR dictionary: {path}")
        data = path.read_bytes()
        packed = output / (name+".zlib")
        payload = struct.pack("<I",len(data))+zlib.compress(data,9)
        if not packed.exists() or packed.read_bytes()!=payload:
            packed.write_bytes(payload)
        # The digest changes the generated source on every dictionary edit,
        # so SCons cannot reuse an object containing an older .incbin payload.
        chunks.append(f'/* {name} SHA256 {hashlib.sha256(data).hexdigest()} */\n'
                      f'.global ir_{name}_start\n.global ir_{name}_end\nir_{name}_start:\n'
                      f'.incbin "{packed.resolve().as_posix()}"\nir_{name}_end:\n.balign 4\n')
    return "".join(chunks)


if "Import" in globals():
    Import("env")  # noqa: F821
    output = Path(env.subst("$BUILD_DIR")) / "infrared-assets"  # noqa: F821
    output.mkdir(parents=True, exist_ok=True)
    source = output / "ir_assets.S"
    content = assembly(Path(env.subst("$PROJECT_DIR")) / "src/app/app_09/assets",output / "packed")  # noqa: F821
    if not source.exists() or source.read_text()!=content:
        source.write_text(content)
    env.BuildSources(str(output / "obj"), str(output), src_filter=["+<ir_assets.S>"])  # noqa: F821
