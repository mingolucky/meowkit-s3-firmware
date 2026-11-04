"""Run the actual ESP32 receiver lifecycle without Arduino hardware.

Usage: python test/infrared/test_receiver_lifecycle.py
Requires a C++17 g++ compiler on PATH. Generated files stay under .pio/.
"""
from pathlib import Path
import argparse
import shutil
import subprocess


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=root / "lib/IRremoteESP8266/src/IRrecv.cpp")
    args = parser.parse_args()
    compiler = shutil.which("g++")
    if not compiler:
        raise SystemExit("g++ is required")
    build = root / ".pio/infrared-receiver-tests"
    build.mkdir(parents=True, exist_ok=True)
    executable = build / "receiver_lifecycle.exe"
    subprocess.run([
        compiler, "-std=c++17", "-O1", "-g", "-DESP32", "-DARDUINO=10819",
        "-DESP_ARDUINO_VERSION_MAJOR=2", "-D_IR_ENABLE_DEFAULT_=false", "-DDECODE_HASH=true",
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "-I" + str(root / "test/infrared/receiver_stubs"),
        "-I" + str(root / "lib/IRremoteESP8266/src"),
        str(args.source), str(root / "test/infrared/receiver_lifecycle.cpp"),
        "-o", str(executable),
    ], check=True, cwd=root)
    failed = []
    scenarios = ["exit_without_learning", "stop_without_start", "back_from_learning",
                 "exit_while_receiving", "retry_learning", "restart_active_receiver",
                 "resume_disabled_receiver", "reopen_app"]
    for scenario in scenarios:
        result = subprocess.run([str(executable), scenario], cwd=root)
        if result.returncode:
            failed.append(scenario)
    if failed:
        raise SystemExit("Failed scenarios: " + ", ".join(failed))
    print(f"All {len(scenarios)} receiver lifecycle scenarios passed.")


if __name__ == "__main__":
    main()
