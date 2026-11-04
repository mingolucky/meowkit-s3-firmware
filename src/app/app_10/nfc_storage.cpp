#include "nfc.h"
#include <SD_MMC.h>
#include <algorithm>

namespace MOONCAKE::APPS {
using namespace meow_nfc;
void App10::refreshFiles() {
    _entries.clear(); _items.clear();
    if (SD_MMC.cardType() == CARD_NONE) { _dirty = true; return; }
    File directory = SD_MMC.open(_directory);
    if (!directory || !directory.isDirectory()) { if (directory) directory.close(); return; }
    for (File file = directory.openNextFile(); file; file = directory.openNextFile()) {
        String name = file.name(); name = name.substring(name.lastIndexOf('/') + 1);
        const bool folder = file.isDirectory(); file.close();
        if (!name.length() || name == "." || name == ".." || name.indexOf('/') >= 0) continue;
        String lower = name; lower.toLowerCase();
        if (!folder && !lower.endsWith(".nfc")) continue;
        if (_entries.size() < 256) _entries.push_back({_directory + "/" + name, name, folder});
    }
    directory.close();
    std::sort(_entries.begin(), _entries.end(), [](const Entry& a, const Entry& b) {
        return a.directory != b.directory ? a.directory : a.name.compareTo(b.name) < 0;
    });
    if (_directory != "/nfc") {
        int slash = _directory.lastIndexOf('/');
        _entries.insert(_entries.begin(), {slash <= 4 ? String("/nfc") : _directory.substring(0, slash), "..", true});
    }
    for (const auto& entry : _entries) _items.push_back({entry.name, entry.directory ? "Folder" : "NFC capture"});
    move(0);
}
bool App10::load(const String& path) {
    if (!_tag) { notice("Open Failed", "Not enough memory."); return false; }
    File file = SD_MMC.open(path, FILE_READ);
    if (!file || file.isDirectory()) { if (file) file.close(); notice("Open Failed", path); return false; }
    const size_t length = file.size();
    if (!length || length > MaxFile) { file.close(); notice("Open Failed", "Empty file or larger than 128 KiB."); return false; }
    std::string text(length, '\0');
    const size_t read = file.read(reinterpret_cast<uint8_t*>(&text[0]), length); file.close();
    std::string error;
    if (read != length || !parseCapture(text, *_tag, error)) {
        notice("Invalid NFC File", read == length ? error.c_str() : "SD read failed."); return false;
    }
    _path = path; _modified = false; return true;
}
bool App10::save(const String& path) {
    if (SD_MMC.cardType() == CARD_NONE || (!SD_MMC.exists("/nfc") && !SD_MMC.mkdir("/nfc"))) {
        notice("Save Failed", "Insert a writable SD card."); return false;
    }
    const String temp = path + ".tmp";
    if (SD_MMC.exists(path) || SD_MMC.exists(temp)) { notice("Save Failed", "File or recovery temp already exists. Choose another name."); return false; }
    const auto text = serializeCapture(*_tag);
    if (text.empty()) { notice("Save Failed", "Invalid capture."); return false; }
    File file = SD_MMC.open(temp, FILE_WRITE);
    if (!file) { notice("Save Failed", "Cannot create capture file."); return false; }
    const auto written = file.write(reinterpret_cast<const uint8_t*>(text.data()), text.size());
    file.flush(); const size_t size = file.size(); file.close();
    if (written != text.size() || size != text.size() || SD_MMC.exists(path) || !SD_MMC.rename(temp, path)) {
        SD_MMC.remove(temp); notice("Save Failed", "SD write or rename failed."); return false;
    }
    _path = path; return true;
}
} // namespace MOONCAKE::APPS
