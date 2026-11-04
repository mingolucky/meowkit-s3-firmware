#include "nfc.h"
#include "nfc_ui.h"
#include <SD_MMC.h>
#include <algorithm>
#include <cstring>

namespace MOONCAKE::APPS {
using namespace meow_nfc;
static String asString(const std::string& s) { return String(s.c_str()); }
void App10::wrap(const String& text) {
    _lines.clear(); String line;
    // Keep UTF-8 code points together when wrapping in the Chinese font.
    int columns = 0;
    for (unsigned p = 0; p < text.length();) {
        const uint8_t c = text[p];
        if (c == '\n') { _lines.push_back(line); line = ""; columns = 0; ++p; continue; }
        unsigned bytes = c < 0x80 ? 1 : (c & 0xe0) == 0xc0 ? 2 : (c & 0xf0) == 0xe0 ? 3 : 4;
        if (p + bytes > text.length()) bytes = 1;
        const int width = bytes > 1 ? 2 : 1;
        if (columns + width > 36) { _lines.push_back(line); line = ""; columns = 0; }
        line += text.substring(p, p + bytes); p += bytes; columns += width;
    }
    if (line.length() || _lines.empty()) _lines.push_back(line);
}
void App10::enter() {
    _items.clear();
    switch (_scene) {
        case Scene::Main:
            _items = {{"Read", "Auto detect NFC-A / B / F / V"},
                {"Saved", "Browse /nfc on the SD card"},
                {"Extra Actions", "Select type / RF field / capabilities"},
                {"Add Manually", "ISO14443-3A UID, ATQA and SAK"},
                {"Debug", "Chip identity, voltage and field"}}; break;
        case Scene::Extra:
            _items = {{"Read Specific Type", "A / B / F / V"}, {"Generate RF Field", "13.56 MHz; B to stop"},
                {"Capabilities", "Features and board limitations"}}; break;
        case Scene::Filter:
            _items = {{"Auto", "Poll all four technologies"}, {"NFC-A", "ISO14443A"},
                {"NFC-B", "ISO14443B"}, {"NFC-F", "FeliCa"}, {"NFC-V", "ISO15693"}}; break;
        case Scene::Files: refreshFiles(); break;
        case Scene::Card:
            _items = {{"Information", tagName(*_tag)},
                {"NDEF", _tag->ndefState == NdefState::Read ? String(_tag->ndefSize) + " bytes captured" : ndefStateName(_tag->ndefState)},
                {"Memory", String(_tag->memorySize) + " / " + _tag->memoryExpected + " bytes"},
                {_path.length() ? "Rename" : "Save", _path.length() ? _path : "/nfc/*.nfc"}};
            if (_path.length()) _items.push_back({"Delete", "Confirm before removing this file"});
            if (_tag->manual) {
                _items.push_back({"Edit UID", asString(hex(_tag->uid, _tag->uidSize))});
                _items.push_back({"Edit ATQA", "On-air / LSB byte first"});
                _items.push_back({"Edit SAK", asString(hex(&_tag->sak, 1))});
            }
            break;
        case Scene::Info: {
            String info = String(tagName(*_tag)) + "\n" + techName(_tag->tech) + "\nUID: " + asString(hex(_tag->uid, _tag->uidSize));
            if (_tag->tech == Tech::A) info += "\nATQA (LSB): " + asString(hex(_tag->atqa, 2)) + "\nSAK: " + asString(hex(&_tag->sak, 1));
            if (_tag->auxSize) info += "\nResponse: " + asString(hex(_tag->aux, _tag->auxSize));
            info += String("\nPublic memory: ") + _tag->memorySize + "/" + _tag->memoryExpected + " bytes";
            info += String("\n") + memoryStateName(_tag->memoryState);
            if (_tag->memoryExpected > _tag->memorySize) info += "\nPartial read: protected, removed or limit reached.";
            if (!_tag->memorySize) info += "\nIdentity captured; no memory dump.";
            if (_tag->imported) info += "\nImported identity/public pages. Other protocol fields stay in the original file.";
            if (_tag->manual) info += "\nManually entered identity.";
            info += "\nEmulation and MFC key extraction are unavailable in this port.";
            wrap(info); break;
        }
        case Scene::Ndef: {
            std::string text;
            if (_tag->ndefState == NdefState::Read) {
                if (!describeNdef(_tag->ndef, _tag->ndefSize, text)) text = "Unsupported or malformed NDEF. Raw bytes remain in the capture file.";
            } else if (_tag->ndefState == NdefState::TooLarge) text = "NDEF is larger than the 2048-byte capture limit. It was not read.";
            else if (_tag->ndefState == NdefState::Failed) text = "NDEF reading failed. Keep the card still and retry.";
            else if (_tag->ndefState == NdefState::Absent) text = "No readable NFC Forum NDEF application was found.";
            else text = "NDEF was not captured in this file.";
            wrap(asString(text)); break;
        }
        case Scene::Notice: wrap(_noticeText); break;
        case Scene::Delete: _items = {{"Cancel", "Keep this file"}, {"Delete File", _path}}; break;
        case Scene::Discard: _items = {{"Keep Card", "Return to the captured card"}, {"Discard Card", "Return without saving"}}; break;
        case Scene::Scan:
            if (_readerReady) { *_status = {}; _status->state = ReaderState::Initializing; _reader.request(ReaderMode::Scan, _filter); }
            break;
        case Scene::Debug: case Scene::Field:
            if (_readerReady) { *_status = {}; _status->state = ReaderState::Initializing;
                _reader.request(_scene == Scene::Debug ? ReaderMode::Debug : ReaderMode::Field); }
            break;
        default: break;
    }
}
void App10::startScan() {
    if (!_readerReady) { notice("NFC Unavailable", "Could not allocate the NFC worker. Close other applications and retry."); return; }
    push(Scene::Scan);
}
void App10::back() {
    if (_scene == Scene::Main) { close(); return; }
    if (_scene == Scene::Card && _modified) { push(Scene::Discard); return; }
    if (_scene == Scene::Files && _directory != "/nfc") {
        const int slash = _directory.lastIndexOf('/'); _directory = slash <= 4 ? "/nfc" : _directory.substring(0, slash);
        _selected = _top = 0; refreshFiles(); _dirty = true; return;
    }
    pop();
}
void App10::accept() {
    switch (_scene) {
        case Scene::Main:
            if (_selected == 0) { _filter = 0; startScan(); }
            else if (_selected == 1) push(Scene::Files);
            else if (_selected == 2) push(Scene::Extra);
            else if (_selected == 3) {
                if (!_tag) { notice("NFC Unavailable", "Not enough memory."); return; }
                *_tag = {}; _tag->tech = Tech::A; _tag->manual = true; _path = ""; _modified = true;
                beginEdit(Edit::Uid);
            } else if (_selected == 4) {
                if (_readerReady) push(Scene::Debug);
                else notice("NFC Unavailable", "Could not allocate the NFC worker.");
            }
            break;
        case Scene::Extra:
            if (_selected == 0) push(Scene::Filter);
            else if (_selected == 1) { if (_readerReady) push(Scene::Field); else notice("NFC Unavailable", "Could not allocate the NFC worker."); }
            else notice("Capabilities", "Reads NFC-A/B/F/V identities and public NDEF (T2/T3/T4/T5). Reads public Type 2 pages and ISO15693 blocks.\nGPIO1 SDA / GPIO2 SCL / 3V3 / GND. No IRQ wire.\nEmulation, reader attacks, MFC key extraction and authenticated memory access are unavailable.\nA exports Flipper ISO14443-3A with capture extensions. B/F/V use MeowKit capture format. No card writes are performed.");
            break;
        case Scene::Filter: _filter = _selected; startScan(); break;
        case Scene::Files:
            if (_entries.empty() || _selected >= int(_entries.size())) break;
            if (_entries[_selected].directory) { _directory = _entries[_selected].path; _selected = _top = 0; refreshFiles(); _dirty = true; }
            else if (load(_entries[_selected].path)) push(Scene::Card);
            break;
        case Scene::Card:
            if (_selected == 0) push(Scene::Info);
            else if (_selected == 1) push(Scene::Ndef);
            else if (_selected == 2) push(Scene::Memory);
            else if (_selected == 3) beginEdit(_path.length() ? Edit::Rename : Edit::Save);
            else if (_path.length() && _selected == 4) push(Scene::Delete);
            else if (_tag->manual) {
                const int base = _path.length() ? 5 : 4;
                beginEdit(_selected == base ? Edit::Uid : _selected == base + 1 ? Edit::Atqa : Edit::Sak);
            }
            break;
        case Scene::Name: case Scene::Hex: keyPress(); break;
        case Scene::Scan: case Scene::Debug: case Scene::Field:
            if (_readerReady && _status->state == ReaderState::Error) {
                const auto retryScene = _scene; scene(retryScene);
            } break;
        case Scene::Delete:
            if (_selected == 0) pop();
            else {
                if (!SD_MMC.remove(_path)) { notice("Delete Failed", "Check the SD card."); break; }
                _path = ""; _modified = false; _history.clear(); scene(Scene::Main);
                push(Scene::Files); notice("Deleted", "The selected NFC file was removed.");
            } break;
        case Scene::Discard:
            if (_selected == 0) pop();
            else { _modified = false; pop(); pop(); }
            break;
        case Scene::Notice: pop(); break;
        default: break;
    }
}
void App10::beginEdit(Edit value) {
    _edit = value; _key = 0; _lower = false; _text[0] = 0;
    if (value == Edit::Rename) {
        String name = _path.substring(_path.lastIndexOf('/') + 1); if (name.endsWith(".nfc")) name.remove(name.length() - 4);
        name.toCharArray(_text, 24);
    } else if (value == Edit::Save) {
        const auto name = hex(_tag->uid, std::min<size_t>(4, _tag->uidSize), false);
        snprintf(_text, sizeof(_text), "NFC_%s", name.c_str());
    } else if (value == Edit::Uid && validUid(Tech::A, _tag->uidSize)) {
        snprintf(_text, sizeof(_text), "%s", hex(_tag->uid, _tag->uidSize, false).c_str());
    } else if (value == Edit::Atqa) snprintf(_text, sizeof(_text), "%s", hex(_tag->atqa, 2, false).c_str());
    else if (value == Edit::Sak) snprintf(_text, sizeof(_text), "%s", hex(&_tag->sak, 1, false).c_str());
    push(value == Edit::Save || value == Edit::Rename ? Scene::Name : Scene::Hex);
}
void App10::keyPress() {
    using namespace meow_nfc::ui;
    const char* key = _scene == Scene::Hex ? HexKeys[_key] : Keys[_key];
    const size_t length = strlen(_text);
    if (!strcmp(key, "OK")) { commitEdit(); return; }
    if (!strcmp(key, "DEL")) { if (length) _text[length - 1] = 0; }
    else if (!strcmp(key, "aA")) _lower = !_lower;
    else {
        const size_t max = _scene == Scene::Name ? 23 : _edit == Edit::Uid ? 20 : _edit == Edit::Atqa ? 4 : 2;
        if (length < max) { char c = !strcmp(key, "SP") ? ' ' : key[0];
            if (_lower && c >= 'A' && c <= 'Z') c += 'a' - 'A'; _text[length] = c; _text[length + 1] = 0; }
    }
    _dirty = true;
}
void App10::commitEdit() {
    if (_edit == Edit::Save || _edit == Edit::Rename) {
        if (!validName(_text)) { notice("Invalid Name", "Use 1-23 letters, digits, spaces, _ or -. No leading/trailing spaces."); return; }
        const String path = _edit == Edit::Save ? String("/nfc/") + _text + ".nfc" :
            _path.substring(0, _path.lastIndexOf('/') + 1) + _text + ".nfc";
        if (path == _path) { pop(); return; }
        if (SD_MMC.exists(path)) { notice("Name Exists", "Choose another name. Existing files are preserved."); return; }
        if (_edit == Edit::Rename) {
            if (!SD_MMC.rename(_path, path)) { notice("Rename Failed", "Check the SD card."); return; }
            _path = path;
        } else if (!save(path)) return;
        _modified = false; pop(); notice("Saved", path); return;
    }
    uint8_t value[10]; size_t n = 0;
    if (!parseHex(_text, value, sizeof(value), n) ||
        (_edit == Edit::Uid ? !validUid(Tech::A, n) : n != (_edit == Edit::Atqa ? 2u : 1u))) {
        notice("Invalid Hex", _edit == Edit::Uid ? "UID must have 8, 14 or 20 hex digits." : _edit == Edit::Atqa ? "ATQA needs 4 hex digits, LSB first." : "SAK needs 2 hex digits."); return;
    }
    if (_edit == Edit::Uid) {
        const bool newCard = !_tag->uidSize;
        memcpy(_tag->uid, value, n); _tag->uidSize = n;
        if (newCard) { _tag->atqa[0] = n == 4 ? 0x04 : n == 7 ? 0x44 : 0x84; _tag->atqa[1] = 0; }
    } else if (_edit == Edit::Atqa) memcpy(_tag->atqa, value, 2);
    else _tag->sak = value[0];
    // Editing a saved manual identity creates a new unsaved card, preserving its file.
    _path = ""; _modified = true; pop();
    if (_scene == Scene::Main) push(Scene::Card);
    else { enter(); _dirty = true; }
}
} // namespace MOONCAKE::APPS
