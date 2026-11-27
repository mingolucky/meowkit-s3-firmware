#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
binary=$(mktemp /tmp/meowkit-idle-power.XXXXXX)
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tools/tests/idle_power_policy.cpp -o "$binary"
"$binary"
