/**
 * @file app_11.cpp
 * @author Mingo
 * @brief App09 — Infrared (Flipper-style TUI)
 *   Main Menu → Learn / Saved Remotes / Universal Remote
 *   File format: Flipper-compatible .ir
 *   TUI: 翠绿 + 白色色调, 方向键 + A/B 输入
 * @version 1.0
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#include "infrared.h"
#include "../../bsp/config.h"
#include "../app_common/hp_ui.h"

/* ── Layout constants (bound to hp_ui shared chrome) ── */
static constexpr int SCR_W         = hp::W;
static constexpr int SCR_H         = hp::H;
static constexpr int HDR_H         = hp::CON_Y0;
static constexpr int FTR_H         = hp::H - hp::FTR_SEP;
static constexpr int ITEM_H        = hp::ITEM_H;           // 24 px — single-line
static constexpr int ITEM2_H       = hp::ITEM2_H;          // 34 px — two-line (BLE/BadUSB style)
static constexpr int MENU_Y0       = hp::CON_Y0 + 2;
static constexpr int MENU_VISIBLE  = hp::LIST_VIS;         // 7 rows (single-line)
static constexpr int MENU2_VISIBLE = hp::LIST2_VIS;        // 5 rows (two-line)

static constexpr const char* IR_DIR = "/infrared";
static constexpr const char* MEOW_IR_DIR = "/meowkit/infrared";
static constexpr const char* IR_LEARN_ICON_PATH = "/assets/ir_icon.png";

/* ── Universal Remote directory (SD card: /infrared/universal/) ── */
static constexpr const char* UNIV_DIR      = "/infrared/universal";
/* Known category count — indices 0-8 map to kVBtnsDefs[], projectors alias=2 */
static constexpr int          kUnivKnownCount = 9;

/* Subtitle for each known category (index matches kVBtnsDefs[]) */
static const char* kUnivSubtitles[] = {
    "Television",       // 0 tv
    "Air conditioner",  // 1 ac
    "Projector",        // 2 projector / projectors
    "Audio system",     // 3 audio
    "Blu-ray / DVD",    // 4 bluray_dvd
    "Digital signage",  // 5 digital_sign
    "Fan control",      // 6 fans
    "LED strip",        // 7 leds
    "PC monitor",       // 8 monitor
};

/* Map filename stem → category index; -1 = generic */
static int univCatIndex(const char* stem) {
    if (strcasecmp(stem, "tv")           == 0) return 0;
    if (strcasecmp(stem, "ac")           == 0) return 1;
    if (strcasecmp(stem, "projector")    == 0) return 2;
    if (strcasecmp(stem, "audio")        == 0) return 3;
    if (strcasecmp(stem, "bluray_dvd")   == 0) return 4;
    if (strcasecmp(stem, "digital_sign") == 0) return 5;
    if (strcasecmp(stem, "fans")         == 0) return 6;
    if (strcasecmp(stem, "leds")         == 0) return 7;
    if (strcasecmp(stem, "monitor")      == 0) return 8;
    if (strcasecmp(stem, "projectors")   == 0) return 2; /* same layout */
    return -1;
}

/* Make a display label from a filename: strip .ir, replace _ with space,
 * short names (≤3 chars) are fully uppercased, longer ones capitalize first. */
static String univDisplayName(const String& filename) {
    String n = filename;
    int dot = n.lastIndexOf('.');
    if (dot >= 0) n = n.substring(0, dot);
    n.replace("_", " ");
    if (n.length() <= 3) { n.toUpperCase(); }
    else if (n.length() > 0) { n.setCharAt(0, toupper((unsigned char)n.charAt(0))); }
    return n;
}

/* ── Virtual remote button definitions
 *    Layout: 2-column grid, rows = ceil(N/2).
 *    Index matches univCatIndex(): TV=0 AC=1 Proj=2 Audio=3 BDvd=4
 *                                  Sign=5 Fan=6 LED=7 Mon=8
 * ── */
struct VBtnDef { const char* label; const char* sigName; };

/* 0 — TV */
static const VBtnDef kVBtns_TV[] = {
    {"POWER",  "Power"},    {"MUTE",   "Mute"},
    {"VOL +",  "Vol_up"},   {"CH +",   "Ch_next"},
    {"VOL -",  "Vol_dn"},   {"CH -",   "Ch_prev"},
};
static constexpr int kVBtns_TV_N = 6;

/* 1 — AC */
static const VBtnDef kVBtns_AC[] = {
    {"OFF",     "Off"},      {"DRY",     "Dh"},
    {"COOL ^",  "Cool_hi"},  {"HEAT ^",  "Heat_hi"},
    {"COOL v",  "Cool_lo"},  {"HEAT v",  "Heat_lo"},
};
static constexpr int kVBtns_AC_N = 6;

/* 2 — Projector (reused for projectors) */
static const VBtnDef kVBtns_Proj[] = {
    {"POWER",  "Power"},    {"MUTE",   "Mute"},
    {"VOL +",  "Vol_up"},   {"VOL -",  "Vol_dn"},
};
static constexpr int kVBtns_Proj_N = 4;

/* 3 — Audio system */
static const VBtnDef kVBtns_Audio[] = {
    {"POWER",  "Power"},    {"MUTE",   "Mute"},
    {"VOL +",  "Vol_up"},   {"VOL -",  "Vol_dn"},
    {"PLAY",   "Play"},     {"PAUSE",  "Pause"},
    {"NEXT",   "Next"},     {"PREV",   "Prev"},
};
static constexpr int kVBtns_Audio_N = 8;

/* 4 — Blu-ray / DVD */
static const VBtnDef kVBtns_BDvd[] = {
    {"POWER",  "Power"},    {"EJECT",  "Open_Close"},
    {"PLAY",   "Play"},     {"PAUSE",  "Pause"},
    {"STOP",   "Stop"},     {"MUTE",   "Mute"},
    {"NEXT",   "Next"},     {"PREV",   "Prev"},
};
static constexpr int kVBtns_BDvd_N = 8;

/* 5 — Digital signage */
static const VBtnDef kVBtns_Sign[] = {
    {"POWER",  "Power"},    {"MUTE",   "Mute"},
    {"VOL +",  "Vol_up"},   {"VOL -",  "Vol_dn"},
};
static constexpr int kVBtns_Sign_N = 4;

/* 6 — Fans */
static const VBtnDef kVBtns_Fan[] = {
    {"POWER",  "Power"},    {"SWING",  "Swing"},
    {"SPD +",  "Speed_up"}, {"SPD -",  "Speed_dn"},
    {"SLEEP",  "Sleep"},    {"TIMER",  "Timer"},
};
static constexpr int kVBtns_Fan_N = 6;

/* 7 — LED strips */
static const VBtnDef kVBtns_LED[] = {
    {"POWER",  "Power"},       {"FLASH",   "Flash"},
    {"BRT +",  "Brightness_up"},{"BRT -",  "Brightness_dn"},
    {"STROBE", "Strobe"},      {"SMOOTH",  "Smooth"},
};
static constexpr int kVBtns_LED_N = 6;

/* 8 — PC monitor */
static const VBtnDef kVBtns_Mon[] = {
    {"POWER",  "Power"},       {"INPUT",  "Input_next"},
    {"BRT +",  "Brightness_up"},{"BRT -", "Brightness_dn"},
};
static constexpr int kVBtns_Mon_N = 4;

/* Indexed by univCatIndex() return value (0-8) */
static const VBtnDef* const kVBtnsDefs[] = {
    kVBtns_TV, kVBtns_AC, kVBtns_Proj,
    kVBtns_Audio, kVBtns_BDvd, kVBtns_Sign,
    kVBtns_Fan, kVBtns_LED, kVBtns_Mon,
};
static const int kVBtnsCount[] = {
    kVBtns_TV_N, kVBtns_AC_N, kVBtns_Proj_N,
    kVBtns_Audio_N, kVBtns_BDvd_N, kVBtns_Sign_N,
    kVBtns_Fan_N, kVBtns_LED_N, kVBtns_Mon_N,
};

