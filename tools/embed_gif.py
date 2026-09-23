"""
PlatformIO pre-build extra script.
Converts boot_animation.gif → splash_gif_data.S  (GNU AS .incbin, instant)
                       → splash_gif.h       (tiny extern declarations)

Result: GIF bytes land in Flash RODATA with zero C compilation overhead.
"""
Import("env")  # noqa: F821  (PlatformIO SCons magic)
import os

project_dir = env.subst("$PROJECT_DIR")
gif_path    = os.path.join(project_dir, "boot_animation.gif")
splash_dir  = os.path.join(project_dir, "src", "splash")

asm_out = os.path.join(splash_dir, "splash_gif_data.S")
hdr_out = os.path.join(splash_dir, "splash_gif.h")

gif_exists = os.path.isfile(gif_path)
gif_size   = os.path.getsize(gif_path) if gif_exists else 0
gif_fwd    = gif_path.replace("\\", "/")   # GAS requires forward slashes on Windows

boot_mp3_path = os.path.join(project_dir, "boot_sound effects.mp3")
button_mp3_path = os.path.join(project_dir, "button_sound effects.mp3")
sound_asm_out = os.path.join(project_dir, "src", "system", "system_sound_assets.S")
sound_cpp_out = os.path.join(project_dir, "src", "system", "system_sound_assets.cpp")

def gif_duration_ms(path):
    """Parse GIF blocks and sum frame delays (1/100 second units)."""
    if not os.path.isfile(path):
        return 0
    data = open(path, "rb").read()
    if len(data) < 13 or data[:3] != b"GIF":
        return 0
    packed = data[10]
    pos = 13 + (3 * (1 << ((packed & 7) + 1)) if packed & 0x80 else 0)
    total_cs = 0
    while pos < len(data):
        introducer = data[pos]
        pos += 1
        if introducer == 0x3B:  # trailer
            break
        if introducer == 0x21:  # extension
            if pos >= len(data): break
            label = data[pos]
            pos += 1
            if label == 0xF9 and pos + 5 <= len(data):
                size = data[pos]
                if size >= 4:
                    total_cs += data[pos + 2] | (data[pos + 3] << 8)
            while pos < len(data):
                size = data[pos]
                pos += 1
                if size == 0: break
                pos += size
        elif introducer == 0x2C:  # image descriptor and compressed pixels
            if pos + 9 > len(data): break
            image_packed = data[pos + 8]
            pos += 9
            if image_packed & 0x80:
                pos += 3 * (1 << ((image_packed & 7) + 1))
            pos += 1  # LZW minimum code size
            while pos < len(data):
                size = data[pos]
                pos += 1
                if size == 0: break
                pos += size
        else:
            break
    return total_cs * 10

