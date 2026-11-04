#include "nfc.h"
#include "nfc_ui.h"
#include "../../bsp/config.h"
#include <algorithm>

namespace MOONCAKE::APPS {
using namespace meow_nfc;
using namespace meow_nfc::ui;
void App10::footer(const char* a, const char* b) {
    const bool activeA = strcmp(a, "---") != 0;
    const String right = (activeA ? String("[A]") + a + " " : String()) + "[B]" + b;
    hp::drawFooter(_device->Lcd, "^v <>", right.c_str());
    const int bw = (strlen(b) + 3) * 8, aw = (strlen(a) + 3) * 8;
    if (activeA) target(hp::W - hp::PAD_X - bw - aw - 12, 215, aw + 8, 24, 1);
    target(hp::W - hp::PAD_X - bw - 4, 215, bw + 8, 24, 2);
    target(2, 215, 34, 24, 5); target(36, 215, 34, 24, 6);
}
void App10::drawList(const char* title) {
    auto& lcd = _device->Lcd;
    hp::drawHeader(lcd, title, _scene == Scene::Card && _modified ? "NEW" : nullptr);
    footer("Open", _scene == Scene::Main ? "Exit" : "Back");
    const int count = _items.size();
    if (!count) { hp::drawDialog(lcd, "No NFC files", "Check /nfc on the SD card"); return; }
    if (count > hp::LIST2_VIS) {
        target(301, 28, 18, 85, 3); target(301, 113, 18, 85, 4);
    }
    for (int row = 0; row < hp::LIST2_VIS && row + _top < count; ++row) {
        const int index = row + _top;
        const auto titleText = clipped(_items[index].title), subtitle = clipped(_items[index].subtitle);
        hp::drawListItemSub(lcd, row, titleText.c_str(), subtitle.c_str(), index == _selected);
        target(2, hp::CON_Y0 + 2 + row * hp::ITEM2_H, 298, hp::ITEM2_H, 1, index);
    }
    hp::drawScrollbar2(lcd, count, _top, hp::LIST2_VIS);
}
void App10::draw() {
    _dirty = false; _lastDraw = millis(); _targets.clear();
    auto& lcd = _device->Lcd;
    hp::drawChrome(lcd); lcd.setFont(&fonts::efontCN_16);
    target(1, 1, 80, 23, 2);
    auto text = [&](int x, int y, const String& s, uint16_t color = hp::COL_FG) {
        lcd.setTextColor(color, hp::COL_BG); lcd.setCursor(x, y); lcd.print(s);
    };
    switch (_scene) {
        case Scene::Main: drawList("NFC"); break;
        case Scene::Files: drawList("Saved NFC"); break;
        case Scene::Extra: drawList("Extra Actions"); break;
        case Scene::Filter: drawList("Read Type"); break;
        case Scene::Card: drawList(_path.length() ? "Saved Card" : "Read Card"); break;
        case Scene::Delete: drawList("Delete NFC File?"); break;
        case Scene::Discard: drawList("Discard Unsaved Card?"); break;
        case Scene::Name: case Scene::Hex: {
            const bool isHex = _scene == Scene::Hex;
            const int count = isHex ? HexKeyCount : KeyCount, cols = isHex ? 6 : 10;
            const char* labels[KeyCount]; char lowerLabels[KeyCount][2]{};
            for (int n = 0; n < count; ++n) {
                labels[n] = isHex ? HexKeys[n] : Keys[n];
                if (!isHex && _lower && labels[n][0] >= 'A' && labels[n][0] <= 'Z' && !labels[n][1]) {
                    lowerLabels[n][0] = labels[n][0] + 'a' - 'A'; labels[n] = lowerLabels[n];
                }
            }
            const char* heading = _edit == Edit::Uid ? "UID:" : _edit == Edit::Atqa ? "ATQA:" : _edit == Edit::Sak ? "SAK:" : "Name:";
            const int maxLen = !isHex ? 23 : _edit == Edit::Uid ? 20 : _edit == Edit::Atqa ? 4 : 2;
            hp::drawHeader(lcd, isHex ? "Manual ISO14443-3A" : _edit == Edit::Rename ? "Rename NFC" : "Save NFC");
            hp::drawVirtualKeyboard(lcd, heading, _text, maxLen, labels, count, cols, _key);
            footer("Key", "Back");
            const int rows = (count + cols - 1) / cols;
            const int keyW = (288 - (cols - 1) * 3) / cols;
            const int keyH = (130 - (rows - 1) * 3) / rows;
            for (int n = 0; n < count; ++n) target(16 + (n % cols) * (keyW + 3), 68 + (n / cols) * (keyH + 3), keyW, keyH, 1, n);
            break;
        }
        case Scene::Info: case Scene::Ndef: case Scene::Notice: {
            const int pages = std::max(1, (int(_lines.size()) + 7) / 8);
            String badge = String(_page + 1) + "/" + pages;
            const String title = _scene == Scene::Info ? "Card Information" : _scene == Scene::Ndef ? "NDEF" : clipped(_noticeTitle, 24);
            hp::drawHeader(lcd, title.c_str(), pages > 1 ? badge.c_str() : nullptr);
            for (int n = 0; n < 8 && _page * 8 + n < int(_lines.size()); ++n)
                text(8, 32 + n * 21, _lines[_page * 8 + n], n == 0 ? hp::COL_ACCENT : hp::COL_FG);
            footer(_scene == Scene::Notice ? "OK" : "---", "Back");
            break;
        }
        case Scene::Memory: {
            const int pages = std::max(1, (int(_tag->memorySize) + 47) / 48);
            const String badge = String(_page + 1) + "/" + pages;
            hp::drawHeader(lcd, "Public Memory", badge.c_str());
            if (!_tag->memorySize) hp::drawDialog(lcd,
                _tag->memoryState == MemoryState::Failed ? "Memory read failed" : "No memory captured",
                _tag->memoryState == MemoryState::Failed ? "Keep card still and scan again" : "Check Information and NDEF");
            else {
                for (size_t row = 0; row < 6; ++row) {
                    const size_t offset = _page * 48 + row * 8; if (offset >= _tag->memorySize) break;
                    char addr[12]; snprintf(addr, sizeof(addr), "%04X: ", unsigned(offset));
                    const auto data = hex(_tag->memory + offset, std::min<size_t>(8, _tag->memorySize - offset));
                    text(8, 36 + row * 26, String(addr) + data.c_str());
                }
                text(8, 194, clipped(String(_tag->memorySize) + "/" + _tag->memoryExpected + " " +
                    memoryStateName(_tag->memoryState)), hp::COL_DIM);
            }
            footer("---", "Back"); break;
        }
        case Scene::Scan: {
            const char* filters[] = {"AUTO", "A", "B", "F", "V"};
            hp::drawHeader(lcd, "Read NFC", filters[_filter]);
            const bool error = !_readerReady || _status->state == ReaderState::Error;
            if (error) {
                hp::drawDialog(lcd, "NFC Hardware Error", "Check module power and I2C wiring");
                if (_status) {
                    text(8, 154, clipped(_status->message), hp::COL_WARN);
                    text(8, 177, String("Code: ") + _status->errorCode + "  I2C errors: " + _status->ioErrors, hp::COL_WARN);
                }
            } else {
                text(24, 48, _status->state == ReaderState::Reading ? "Card found. Reading..." : "Hold card near antenna", hp::COL_ACCENT);
                hp::drawScanWave(lcd, 58, millis() / 5);
                text(16, 144, clipped(_status->message));
                text(16, 173, "NFC-A / B / F / V", hp::COL_DIM);
                text(16, 194, "Keep card still until complete", hp::COL_DIM);
            }
            footer(error ? "Retry" : "---", "Stop"); break;
        }
        case Scene::Debug: case Scene::Field: {
            const bool error = !_readerReady || _status->state == ReaderState::Error;
            hp::drawHeader(lcd, _scene == Scene::Field ? "RF Field" : "NFC Debug",
                error ? "ERR" : _scene == Scene::Field ? (_status && _status->fieldOn ? "ON" : "WAIT") : "I2C");
            if (!_status) { hp::drawDialog(lcd, "NFC Unavailable", "Worker could not start"); footer("---", "Back"); break; }
            char id[8]; snprintf(id, sizeof(id), "0x%02X", _status->chipId);
            hp::drawStat(lcd, 0, "Chip ID", id);
            hp::drawStat(lcd, 1, "Supply", (String(_status->supplyMv) + " mV").c_str());
            const String bus = String("SDA=") + HAL_PIN_I2C_SDA + " SCL=" + HAL_PIN_I2C_SCL +
                " / " + (HAL_I2C_FREQ / 1000) + "kHz";
            hp::drawStat(lcd, 2, "Bus", bus.c_str());
            hp::drawStat(lcd, 3, "IRQ", "Not wired / polling");
            hp::drawStat(lcd, 4, "I2C errors", String(_status->ioErrors).c_str());
            hp::drawStat(lcd, 5, "External RF", _status->externalField ? "Detected" : "No");
            text(8, 180, clipped(_status->message), error ? hp::COL_WARN : hp::COL_FG);
            text(8, 198, error ? String("Error code ") + _status->errorCode :
                _scene == Scene::Field ? "B stops the transmitter" : "ST25R3916 / 27.12 MHz XTAL", hp::COL_DIM);
            footer(error ? "Retry" : "---", "Stop"); break;
        }
    }
}
} // namespace MOONCAKE::APPS
