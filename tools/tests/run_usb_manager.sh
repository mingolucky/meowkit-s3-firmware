#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
binary=$(mktemp /tmp/meowkit-usb-manager.XXXXXX)
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DARDUINO_USB_MODE=0 -DARDUINO_USB_CDC_ON_BOOT=1 -Itools/tests/usb_manager_fakes \
  tools/tests/usb_manager.cpp src/system/usb_manager.cpp -o "$binary"
"$binary"