/* Compute button rect for a 2-column grid within the content area.
 *   Area: x=5..314, y=CON_Y0+4..CON_Y1-4.  GAP=6 between buttons. */
static void _getVBtnRect(int idx, int numBtns,
                          int& bx, int& by, int& bw, int& bh)
{
    const int GAP = 6, AX = 5, AY = hp::CON_Y0 + 4;
    const int AW  = hp::W - 10;
    const int AH  = hp::CON_Y1 - hp::CON_Y0 - 8;
    int rows = (numBtns + 1) / 2;
    bw = (AW - GAP) / 2;
    bh = (AH - (rows - 1) * GAP) / rows;
    bx = AX + (idx % 2) * (bw + GAP);
    by = AY + (idx / 2) * (bh + GAP);
}

/* ── Shared virtual-keyboard keymap (4 x 10) ── */
static const char* kNameKeys[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "_", "-", ".", "[DEL]"
};
static constexpr int kNameCols = 10;
static constexpr int kNameKeyCount = (int)(sizeof(kNameKeys) / sizeof(kNameKeys[0]));

/* Load a file from SD into PSRAM. Caller owns the returned buffer. */
static uint8_t* _irLoadSdFile(const char* path, size_t& outLen)
{
    outLen = 0;
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) return nullptr;
    size_t sz = f.size();
    if (sz == 0) {
        f.close();
        return nullptr;
    }
    uint8_t* buf = (uint8_t*)ps_malloc(sz);
    if (!buf) {
        f.close();
        return nullptr;
    }
    size_t got = f.read(buf, sz);
    f.close();
    if (got != sz) {
        free(buf);
        return nullptr;
    }
    outLen = sz;
    return buf;
}

namespace MOONCAKE::APPS
{
    /* ════════════════════════════════════════════════════════════
     *  Constructor / Lifecycle
     * ════════════════════════════════════════════════════════════ */

    App09::App09(DEVICES* device) : _device(device)
    {
        setAppInfo().name = "Infrared";
    }

    void App09::onOpen()
    {
        _irSend = new IRsend(HAL_PIN_IR_TX);
        _irSend->begin();
        _irRecv = nullptr;

        /* Safe reset: do NOT memset structs containing std::vector members
         * (memset over a vector orphans its heap allocation → leak on every
         * reopen, which is what caused "second SD load fails"). */
        _learnedSig.name[0]   = '\0';
        _learnedSig.isRaw     = false;
        _learnedSig.protocol  = UNKNOWN;
        _learnedSig.value     = 0;
        _learnedSig.bits      = 0;
        _learnedSig.address   = 0;
        _learnedSig.command   = 0;
        _learnedSig.frequency = 38000;
        _learnedSig.rawData.clear();

        _currentRemote.filename[0] = '\0';
        _currentRemote.path[0]     = '\0';
        _currentRemote.signals.clear();

        memset(_editBuf, 0, sizeof(_editBuf));
        _editPos = 0;
        _editCharIdx = 0;
        _vkSel = 0;
        _learnIconTried = false;
        _learnIconLen = 0;
        _learnIconPng = nullptr;

        /* Ensure IR directory exists on SD */
        if (!SD_MMC.exists(IR_DIR)) SD_MMC.mkdir(IR_DIR);
        if (!SD_MMC.exists("/meowkit")) SD_MMC.mkdir("/meowkit");
        if (!SD_MMC.exists(MEOW_IR_DIR)) SD_MMC.mkdir(MEOW_IR_DIR);
        strcpy(_remoteRoot, IR_DIR);
        strcpy(_remoteDir, IR_DIR);

        _switchScene(IrScene::MainMenu);
    }

    void App09::onRunning()
    {
        _device->button.update();
        _device->button.tick();

        /* Long-press B = exit app */
        if (_device->button.B.isLongPress()) {
            close();
            return;
        }

        /* Scene enter (draw) + run (input) */
        if (_sceneDirty) {
            _sceneDirty = false;
            switch (_scene) {
                case IrScene::MainMenu:      _enterMainMenu();      break;
                case IrScene::LearnWait:     _enterLearnWait();     break;
                case IrScene::LearnResult:   _enterLearnResult();   break;
                case IrScene::LearnSaveName: _enterLearnSaveName(); break;
                case IrScene::RemoteList:    _enterRemoteList();    break;
                case IrScene::RemoteView:    _enterRemoteView();    break;
                case IrScene::UniversalMenu: _enterUniversalMenu(); break;
                case IrScene::UniversalTV:   _enterUniversalTV();   break;
                case IrScene::TVBGone:       _enterTVBGone();       break;
                case IrScene::Sending:       _enterSending();       break;
            }
        }

        switch (_scene) {
            case IrScene::MainMenu:      _runMainMenu();      break;
            case IrScene::LearnWait:     _runLearnWait();     break;
            case IrScene::LearnResult:   _runLearnResult();   break;
            case IrScene::LearnSaveName: _runLearnSaveName(); break;
            case IrScene::RemoteList:    _runRemoteList();    break;
            case IrScene::RemoteView:    _runRemoteView();    break;
            case IrScene::UniversalMenu: _runUniversalMenu(); break;
            case IrScene::UniversalTV:   _runUniversalTV();   break;
            case IrScene::TVBGone:       _runTVBGone();       break;
            case IrScene::Sending:       _runSending();       break;
        }
    }

    void App09::onClose()
    {
        _stopRx();
        if (_irSend) { delete _irSend; _irSend = nullptr; }
        if (_irRecv) { delete _irRecv; _irRecv = nullptr; }
        _fileList.clear();
        _currentRemote.signals.clear();
        _univActions.clear();
        _univBlastQ.clear();
        if (_learnIconPng) {
            free(_learnIconPng);
            _learnIconPng = nullptr;
        }
        _learnIconLen = 0;
        _learnIconTried = false;
    }

    /* ════════════════════════════════════════════════════════════
     *  Scene management
     * ════════════════════════════════════════════════════════════ */

    void App09::_switchScene(IrScene s)
    {
        _prevScene = _scene;
        _scene = s;
        _sceneDirty = true;
        _menuSel = 0;
        _scrollOffset = 0;
    }

    /* ════════════════════════════════════════════════════════════
     *  TUI drawing helpers
     * ════════════════════════════════════════════════════════════ */

    void App09::_drawHeader(const char* title)
    {
        auto& Lcd = _device->Lcd;
        hp::drawChrome(Lcd);
        hp::drawHeader(Lcd, title);
    }

    void App09::_drawMenuItem(int y, int index, const char* text, bool selected)
    {
        (void)index;
        int row = (y - MENU_Y0) / ITEM_H;
        if (row < 0) row = 0;
        if (row >= hp::LIST_VIS) row = hp::LIST_VIS - 1;
        hp::drawListItem(_device->Lcd, row, text, selected);
        hp::drawScrollbar(_device->Lcd, _menuCount, _scrollOffset, MENU_VISIBLE);
    }

    /* Two-line list item (BLE Spam / Bad USB style): title + dimmed subtitle. */
    void App09::_drawMenuItem2(int y, int index, const char* title,
                               const char* sub, bool selected)
    {
        (void)index;
        int row = (y - MENU_Y0) / ITEM2_H;
        if (row < 0) row = 0;
        if (row >= hp::LIST2_VIS) row = hp::LIST2_VIS - 1;
        hp::drawListItemSub(_device->Lcd, row, title, sub, selected);
        hp::drawScrollbar2(_device->Lcd, _menuCount, _scrollOffset, MENU2_VISIBLE);
    }

    void App09::_drawFooter(const char* left, const char* right)
    {
        hp::drawFooter(_device->Lcd, left, right);
    }

    void App09::_drawFooter3(const char* dirHint, const char* aHint, const char* bHint)
    {
        hp::drawFooter3(_device->Lcd, dirHint, aHint, bHint);
    }

