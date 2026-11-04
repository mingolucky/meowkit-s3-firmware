/**
 * App09: Momentum infrared application flow adapted to MeowKit hardware.
 * Upstream: Next-Flip/Momentum-Firmware, applications/main/infrared (GPL-3.0).
 */
#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include <IRrecv.h>
#include <IRsend.h>
#include <SD_MMC.h>
#include "../app_common/hp_ui.h"
#include "infrared_signal.h"
#include <atomic>

namespace MOONCAKE::APPS {
class App09 : public mooncake::AppAbility {
public:
    explicit App09(DEVICES* device);
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    enum class Scene : uint8_t {
        Main, Files, Remote, Learn, Result, Discard, Name, Edit, ButtonSelect,
        Move, Delete, Universal, Loading, Panel, Actions, Scan, Message
    };
    enum class NameFor : uint8_t { Learn, Button, Remote };
    enum class EditFor : uint8_t { Rename, Move, Delete };
    struct Input {
        bool up = false, down = false, left = false, right = false;
        bool a = false, b = false, held = false;
    } _input;
    meow_ir::DirectionRepeat _directionRepeat[4];
    struct Item { String title, subtitle; };
    struct Frame { Scene scene; int selected, top; String directory; };
    struct LibraryRecord { uint16_t action; uint32_t offset; };

    DEVICES* _device;
    Scene _scene = Scene::Main;
    bool _dirty = true, _armed = false, _drawnTxBusy = false;
    int _selected = 0, _top = 0, _mainSelected = 0, _universalSelected = 0;
    std::vector<Item> _items;
    std::vector<Frame> _history;
    String _directory = "/infrared";
    std::vector<String> _entries;
    bool _libraryBrowser = false;

    std::vector<meow_ir::Signal> _signals;
    String _remotePath, _remoteName;
    meow_ir::Signal _learned;
    bool _learningNew = true, _easy = false;
    uint8_t _decodeMode = 0; // Auto, Decode only, RAW
    size_t _easyIndex = 0;
    Scene _learnOrigin = Scene::Main;
    bool _discardRetry = false;
    NameFor _nameFor = NameFor::Learn;
    EditFor _editFor = EditFor::Rename;
    char _name[24] = {};
    bool _lowercase = false, _symbols = false;
    int _key = 0, _editIndex = 0, _moveIndex = -1;
    std::vector<meow_ir::Signal> _moveBackup;
    String _deletePath;
    bool _deleteRemote = false, _deleteFolder = false;

    int _category = 0;
    String _libraryPath, _libraryTitle;
    std::vector<LibraryRecord> _records;
    std::vector<String> _actions;
    std::vector<size_t> _queue;
    meow_ir::Scan _scan;
    bool _scanPending = false;
    uint32_t _lastScan = 0, _lastDraw = 0;
    String _scanName;
    File _loadingFile;
    Scene _loadDestination = Scene::Panel;
    String _loadType, _loadVersion;
    bool _loadHeaderChecked = false;

    String _messageTitle, _messageDetail, _lastError;
    uint32_t _messageUntil = 0;
    IRrecv* _receiver = nullptr;
    IRsend* _sender = nullptr;
    InfraredDecoderHandler* _decoder = nullptr;
    bool _rxEnabled = false;
    TaskHandle_t _txTask = nullptr;
    std::atomic<bool> _txBusy{false}, _txCancel{false}, _txShutdown{false};
    std::atomic<bool> _txStopped{true}, _txSucceeded{true};
    meow_ir::Signal _txSignal;
    const meow_ir::Signal* _heldSource = nullptr;
    bool _txRepeat = false, _holdLatched = false;
    uint32_t _lastTx = 0;
    uint32_t _ledPhaseStart = 0, _ledUntil = 0;
    bool _ledActive = false, _ledOn = false;

    void _sampleInput();
    void _setScene(Scene scene);
    void _push(Scene scene);
    void _pop();
    void _returnTo(Scene scene);
    void _back();
    void _enter();
    void _run();
    void _activate();
    void _draw();
    void _drawList(const char* title, const char* action = "Open");
    void _drawFooter(const char* left, const char* a, const char* b);
    void _drawButton(int x, int y, int w, int h, const char* label,
                     bool selected = false);
    int _listVisible() const;
    bool _singleLineList() const;
    void _drawNameKeyboard();
    void _moveNameKey(int horizontal, int vertical);
    void _updateLed();
    void _message(const char* title, const String& detail, uint32_t duration = 0);
    bool _navigate();
    void _openFiles(bool library);
    void _refreshFiles();
    void _startLearn(bool newRemote);
    void _receive();
    void _beginName(NameFor kind, const char* initial);
    void _commitName();
    void _finishLearn();
    void _nextEasy();
    bool _duplicateName(const char* name, int except = -1) const;
    void _readSettings();
    void _writeSettings();
    void _loadCategory(int category);
    void _startScan(const String& name);
    void _runScan();

    bool _loadRemote(const String& path);
    bool _writeRemote(const String& path, const std::vector<meow_ir::Signal>& signals, bool create);
    bool _beginLibraryLoad(const String& path, Scene destination);
    void _runLibraryLoad();
    void _failLibraryLoad(const String& error);
    bool _readRecord(const LibraryRecord& record, meow_ir::Signal& signal);
    bool _renameRemote(const char* name);
    bool _emptyDirectory(const String& path);
    void _startRx();
    void _stopRx();
    bool _startTx(const meow_ir::Signal& signal, bool repeat = false);
    void _stopTx();
    void _runHeld(const meow_ir::Signal& signal);
    static void _txEntry(void* context);
    void _txWorker();
};
} // namespace MOONCAKE::APPS
