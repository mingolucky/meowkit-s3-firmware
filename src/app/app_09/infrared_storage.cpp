#include "infrared.h"
#include <cstring>

namespace {
bool splitLine(String line, String& key, String& value) {
    line.trim();
    if (line.isEmpty() || line.startsWith("#")) return false;
    int colon = line.indexOf(':');
    if (colon < 1) return false;
    key = line.substring(0, colon); key.trim();
    value = line.substring(colon + 1); value.trim();
    return true;
}

bool header(File& file, bool library) {
    String type, version, key, value;
    if (file.size() < 4) return false;
    while (file.available()) {
        const uint32_t position = file.position();
        String line = file.readStringUntil('\n');
        if (position == 0 && line.startsWith("\xEF\xBB\xBF")) line.remove(0, 3);
        if (!splitLine(line, key, value)) continue;
        if (key == "name") { file.seek(position); break; }
        if (key == "Filetype") type = value;
        else if (key == "Version") version = value;
    }
    return type == (library ? "IR library file" : "IR signals file") && version == "1";
}

bool readRecord(File& file, meow_ir::Signal& signal, String& error) {
    meow_ir::RecordParser parser;
    bool begun = false;
    while (file.available()) {
        const uint32_t position = file.position();
        String key, value;
        if (!splitLine(file.readStringUntil('\n'), key, value)) continue;
        if (key == "name") {
            if (begun) { file.seek(position); break; }
            begun = true;
        }
        if (begun) parser.field(key.c_str(), value.c_str());
        if (!parser.ok) break;
        yield();
    }
    if (!begun || !parser.valid()) {
        error = begun ? String("Invalid/unsupported: ") + parser.signal.name.c_str() : "No signal";
        return false;
    }
    signal = std::move(parser.signal);
    return true;
}

String basename(const String& path) {
    String name = path.substring(path.lastIndexOf('/') + 1);
    if (name.endsWith(".ir") || name.endsWith(".IR")) name.remove(name.length() - 3);
    return name;
}
} // namespace

namespace MOONCAKE::APPS {
bool App09::_loadRemote(const String& path) {
    File file = SD_MMC.open(path, FILE_READ);
    if (!file || !header(file, false)) {
        _lastError = "Expected IR signals file / Version 1";
        return false;
    }
    std::vector<meow_ir::Signal> loaded;
    while (file.available()) {
        // Trailing comments and empty lines do not create an extra record.
        uint32_t next = file.position();
        String key, value;
        bool found = false;
        while (file.available()) {
            next = file.position();
            if (splitLine(file.readStringUntil('\n'), key, value) && key == "name") {
                found = true; break;
            }
        }
        if (!found) break;
        if (loaded.size() >= meow_ir::MaxButtons) { _lastError = "Too many buttons (max 256)"; return false; }
        file.seek(next);
        meow_ir::Signal signal;
        if (!readRecord(file, signal, _lastError)) return false;
        loaded.push_back(std::move(signal));
    }
    _signals = std::move(loaded);
    _remotePath = path;
    _remoteName = basename(path);
    return true;
}

bool App09::_writeRemote(const String& path, const std::vector<meow_ir::Signal>& signals, bool create) {
    if (path.length() > 220 || signals.size() > meow_ir::MaxButtons) { _lastError = "Remote too large"; return false; }
    for (const auto& signal : signals) {
        if (!meow_ir::validSignal(signal)) { _lastError = "Invalid signal; original kept"; return false; }
    }
    const String temporary = path + ".mk-tmp", backup = path + ".mk-bak";
    // These names are reserved by the transaction. Never overwrite stale backups.
    if (SD_MMC.exists(temporary) || SD_MMC.exists(backup)) {
        _lastError = "Recover .mk-tmp/.mk-bak on SD first";
        return false;
    }
    const bool existed = SD_MMC.exists(path);
    if (create && existed) { _lastError = "File already exists"; return false; }
    File file = SD_MMC.open(temporary, FILE_WRITE);
    if (!file) { _lastError = "Cannot create temporary file"; return false; }
    size_t expected = 0;
    bool ok = true;
    auto write = [&](const std::string& text) {
        expected += text.size();
        ok &= file.write(reinterpret_cast<const uint8_t*>(text.data()), text.size()) == text.size();
    };
    write("Filetype: IR signals file\nVersion: 1\n");
    for (const auto& signal : signals) { write(meow_ir::serialize(signal)); yield(); }
    file.flush();
    ok &= file.size() == expected;
    file.close();
    if (!ok) {
        SD_MMC.remove(temporary);
        _lastError = "Write failed; original kept";
        return false;
    }
    if (existed && !SD_MMC.rename(path, backup)) {
        SD_MMC.remove(temporary);
        _lastError = "Cannot back up original";
        return false;
    }
    if (!SD_MMC.rename(temporary, path)) {
        if (existed) SD_MMC.rename(backup, path);
        SD_MMC.remove(temporary);
        _lastError = "Commit failed; check backup";
        return false;
    }
    if (existed) SD_MMC.remove(backup);
    return true;
}

bool App09::_renameRemote(const char* name) {
    if (!infrared_file::validName(name) || strlen(name) > meow_ir::MaxName) {
        _lastError = "Invalid file name"; return false;
    }
    const String destination = _remotePath.substring(0, _remotePath.lastIndexOf('/') + 1) + name + ".ir";
    if (destination == _remotePath) return true;
    if (SD_MMC.exists(destination)) { _lastError = "Name already exists"; return false; }
    if (!SD_MMC.rename(_remotePath, destination)) { _lastError = "Rename failed"; return false; }
    _remotePath = destination;
    _remoteName = name;
    return true;
}

void App09::_refreshFiles() {
    _entries.clear();
    _items.clear();
    if (_directory != "/infrared") _entries.push_back("..");
    File root = SD_MMC.open(_directory);
    if (!root || !root.isDirectory()) { _lastError = "Cannot open SD folder"; return; }
    std::vector<String> directories, files;
    for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
        String name = entry.name();
        name = name.substring(name.lastIndexOf('/') + 1);
        if (name.length() && name[0] != '.') {
            if (entry.isDirectory()) {
                // Universal dictionaries belong to Universal Remotes. Keep
                // legacy personal remotes at /infrared visible alongside remote/.
                if (!_libraryBrowser && _directory == "/infrared" &&
                    (name.equalsIgnoreCase("universal") || name.equalsIgnoreCase("assets"))) continue;
                directories.push_back(name + "/");
            }
            else {
                String lower = name; lower.toLowerCase();
                if (lower.endsWith(".ir")) files.push_back(name);
            }
        }
        entry.close();
    }
    std::sort(directories.begin(), directories.end());
    std::sort(files.begin(), files.end());
    _entries.insert(_entries.end(), directories.begin(), directories.end());
    _entries.insert(_entries.end(), files.begin(), files.end());
    for (const auto& entry : _entries) {
        _items.push_back({entry, ""});
    }
    meow_ir::listMove(0, _items.size(), _listVisible(), _selected, _top);
}

