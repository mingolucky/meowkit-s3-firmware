#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-ir-save-tests.XXXXXX)
trap 'rm -f "$temp_dir/"*.o "$temp_dir/scan" "$temp_dir/save" "$temp_dir/toggle"; rmdir "$temp_dir"' EXIT
flags=(-O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer)
g++ -std=c++17 "${flags[@]}" tools/tests/ir_scan.cpp -o "$temp_dir/scan"
"$temp_dir/scan"
g++ -std=c++17 "${flags[@]}" -Ilib/MomentumInfrared/src tools/tests/ir_native_toggle.cpp -o "$temp_dir/toggle"
"$temp_dir/toggle"
for source in lib/MomentumInfrared/src/*.c lib/MomentumInfrared/src/*/*.c; do
    gcc -std=c11 -O1 -g -fsanitize=address,undefined -Ilib/MomentumInfrared/src -c "$source" -o "$temp_dir/$(basename "$source").o"
done
g++ -std=c++17 "${flags[@]}" -Ilib/MomentumInfrared/src tools/tests/ir_saved_signal.cpp "$temp_dir/"*.o -o "$temp_dir/save"
"$temp_dir/save"
