#include "infrared.h"
#include "infrared_categories.h"
#include "infrared_ui_data.h"

namespace MOONCAKE::APPS {
using namespace meow_ir::ui;
void App09::_drawNameKeyboard() {
    auto& lcd = _device->Lcd;
    char count[12]; snprintf(count, sizeof(count), "%u/23", unsigned(strlen(_name)));
    lcd.setTextColor(hp::COL_DIM, hp::COL_BG);
    lcd.setCursor(16, 31); lcd.print(_symbols ? "Symbols" : _lowercase ? "Lowercase" : "Uppercase");
    lcd.setCursor(256, 31); lcd.print(count);
    lcd.drawRect(12, 49, 296, 22, hp::COL_FG);
    lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
    lcd.setCursor(18, 52); lcd.print(_name);
    if (strlen(_name) < sizeof(_name) - 1) lcd.print('_');
    for (int i = 0; i < characterCount(_symbols); ++i) {
        const char c = keyCharacter(i, _symbols, _lowercase);
        char label[] = {c, 0};
        _drawButton(16 + (i % 10) * 29, 77 + (i / 10) * 26, 26, 23,
                    c == ' ' ? "SP" : label, i == _key);
    }
    const char* controls[] = {"Aa", _symbols ? "ABC" : "Sym", "Del", "Save"};
    for (int i = 0; i < 4; ++i)
        _drawButton(16 + i * 73, 184, 68, 23, controls[i], CharacterCells + i == _key);
}
void App09::_drawFooter(const char* left, const char* a, const char* b) {
    const String aHint = a && *a ? String("[A]") + a : String("");
    const String bHint = b && *b ? String("[B]") + b : String("");
    hp::drawFooter3(_device->Lcd, left, aHint.c_str(), bHint.c_str());
}
void App09::_drawButton(int x, int y, int w, int h, const char* label,
                        bool selected) {
    auto& lcd = _device->Lcd;
    uint16_t background = selected ? hp::COL_HL : hp::COL_BG;
    lcd.fillRect(x, y, w, h, background);
    lcd.drawRect(x, y, w, h, hp::COL_FG);
    lcd.setTextColor(selected ? hp::COL_BG : hp::COL_FG, background);
    String caption = label;
    if (caption.length() > 1) caption.replace("_", " ");
    caption = clipped(caption, (w - 6) / 8);
    lcd.setCursor(x + std::max(3, (w - int(caption.length()) * 8) / 2), y + (h - 16) / 2);
    lcd.print(caption);
}
void App09::_drawList(const char* title, const char* action) {
    auto& lcd = _device->Lcd;
    hp::drawHeader(lcd, title, _drawnTxBusy ? "TX" : nullptr, hp::COL_WARN);
    _drawFooter(_scene == Scene::Files && !_libraryBrowser ? "^v R:Del" : "[^v]Select", action,
                _scene == Scene::Main ? "Exit" : "Back");
    const int count = _items.size();
    const bool single = _singleLineList();
    const int visible = _listVisible();
    meow_ir::listMove(0, count, visible, _selected, _top);
    if (!count) {
        hp::drawDialog(lcd, "No items", _scene == Scene::Files ? "Check /infrared on SD" : "Learn a signal first");
        return;
    }
    for (int row = 0; row < visible && _top + row < count; ++row) {
        int index = _top + row;
        auto titleText = clipped(_items[index].title), sub = clipped(_items[index].subtitle);
        if (single) hp::drawListItem(lcd, row, titleText.c_str(), index == _selected);
        else hp::drawListItemSub(lcd, row, titleText.c_str(), sub.c_str(), index == _selected);
    }
    if (single) hp::drawScrollbar(lcd, count, _top, visible);
    else hp::drawScrollbar2(lcd, count, _top, visible);
}
void App09::_draw() {
    _dirty = false; _lastDraw = millis(); _drawnTxBusy = _txBusy.load();
    auto& lcd = _device->Lcd;
    hp::drawChrome(lcd);
    lcd.setFont(&fonts::efontCN_16);
    switch (_scene) {
        case Scene::Main: _drawList("Infrared", "Enter"); break;
        case Scene::Files: _drawList(_libraryBrowser ? "Library File" : "Saved Remotes"); break;
        case Scene::Remote: _drawList(clipped(_remoteName, 24).c_str(), _selected < int(_signals.size()) ? "Hold" : "Enter"); break;
        case Scene::Edit: _drawList("Edit Remote", "Select"); break;
        case Scene::ButtonSelect: _drawList(_editFor == EditFor::Rename ? "Rename Button" : "Delete Button", "Select"); break;
        case Scene::Move: _drawList(_moveIndex < 0 ? "Move: Pick Button" : "Move: Up / Down", _moveIndex < 0 ? "Pick" : "Save"); break;
        case Scene::Universal: _drawList("Universal Remotes", "Enter"); break;
        case Scene::Actions: _drawList(clipped(_libraryTitle, 24).c_str(), "Scan"); break;
        case Scene::Name:
            hp::drawHeader(lcd, _nameFor == NameFor::Learn ? "Name the Button" :
                _nameFor == NameFor::Button ? "Rename Button" : "Rename Remote");
            _drawFooter("^v<> Move", _key == KeySave ? "Save" : "Key", "Back");
            _drawNameKeyboard();
            break;
        case Scene::Loading:
            hp::drawHeader(lcd, "Loading IR Library");
            _drawFooter("", "--", "Cancel");
            hp::drawHourglassScreen(lcd, clipped(_libraryTitle).c_str(), millis() / 8);
            break;
        case Scene::Learn:
            hp::drawHeader(lcd, "Learn Signal", "RX", hp::COL_WARN);
            _drawFooter("L:Mode R:Decode", _easy ? "Skip" : "", "Back");
            lcd.setTextColor(hp::COL_FG, hp::COL_BG);
            lcd.setCursor(12, 40); lcd.print("Point remote at IR port");
            lcd.setCursor(12, 62); lcd.print("and press a button");
            lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
            lcd.setCursor(12, 80); lcd.print(_easy ? EasyNames[_easyIndex] : "Waiting for signal...");
            hp::drawScanWave(lcd, 72, millis() / 8);
            hp::drawStat(lcd, 5, "Mode", _easy ? "Easy" : "Manual");
            hp::drawStat(lcd, 6, "Decoding", decodeName(_decodeMode));
            break;
        case Scene::Result: {
            hp::drawHeader(lcd, "Signal Received", _drawnTxBusy ? "TX" : "OK", hp::COL_WARN);
            _drawFooter("L:Retry R:Save", "Send", "Back");
            char text[64];
            hp::drawStat(lcd, 0, "Protocol", protocolName(_learned), hp::COL_FG);
            if (_learned.raw) {
                snprintf(text, sizeof(text), "%u", unsigned(_learned.timings.size())); hp::drawStat(lcd, 1, "Samples", text);
                snprintf(text, sizeof(text), "%lu Hz", static_cast<unsigned long>(_learned.frequency)); hp::drawStat(lcd, 2, "Carrier", text);
                snprintf(text, sizeof(text), "%.0f%%", double(_learned.duty * 100)); hp::drawStat(lcd, 3, "Duty", text);
            } else {
                snprintf(text, sizeof(text), "0x%08lX", static_cast<unsigned long>(_learned.address)); hp::drawStat(lcd, 1, "Address", text);
                snprintf(text, sizeof(text), "0x%08lX", static_cast<unsigned long>(_learned.command)); hp::drawStat(lcd, 2, "Command", text);
            }
            lcd.setCursor(12, 178); lcd.print("Hold A to repeat");
            break;
        }
        case Scene::Discard:
            hp::drawHeader(lcd, _discardRetry ? "Retry Reading?" : "Exit Learning?");
            hp::drawDialog(lcd, "Unsaved signal will be lost", "[A] Discard     [B] Stay");
            _drawFooter("", "Discard", "Stay");
            break;
        case Scene::Delete:
            hp::drawHeader(lcd, _deleteRemote ? (_deleteFolder ? "Delete Folder?" : "Delete Remote?") : "Delete Button?");
            hp::drawDialog(lcd, "Confirm deletion", clipped(_deleteRemote ? _deletePath : String(_signals[_editIndex].name.c_str())).c_str());
            _drawFooter("", "Delete", "Cancel");
            break;
        case Scene::Panel: {
            const auto& category = meow_ir::Categories[_category];
            hp::drawHeader(lcd, category.title);
            _drawFooter("^v<> Select", "Scan", "Back");
            const int rows = (category.count + 1) / 2, gap = 6;
            const int height = (hp::CON_H - 12 - (rows - 1) * gap) / rows;
            for (int i = 0; i < category.count; ++i)
                _drawButton(8 + (i % 2) * 155, hp::CON_Y0 + 6 + (i / 2) * (height + gap),
                    149, height, category.actions[i], i == _selected);
            break;
        }
        case Scene::Scan: {
            hp::drawHeader(lcd, "Universal Scan", _scan.paused ? "PAUSED" : "TX", hp::COL_WARN);
            _drawFooter(_scan.paused ? "[<>]Select" : "", _scan.paused ? "Send" : "Pause", "Stop");
            hp::drawStat(lcd, 0, "Action", _scanName.c_str());
            char text[48];
            const size_t current = std::min(_scan.index + 1, _scan.total);
            snprintf(text, sizeof(text), "%u / %u", unsigned(current), unsigned(_scan.total));
            hp::drawStat(lcd, 1, "Signal", text, hp::COL_ACCENT);
            hp::drawStat(lcd, 2, "State", _scan.paused ? "Paused / manual test" : "Scanning library");
            hp::drawProgress(lcd, 12, 126, 296, _scan.total ? float(current) / _scan.total : 0.0f, hp::COL_FG);
            if (_scan.paused) {
                lcd.setCursor(12, 164); lcd.print("Left / Right: Select signal");
                lcd.setCursor(12, 186); lcd.print("Up: Resume scanning");
            }
            break;
        }
        case Scene::Message:
            hp::drawHeader(lcd, clipped(_messageTitle, 26).c_str());
            _drawFooter("", "OK", "Back");
            lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
            for (size_t offset = 0, row = 0; offset < _messageDetail.length() && row < 7; offset += 36, ++row) {
                lcd.setCursor(12, 44 + row * 22); lcd.print(_messageDetail.substring(offset, offset + 36));
            }
            break;
    }
}
} // namespace MOONCAKE::APPS
