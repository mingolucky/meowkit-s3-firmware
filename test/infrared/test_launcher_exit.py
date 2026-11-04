"""Exercise production launcher exit methods with the real Mooncake lifecycle.

Usage: python test/infrared/test_launcher_exit.py
Only device/display dependencies are replaced by the native harness.
"""
from pathlib import Path
import shutil
import subprocess


def main():
    root = Path(__file__).resolve().parents[2]
    compiler = shutil.which("g++")
    if not compiler:
        raise SystemExit("g++ is required")
    build = root / ".pio/infrared-receiver-tests"
    build.mkdir(parents=True, exist_ok=True)
    source = (root / "src/app/launcher/launcher.cpp").read_text(encoding="utf-8")
    methods = []
    for name in ("updateRunningApp", "closeRunningApp"):
        start = source.index(f"void Launcher::{name}()")
        body = source.index("{", start)
        depth = 1
        end = body + 1
        while depth:
            depth += (source[end] == "{") - (source[end] == "}")
            end += 1
        methods.append(source[start:end])
    (build / "launcher_exit_methods.inc").write_text("\n\n".join(methods), encoding="utf-8")
    mooncake = root / "lib/mooncake/src"
    executable = build / "launcher_exit.exe"
    subprocess.run([
        compiler, "-std=c++17", "-O1", "-I" + str(mooncake), "-I" + str(build),
        str(root / "test/infrared/launcher_exit.cpp"),
        *(str(path) for path in sorted(mooncake.rglob("*.cpp"))),
        "-o", str(executable),
    ], check=True, cwd=root)
    subprocess.run([str(executable)], check=True, cwd=root)


if __name__ == "__main__":
    main()
