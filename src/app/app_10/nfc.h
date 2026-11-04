#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include "../app_common/hp_ui.h"
#include "nfc_reader.h"
#include <vector>

namespace MOONCAKE::APPS {
class App10 : public mooncake::AppAbility {
public:
    explicit App10(DEVICES* device);
    void onOpen() override;
    void onRunning() override;
    void onClose() override;
private:
    enum class Scene : uint8_t { Main, Files, Extra, Filter, Scan, Card, Info,
        Memory, Ndef, Name, Hex, Delete, Discard, Debug, Field, Notice };
    enum class Edit : uint8_t { Save, Rename, Uid, Atqa, Sak };
    struct Item { String title, subtitle; };
    struct Entry { String path, name; bool directory; };
    struct Target { int x, y, w, h, value; uint8_t action; };
    struct Frame { Scene scene; int selected, top; String directory; };
    DEVICES* _device;
    meow_nfc::Reader _reader;
    std::unique_ptr<meow_nfc::ReaderSnapshot> _status;
    std::unique_ptr<meow_nfc::Tag> _tag;
    bool _readerReady = false, _dirty = true, _armed = false, _touchDown = false;
    bool _modified = false, _lower = false;
    Scene _scene = Scene::Main, _touchScene = Scene::Main;
    Edit _edit = Edit::Save;
    int _selected = 0, _top = 0, _key = 0, _touchTarget = -1, _touchX = 0, _touchY = 0;
    int _page = 0;
    uint8_t _filter = 0;
    uint32_t _lastPoll = 0, _lastDraw = 0, _repeatAt[4]{};
    bool _repeatHeld[4]{};
    char _text[32]{};
    String _directory = "/nfc", _path, _noticeTitle, _noticeText;
    std::vector<Item> _items;
    std::vector<Entry> _entries;
    std::vector<Target> _targets;
    std::vector<Frame> _history;
    std::vector<String> _lines;
    void scene(Scene value);
    void push(Scene value);
    void pop();
    void enter();
    void back();
    void accept();
    void input();
    void move(int delta);
    void draw();
    void drawList(const char* title);
    void footer(const char* a = "Open", const char* b = "Back");
    void target(int x, int y, int w, int h, uint8_t action, int value = -1);
    void notice(const char* title, const String& message);
    void startScan();
    void beginEdit(Edit value);
    void commitEdit();
    void keyPress();
    void refreshFiles();
    bool load(const String& path);
    bool save(const String& path);
    void wrap(const String& text);
};
} // namespace MOONCAKE::APPS
