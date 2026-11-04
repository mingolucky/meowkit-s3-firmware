#include "infrared.h"
#include "infrared_categories.h"
#include "infrared_ui_data.h"
#include <cstring>
#include <cctype>

namespace MOONCAKE::APPS {
using namespace meow_ir::ui;
void App09::_openFiles(bool library) {
    _libraryBrowser = library;
    _directory = "/infrared";
    _push(Scene::Files);
}
void App09::_startLearn(bool newRemote) {
    _learningNew = newRemote;
    _learnOrigin = newRemote ? Scene::Main : Scene::Remote;
    _easyIndex = 0; _nextEasy();
    _push(Scene::Learn);
}
void App09::_nextEasy() {
    if (_easyIndex >= EasyCount) _easyIndex = 0;
    if (_learningNew) return;
    for (size_t n = _easyIndex; n < EasyCount; ++n) {
        bool exists = false;
        for (const auto& s : _signals) if (!strcasecmp(s.name.c_str(), EasyNames[n])) { exists = true; break; }
        if (!exists) { _easyIndex = n; return; }
    }
}
void App09::_beginName(NameFor kind, const char* initial) {
    _nameFor = kind; _key = 0; _lowercase = _symbols = false;
    snprintf(_name, sizeof(_name), "%s", initial);
    _push(Scene::Name);
}
bool App09::_duplicateName(const char* name, int except) const {
    for (size_t i = 0; i < _signals.size(); ++i)
        if (int(i) != except && !strcasecmp(_signals[i].name.c_str(), name)) return true;
    return false;
}
void App09::_commitName() {
    const bool valid = _nameFor == NameFor::Remote ? infrared_file::validName(_name) : infrared_file::validButtonName(_name);
    if (!valid) {
        _message("Invalid Name", _nameFor == NameFor::Remote ? "Avoid / \\ : * ? \" < > | and trailing dots/spaces" : "Use a name without leading/trailing spaces");
        return;
    }
    if (_nameFor == NameFor::Remote) {
        if (!_renameRemote(_name)) { _message("Rename Failed", _lastError); return; }
        _returnTo(Scene::Remote); _message("Renamed", _remoteName, 1200); return;
    }
    if ((!_learningNew || _nameFor == NameFor::Button) &&
        _duplicateName(_name, _nameFor == NameFor::Button ? _editIndex : -1)) {
        _message("Name Exists", "Choose another button name"); return;
    }
    if (_nameFor == NameFor::Learn) { _learned.name = _name; _finishLearn(); }
    else {
        auto changed = _signals; changed[_editIndex].name = _name;
        if (!_writeRemote(_remotePath, changed, false)) { _message("Save Failed", _lastError); return; }
        _signals = std::move(changed);
        _returnTo(Scene::Remote); _message("Renamed", _name, 1200);
    }
}
void App09::_finishLearn() {
    auto changed = _learningNew ? std::vector<meow_ir::Signal>{} : _signals;
    if (changed.size() >= meow_ir::MaxButtons) { _message("Remote Full", "Maximum 256 buttons"); return; }
    changed.push_back(_learned);
    String path = _remotePath;
    if (_learningNew) {
        if (!SD_MMC.exists("/infrared/remote") && !SD_MMC.mkdir("/infrared/remote")) {
            _message("Save Failed", "Cannot create /infrared/remote"); return;
        }
        path = "/infrared/remote/Remote.ir";
        for (unsigned n = 2; SD_MMC.exists(path) && n < 100000; ++n)
            path = String("/infrared/remote/Remote") + n + ".ir";
    }
    if (!_writeRemote(path, changed, _learningNew)) { _message("Save Failed", _lastError); return; }
    _signals = std::move(changed); _remotePath = path;
    _remoteName = path.substring(path.lastIndexOf('/') + 1); _remoteName.remove(_remoteName.length() - 3);
    _learningNew = false;
    _returnTo(Scene::Remote); _message("Saved", _learned.name.c_str(), 1500);
}
void App09::_loadCategory(int category) {
    _category = category;
    const auto& info = meow_ir::Categories[category];
    String path;
    for (const char* folder : {"/infrared/assets/", "/infrared/universal/"}) {
        String candidate = String(folder) + info.file + ".ir";
        if (SD_MMC.exists(candidate)) { path = candidate; break; }
        if (category == 1) {
            candidate = String(folder) + "projector.ir";
            if (SD_MMC.exists(candidate)) { path = candidate; break; }
        }
    }
    if (path.isEmpty() || !_beginLibraryLoad(path, Scene::Panel)) {
        _message("Database Error", path.isEmpty() ? String("Missing ") + info.file + ".ir" : _lastError); return;
    }
}
void App09::_moveNameKey(int horizontal, int vertical) {
    const int count = characterCount(_symbols);
    int row = _key / 10;
    int col = row == 4 ? ((_key - CharacterCells) * 10 + 5) / 4 : _key % 10;
    if (horizontal) {
        if (row == 4) { _key = CharacterCells + (_key - CharacterCells + horizontal + 4) % 4; return; }
        const int columns = std::min(10, count - row * 10);
        col = (col + horizontal + columns) % columns;
    }
    if (vertical) row = (row + vertical + 5) % 5;
    if (row == 4) _key = CharacterCells + std::min(3, col * 4 / 10);
    else _key = row * 10 + std::min(col, std::min(10, count - row * 10) - 1);
}
void App09::_startScan(const String& name) {
    _queue.clear();
    for (size_t i = 0; i < _records.size(); ++i)
        if (_actions[_records[i].action].equalsIgnoreCase(name)) _queue.push_back(i);
    if (_queue.empty()) { _message("No Signals", String("No records for ") + name); return; }
    _scanName = name; _scan.start(_queue.size()); _scanPending = false; _lastScan = millis() - 150;
    _push(Scene::Scan);
}
void App09::_runScan() {
    if (_input.b) {
        _scan.running = false; _scanPending = false; _pop();
        return;
    }
    if (_input.a) {
        if (!_scan.paused) { _stopTx(); _scanPending = false; _scan.pause(); _dirty = true; }
        else if (!_txBusy) {
            meow_ir::Signal signal;
            if (!_readRecord(_records[_queue[_scan.index]], signal)) { _message("Read Failed", _lastError); return; }
            _startTx(signal);
        }
    }
    if (_scan.paused) {
        if (_input.left) { _scan.move(-1); _dirty = true; }
        if (_input.right) { _scan.move(1); _dirty = true; }
        if (_input.up && !_txBusy) { _scan.resume(); _lastScan = millis(); _dirty = true; }
        return;
    }
    if (_scanPending && !_txBusy) {
        _scanPending = false;
        if (!_txSucceeded.load()) { _scan.running = false; _message("Send Failed", "Unsupported legacy signal"); return; }
        _scan.sent(); _lastScan = millis(); _dirty = true;
    }
    if (!_scan.running) { _pop(); return; }
    if (!_scanPending && !_txBusy && millis() - _lastScan >= 100) {
        meow_ir::Signal signal;
        if (!_readRecord(_records[_queue[_scan.index]], signal)) {
            _scan.running = false; _message("Read Failed", _lastError); return;
        }
        if (!_startTx(signal)) { _scan.running = false; _message("TX Unavailable", "Cannot start infrared worker"); return; }
        _scanPending = true;
    }
}
bool App09::_navigate() {
    const int delta = (_input.down ? 1 : 0) - (_input.up ? 1 : 0);
    if (!delta) return false;
    const int previous = _selected;
    meow_ir::listMove(delta, _items.size(), _listVisible(), _selected, _top);
    if (_scene == Scene::Move && _moveIndex >= 0 && _selected != previous) {
        std::swap(_signals[previous], _signals[_selected]); _moveIndex = _selected; _enter();
    }
    _dirty = true;
    return true;
}
void App09::_back() {
    if (_scene == Scene::Main) { close(); return; }
    if (_scene == Scene::Result) { _discardRetry = false; _push(Scene::Discard); return; }
    if (_scene == Scene::Learn) { _returnTo(_learnOrigin); return; }
    if (_scene == Scene::Move) { _signals = std::move(_moveBackup); _moveIndex = -1; _pop(); return; }
    _pop();
}
void App09::_run() {
    if (_scene == Scene::Loading) { _runLibraryLoad(); return; }
    if (_scene == Scene::Scan) { _runScan(); return; }
    if (_scene == Scene::Message && _messageUntil && int32_t(millis() - _messageUntil) >= 0) { _pop(); return; }
    if (_scene == Scene::Result) {
        _runHeld(_learned);
        if (_scene != Scene::Result) return;
        if (_input.left) { _discardRetry = true; _push(Scene::Discard); return; }
        if (_input.right) {
            char initial[24];
            if (_easy) snprintf(initial, sizeof(initial), "%s", EasyNames[_easyIndex]);
            else if (_learned.raw) snprintf(initial, sizeof(initial), "RAW_%u", unsigned(_learned.timings.size()));
            else snprintf(initial, sizeof(initial), "%.4s_%lX", protocolName(_learned), static_cast<unsigned long>(_learned.command));
            _beginName(NameFor::Learn, initial); return;
        }
    }
    if (_input.b) { _back(); return; }
    if (_scene == Scene::Learn) {
        if (!_rxEnabled) _startRx();
        if (_input.right) {
            _decodeMode = (_decodeMode + 1) % 3; _writeSettings(); _dirty = true;
        }
        if (_input.left) {
            _easy = !_easy; _writeSettings();
            _dirty = true;
        }
        if (_easy && _input.a) {
            _easyIndex = std::min(_easyIndex + 1, EasyCount - 1); _nextEasy(); _dirty = true;
        }
        _receive();
        if (_scene == Scene::Learn && millis() - _lastDraw > 60 && !_dirty) {
            hp::drawScanWave(_device->Lcd, 72, millis() / 8); _lastDraw = millis();
        }
        return;
    }
    if (_scene == Scene::Name) {
        const int previous = _key;
        _moveNameKey(int(_input.right) - int(_input.left), int(_input.down) - int(_input.up));
        if (previous != _key) _dirty = true;
    } else if (_scene == Scene::Panel) {
        const int count = meow_ir::Categories[_category].count, rows = (count + 1) / 2;
        int row = _selected / 2, col = _selected % 2;
        if (_input.up) row = (row + rows - 1) % rows;
        if (_input.down) row = (row + 1) % rows;
        if (_input.left || _input.right) col ^= 1;
        const int next = std::min(count - 1, row * 2 + col);
        if (next != _selected) { _selected = next; _dirty = true; }
    } else _navigate();
    if (_scene == Scene::Remote && _selected < int(_signals.size())) {
        _runHeld(_signals[_selected]);
        if (_scene != Scene::Remote) return;
    }
    if (_scene == Scene::Files && _input.right && !_libraryBrowser && !_entries.empty()) {
        const auto& entry = _entries[_selected];
        if (entry == "..") return;
        _deleteFolder = entry.endsWith("/"); _deleteRemote = true;
        _deletePath = _directory + "/" + entry;
        if (_deleteFolder) _deletePath.remove(_deletePath.length() - 1);
        _push(Scene::Delete); return;
    }
    if (_input.a) _activate();
}
void App09::_activate() {
    switch (_scene) {
        case Scene::Main:
            _mainSelected = _selected;
            switch (_selected) {
                case 0: _push(Scene::Universal); break;
                case 1: _startLearn(true); break;
                case 2: _openFiles(false); break;
            }
            break;
        case Scene::Files: {
            if (_entries.empty()) break;
            String entry = _entries[_selected];
            if (entry == "..") {
                _directory = _directory.substring(0, _directory.lastIndexOf('/'));
                _selected = _top = 0; _refreshFiles(); _dirty = true;
            } else if (entry.endsWith("/")) {
                entry.remove(entry.length() - 1);
                if (_directory.length() + entry.length() > 210) { _message("Path Too Long", "Choose a shorter folder path"); break; }
                _directory += "/" + entry; _selected = _top = 0; _refreshFiles(); _dirty = true;
            } else if (_libraryBrowser) {
                if (!_beginLibraryLoad(_directory + "/" + entry, Scene::Actions)) _message("Library Error", _lastError);
            } else {
                if (_loadRemote(_directory + "/" + entry)) _push(Scene::Remote);
                else _message("Remote Error", _lastError);
            }
            break;
        }
        case Scene::Remote:
            if (_selected == int(_signals.size())) _startLearn(false);
            else if (_selected == int(_signals.size()) + 1) _push(Scene::Edit);
            break;
        case Scene::Edit:
            if (_selected == 0) _startLearn(false);
            else if (_selected == 4) _beginName(NameFor::Remote, _remoteName.c_str());
            else if (_selected == 5) {
                _deleteRemote = true; _deleteFolder = false; _deletePath = _remotePath; _push(Scene::Delete);
            } else if (_signals.empty()) _message("No Buttons", "Learn a button first");
            else if (_selected == 2) { _moveBackup = _signals; _moveIndex = -1; _push(Scene::Move); }
            else { _editFor = _selected == 1 ? EditFor::Rename : EditFor::Delete; _push(Scene::ButtonSelect); }
            break;
        case Scene::ButtonSelect:
            if (_signals.empty()) break;
            _editIndex = _selected;
            if (_editFor == EditFor::Rename) _beginName(NameFor::Button, _signals[_editIndex].name.c_str());
            else { _deleteRemote = _deleteFolder = false; _push(Scene::Delete); }
            break;
        case Scene::Move:
            if (_moveIndex < 0) { _moveIndex = _selected; _dirty = true; }
            else if (_selected != _moveIndex) {
                auto signal = std::move(_signals[_moveIndex]); _signals.erase(_signals.begin() + _moveIndex);
                _signals.insert(_signals.begin() + _selected, std::move(signal)); _moveIndex = _selected;
                _enter(); _dirty = true;
            } else if (_writeRemote(_remotePath, _signals, false)) {
                _moveBackup.clear(); _moveIndex = -1; _returnTo(Scene::Remote);
            } else _message("Save Failed", _lastError);
            break;
        case Scene::Name: {
            const size_t length = strlen(_name);
            if (_key == KeySave) _commitName();
            else if (_key == KeyDelete) { if (length) _name[length - 1] = 0; _dirty = true; }
            else if (_key == KeyCase) { _lowercase = !_lowercase; _dirty = true; }
            else if (_key == KeySymbols) { _symbols = !_symbols; _dirty = true; }
            else if (length < sizeof(_name) - 1) {
                const char c = keyCharacter(_key, _symbols, _lowercase);
                if (c) { _name[length] = c; _name[length + 1] = 0; _dirty = true; }
            }
            break;
        }
        case Scene::Discard:
            if (_discardRetry) _returnTo(Scene::Learn); else _returnTo(_learnOrigin);
            break;
        case Scene::Delete: {
            bool ok;
            if (_deleteRemote) {
                ok = _deleteFolder ? (_emptyDirectory(_deletePath) && SD_MMC.rmdir(_deletePath)) : SD_MMC.remove(_deletePath);
                if (ok) {
                    if (_deletePath == _remotePath) { _signals.clear(); _remotePath = _remoteName = ""; }
                    _libraryBrowser = false;
                    _directory = _deletePath.substring(0, _deletePath.lastIndexOf('/'));
                    _returnTo(Scene::Files);
                }
            } else {
                auto changed = _signals; changed.erase(changed.begin() + _editIndex);
                ok = _writeRemote(_remotePath, changed, false);
                if (ok) { _signals = std::move(changed); _returnTo(Scene::Remote); }
            }
            if (!ok) _message("Delete Failed", _deleteFolder ? "Folder must be empty" : "SD write/delete failed");
            else _message("Deleted", "Saved on SD", 1000);
            break;
        }
        case Scene::Universal:
            _universalSelected = _selected;
            if (_selected == 9) _openFiles(true); else _loadCategory(_selected);
            break;
        case Scene::Panel: _startScan(meow_ir::Categories[_category].actions[_selected]); break;
        case Scene::Actions: if (!_actions.empty()) _startScan(_actions[_selected]); break;
        case Scene::Message: _pop(); break;
        default: break;
    }
}
} // namespace MOONCAKE::APPS
