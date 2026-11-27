#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-msc-tests.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT
mkdir -p "$temp_dir/src/system" "$temp_dir/src/bsp/sdmmc" "$temp_dir/tools/tests"
cp src/system/sd_clock.h src/system/usb_msc_transfer.h src/system/usb_msc.cpp src/system/usb_msc.h "$temp_dir/src/system/"
cp src/bsp/config.h "$temp_dir/src/bsp/"
cp src/bsp/sdmmc/SDMMC_Class.hpp "$temp_dir/src/bsp/sdmmc/"
cp tools/tests/sd_clock.cpp tools/tests/usb_msc_transfer.cpp tools/tests/usb_msc_lifecycle.cpp "$temp_dir/tools/tests/"
cp -r tools/tests/msc_fakes "$temp_dir/tools/tests/"
cd "$temp_dir"
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tools/tests/usb_msc_transfer.cpp -o transfer
./transfer

g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Itools/tests/msc_fakes tools/tests/usb_msc_lifecycle.cpp src/system/usb_msc.cpp -pthread -o lifecycle
./lifecycle

g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tools/tests/sd_clock.cpp -o clock
./clock
