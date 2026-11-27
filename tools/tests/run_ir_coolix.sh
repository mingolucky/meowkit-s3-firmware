#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
binary=$(mktemp /tmp/meowkit-coolix.XXXXXX)
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -DUNIT_TEST -D_IR_ENABLE_DEFAULT_=false -DSEND_COOLIX=true -DDECODE_COOLIX=true \
  -DSEND_NEC=true -DDECODE_NEC=true -DSEND_SONY=true -DDECODE_SONY=true \
  -DSEND_DAIKIN216=true -DDECODE_DAIKIN216=true \
  -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -Ilib/IRremoteESP8266/src -Ilib/IRremoteESP8266/test \
  tools/tests/ir_coolix.cpp lib/IRremoteESP8266/src/{IRsend,IRrecv,IRutils,IRtimer,IRtext,ir_Coolix,ir_NEC,ir_Sony,ir_Daikin}.cpp \
  -o "$binary"
"$binary" tools/tests/fixtures/coolix-device-raw.txt
