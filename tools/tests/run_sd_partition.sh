#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-partition-tests.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT
mkdir -p "$temp_dir/src/system" "$temp_dir/tools/tests"
cp src/system/sd_partition.h src/system/sd_recovery.h src/system/sd_write_file.h "$temp_dir/src/system/"
cp tools/tests/sd_partition.cpp tools/tests/sd_recovery.cpp "$temp_dir/tools/tests/"
cp -r tools/tests/sd_fixtures "$temp_dir/tools/tests/"
cd "$temp_dir"
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tools/tests/sd_partition.cpp -o test
./test

g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tools/tests/sd_recovery.cpp -Wl,--wrap=fsync -o recovery
./recovery
