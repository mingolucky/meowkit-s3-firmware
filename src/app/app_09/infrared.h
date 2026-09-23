/**
 * @file app_11.h
 * @author Mingo
 * @brief App09 — Infrared (Learn / Saved Remotes / Universal Remote)
 *        Flipper-style TUI  •  翠绿 + 白色色调
 * @version 1.0
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SD_MMC.h>
#include <FS.h>
#include <vector>
#include <algorithm>

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /* ── Flipper-compatible .ir signal storage ── */
    struct IrSignal {
        char     name[32];
        char     parsedProtocol[16];
        bool     isRaw;
        /* decoded */
        decode_type_t protocol;
        uint64_t value;
        uint16_t bits;
        uint32_t address;
        uint32_t command;
        /* raw */
        std::vector<uint32_t> rawData;   /* µs timings */
        uint32_t frequency;              /* carrier, default 38000 */
        float dutyCycle;                 /* raw carrier duty cycle */
        bool isSupported;                /* parsed/raw replay is supported */
    };

    struct IrRemote {
        char     filename[64];
        char     path[128];
        std::vector<IrSignal> signals;
    };

    /* ── Scene IDs ── */
    enum class IrScene : uint8_t {
        MainMenu,
        LearnWait,
        LearnResult,
        LearnSaveLocation,
        LearnSaveFolderPicker,
        LearnSaveFolderName,
        LearnSaveName,
        LearnSaveExistingConfirm,
        RemoteList,
        DeleteConfirm,
        RemoteView,
        UniversalMenu,
        UniversalTV,
        TVBGone,        /* Iterates all signals in tv.ir like TV-B-Gone */
        Sending,
    };

    /* ── TUI colors ── */
    static constexpr uint16_t COL_BG        = TFT_BLACK;
    static constexpr uint16_t COL_FG        = 0x07E0;  /* pure green */
    static constexpr uint16_t COL_FG_DIM    = TFT_WHITE;
    static constexpr uint16_t COL_ACCENT    = TFT_WHITE;
    static constexpr uint16_t COL_HIGHLIGHT = 0x2E65;  /* dark green fill */
    static constexpr uint16_t COL_ERR       = TFT_RED;

    /* ── App class ── */
    class App09 : public AppAbility {
    public:
        App09(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        DEVICES* _device = nullptr;

        /* ── Scene stack ── */
        IrScene _scene = IrScene::MainMenu;
        IrScene _prevScene = IrScene::MainMenu;
        bool    _sceneDirty = true;

        void _switchScene(IrScene s);

        /* ── TUI helpers ── */
        void _drawHeader(const char* title);
        void _drawMenuItem(int y, int index, const char* text, bool selected);
        void _drawMenuItem2(int y, int index, const char* title, const char* sub, bool selected);
        void _drawFooter3(const char* dirHint, const char* aHint, const char* bHint);
        void _drawMsgBox(const char* line1, const char* line2 = nullptr);
        void _drawNameEditor();

        /* ── Scene handlers ── */
        void _enterMainMenu();
        void _runMainMenu();

        void _enterLearnWait();
        void _runLearnWait();

        void _enterLearnResult();
        void _runLearnResult();

        void _enterLearnSaveName();
        void _runLearnSaveName();
        void _enterLearnSaveLocation();
        void _runLearnSaveLocation();
        void _enterLearnSaveFolderPicker();
        void _runLearnSaveFolderPicker();
        void _enterLearnSaveFolderName();
        void _runLearnSaveFolderName();
        void _enterLearnSaveExistingConfirm();
        void _runLearnSaveExistingConfirm();

        void _enterRemoteList();
        void _runRemoteList();
        void _goUpRemoteDir();
        void _enterDeleteConfirm();
        void _runDeleteConfirm();

        void _enterRemoteView();
        void _runRemoteView();

        void _enterUniversalMenu();
        void _runUniversalMenu();

        void _enterUniversalTV();
        void _runUniversalTV();
        void _runUniversalBlast(const char* triggerLabel);

        void _enterTVBGone();
        void _runTVBGone();
        void _drawTVBGoneFrame();
        void _drawTVBGoneFooter();

        void _enterSending();
        void _runSending();

        /* ── File I/O (Flipper .ir format) ── */
        bool _loadRemote(const char* path, IrRemote& remote, int maxSignals = 0, const char* filterName = nullptr);
        bool _saveSignalToFile(const char* dir, const char* remoteName, const IrSignal& sig);
        bool _appendSignalToFile(const char* path, const IrSignal& sig);
        void _listIrFiles(const char* dir, std::vector<String>& out, bool includeFolders = false);
        bool _isDirectoryEmpty(const char* path);

        /* ── IR hardware ── */
        IRrecv*  _irRecv  = nullptr;
        IRsend*  _irSend  = nullptr;

        bool _txSignal(const IrSignal& sig);
        void _startRx();
        void _stopRx();

        /* ── State ── */
        int  _menuSel      = 0;
        int  _menuCount    = 0;
        int  _scrollOffset = 0;
        char _remoteRoot[96];

        IrSignal  _learnedSig;
        IrRemote  _currentRemote;
        std::vector<String> _fileList;
        char      _remoteDir[96];   /* current folder being browsed in Saved Remotes, e.g. "/infrared" or "/infrared/tv" */
        char      _saveDir[96];     /* destination for a learned signal */
        char      _savePickerDir[96];
        char      _savePath[128];
        char      _deletePath[128];
        bool      _deleteFolder = false;
        bool      _saveFolderPending = false;
        bool      _restoreFolderName = false;
        bool      _preserveSignalName = false;

        /* save-name editor */
        char _editBuf[24];
        int  _editPos;
        int  _editCharIdx;
        int  _vkSel = 0;

        /* Learn page icon cache (PNG bytes loaded from SD once) */
        uint8_t* _learnIconPng = nullptr;
        size_t   _learnIconLen = 0;
        bool     _learnIconTried = false;

        /* universal remote */
        std::vector<String>  _univActions;      /* unique signal names from loaded .ir */
        std::vector<int>     _univBlastQ;       /* indices into _currentRemote.signals to blast */
        int                  _univBlastIdx = 0; /* current position in _univBlastQ */
        bool                 _univBlasting = false;
        char                 _univBlastName[32]; /* action being blasted */
        char                 _univCatTitle[32];  /* category label for header */

        /* virtual-remote grid navigation (directional, not touch) */
        int  _vbSel = 0;            /* selected button index in current grid */

        /* TV-B-Gone state */
        enum class TvbgState : uint8_t { Idle, Running, Paused };
        TvbgState     _tvbgState    = TvbgState::Idle;
        int           _tvbgIdx       = 0;     /* next signal index to send / currently displayed */
        int           _tvbgTotal     = 0;     /* total signals in tv.ir */
        bool          _tvbgPaused    = false; /* deprecated, kept for ABI; mirrors state==Paused */
        unsigned long _tvbgLastSend  = 0;     /* millis() of last send */
        unsigned long _tvbgGapMs     = 250;   /* gap between sends */

        /* sending overlay */
        const char* _sendLabel = nullptr;
        uint32_t    _sendStart = 0;
    };
}
