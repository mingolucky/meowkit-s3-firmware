#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
temp_dir=$(mktemp -d /tmp/meowkit-ble-scan.XXXXXX)
trap 'rm -rf "$temp_dir"' EXIT
flags=('-DMEOW_BLE_TARGET_ID="4G123456789"' -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer)
g++ "${flags[@]}" tools/tests/ble_scan_data.cpp -o "$temp_dir/data"
"$temp_dir/data"
g++ "${flags[@]}" -Itools/tests/ble_scan_fakes tools/tests/ble_scanner.cpp \
    src/app/app_12/ble_scanner.cpp -o "$temp_dir/scanner"
"$temp_dir/scanner"
# Replace only the hardware aggregator; compile the actual scanner and app UI.
mkdir -p "$temp_dir/src/app/app_12" "$temp_dir/src/app/app_common" "$temp_dir/src/bsp" "$temp_dir/src/system"
cp src/app/app_12/*.{h,cpp} "$temp_dir/src/app/app_12/"
cp src/app/app_common/hp_ui.h "$temp_dir/src/app/app_common/"
cp src/system/bluetooth_manager.h "$temp_dir/src/system/"
cp tools/tests/ble_scan_fakes/devices.h "$temp_dir/src/bsp/devices.h"
g++ "${flags[@]}" -Wno-misleading-indentation -Itools/tests/ble_scan_fakes -I"$temp_dir/src/app/app_12" \
    tools/tests/ble_scan_ui.cpp "$temp_dir/src/app/app_12/ble_scan_app.cpp" \
    "$temp_dir/src/app/app_12/ble_scanner.cpp" -o "$temp_dir/ui"
"$temp_dir/ui"
python3 - <<'PY'
from pathlib import Path
import re
root=Path('.')
menu=(root/'src/ui/screens/ui_apps_menu.c').read_text()
header=(root/'src/ui/screens/ui_apps_menu.h').read_text()
registry=(root/'src/app/app.h').read_text()
assert '#define APPS_MENU_MAX_APPS  11' in header
for name in ('app_ptrs','name_ptrs'):
    blocks=re.findall(rf'{name}\[APPS_MENU_MAX_APPS\]\s*=\s*\{{(.*?)\}}',menu,re.S)
    assert len(blocks)==2
    for block in blocks:
        assert len(re.findall(r'&ui_(?:app|name)_\d+',block))==11
assert 'App12>(dev)' in registry and 'BLE Scanner' in menu
print('BLE scanner launcher slot checks passed')
PY