bool App09::_emptyDirectory(const String& path) {
    File directory = SD_MMC.open(path);
    if (!directory || !directory.isDirectory()) return false;
    File entry = directory.openNextFile();
    return !entry;
}

bool App09::_beginLibraryLoad(const String& path, Scene destination) {
    _loadingFile = SD_MMC.open(path);
    if (!_loadingFile || _loadingFile.isDirectory()) { _loadingFile.close(); _lastError = "Cannot open IR library"; return false; }
    _records.clear(); _actions.clear();
    _loadType = _loadVersion = ""; _loadHeaderChecked = false;
    _libraryPath = path; _libraryTitle = basename(path);
    _loadDestination = destination;
    _push(Scene::Loading);
    return true;
}

void App09::_failLibraryLoad(const String& error) {
    _lastError = error;
    _records.clear(); _actions.clear();
    _pop(); // Replace the loading page, so Back cannot return to a dead loader.
    _message("Library Error", _lastError);
}

void App09::_runLibraryLoad() {
    if (_input.b) { _records.clear(); _actions.clear(); _pop(); return; }
    if (millis() - _lastDraw >= 64) _dirty = true;
    const uint32_t started = millis();
    // Cooperatively index a bounded slice; the UI can animate and cancel
    // between slices. No delay or artificial minimum loading duration.
    for (unsigned lines = 0; lines < 256 && millis() - started < 6; ++lines) {
        if (!_loadingFile.available()) {
            if (_loadType != "IR library file" || _loadVersion != "1") {
                _failLibraryLoad("Expected IR library file / Version 1"); return;
            }
            if (_records.empty()) { _failLibraryLoad("Library has no signals"); return; }
            _setScene(_loadDestination); return;
        }
        uint32_t position = _loadingFile.position();
        String line = _loadingFile.readStringUntil('\n');
        if (!position && line.startsWith("\xEF\xBB\xBF")) line.remove(0, 3);
        String key, name;
        if (!splitLine(line, key, name)) continue;
        if (!_loadHeaderChecked) {
            if (key == "Filetype") _loadType = name;
            else if (key == "Version") _loadVersion = name;
            if (key != "name") continue;
            if (_loadType != "IR library file" || _loadVersion != "1") {
                _failLibraryLoad("Expected IR library file / Version 1"); return;
            }
            _loadHeaderChecked = true;
        }
        if (key != "name") continue;
        if (name.isEmpty() || name.length() > meow_ir::MaxName || _records.size() >= 20000) {
            _failLibraryLoad("Invalid or oversized library"); return;
        }
        size_t action = 0;
        while (action < _actions.size() && !_actions[action].equalsIgnoreCase(name)) ++action;
        if (action == _actions.size()) {
            if (_actions.size() == meow_ir::MaxButtons) { _failLibraryLoad("Too many library actions"); return; }
            _actions.push_back(name);
        }
        _records.push_back({uint16_t(action), position});
    }
    yield();
}

bool App09::_readRecord(const LibraryRecord& record, meow_ir::Signal& signal) {
    File file = SD_MMC.open(_libraryPath);
    if (!file || !file.seek(record.offset)) { _lastError = "Library removed or unreadable"; return false; }
    if (!readRecord(file, signal, _lastError)) return false;
    if (record.action >= _actions.size() || !_actions[record.action].equalsIgnoreCase(signal.name.c_str())) {
        _lastError = "Library changed; reopen it"; return false;
    }
    return true;
}

void App09::_readSettings() {
    _easy = false; _decodeMode = 0;
    File file = SD_MMC.open("/infrared/.meowkit-settings");
    while (file && file.available()) {
        String key, value;
        if (!splitLine(file.readStringUntil('\n'), key, value)) continue;
        if (key == "Easy") _easy = value == "1";
        if (key == "Decode" && value.length() == 1 && value[0] >= '0' && value[0] <= '2')
            _decodeMode = value[0] - '0';
    }
}

void App09::_writeSettings() {
    File file = SD_MMC.open("/infrared/.meowkit-settings", FILE_WRITE);
    if (file) file.printf("Easy: %u\nDecode: %u\n", unsigned(_easy), unsigned(_decodeMode));
}
} // namespace MOONCAKE::APPS