def mp3_prefix_bytes(path, limit_ms):
    """Return a complete-frame MP3 prefix covering limit_ms, including ID3."""
    if not os.path.isfile(path) or limit_ms <= 0:
        return 0
    data = open(path, "rb").read()
    pos = 0
    if data[:3] == b"ID3" and len(data) >= 10:
        pos = 10 + ((data[6] & 0x7f) << 21) + ((data[7] & 0x7f) << 14) + \
              ((data[8] & 0x7f) << 7) + (data[9] & 0x7f)
    elapsed_samples = 0
    elapsed_rate = 1
    bitrates = {
        (3, 1): [0,32,40,48,56,64,80,96,112,128,160,192,224,256,320],
        (2, 1): [0,8,16,24,32,40,48,56,64,80,96,112,128,144,160],
        (0, 1): [0,8,16,24,32,40,48,56,64,80,96,112,128,144,160],
    }
    rates = {3: [44100,48000,32000], 2: [22050,24000,16000],
             0: [11025,12000,8000]}
    while pos + 4 <= len(data):
        h = int.from_bytes(data[pos:pos + 4], "big")
        if (h & 0xffe00000) != 0xffe00000:
            pos += 1
            continue
        version = (h >> 19) & 3
        layer = (h >> 17) & 3
        bitrate_i = (h >> 12) & 15
        rate_i = (h >> 10) & 3
        padding = (h >> 9) & 1
        if version == 1 or layer != 1 or bitrate_i in (0, 15) or rate_i == 3:
            pos += 1
            continue
        bitrate = bitrates[(version, 1)][bitrate_i] * 1000
        rate = rates[version][rate_i]
        frame_len = ((144 if version == 3 else 72) * bitrate // rate) + padding
        samples = 1152 if version == 3 else 576
        if frame_len < 4 or pos + frame_len > len(data):
            break
        pos += frame_len
        elapsed_samples += samples
        elapsed_rate = rate
        if elapsed_samples * 1000 >= limit_ms * elapsed_rate:
            return pos
    return pos

# ── build output ──────────────────────────────────────────────────────────────
if gif_exists:
    asm_body = (
        "    .section .rodata\n"
        "    .balign 4\n"
        "    .global splash_gif_data\n"
        "splash_gif_data:\n"
        f'    .incbin "{gif_fwd}"\n'
        "    .balign 4\n"
    )
    hdr_body = (
        "#pragma once\n"
        "#include <stdint.h>\n\n"
        "#ifdef __cplusplus\n"
        'extern "C" {\n'
        "#endif\n"
        "extern const uint8_t splash_gif_data[];\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n\n"
        f"static const size_t splash_gif_len = {gif_size}u;\n"
    )
    print(f"[embed_gif] {gif_fwd}  ({gif_size/1024:.1f} KB) → .incbin in Flash RODATA")

else:
    # No GIF present — emit a zero-length symbol so the project still compiles.
    # SplashScreen::show() checks splash_gif_len == 0 and shows text fallback.
    asm_body = (
        "    .section .rodata\n"
        "    .balign 4\n"
        "    .global splash_gif_data\n"
        "splash_gif_data:\n"
    )
    hdr_body = (
        "#pragma once\n"
        "#include <stdint.h>\n\n"
        "#ifdef __cplusplus\n"
        'extern "C" {\n'
        "#endif\n"
        "extern const uint8_t splash_gif_data[];\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n\n"
        "static const size_t splash_gif_len = 0u;  // boot_animation.gif not found\n"
    )
    print("[embed_gif] boot_animation.gif not found — splash will use text fallback")

# Write only when content changed (avoids needless recompilation)
for path, body in ((asm_out, asm_body), (hdr_out, hdr_body)):
    existing = open(path).read() if os.path.isfile(path) else ""
    if existing != body:
        with open(path, "w") as f:
            f.write(body)

# Embed system sounds in flash. MP3 is decoded at runtime; the boot service
# stops decoding at the GIF's calculated duration.
boot_exists = os.path.isfile(boot_mp3_path)
button_exists = os.path.isfile(button_mp3_path)
boot_source_size = os.path.getsize(boot_mp3_path) if boot_exists else 0
button_size = os.path.getsize(button_mp3_path) if button_exists else 0
duration_ms = gif_duration_ms(gif_path)
boot_size = mp3_prefix_bytes(boot_mp3_path, duration_ms) if boot_exists else 0

def incbin_symbol(symbol, path, exists, length=0):
    if not exists:
        return f"    .global {symbol}\n{symbol}:\n"
    return (f"    .global {symbol}\n{symbol}:\n"
            f'    .incbin "{path.replace(chr(92), "/")}", 0, {length}\n'
            "    .balign 4\n")

sound_asm = ("    .section .rodata\n"
             "    .balign 4\n" +
             incbin_symbol("boot_sound_mp3_data", boot_mp3_path, boot_exists, boot_size) +
             incbin_symbol("button_sound_mp3_data", button_mp3_path, button_exists, button_size))
sound_cpp = ('#include "system_sound_assets.h"\n\n'
             f"const size_t boot_sound_mp3_len = {boot_size}u;\n"
             f"const size_t button_sound_mp3_len = {button_size}u;\n"
             f"const uint32_t boot_animation_duration_ms = {duration_ms}u;\n")

for path, body in ((sound_asm_out, sound_asm), (sound_cpp_out, sound_cpp)):
    existing = open(path).read() if os.path.isfile(path) else ""
    if existing != body:
        with open(path, "w") as f:
            f.write(body)

print(f"[embed_sound] boot={boot_size/1024:.1f}/{boot_source_size/1024:.1f} KB, "
      f"button={button_size/1024:.1f} KB, limit={duration_ms} ms")
