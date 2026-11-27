#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-ir-match-tests.XXXXXX)
trap 'rm -f "$temp_dir/"*.o "$temp_dir/matches"; rmdir "$temp_dir"' EXIT
flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
for source in lib/MomentumInfrared/src/*.c lib/MomentumInfrared/src/*/*.c; do
    gcc -std=c11 "${flags[@]}" -Ilib/MomentumInfrared/src -c "$source" -o "$temp_dir/$(basename "$source").o"
done
g++ -std=c++17 "${flags[@]}" -Wall -Wextra -Werror -Ilib/MomentumInfrared/src \
    tools/tests/ir_saved_matches.cpp "$temp_dir/"*.o -o "$temp_dir/matches"
"$temp_dir/matches"
