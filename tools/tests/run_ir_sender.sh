#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
binary=$(mktemp /tmp/meowkit-ir-sender.XXXXXX)
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DUNIT_TEST -D_IR_ENABLE_DEFAULT_=false -DSEND_COOLIX=true \
  -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -Itools/tests/ir_sender_fakes -Ilib/IRremoteESP8266/src -Ilib/IRremoteESP8266/test \
  tools/tests/ir_sender.cpp lib/IRremoteESP8266/src/{IRsend,IRutils,IRtimer,IRtext,ir_Coolix}.cpp -o "$binary"
"$binary"