    void App09::_drawMsgBox(const char* line1, const char* line2)
    {
        hp::drawDialog(_device->Lcd, line1, line2);
    }

    /* ════════════════════════════════════════════════════════════
     *  Main Menu
     * ════════════════════════════════════════════════════════════ */

    static const char* kMainItems[] = {
        "Universal Remote",
        "Learn New Signal",
        "Imported Remotes",
        "Saved Remotes",
    };
    static constexpr int kMainCount = 4;

    void App09::_enterMainMenu()
    {
        _menuCount = kMainCount;
        _drawHeader("Infrared");
        _drawFooter3("[^v]Select", "[A]Enter", "[B]Exit");
        for (int i = 0; i < kMainCount; i++) {
            _drawMenuItem(MENU_Y0 + i * ITEM_H, i, kMainItems[i], i == _menuSel);
            if (i != _menuSel) {
                int sepY = hp::CON_Y0 + 2 + i * hp::ITEM_H + hp::ITEM_H - 1;
                _device->Lcd.drawFastHLine(1, sepY, hp::W - 3 - hp::SBAR_W, hp::COL_BG);
            }
        }
    }

    void App09::_runMainMenu()
    {
        bool redraw = false;
        if (_device->button.Up.pressed()   && _menuSel > 0)              { _menuSel--; redraw = true; }
        if (_device->button.Down.pressed() && _menuSel < kMainCount - 1) { _menuSel++; redraw = true; }

        if (redraw) {
            for (int i = 0; i < kMainCount; i++) {
                _drawMenuItem(MENU_Y0 + i * ITEM_H, i, kMainItems[i], i == _menuSel);
                if (i != _menuSel) {
                    int sepY = hp::CON_Y0 + 2 + i * hp::ITEM_H + hp::ITEM_H - 1;
                    _device->Lcd.drawFastHLine(1, sepY, hp::W - 3 - hp::SBAR_W, hp::COL_BG);
                }
            }
        }

        if (_device->button.A.pressed()) {
            switch (_menuSel) {
                case 0: _switchScene(IrScene::UniversalMenu); break;
                case 1: _switchScene(IrScene::LearnWait);     break;
                case 2: strcpy(_remoteRoot, IR_DIR); strcpy(_remoteDir, IR_DIR); _switchScene(IrScene::RemoteList); break;
                case 3: strcpy(_remoteRoot, MEOW_IR_DIR); strcpy(_remoteDir, MEOW_IR_DIR); _switchScene(IrScene::RemoteList); break;
            }
        }
        if (_device->button.B.pressed()) {
            close();
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Learn — Wait for signal
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterLearnWait()
    {
        if (!_learnIconTried) {
            _learnIconTried = true;
            _learnIconPng = _irLoadSdFile(IR_LEARN_ICON_PATH, _learnIconLen);
        }

        _drawHeader("Learn Signal");
        hp::drawHeader(_device->Lcd, "Learn Signal", "RX", hp::COL_WARN);
        _drawFooter3(nullptr, nullptr, "[B]Back");

        auto& Lcd = _device->Lcd;
        Lcd.setTextColor(COL_FG, COL_BG);
        Lcd.setCursor(20, 30);
        Lcd.print("Point remote at IR port");
        Lcd.setCursor(20, 50);
        Lcd.print("and press any button...");

        if (_learnIconPng && _learnIconLen > 0) {
            hp::drawPng(Lcd, 78, 74, _learnIconPng, _learnIconLen);
        }

        Lcd.setCursor(20, 150);
        Lcd.setTextColor(COL_FG_DIM, COL_BG);
        Lcd.print("Waiting for IR signal...");

        _startRx();
    }

    void App09::_runLearnWait()
    {
        if (_device->button.B.pressed()) {
            _stopRx();
            _switchScene(IrScene::MainMenu);
            return;
        }

        if (!_irRecv) return;

        decode_results results;
        if (_irRecv->decode(&results)) {
            /* Store the received signal (do NOT memset over the vector) */
            _learnedSig.rawData.clear();
            _learnedSig.name[0]   = '\0';
            _learnedSig.isRaw     = false;
            _learnedSig.protocol  = UNKNOWN;
            _learnedSig.value     = 0;
            _learnedSig.bits      = 0;
            _learnedSig.address   = 0;
            _learnedSig.command   = 0;
            _learnedSig.frequency = 38000;
            strncpy(_learnedSig.name, "Signal", sizeof(_learnedSig.name) - 1);

            if (results.decode_type != UNKNOWN && results.decode_type != (decode_type_t)(-1)) {
                _learnedSig.isRaw    = false;
                _learnedSig.protocol = results.decode_type;
                _learnedSig.value    = results.value;
                _learnedSig.bits     = results.bits;
                _learnedSig.address  = results.address;
                _learnedSig.command  = results.command;
            } else {
                _learnedSig.isRaw     = true;
                _learnedSig.frequency = 38000;
                _learnedSig.rawData.clear();
                for (uint16_t i = 1; i < results.rawlen; i++) {
                    _learnedSig.rawData.push_back(results.rawbuf[i] * kRawTick);
                }
            }

            _stopRx();
            _switchScene(IrScene::LearnResult);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Learn — Result display
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterLearnResult()
    {
        _drawHeader("Signal Received");
        hp::drawHeader(_device->Lcd, "Signal Received", "OK", hp::COL_FG);
        _drawFooter3("[O]Retry", "[A]Send", "[B]Save");

        auto& Lcd = _device->Lcd;
        int y = MENU_Y0;

        if (_learnedSig.isRaw) {
            Lcd.setTextColor(COL_ACCENT, COL_BG);
            Lcd.setCursor(20, y);
            Lcd.print("Type: RAW");
            y += 22;
            Lcd.setTextColor(COL_FG, COL_BG);
            Lcd.setCursor(20, y);
            Lcd.printf("Samples: %d", (int)_learnedSig.rawData.size());
            y += 22;
            Lcd.setCursor(20, y);
            Lcd.printf("Frequency: %luHz", _learnedSig.frequency);
        } else {
            String proto = typeToString(_learnedSig.protocol, false);
            Lcd.setTextColor(COL_ACCENT, COL_BG);
            Lcd.setCursor(20, y);
            Lcd.printf("Protocol: %s", proto.c_str());
            y += 22;
            Lcd.setTextColor(COL_FG, COL_BG);
            Lcd.setCursor(20, y);
            Lcd.printf("Value: 0x%llX", _learnedSig.value);
            y += 22;
            Lcd.setCursor(20, y);
            Lcd.printf("Bits: %d", _learnedSig.bits);
            y += 22;
            Lcd.setCursor(20, y);
            Lcd.printf("Addr: 0x%X  Cmd: 0x%X",
                        _learnedSig.address, _learnedSig.command);
        }
    }

    void App09::_runLearnResult()
    {
        /* [O] = joystick: any direction triggers Retry */
        if (_device->button.Up.pressed()    ||
            _device->button.Down.pressed()  ||
            _device->button.Left.pressed()  ||
            _device->button.Right.pressed()) {
            _switchScene(IrScene::LearnWait);
            return;
        }
        if (_device->button.A.pressed()) {
            _txSignal(_learnedSig);
            hp::drawToast(_device->Lcd, "SENT", hp::COL_FG);
            delay(300);
            _sceneDirty = true;
        }
        if (_device->button.B.pressed()) {
            _switchScene(IrScene::LearnSaveName);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Learn — Save name editor
     * ════════════════════════════════════════════════════════════ */

    void App09::_drawNameEditor()
    {
        hp::drawVirtualKeyboard(
            _device->Lcd,
            "Name:",
            _editBuf,
            (int)sizeof(_editBuf) - 1,
            kNameKeys,
            kNameKeyCount,
            kNameCols,
            _vkSel
        );
    }

    void App09::_enterLearnSaveName()
    {
        strncpy(_editBuf, _learnedSig.name, sizeof(_editBuf) - 1);
        _editBuf[sizeof(_editBuf) - 1] = '\0';
        _editPos = strlen(_editBuf);
        _editCharIdx = 0;
        _vkSel = 0;

        _drawHeader("SAVE SIGNAL");
        _drawFooter3("[^v<>]Move", "[A]Select", "[B]Save");

        _drawNameEditor();
    }

    void App09::_runLearnSaveName()
    {
        bool redraw = false;
        if (_device->button.Up.pressed() && _vkSel >= kNameCols) {
            _vkSel -= kNameCols;
            redraw = true;
        }
        if (_device->button.Down.pressed() && _vkSel + kNameCols < kNameKeyCount) {
            _vkSel += kNameCols;
            redraw = true;
        }
        if (_device->button.Left.pressed() && (_vkSel % kNameCols) > 0) {
            _vkSel--;
            redraw = true;
        }
        if (_device->button.Right.pressed() && (_vkSel % kNameCols) < (kNameCols - 1) && _vkSel + 1 < kNameKeyCount) {
            _vkSel++;
            redraw = true;
        }

        if (_device->button.A.pressed()) {
            const char* key = kNameKeys[_vkSel];
            int nameLen = strlen(_editBuf);
            if (strcmp(key, "[DEL]") == 0) {
                if (nameLen > 0) {
                    _editBuf[nameLen - 1] = '\0';
                    redraw = true;
                }
            } else if (nameLen < (int)sizeof(_editBuf) - 1) {
                _editBuf[nameLen] = key[0];
                _editBuf[nameLen + 1] = '\0';
                redraw = true;
            }
        }

        if (redraw) _drawNameEditor();

        if (_device->button.B.pressed()) {
            int nameLen = strlen(_editBuf);
            while (nameLen > 0 && (_editBuf[nameLen-1] == ' ' || _editBuf[nameLen-1] == '_')) {
                _editBuf[--nameLen] = '\0';
            }
            if (nameLen == 0) strcpy(_editBuf, "Signal");

            strncpy(_learnedSig.name, _editBuf, sizeof(_learnedSig.name) - 1);

            char path[128];
            snprintf(path, sizeof(path), "%s/%s.ir", MEOW_IR_DIR, _editBuf);

            bool ok;
            if (SD_MMC.exists(path)) {
                ok = _appendSignalToFile(path, _learnedSig);
            } else {
                ok = _saveSignalToFile(MEOW_IR_DIR, _editBuf, _learnedSig);
            }

            if (ok) {
                _drawMsgBox("Signal saved!", path);
            } else {
                _drawMsgBox("Save FAILED!", "Check SD card");
            }
            delay(1500);
            _switchScene(IrScene::MainMenu);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Saved Remotes — File list
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterRemoteList()
    {
        _fileList.clear();
        _listIrFiles(_remoteDir, _fileList);
        _menuCount = _fileList.size();

        _drawHeader("Saved Remotes");
        _drawFooter3("[^v]Select", "[A]Open", "[B]Back");

        if (_menuCount == 0) {
            auto& Lcd = _device->Lcd;
            hp::clearContent(Lcd);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.setTextColor(hp::COL_DIM, hp::COL_BG);
            Lcd.setCursor(20, 80);
            Lcd.print("No .ir files found");
            Lcd.setCursor(20, 105);
            Lcd.printf("Place in: %s/", _remoteDir);
        } else {
            int end = _menuCount < MENU2_VISIBLE ? _menuCount : MENU2_VISIBLE;
            for (int i = 0; i < end; i++) {
                String name = _fileList[i + _scrollOffset];
                int dot = name.lastIndexOf('.');
                if (dot >= 0) name = name.substring(0, dot);
                _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i,
                               name.c_str(), "IR Remote", i == _menuSel);
            }
        }
    }

    void App09::_runRemoteList()
    {
        if (_menuCount == 0) {
            if (_device->button.B.pressed()) _switchScene(IrScene::MainMenu);
            return;
        }

        bool redraw = false;

        if (_device->button.Up.pressed()) {
            if (_menuSel > 0) _menuSel--;
            else if (_scrollOffset > 0) _scrollOffset--;
            redraw = true;
        }
        if (_device->button.Down.pressed()) {
            if (_menuSel < MENU2_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1)
                _menuSel++;
            else if (_scrollOffset + MENU2_VISIBLE < _menuCount)
                _scrollOffset++;
            redraw = true;
        }

        if (redraw) {
            int visible = _menuCount - _scrollOffset;
            if (visible > MENU2_VISIBLE) visible = MENU2_VISIBLE;
            for (int i = 0; i < MENU2_VISIBLE; i++) {
                if (i < visible) {
                    String name = _fileList[i + _scrollOffset];
                    int dot = name.lastIndexOf('.');
                    if (dot >= 0) name = name.substring(0, dot);
                    _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i,
                                   name.c_str(), "IR Remote", i == _menuSel);
                } else {
                    _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM2_H, SCR_W, ITEM2_H, hp::COL_BG);
                }
            }
        }

        if (_device->button.A.pressed()) {
            int idx = _menuSel + _scrollOffset;
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", _remoteDir, _fileList[idx].c_str());

            if (_loadRemote(path, _currentRemote)) {
                _switchScene(IrScene::RemoteView);
            } else {
                _drawMsgBox("Failed to load!", path);
                delay(1500);
                _sceneDirty = true;
            }
        }
        if (_device->button.B.pressed()) {
            _switchScene(IrScene::MainMenu);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Remote View — Show signals, send on A
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterRemoteView()
    {
        _menuCount = _currentRemote.signals.size();

        _drawHeader(_currentRemote.filename);
        _drawFooter3("[^v]Select", "[A]Send", "[B]Back");

        if (_menuCount == 0) {
            auto& Lcd = _device->Lcd;
            hp::clearContent(Lcd);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.setTextColor(hp::COL_DIM, hp::COL_BG);
            Lcd.setCursor(20, 80);
            Lcd.print("Empty remote (no signals)");
        } else {
            int end = _menuCount < MENU2_VISIBLE ? _menuCount : MENU2_VISIBLE;
            for (int i = 0; i < end; i++) {
                const IrSignal& sig = _currentRemote.signals[i + _scrollOffset];
                char sub[40];
                if (sig.isRaw) {
                    snprintf(sub, sizeof(sub), "RAW  %d samples", (int)sig.rawData.size());
                } else {
                    snprintf(sub, sizeof(sub), "%s  0x%llX",
                             typeToString(sig.protocol, false).c_str(), sig.value);
                }
                _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i, sig.name, sub, i == _menuSel);
            }
        }
    }

    void App09::_runRemoteView()
    {
        if (_menuCount == 0) {
            if (_device->button.B.pressed()) _switchScene(IrScene::RemoteList);
            return;
        }

        bool redraw = false;

        if (_device->button.Up.pressed()) {
            if (_menuSel > 0) _menuSel--;
            else if (_scrollOffset > 0) _scrollOffset--;
            redraw = true;
        }
        if (_device->button.Down.pressed()) {
            if (_menuSel < MENU2_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1) _menuSel++;
            else if (_scrollOffset + MENU2_VISIBLE < _menuCount) _scrollOffset++;
            redraw = true;
        }

        if (redraw) {
            int visible = _menuCount - _scrollOffset;
            if (visible > MENU2_VISIBLE) visible = MENU2_VISIBLE;
            for (int i = 0; i < MENU2_VISIBLE; i++) {
                int dataIdx = i + _scrollOffset;
                if (i < visible) {
                    const IrSignal& sig = _currentRemote.signals[dataIdx];
                    char sub[40];
                    if (sig.isRaw) {
                        snprintf(sub, sizeof(sub), "RAW  %d samples", (int)sig.rawData.size());
                    } else {
                        snprintf(sub, sizeof(sub), "%s  0x%llX",
                                 typeToString(sig.protocol, false).c_str(), sig.value);
                    }
                    _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i, sig.name, sub, i == _menuSel);
                } else {
                    _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM2_H, SCR_W, ITEM2_H, hp::COL_BG);
                }
            }
        }

        if (_device->button.A.pressed()) {
            int idx = _menuSel + _scrollOffset;
            _txSignal(_currentRemote.signals[idx]);
            hp::drawToast(_device->Lcd, "SENT", hp::COL_FG);
            delay(200);
            _sceneDirty = true;
        }
        if (_device->button.B.pressed()) {
            _switchScene(IrScene::RemoteList);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Universal Remote Menu
     * ════════════════════════════════════════════════════════════ */

    /* Helper: build the subtitle string for a universal-menu item. */
    static const char* _univMenuSub(const char* stem) {
        int idx = univCatIndex(stem);
        return (idx >= 0 && idx < kUnivKnownCount) ? kUnivSubtitles[idx] : "IR device control";
    }

    void App09::_enterUniversalMenu()
    {
        _fileList.clear();
        _listIrFiles(UNIV_DIR, _fileList);
        _menuCount = _fileList.size();

        _drawHeader("Universal Remote");
        _drawFooter3("[^v]Select", "[A]Enter", "[B]Back");

        if (_menuCount == 0) {
            auto& Lcd = _device->Lcd;
            hp::clearContent(Lcd);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.setTextColor(hp::COL_DIM, hp::COL_BG);
            Lcd.setCursor(20, 80);
            Lcd.print("No .ir files found");
            Lcd.setCursor(20, 105);
            Lcd.printf("Place in: %s/", UNIV_DIR);
        } else {
            int end = _menuCount < MENU2_VISIBLE ? _menuCount : MENU2_VISIBLE;
            for (int i = 0; i < end; i++) {
                String name = univDisplayName(_fileList[i + _scrollOffset]);
                String stem = _fileList[i + _scrollOffset];
                int dot = stem.lastIndexOf('.');
                if (dot >= 0) stem = stem.substring(0, dot);
                _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i, name.c_str(),
                               _univMenuSub(stem.c_str()), i == _menuSel);
            }
        }
    }

    void App09::_runUniversalMenu()
    {
        if (_menuCount == 0) {
            if (_device->button.B.pressed()) _switchScene(IrScene::MainMenu);
            return;
        }

        bool redraw = false;
        if (_device->button.Up.pressed()) {
            if (_menuSel > 0) _menuSel--;
            else if (_scrollOffset > 0) _scrollOffset--;
            redraw = true;
        }
        if (_device->button.Down.pressed()) {
            if (_menuSel < MENU2_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1)
                _menuSel++;
            else if (_scrollOffset + MENU2_VISIBLE < _menuCount)
                _scrollOffset++;
            redraw = true;
        }

        if (redraw) {
            int visible = _menuCount - _scrollOffset;
            if (visible > MENU2_VISIBLE) visible = MENU2_VISIBLE;
            for (int i = 0; i < MENU2_VISIBLE; i++) {
                if (i < visible) {
                    String name = univDisplayName(_fileList[i + _scrollOffset]);
                    String stem = _fileList[i + _scrollOffset];
                    int dot = stem.lastIndexOf('.');
                    if (dot >= 0) stem = stem.substring(0, dot);
                    _drawMenuItem2(MENU_Y0 + i * ITEM2_H, i, name.c_str(),
                                   _univMenuSub(stem.c_str()), i == _menuSel);
                } else {
                    _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM2_H, SCR_W, ITEM2_H, hp::COL_BG);
                }
            }
        }

        if (_device->button.A.pressed()) {
            int dataIdx = _menuSel + _scrollOffset;

            String stem = _fileList[dataIdx];
            int dot = stem.lastIndexOf('.');
            if (dot >= 0) stem = stem.substring(0, dot);

            String dispName = univDisplayName(_fileList[dataIdx]);
            strncpy(_univCatTitle, dispName.c_str(), sizeof(_univCatTitle) - 1);
            _univCatTitle[sizeof(_univCatTitle) - 1] = '\0';

            char path[128];
            snprintf(path, sizeof(path), "%s/%s", UNIV_DIR, _fileList[dataIdx].c_str());

            int catIdx = univCatIndex(stem.c_str());
            _univBlastIdx = catIdx;
            _vbSel = 0;
            _univBlastQ.clear();
            _univBlastName[0] = '\0';

            if (catIdx >= 0) {
                /* Known category: button panel is hardcoded — no preload needed.
                 * Filtered load happens per-button in _runUniversalTV so every
                 * brand's signal is available without any signal-count cap. */
                _currentRemote.signals.clear();
                strncpy(_currentRemote.path, path, sizeof(_currentRemote.path) - 1);
                _currentRemote.path[sizeof(_currentRemote.path) - 1] = '\0';
            } else {
                /* Unknown category: load all signals (capped) to use as buttons. */
                hp::drawHeader(_device->Lcd, _univCatTitle, "Reading...", hp::COL_WARN);
                bool loaded = _loadRemote(path, _currentRemote, 100);
                if (!loaded) {
                    hp::drawDialog(_device->Lcd, "File not found!", path, hp::COL_ERR);
                    delay(1500);
                    _sceneDirty = true;
                    return;
                }
            }
            _switchScene(IrScene::UniversalTV);
        }
        if (_device->button.B.pressed()) {
            _switchScene(IrScene::MainMenu);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  Universal Remote — Virtual button panel (directional nav)
     *  2-column grid for the selected device category.
     *  Up/Down/Left/Right moves selection; A sends; B back.
     *  Special: when category == TV and POWER is selected, A launches
     *  TV-B-Gone mode (iterate every signal in tv.ir).
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterUniversalTV()
    {
        auto& Lcd = _device->Lcd;
        hp::drawChrome(Lcd);
        hp::drawHeader(Lcd, _univCatTitle, "REMOTE", hp::COL_FG);
        hp::drawFooter3(Lcd, "[^v<>]Select", "[A]Blast", "[B]Back");

        if (_univBlastIdx >= 0 && _univBlastIdx < kUnivKnownCount) {
            /* Known category: use hardcoded button definitions */
            int numBtns = kVBtnsCount[_univBlastIdx];
            const VBtnDef* btns = kVBtnsDefs[_univBlastIdx];
            if (_vbSel < 0 || _vbSel >= numBtns) _vbSel = 0;
            for (int i = 0; i < numBtns; i++) {
                int bx, by, bw, bh;
                _getVBtnRect(i, numBtns, bx, by, bw, bh);
                hp::drawVirtualButton(Lcd, bx, by, bw, bh, btns[i].label, i == _vbSel);
            }
        } else {
            /* Generic category: use signal names from the loaded remote as buttons */
            int numBtns = (int)_currentRemote.signals.size();
            if (numBtns > 12) numBtns = 12;
            if (_vbSel < 0 || _vbSel >= numBtns) _vbSel = 0;
            if (numBtns == 0) {
                Lcd.setTextColor(COL_FG_DIM, COL_BG);
                Lcd.setCursor(20, 80);
                Lcd.print("No signals in file");
            } else {
                for (int i = 0; i < numBtns; i++) {
                    int bx, by, bw, bh;
                    _getVBtnRect(i, numBtns, bx, by, bw, bh);
                    hp::drawVirtualButton(Lcd, bx, by, bw, bh,
                                          _currentRemote.signals[i].name, i == _vbSel);
                }
            }
        }
    }

    void App09::_runUniversalTV()
    {
        if (_device->button.B.pressed()) {
            _switchScene(IrScene::UniversalMenu);
            return;
        }

        bool knownCat = (_univBlastIdx >= 0 && _univBlastIdx < kUnivKnownCount);
        int numBtns = knownCat
            ? kVBtnsCount[_univBlastIdx]
            : (int)_currentRemote.signals.size();
        if (numBtns > 12 && !knownCat) numBtns = 12;
        if (numBtns == 0) return;

        int rows   = (numBtns + 1) / 2;
        int oldSel = _vbSel;
        int row    = _vbSel / 2;
        int col    = _vbSel % 2;

        if (_device->button.Up.pressed())    { row = (row - 1 + rows) % rows; }
        if (_device->button.Down.pressed())  { row = (row + 1) % rows; }
        if (_device->button.Left.pressed())  { col = (col - 1 + 2) % 2; }
        if (_device->button.Right.pressed()) { col = (col + 1) % 2; }

        int newSel = row * 2 + col;
        if (newSel >= numBtns) newSel = numBtns - 1;
        if (newSel != oldSel) {
            _vbSel = newSel;
            auto& Lcd = _device->Lcd;
            int bx, by, bw, bh;
            if (knownCat) {
                const VBtnDef* btns = kVBtnsDefs[_univBlastIdx];
                _getVBtnRect(oldSel, numBtns, bx, by, bw, bh);
                hp::drawVirtualButton(Lcd, bx, by, bw, bh, btns[oldSel].label, false);
                _getVBtnRect(newSel, numBtns, bx, by, bw, bh);
                hp::drawVirtualButton(Lcd, bx, by, bw, bh, btns[newSel].label, true);
            } else {
                _getVBtnRect(oldSel, numBtns, bx, by, bw, bh);
                hp::drawVirtualButton(Lcd, bx, by, bw, bh, _currentRemote.signals[oldSel].name, false);
                _getVBtnRect(newSel, numBtns, bx, by, bw, bh);
                hp::drawVirtualButton(Lcd, bx, by, bw, bh, _currentRemote.signals[newSel].name, true);
            }
        }

        if (_device->button.A.pressed()) {
            const char* sigName;
            const char* btnLabel;
            if (knownCat) {
                sigName  = kVBtnsDefs[_univBlastIdx][_vbSel].sigName;
                btnLabel = kVBtnsDefs[_univBlastIdx][_vbSel].label;
            } else {
                int idx = (_vbSel < (int)_currentRemote.signals.size()) ? _vbSel : 0;
                sigName  = _currentRemote.signals[idx].name;
                btnLabel = sigName;
            }
            strncpy(_univBlastName, btnLabel, sizeof(_univBlastName) - 1);
            _univBlastName[sizeof(_univBlastName) - 1] = '\0';

            if (knownCat) {
                /* Known category: load the .ir file filtered to only signals
                 * matching this button's sigName.  No maxSignals cap — the
                 * filter itself bounds memory to (brands × 1 signal). */
                hp::drawHeader(_device->Lcd, _univCatTitle, "Loading...", hp::COL_WARN);
                bool loaded = _loadRemote(_currentRemote.path, _currentRemote, 0, sigName);
                if (!loaded || _currentRemote.signals.empty()) {
                    hp::drawDialog(_device->Lcd, "No signals for:", sigName, hp::COL_ERR);
                    delay(1500);
                    _sceneDirty = true;
                    return;
                }
                /* Every loaded signal is already the target action → queue = all */
                _univBlastQ.clear();
                for (int i = 0; i < (int)_currentRemote.signals.size(); i++)
                    _univBlastQ.push_back(i);
            } else {
                /* Unknown category: filter from already-loaded signals */
                _univBlastQ.clear();
                for (int i = 0; i < (int)_currentRemote.signals.size(); i++) {
                    if (strcasecmp(_currentRemote.signals[i].name, sigName) == 0)
                        _univBlastQ.push_back(i);
                }
                if (_univBlastQ.empty()) {
                    for (int i = 0; i < (int)_currentRemote.signals.size(); i++)
                        _univBlastQ.push_back(i);
                }
            }

            _switchScene(IrScene::TVBGone);
        }
    }

    void App09::_runUniversalBlast(const char* triggerLabel)
    {
        auto& Lcd = _device->Lcd;
        const int total = (int)_currentRemote.signals.size();
        if (total <= 0) {
            hp::drawDialog(Lcd, "No signals loaded", "Check /infrared/universal", hp::COL_ERR);
            delay(1200);
            _sceneDirty = true;
            return;
        }

        const char* action = (triggerLabel && triggerLabel[0]) ? triggerLabel : "UNIVERSAL";
        _device->led.setColor(WS2812B_Class::RED);

        for (int i = 0; i < total; i++) {
            const IrSignal& sig = _currentRemote.signals[i];
            const char* sigName = (sig.name[0] != '\0') ? sig.name : "(unnamed)";
            float pct = (float)i / (float)total;
            hp::drawExecPopup(Lcd, "UNIVERSAL TX", action, sigName, pct, i, total, hp::COL_WARN);

            _txSignal(sig);
            delay(120);
        }

        hp::drawExecPopup(Lcd, "UNIVERSAL TX", action, "Batch complete", 1.0f, total, total, hp::COL_FG);
        _device->led.setColor(WS2812B_Class::GREEN);
        delay(260);
        _device->led.off();
        _sceneDirty = true;
    }

    /* ════════════════════════════════════════════════════════════
     *  TV-B-Gone — iterate every signal in the loaded tv.ir
     *  State machine:
     *    Idle    : A=Start, B=Back, ^v navigate signal
     *    Running : A=Pause, B=Stop (→Idle)
     *    Paused  : A=Send 1 (current signal), B=Resume (→Running),
     *              ^v navigate signal
     * ════════════════════════════════════════════════════════════ */

    void App09::_drawTVBGoneFooter()
    {
        switch (_tvbgState) {
            case TvbgState::Idle:
                _drawFooter3("[^v]Sel", "[A]Start", "[B]Back");
                break;
            case TvbgState::Running:
                _drawFooter3(nullptr, "[A]Pause", "[B]Stop");
                break;
            case TvbgState::Paused:
                _drawFooter3("[^v]Sel", "[A]Send 1", "[B]Resume");
                break;
        }
    }

    void App09::_drawTVBGoneFrame()
    {
        auto& Lcd = _device->Lcd;
        const int bw = 280, bh = 160;
        const int bx = (hp::W - bw) / 2, by = (hp::H - bh) / 2;
        uint16_t bd;
        const char* title;
        switch (_tvbgState) {
            case TvbgState::Idle:    bd = hp::COL_DIM;  title = "[ READY ]";       break;
            case TvbgState::Paused:  bd = hp::COL_WARN; title = "[ PAUSED ]";      break;
            case TvbgState::Running:
            default:                 bd = hp::COL_FG;   title = "[ SENDING... ]";  break;
        }

        Lcd.fillRect(bx, by, bw, bh, hp::COL_BG);
        Lcd.drawRect(bx,     by,     bw,     bh,     bd);
        Lcd.drawRect(bx + 2, by + 2, bw - 4, bh - 4, hp::COL_DIM);

        Lcd.setFont(&fonts::efontCN_16);

        /* Title bar */
        Lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
        int tw = (int)strlen(title) * 8;
        Lcd.setCursor(bx + (bw - tw) / 2, by + 8);
        Lcd.print(title);

        /* Two-line message — button name + category */
        char line1[48];
        snprintf(line1, sizeof(line1), "[ %s ]", _univBlastName[0] ? _univBlastName : "Signal");
        char line2buf[48];
        snprintf(line2buf, sizeof(line2buf), "%s — all brands",
                 _univCatTitle[0] ? _univCatTitle : "Universal");
        const char* line2 = line2buf;
        Lcd.setTextColor(hp::COL_FG, hp::COL_BG);
        int t1 = (int)strlen(line1) * 8;
        int t2 = (int)strlen(line2) * 8;
        Lcd.setCursor(bx + (bw - t1) / 2, by + 28);
        Lcd.print(line1);
        Lcd.setCursor(bx + (bw - t2) / 2, by + 44);
        Lcd.print(line2);

        /* Currently-selected signal name */
        int total = _tvbgTotal > 0 ? _tvbgTotal : 1;
        int cur   = _tvbgIdx; if (cur > total) cur = total;
        int dispIdx = cur < total ? cur : total - 1;
        char sigBuf[40];
        if (_tvbgTotal > 0 && dispIdx >= 0) {
            int rawIdx = _univBlastQ.empty() ? dispIdx : _univBlastQ[dispIdx];
            snprintf(sigBuf, sizeof(sigBuf), "> %s",
                     _currentRemote.signals[rawIdx].name);
        } else {
            snprintf(sigBuf, sizeof(sigBuf), "> (no signals)");
        }
        Lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
        int sw = (int)strlen(sigBuf) * 8;
        if (sw > bw - 16) sw = bw - 16;
        Lcd.setCursor(bx + (bw - sw) / 2, by + 64);
        Lcd.print(sigBuf);

        /* Big percentage */
        int pct = (cur * 100) / total;
        char pctBuf[8]; snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
        Lcd.setTextSize(2);
        Lcd.setTextColor(hp::COL_ACCENT, hp::COL_BG);
        int pw = (int)strlen(pctBuf) * 16;
        Lcd.setCursor(bx + (bw - pw) / 2, by + 84);
        Lcd.print(pctBuf);
        Lcd.setTextSize(1);

        /* "cur/total" */
        char cntBuf[16]; snprintf(cntBuf, sizeof(cntBuf), "%d/%d", cur, total);
        Lcd.setTextColor(hp::COL_DIM, hp::COL_BG);
        int cw = (int)strlen(cntBuf) * 8;
        Lcd.setCursor(bx + (bw - cw) / 2, by + 118);
        Lcd.print(cntBuf);

        /* Progress bar */
        int barX = bx + 14, barY = by + 136, barW = bw - 28;
        Lcd.drawRect(barX, barY, barW, 10, bd);
        int filled = ((barW - 4) * cur) / total;
        if (filled > 0) Lcd.fillRect(barX + 2, barY + 2, filled, 6, bd);
        Lcd.fillRect(barX + 2 + filled, barY + 2, barW - 4 - filled, 6, hp::COL_BG);
    }

    void App09::_enterTVBGone()
    {
        auto& Lcd = _device->Lcd;
        hp::drawChrome(Lcd);
        const char* badge = _univBlastName[0] ? _univBlastName : "TX";
        hp::drawHeader(Lcd, _univCatTitle[0] ? _univCatTitle : "Universal", badge, hp::COL_WARN);

        _tvbgIdx      = 0;
        _tvbgTotal    = _univBlastQ.empty()
                        ? (int)_currentRemote.signals.size()
                        : (int)_univBlastQ.size();
        _tvbgState    = TvbgState::Idle;
        _tvbgPaused   = false;
        _tvbgLastSend = 0;
        _device->led.off();

        _drawTVBGoneFooter();
        _drawTVBGoneFrame();
    }

    void App09::_runTVBGone()
    {
        /* Up/Down navigates current signal (Idle / Paused only) */
        if (_tvbgState != TvbgState::Running && _tvbgTotal > 0) {
            bool nav = false;
            if (_device->button.Up.pressed()   && _tvbgIdx > 0)              { _tvbgIdx--; nav = true; }
            if (_device->button.Down.pressed() && _tvbgIdx < _tvbgTotal - 1) { _tvbgIdx++; nav = true; }
            if (nav) _drawTVBGoneFrame();
        }

        /* B = state-dependent */
        if (_device->button.B.pressed()) {
            switch (_tvbgState) {
                case TvbgState::Idle:
                    _device->led.off();
                    _switchScene(_prevScene);
                    return;
                case TvbgState::Running:
                    /* Stop → back to Idle, keep position */
                    _tvbgState = TvbgState::Idle;
                    _device->led.off();
                    _drawTVBGoneFooter();
                    _drawTVBGoneFrame();
                    return;
                case TvbgState::Paused:
                    /* Resume */
                    _tvbgState = TvbgState::Running;
                    _tvbgPaused = false;
                    _tvbgLastSend = 0;
                    _device->led.setColor(WS2812B_Class::RED);
                    _drawTVBGoneFooter();
                    _drawTVBGoneFrame();
                    return;
            }
        }

        /* A = state-dependent */
        if (_device->button.A.pressed()) {
            switch (_tvbgState) {
                case TvbgState::Idle:
                    if (_tvbgTotal <= 0) return;
                    /* Start: if at end, restart from 0 */
                    if (_tvbgIdx >= _tvbgTotal) _tvbgIdx = 0;
                    _tvbgState = TvbgState::Running;
                    _tvbgLastSend = 0;
                    _device->led.setColor(WS2812B_Class::RED);
                    _drawTVBGoneFooter();
                    _drawTVBGoneFrame();
                    return;
                case TvbgState::Running:
                    _tvbgState = TvbgState::Paused;
                    _tvbgPaused = true;
                    _device->led.setColor(WS2812B_Class::YELLOW);
                    _drawTVBGoneFooter();
                    _drawTVBGoneFrame();
                    return;
                case TvbgState::Paused:
                    /* Send 1 — transmit currently displayed signal once */
                    if (_tvbgIdx >= 0 && _tvbgIdx < _tvbgTotal) {
                        int rawIdx = _univBlastQ.empty() ? _tvbgIdx : _univBlastQ[_tvbgIdx];
                        _txSignal(_currentRemote.signals[rawIdx]);
                        _device->led.setColor(WS2812B_Class::GREEN);
                        delay(80);
                        _device->led.setColor(WS2812B_Class::YELLOW);
                    }
                    return;
            }
        }

        if (_tvbgState != TvbgState::Running) return;

        /* Auto-iterate: send next signal every _tvbgGapMs */
        if (_tvbgIdx >= _tvbgTotal) {
            /* Done — return to Idle, leave idx at end so user can replay */
            _tvbgState = TvbgState::Idle;
            _device->led.setColor(WS2812B_Class::GREEN);
            _drawTVBGoneFooter();
            _drawTVBGoneFrame();
            delay(600);
            _device->led.off();
            return;
        }

        unsigned long now = millis();
        if (now - _tvbgLastSend < _tvbgGapMs) return;
        _tvbgLastSend = now;

        int rawIdx = _univBlastQ.empty() ? _tvbgIdx : _univBlastQ[_tvbgIdx];
        _txSignal(_currentRemote.signals[rawIdx]);
        _tvbgIdx++;
        _drawTVBGoneFrame();
    }

    /* ════════════════════════════════════════════════════════════
     *  Sending overlay (brief)
     * ════════════════════════════════════════════════════════════ */

    void App09::_enterSending()
    {
        _sendStart = millis();
        _drawMsgBox(_sendLabel ? _sendLabel : "Sending...");
    }

    void App09::_runSending()
    {
        if (millis() - _sendStart > 500) {
            _switchScene(_prevScene);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  IR hardware helpers
     * ════════════════════════════════════════════════════════════ */

    void App09::_startRx()
    {
        _stopRx();
        _irRecv = new IRrecv(HAL_PIN_IR_RX, 1024, 50, true);
        _irRecv->enableIRIn();
    }

    void App09::_stopRx()
    {
        if (_irRecv) {
            _irRecv->disableIRIn();
            delete _irRecv;
            _irRecv = nullptr;
        }
    }

    void App09::_txSignal(const IrSignal& sig)
    {
        if (!_irSend) return;

        if (sig.isRaw) {
            if (!sig.rawData.empty()) {
                _irSend->sendRaw(sig.rawData.data(), sig.rawData.size(),
                                 sig.frequency / 1000);
            }
        } else {
            _irSend->send(sig.protocol, sig.value, sig.bits);
        }
    }

    /* ════════════════════════════════════════════════════════════
     *  File I/O — Flipper-compatible .ir format
     *
     *  Format:
     *    Filetype: IR signals file
     *    Version: 1
     *    #
     *    name: Power
     *    type: parsed
     *    protocol: NEC
     *    address: 04 00
     *    command: 08 00
     *    #
     *    name: RawSignal
     *    type: raw
     *    frequency: 38000
     *    duty_cycle: 0.330000
     *    data: 9024 4512 564 564 ...
     * ════════════════════════════════════════════════════════════ */

    static const struct { const char* name; decode_type_t type; } kProtoMap[] = {
        { "NEC",       NEC       },
        { "NECext",    NEC       },
        { "Samsung32", SAMSUNG   },
        { "SAMSUNG",   SAMSUNG   },
        { "RC5",       RC5       },
        { "RC5X",      RC5X      },
        { "RC6",       RC6       },
        { "Sony",      SONY      },
        { "SIRC",      SONY      },
        { "SIRC15",    SONY      },
        { "SIRC20",    SONY      },
        { "Panasonic", PANASONIC },
        { "Kaseikyo",  PANASONIC },
        { "LG",        LG        },
        { "LG32",      LG        },
        { "JVC",       JVC       },
        { "PIONEER",   PIONEER   },
        { "SHARP",     SHARP     },
    };
    static constexpr int kProtoMapSize = sizeof(kProtoMap) / sizeof(kProtoMap[0]);

    static decode_type_t flipperProtoToType(const char* name) {
        for (int i = 0; i < kProtoMapSize; i++) {
            if (strcasecmp(name, kProtoMap[i].name) == 0) return kProtoMap[i].type;
        }
        return UNKNOWN;
    }

    static const char* typeToFlipperProto(decode_type_t t) {
        for (int i = 0; i < kProtoMapSize; i++) {
            if (kProtoMap[i].type == t) return kProtoMap[i].name;
        }
        return "UNKNOWN";
    }

    static uint32_t parseHexBytes(const char* s) {
        uint32_t result = 0;
        int shift = 0;
        const char* p = s;
        while (*p) {
            while (*p == ' ') p++;
            if (!*p) break;
            char* end;
            uint32_t byte = strtoul(p, &end, 16);
            result |= (byte << shift);
            shift += 8;
            p = end;
        }
        return result;
    }

    static void formatHexBytes(uint32_t val, int nbytes, char* out, int outSize) {
        int pos = 0;
        for (int i = 0; i < nbytes && pos < outSize - 3; i++) {
            if (i > 0) out[pos++] = ' ';
            uint8_t b = (val >> (i * 8)) & 0xFF;
            pos += snprintf(out + pos, outSize - pos, "%02X", b);
        }
        out[pos] = '\0';
    }

    static void writeSignalEntry(File& f, const IrSignal& sig)
    {
        f.printf("name: %s\n", sig.name);

        if (sig.isRaw) {
            f.println("type: raw");
            f.printf("frequency: %lu\n", sig.frequency);
            f.println("duty_cycle: 0.330000");
            f.print("data:");
            for (size_t i = 0; i < sig.rawData.size(); i++) {
                f.printf(" %u", sig.rawData[i]);
            }
            f.println();
        } else {
            f.println("type: parsed");
            f.printf("protocol: %s\n", typeToFlipperProto(sig.protocol));

            char addrBuf[32], cmdBuf[32];
            int nbytes = (sig.bits + 7) / 8;
            if (nbytes < 2) nbytes = 2;
            if (nbytes > 4) nbytes = 4;
            formatHexBytes(sig.address, nbytes, addrBuf, sizeof(addrBuf));
            formatHexBytes(sig.command, nbytes, cmdBuf, sizeof(cmdBuf));
            f.printf("address: %s\n", addrBuf);
            f.printf("command: %s\n", cmdBuf);
        }
    }

    bool App09::_loadRemote(const char* path, IrRemote& remote, int maxSignals, const char* filterName)
    {
        File f = SD_MMC.open(path, FILE_READ);
        if (!f) return false;

        remote.signals.clear();
        strncpy(remote.path, path, sizeof(remote.path) - 1);
        remote.path[sizeof(remote.path) - 1] = '\0';

        const char* slash = strrchr(path, '/');
        const char* name = slash ? slash + 1 : path;
        strncpy(remote.filename, name, sizeof(remote.filename) - 1);
        remote.filename[sizeof(remote.filename) - 1] = '\0';
        char* dot = strrchr(remote.filename, '.');
        if (dot) *dot = '\0';

        /* Reset a signal in-place WITHOUT memsetting over the std::vector
         * member.  memset over a vector silently orphans its heap allocation
         * and was the root cause of "second SD load fails" — the heap was
         * being leaked one signal at a time on every reload. */
        auto resetSig = [](IrSignal& s) {
            s.rawData.clear();
            s.name[0]   = '\0';
            s.isRaw     = false;
            s.protocol  = UNKNOWN;
            s.value     = 0;
            s.bits      = 0;
            s.address   = 0;
            s.command   = 0;
            s.frequency = 38000;
        };

        IrSignal sig;
        resetSig(sig);
        bool inSignal = false;
        String line;

        while (f.available()) {
            /* Stop early if signal cap reached (only when no name filter is
             * active — filtered loads are inherently bounded by brand count). */
            if (!filterName && maxSignals > 0 && (int)remote.signals.size() >= maxSignals) break;

            line = f.readStringUntil('\n');
            line.trim();

            if (line.startsWith("name: ")) {
                if (inSignal) {
                    /* When filtering, only keep signals whose name matches. */
                    if (!filterName || strcasecmp(sig.name, filterName) == 0) {
                        remote.signals.push_back(std::move(sig));
                    }
                }
                resetSig(sig);
                strncpy(sig.name, line.c_str() + 6, sizeof(sig.name) - 1);
                sig.name[sizeof(sig.name) - 1] = '\0';
                inSignal = true;
            }
            else if (line.startsWith("type: ")) {
                String t = line.substring(6);
                t.trim();
                sig.isRaw = (t == "raw");
            }
            else if (line.startsWith("protocol: ")) {
                String proto = line.substring(10);
                proto.trim();
                sig.protocol = flipperProtoToType(proto.c_str());
            }
            else if (line.startsWith("address: ")) {
                sig.address = parseHexBytes(line.c_str() + 9);
            }
            else if (line.startsWith("command: ")) {
                sig.command = parseHexBytes(line.c_str() + 9);
                if (!sig.isRaw) {
                    sig.bits = 32;
                    sig.value = ((uint64_t)sig.address) | ((uint64_t)sig.command << 16);
                }
            }
            else if (line.startsWith("frequency: ")) {
                sig.frequency = line.substring(11).toInt();
            }
            else if (line.startsWith("data: ")) {
                const char* p = line.c_str() + 6;
                while (*p) {
                    while (*p == ' ') p++;
                    if (!*p) break;
                    char* end;
                    long val = strtol(p, &end, 10);
                    if (end == p) break;
                    sig.rawData.push_back((uint16_t)val);
                    p = end;
                }
            }
        }

        if (inSignal) {
            if (!filterName || strcasecmp(sig.name, filterName) == 0) {
                remote.signals.push_back(std::move(sig));
            }
        }

        f.close();
        return true;
    }

    bool App09::_saveSignalToFile(const char* dir, const char* remoteName, const IrSignal& sig)
    {
        char path[128];
        snprintf(path, sizeof(path), "%s/%s.ir", dir, remoteName);

        File f = SD_MMC.open(path, FILE_WRITE);
        if (!f) return false;

        f.println("Filetype: MeowKit IR signals file");
        f.println("Version: 1");
        f.println("#");
        writeSignalEntry(f, sig);
        f.close();
        return true;
    }

    bool App09::_appendSignalToFile(const char* path, const IrSignal& sig)
    {
        File f = SD_MMC.open(path, FILE_APPEND);
        if (!f) return false;

        f.println("#");
        writeSignalEntry(f, sig);
        f.close();
        return true;
    }

    void App09::_listIrFiles(const char* dir, std::vector<String>& out)
    {
        File root = SD_MMC.open(dir);
        if (!root || !root.isDirectory()) return;

        File file = root.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                String name = file.name();
                if (name.endsWith(".ir") || name.endsWith(".IR")) {
                    const char* slash = strrchr(file.name(), '/');
                    out.push_back(slash ? String(slash + 1) : name);
                }
            }
            file.close();
            file = root.openNextFile();
        }
        root.close();
    }

}  /* namespace MOONCAKE::APPS */
