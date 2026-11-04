/**
 * Momentum infrared flow adapted to MeowKit. See PORTING.md.
 */
#include "infrared.h"
#include "infrared_categories.h"
#include "infrared_ui_data.h"

namespace MOONCAKE::APPS {
using namespace meow_ir::ui;
App09::App09(DEVICES* device) : _device(device) { setAppInfo().name = "Infrared"; }

void App09::onOpen() {
    _signals.clear(); _records.clear(); _actions.clear(); _queue.clear(); _history.clear();
    _moveBackup.clear(); _entries.clear();
    _remotePath = _remoteName = "";
    _directory = "/infrared";
    _mainSelected = _universalSelected = 0;
    _scan = {}; _scanPending = false; _holdLatched = false;
    _learned = {}; _rxEnabled = false;
    _ledActive = _ledOn = false;
    _device->led.off();
    _sender = new IRsend(HAL_PIN_IR_TX);
    _sender->begin();
    _receiver = new IRrecv(HAL_PIN_IR_RX, meow_ir::MaxTimings + 1, 150, true);
    _decoder = infrared_alloc_decoder();
    _txShutdown.store(false); _txCancel.store(false); _txBusy.store(false);
    _txSucceeded.store(true); _txStopped.store(false);
    if (xTaskCreate(_txEntry, "MeowIR-TX", 4096, this, 1, &_txTask) != pdPASS) {
        _txTask = nullptr; _txStopped.store(true);
    }
    if (SD_MMC.cardType() != CARD_NONE) {
        SD_MMC.mkdir("/infrared"); SD_MMC.mkdir("/infrared/remote");
    }
    _readSettings();
    _setScene(Scene::Main);
    _draw();
}
void App09::onRunning() {
    _sampleInput();
    _run();
    _updateLed();
    if (_drawnTxBusy != _txBusy.load()) _dirty = true;
    if (_dirty) _draw();
}
void App09::onClose() {
    _loadingFile.close();
    _stopRx(); _stopTx(); _txShutdown.store(true);
    if (_txTask) {
        xTaskNotifyGive(_txTask);
        while (!_txStopped.load()) delay(1);
        _txTask = nullptr;
    }
    delete _receiver; _receiver = nullptr;
    delete _sender; _sender = nullptr;
    if (_decoder) infrared_free_decoder(_decoder);
    _decoder = nullptr;
    _device->led.off();
    _signals.clear(); _learned = {}; _txSignal = {};
    _records.clear(); _queue.clear(); _actions.clear(); _moveBackup.clear();
    _entries.clear(); _items.clear(); _history.clear();
}
void App09::_setScene(Scene scene) {
    if (_scene == Scene::Loading) _loadingFile.close();
    _stopRx(); _stopTx();
    _scene = scene; _selected = _top = 0;
    _armed = false; _dirty = true;
    for (auto& repeat : _directionRepeat) repeat = {};
    _enter();
}
void App09::_push(Scene scene) {
    _history.push_back({_scene, _selected, _top, _directory});
    _setScene(scene);
}
void App09::_pop() {
    if (_history.empty()) { _setScene(Scene::Main); return; }
    auto frame = _history.back(); _history.pop_back();
    _directory = frame.directory;
    _setScene(frame.scene);
    _selected = frame.selected; _top = frame.top;
    if (!_items.empty()) meow_ir::listMove(0, _items.size(), _listVisible(), _selected, _top);
}
void App09::_returnTo(Scene scene) {
    for (int i = int(_history.size()) - 1; i >= 0; --i) {
        if (_history[i].scene != scene) continue;
        auto frame = _history[i]; _history.resize(i);
        _directory = frame.directory;
        _setScene(scene);
        _selected = frame.selected; _top = frame.top;
        if (!_items.empty()) meow_ir::listMove(0, _items.size(), _listVisible(), _selected, _top);
        return;
    }
    _history.clear();
    if (scene != Scene::Main) _history.push_back({Scene::Main, _mainSelected, 0, "/infrared"});
    _setScene(scene);
}
void App09::_message(const char* title, const String& detail, uint32_t duration) {
    _messageTitle = title; _messageDetail = detail;
    _messageUntil = duration ? millis() + duration : 0;
    _push(Scene::Message);
}
int App09::_listVisible() const {
    return _singleLineList() ? hp::LIST_VIS : hp::LIST2_VIS;
}
bool App09::_singleLineList() const {
    return _scene == Scene::Main || _scene == Scene::Files || _scene == Scene::Universal;
}
void App09::_sampleInput() {
    auto& b = _device->button;
    b.update(); b.tick(); _input = {};
    const bool a = b.A.pressed(), back = b.B.pressed();
    bool up = b.Up.pressed(), down = b.Down.pressed();
    bool left = b.Left.pressed(), right = b.Right.pressed();
    const bool held = b.A.state() == Button_Class::PRESSED;
    // Only physical keys gate a scene transition. Touching the display has
    // no effect, including while the key that opened a scene is released.
    if (!_armed) {
        if (!held && b.B.state() == Button_Class::RELEASED &&
            b.Up.state() == Button_Class::RELEASED && b.Down.state() == Button_Class::RELEASED &&
            b.Left.state() == Button_Class::RELEASED && b.Right.state() == Button_Class::RELEASED) _armed = true;
        return;
    }
    _input.a = a; _input.b = back; _input.held = held;
    if (_scene != Scene::Learn && _scene != Scene::Result) {
        const uint32_t now = millis();
        up = _directionRepeat[0].poll(b.Up.state() == Button_Class::PRESSED, up, now);
        down = _directionRepeat[1].poll(b.Down.state() == Button_Class::PRESSED, down, now);
        left = _directionRepeat[2].poll(b.Left.state() == Button_Class::PRESSED, left, now);
        right = _directionRepeat[3].poll(b.Right.state() == Button_Class::PRESSED, right, now);
    }
    _input.up = up; _input.down = down; _input.left = left; _input.right = right;
}
void App09::_enter() {
    _items.clear();
    switch (_scene) {
        case Scene::Main:
            _items = {{"Universal Remotes", ""},
                      {"Learn New Remote", ""},
                      {"Saved Remotes", ""}};
            _selected = _mainSelected; break;
        case Scene::Files: _refreshFiles(); break;
        case Scene::Remote:
            for (const auto& s : _signals) _items.push_back({s.name.c_str(), protocolName(s)});
            _items.push_back({"+ Add Button", "Learn into this remote"});
            _items.push_back({"Edit", "Rename / reorder / delete"}); break;
        case Scene::Edit:
            _items = {{"Add Button", "Learn into this remote"}, {"Rename Button", "Change a button name"},
                {"Move Button", "Change button order"}, {"Delete Button", "Remove one signal"},
                {"Rename Remote", "Rename the .ir file"}, {"Delete Remote", "Remove this remote file"}};
            break;
        case Scene::ButtonSelect: case Scene::Move:
            for (const auto& s : _signals) _items.push_back({s.name.c_str(), protocolName(s)});
            break;
        case Scene::Universal:
            for (const auto& c : meow_ir::Categories) _items.push_back({c.title, ""});
            _items.push_back({"Load from Library File", ""});
            _selected = _universalSelected; break;
        case Scene::Actions:
            for (const auto& action : _actions) _items.push_back({action, "Scan every matching record"});
            break;
        default: break;
    }
    if (!_items.empty()) meow_ir::listMove(0, _items.size(), _listVisible(), _selected, _top);
}
} // namespace MOONCAKE::APPS
