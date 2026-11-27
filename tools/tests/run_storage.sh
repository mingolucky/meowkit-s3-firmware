#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-storage-tests.XXXXXX)
trap 'rm -f "$temp_dir/mount" "$temp_dir/ir" "$temp_dir/fat" "$temp_dir/assets.S" "$temp_dir/packed/"*.zlib; rmdir "$temp_dir/packed" "$temp_dir"' EXIT
flags=(-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer)
g++ "${flags[@]}" -Itools/tests/sd_fakes tools/tests/sd_mount.cpp src/bsp/sdmmc/SDMMC_Class.cpp -o "$temp_dir/mount"
"$temp_dir/mount"
g++ "${flags[@]}" tools/tests/fat_vfs_writer.cpp -Wl,--wrap=fdopen -Wl,--wrap=fsync -o "$temp_dir/fat"
"$temp_dir/fat"
python3 - "$temp_dir/assets.S" <<'PY'
import importlib.util,sys
from pathlib import Path
spec=importlib.util.spec_from_file_location('assets','build_scripts/infrared_assets.py')
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
Path(sys.argv[1]).write_text(module.assembly(Path('src/app/app_09/assets'),Path(sys.argv[1]).parent/'packed')+'\n.section .note.GNU-stack,"",@progbits\n')
PY
g++ "${flags[@]}" -Itools/tests/ir_fakes tools/tests/ir_storage.cpp src/app/app_09/infrared_builtins.cpp "$temp_dir/assets.S" -lz -o "$temp_dir/ir"
"$temp_dir/ir"
