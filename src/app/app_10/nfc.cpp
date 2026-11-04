// Application flow follows Momentum NFC. Hardware and display are MeowKit adapters.
#include "nfc.h"
#include "nfc_ui.h"
#include <SD_MMC.h>
#include <algorithm>
#include <new>

namespace MOONCAKE::APPS {
using namespace meow_nfc;
App10::App10(DEVICES* device) : _device(device) { setAppInfo().name = "NFC"; }
void App10::onOpen() {
    _tag.reset(new (std::nothrow) Tag); _status.reset(new (std::nothrow) ReaderSnapshot);
    _history.clear(); _targets.clear(); _entries.clear(); _items.clear(); _lines.clear();
    _directory = "/nfc"; _path = ""; _filter = 0; _modified = false;
    _touchDown = false; _lastPoll = _lastDraw = 0;
    _readerReady = _tag && _status && _reader.begin();
    if (SD_MMC.cardType() != CARD_NONE) SD_MMC.mkdir("/nfc");
    scene(Scene::Main); draw();
}
void App10::onRunning() {
    input();
    if (_readerReady && (_scene == Scene::Scan || _scene == Scene::Debug || _scene == Scene::Field) &&
        millis() - _lastPoll >= 100) {
        _lastPoll = millis();
        if (_reader.snapshot(*_status)) {
            if (_scene == Scene::Scan && _status->state == ReaderState::Found) {
                *_tag = _status->tag; _path = ""; _modified = true;
                scene(Scene::Card);
            } else if (millis() - _lastDraw >= 200) _dirty = true;
        }
    }
    if (_dirty) draw();
}
void App10::onClose() {
    _reader.end(); _readerReady = false;
    _status.reset(); _tag.reset();
    _entries.clear(); _targets.clear(); _items.clear(); _history.clear(); _lines.clear();
}
void App10::scene(Scene value) {
    if (_scene == Scene::Scan || _scene == Scene::Debug || _scene == Scene::Field)
        _reader.request(ReaderMode::Idle);
    _scene = value; _selected = _top = _page = 0; _armed = false; _dirty = true;
    _touchTarget = -1; _targets.clear();
    for (int n = 0; n < 4; ++n) { _repeatAt[n] = 0; _repeatHeld[n] = false; }
    enter();
}
void App10::push(Scene value) { _history.push_back({_scene, _selected, _top, _directory}); scene(value); }
void App10::pop() {
    if (_history.empty()) { scene(Scene::Main); return; }
    auto frame = _history.back(); _history.pop_back(); _directory = frame.directory;
    scene(frame.scene); _selected = frame.selected; _top = frame.top; move(0);
}
void App10::notice(const char* title, const String& text) { _noticeTitle = title; _noticeText = text; push(Scene::Notice); }
void App10::target(int x, int y, int w, int h, uint8_t action, int value) { _targets.push_back({x,y,w,h,value,action}); }
void App10::move(int delta) {
    if (_items.empty()) { _selected = _top = 0; return; }
    _selected = std::max(0, std::min(int(_items.size()) - 1, _selected + delta));
    if (_selected < _top) _top = _selected;
    if (_selected >= _top + hp::LIST2_VIS) _top = _selected - hp::LIST2_VIS + 1;
    _top = std::max(0, std::min(_top, std::max(0, int(_items.size()) - hp::LIST2_VIS)));
    _dirty = true;
}
void App10::input() {
    auto& b = _device->button; b.update(); b.tick();
    bool a = b.A.pressed(), backPressed = b.B.pressed();
    bool directions[] = {b.Up.pressed(), b.Down.pressed(), b.Left.pressed(), b.Right.pressed()};
    const bool held[] = {b.Up.state() == Button_Class::PRESSED, b.Down.state() == Button_Class::PRESSED,
        b.Left.state() == Button_Class::PRESSED, b.Right.state() == Button_Class::PRESSED};
    const bool touched = _device->ctp.isTouched();
    int x = _touchX, y = _touchY; if (touched) _device->ctp.getPos(x, y);
    if (!_armed) {
        _touchDown = touched;
        if (!touched && b.A.state() == Button_Class::RELEASED && b.B.state() == Button_Class::RELEASED &&
            !held[0] && !held[1] && !held[2] && !held[3]) _armed = true;
        return;
    }
    const auto now = millis();
    for (int n = 0; n < 4; ++n) {
        if (!held[n]) _repeatHeld[n] = false;
        else if (!_repeatHeld[n]) { _repeatHeld[n] = true; _repeatAt[n] = now + 420; }
        else if (int32_t(now - _repeatAt[n]) >= 0) { directions[n] = true; _repeatAt[n] = now + 110; }
    }
    int selectedTouch = -1; uint8_t touchAction = 0;
    auto inside = [&](const Target& t) { return x >= t.x && x < t.x+t.w && y >= t.y && y < t.y+t.h; };
    if (touched && !_touchDown) {
        _touchTarget = -1; _touchScene = _scene; _touchX = x; _touchY = y;
        for (size_t n = 0; n < _targets.size(); ++n) if (inside(_targets[n])) { _touchTarget = n; break; }
    } else if (_touchDown && _touchScene == _scene) {
        if (touched && abs(y - _touchY) > 28 && _scene != Scene::Name && _scene != Scene::Hex) {
            directions[y > _touchY ? 0 : 1] = true; _touchTarget = -1; _touchY = y;
        } else if (!touched && _touchTarget >= 0 && _touchTarget < int(_targets.size()) && inside(_targets[_touchTarget])) {
            const auto t = _targets[_touchTarget]; selectedTouch = t.value; touchAction = t.action;
        }
    }
    _touchDown = touched;
    // Release coordinates stay at the touch origin. Reject drags on live samples.
    if (touched && _touchTarget >= 0 && _touchTarget < int(_targets.size()) && !inside(_targets[_touchTarget])) _touchTarget = -1;
    if (touchAction == 1) a = true;
    if (touchAction == 2) backPressed = true;
    if (touchAction == 3) directions[0] = true;
    if (touchAction == 4) directions[1] = true;
    if (touchAction == 5) directions[2] = true;
    if (touchAction == 6) directions[3] = true;
    if (selectedTouch >= 0) { if (_scene == Scene::Name || _scene == Scene::Hex) _key = selectedTouch; else _selected = selectedTouch; _dirty = true; }
    if (backPressed) { back(); return; }
    if (_scene == Scene::Name || _scene == Scene::Hex) {
        const int cols = _scene == Scene::Hex ? 6 : 10;
        const int count = _scene == Scene::Hex ? meow_nfc::ui::HexKeyCount : meow_nfc::ui::KeyCount;
        const int delta = directions[0] ? -cols : directions[1] ? cols : directions[2] ? -1 : directions[3] ? 1 : 0;
        _key = std::max(0, std::min(count - 1, _key + delta)); if (delta) _dirty = true;
    } else if (_scene == Scene::Info || _scene == Scene::Ndef || _scene == Scene::Memory || _scene == Scene::Notice) {
        const int last = _scene == Scene::Memory ? std::max(0, (int(_tag->memorySize) + 47) / 48 - 1) : std::max(0, (int(_lines.size()) + 7) / 8 - 1);
        const int delta = directions[0] || directions[2] ? -1 : directions[1] || directions[3] ? 1 : 0;
        _page = std::max(0, std::min(last, _page + delta)); if (delta) _dirty = true;
    } else if (directions[0] || directions[1] || directions[2] || directions[3]) {
        move(directions[0] ? -1 : directions[1] ? 1 : directions[2] ? -hp::LIST2_VIS : hp::LIST2_VIS);
    }
    if (a) accept();
}
} // namespace MOONCAKE::APPS
