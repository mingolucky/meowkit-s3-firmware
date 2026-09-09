/**
 * @file app_03.cpp
 * @brief App03 — Classic Analog VU Meter
 *
 * Render pipeline:
 *   1. memcpy bgPatch → workSpr   (fresh BG stamp each frame)
 *   2. needle → workSpr           (chroma-keyed rotate/zoom)
 *   3. workSpr → LCD              (single push, no ghost trails)
 */
#include "vu_meter.h"
#include <SD_MMC.h>
#include "vu_face.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace MOONCAKE::APPS
{

/* ══════════════════════════════════════════════════════════════════
 *  Constants
 * ══════════════════════════════════════════════════════════════════ */

static constexpr char  BG_PATH[] = "/vu_meter/vu_meter_bg.png";

/* ── Angle calibration ─────────────────────────────────────────────
 *
 * La plage balayée n'est plus une constante : elle dépend du fond affiché.
 * Voir _angleMin / _angleMax dans vu_meter.h — l'illustration de la carte SD
 * et la face dessinée n'ont pas la même graduation. */

/* ── Needle sprite geometry ─────────────────────────────────────── */
static constexpr int NEEDLE_W     = 12;
static constexpr int NEEDLE_THICK =  4;
static constexpr int NEEDLE_R     =  2;
static constexpr int NEEDLE_TIP_Y =  2;

static constexpr uint16_t CHROMA16 = TFT_MAGENTA;

/* ── DSP parameters ─────────────────────────────────────────────── */
static constexpr float    kGain        = 4.0f;
static constexpr float    kDbFloor     = -36.0f;
static constexpr float    kDbCeil      =  -6.0f;
static constexpr float    kEmaAttack   = 0.35f;
static constexpr float    kEmaRelease  = 0.08f;
static constexpr uint32_t kSampleRate  = 16000;
static constexpr size_t   kDmaBufSamp  = 256;
static constexpr int      kDmaBufCount = 6;

static constexpr uint32_t kFrameMs     = 33;
static constexpr uint32_t kHeartbeatMs = 500;
static constexpr float    kAngleThr    = 0.3f;

/* ══════════════════════════════════════════════════════════════════
 *  Constructor
 * ══════════════════════════════════════════════════════════════════ */
App03::App03(DEVICES* device) : _device(device)
{
    setAppInfo().name = "VU Meter";
}

/* ══════════════════════════════════════════════════════════════════
 *  Static helpers
 * ══════════════════════════════════════════════════════════════════ */

uint8_t* App03::_readSD(const char* path, size_t& outLen)
{
    outLen = 0;
    File f = SD_MMC.open(path, "r");
    if (!f) return nullptr;
    size_t sz = (size_t)f.size();
    if (!sz) { f.close(); return nullptr; }
    uint8_t* buf = (uint8_t*)ps_malloc(sz);
    if (!buf) { f.close(); return nullptr; }
    if (f.read(buf, sz) != sz) { free(buf); f.close(); return nullptr; }
    f.close();
    outLen = sz;
    return buf;
}

float App03::_calcAmplitude(const int16_t* buf, size_t mono)
{
    if (!mono) return kDbFloor;
    double sum = 0.0;
    for (size_t i = 0; i < mono; ++i) {
        double s = (double)buf[i] * kGain;
        sum += s * s;
    }
    float rms = (float)sqrt(sum / (double)mono);
    if (rms < 1.0f) return kDbFloor;
    return 20.0f * log10f(rms / 32767.0f);
}

/* ══════════════════════════════════════════════════════════════════
 *  onOpen — returns immediately, no visual change
 *  Previous screen stays visible until BG PNG is pushed in _doHeavyInit
 * ══════════════════════════════════════════════════════════════════ */
void App03::onOpen()
{
    // If a previous crash left the I2S driver installed, uninstall it now.
    // esp_err_t is ignored — the call is a no-op when the driver is absent.
    i2s_driver_uninstall(I2S_NUM_1);

    _initState        = InitState::Loading;
    _taskRun          = false;
    _captureTask      = nullptr;
    _latestAngle.store(_angleMin);
    _lastFrameMs      = 0;
    _lastRenderAngle  = -999.0f;
    _pivotX           = kPivotX;
    _pivotY           = kPivotY;
}

/* ══════════════════════════════════════════════════════════════════
 *  _doHeavyInit
 * ══════════════════════════════════════════════════════════════════ */
void App03::_doHeavyInit()
{
    auto fail = [&](const char* msg) {
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.setTextColor(TFT_RED, TFT_BLACK);
        _device->Lcd.setTextFont(2);
        _device->Lcd.drawCentreString(msg, 160, 110);
        _initState = InitState::Failed;
    };

    auto delSp = [](LGFX_Sprite*& sp) {
        if (sp) { sp->deleteSprite(); delete sp; sp = nullptr; }
    };

    /* 1. ES7210 codec */
    if (!_codec.begin(kSampleRate, ES7210_BIT_16, ES7210_FMT_I2S)) {
        fail("ES7210 init failed"); return;
    }
    _codec.selectMic(ES7210_MIC1 | ES7210_MIC2);
    _codec.setGain(ES7210_GAIN_30DB);
    _codec.start();

    /* 2. I2S_NUM_1 RX */
    {
        i2s_config_t cfg = {};
        cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
        cfg.sample_rate          = kSampleRate;
        cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
        cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
        cfg.dma_buf_count        = kDmaBufCount;
        cfg.dma_buf_len          = (int)kDmaBufSamp;
        cfg.use_apll             = true;
        cfg.tx_desc_auto_clear   = false;
        cfg.fixed_mclk           = 0;
        if (i2s_driver_install(I2S_NUM_1, &cfg, 0, nullptr) != ESP_OK) {
            _codec.stop(); _codec.end();
            fail("I2S install failed"); return;
        }
        i2s_pin_config_t pins = {};
        pins.mck_io_num   = HAL_PIN_I2S_MCLK;
        pins.bck_io_num   = HAL_PIN_I2S_BCLK;
        pins.ws_io_num    = HAL_PIN_I2S_WS;
        pins.data_out_num = I2S_PIN_NO_CHANGE;
        pins.data_in_num  = HAL_PIN_I2S_DIN;
        if (i2s_set_pin(I2S_NUM_1, &pins) != ESP_OK) {
            _codec.stop(); _codec.end(); i2s_driver_uninstall(I2S_NUM_1);
            fail("I2S pin cfg failed"); return;
        }
        i2s_start(I2S_NUM_1);
    }

    /* 3. bgPatch — full BG reference covering y=kPatchY..240
     *    kPatchH=160 ensures mask seeding always finds valid BG rows */
    _bgPatch = new LGFX_Sprite(&_device->Lcd);
    _bgPatch->setPsram(true);
    _bgPatch->setColorDepth(16);
    if (!_bgPatch->createSprite(kPatchW, kPatchH)) {
        _codec.stop(); _codec.end(); i2s_driver_uninstall(I2S_NUM_1);
        fail("PSRAM alloc failed (bgPatch)"); return;
    }
    _bgPatch->fillSprite(TFT_BLACK);

    /* 4. workSpr — per-frame compositing buffer (above-mask zone) */
    _workSpr = new LGFX_Sprite(&_device->Lcd);
    _workSpr->setPsram(true);
    _workSpr->setColorDepth(16);
    if (!_workSpr->createSprite(kPatchW, kWorkH)) {
        delSp(_bgPatch);
        _codec.stop(); _codec.end(); i2s_driver_uninstall(I2S_NUM_1);
        fail("PSRAM alloc failed (workSpr)"); return;
    }

    /* 5. Background PNG: push to LCD immediately (user sees BG, no black screen),
     *    then decode crop into bgPatch for per-frame erasure */
    {
        size_t bgLen = 0;
        uint8_t* bgBuf = _readSD(BG_PATH, bgLen);
        if (bgBuf) {
            _device->Lcd.drawPng(bgBuf, bgLen, 0, 0);           // visible immediately
            _bgPatch->drawPng(bgBuf, bgLen, -kPatchX, -kPatchY); // decode into reference
            free(bgBuf);
        } else {
            /* Pas de carte, ou carte sans les ressources : on dessine la face
             * plutôt que d'échouer. L'app reste utilisable sur un appareil neuf,
             * et l'aiguille adopte le balayage que cette face gradue. */
            _angleMin = VU_FACE_ANGLE_MIN;
            _angleMax = VU_FACE_ANGLE_MAX;
            _latestAngle.store(_angleMin);
            vu_face_draw(&_device->Lcd, 0, 0);
            vu_face_draw(_bgPatch, -kPatchX, -kPatchY);
        }
    }

    /* 6. Needle sprite — pivot row = NEEDLE_TIP_Y + kNeedleLen */
    {
        const int pivY = NEEDLE_TIP_Y + kNeedleLen;
        const int sprH = pivY + 5;
        LGFX_Sprite* ns = new LGFX_Sprite(&_device->Lcd);
        ns->setPsram(true);
        ns->setColorDepth(16);
        if (ns->createSprite(NEEDLE_W, sprH)) {
            const int cx = NEEDLE_W / 2;
            ns->fillSprite(CHROMA16);
            /* Shadow: dark grey, 1 px right + 1 px down */
            ns->fillRoundRect(cx - NEEDLE_THICK/2 + 1, NEEDLE_TIP_Y + 1,
                              NEEDLE_THICK, kNeedleLen - 2, NEEDLE_R, 0x2104u);
            /* Body: white */
            ns->fillRoundRect(cx - NEEDLE_THICK/2, NEEDLE_TIP_Y,
                              NEEDLE_THICK, kNeedleLen - 2, NEEDLE_R, TFT_WHITE);
            ns->setPivot((float)cx, (float)pivY);
            _needleSprite = ns;
        } else {
            delete ns;
        }
    }

    /* 7. Start DSP capture task on core 0 */
    _taskRun = true;
    xTaskCreatePinnedToCore(_captureTaskEntry, "vu_cap",
                            4096, this, 2, &_captureTask, 0);

    _initState = InitState::Ready;
}

/* ══════════════════════════════════════════════════════════════════
 *  onClose
 * ══════════════════════════════════════════════════════════════════ */
void App03::onClose()
{
    _taskRun = false;
    if (_captureTask) {
        for (int i = 0; i < 25 && eTaskGetState(_captureTask) != eDeleted; ++i)
            vTaskDelay(pdMS_TO_TICKS(10));
        if (eTaskGetState(_captureTask) != eDeleted)
            vTaskDelete(_captureTask);
        _captureTask = nullptr;
    }

    if (_initState == InitState::Ready) {
        _codec.stop();
        _codec.end();
        i2s_stop(I2S_NUM_1);
        i2s_driver_uninstall(I2S_NUM_1);
    }

    auto del = [](LGFX_Sprite*& sp) {
        if (sp) { sp->deleteSprite(); delete sp; sp = nullptr; }
    };
    del(_bgPatch);
    del(_workSpr);
    del(_needleSprite);

    _initState = InitState::Idle;
    _device->Lcd.fillScreen(TFT_BLACK);
}

/* ══════════════════════════════════════════════════════════════════
 *  onRunning — ~30 fps
 * ══════════════════════════════════════════════════════════════════ */
void App03::onRunning()
{
    if (_initState == InitState::Loading) {
        _doHeavyInit();
        return;
    }
    if (_initState == InitState::Failed || !_bgPatch || !_workSpr) return;

    float    ang = _latestAngle.load();
    uint32_t now = millis();

    bool angleJump = fabsf(ang - _lastRenderAngle) >= kAngleThr;
    bool heartbeat = (now - _lastFrameMs) >= kHeartbeatMs;
    bool frameDue  = (now - _lastFrameMs) >= kFrameMs;

    if (!angleJump && !heartbeat) return;
    if (!frameDue)                return;

    _lastFrameMs     = now;
    _lastRenderAngle = ang;

    /* 1. Stamp fresh BG into work sprite (memcpy ~kPatchW×kWorkH×2 bytes) */
    memcpy(_workSpr->getBuffer(),
           _bgPatch->getBuffer(),
           (size_t)kPatchW * kWorkH * sizeof(uint16_t));

    /* 2. Composite needle onto work sprite.
     *    Pivot position in workSpr coords = (pivotX-kPatchX, pivotY-kPatchY).
     *    Rows >= kWorkH are outside workSpr and silently clipped — those rows
     *    are in the mask zone and handled by maskSprite below. */
    if (_needleSprite)
        _needleSprite->pushRotateZoom(_workSpr,
                                      _pivotX - (float)kPatchX,
                                      _pivotY - (float)kPatchY,
                                      ang, 1.0f, 1.0f, kChroma24);

    /* 3. Push composited frame to LCD */
    _workSpr->pushSprite(&_device->Lcd, kPatchX, kPatchY);
}

/* ══════════════════════════════════════════════════════════════════
 *  DSP capture task — core 0
 * ══════════════════════════════════════════════════════════════════ */
void App03::_captureTaskEntry(void* arg)
{
    static_cast<App03*>(arg)->_captureLoop();
    vTaskDelete(nullptr);
}

void App03::_captureLoop()
{
    constexpr size_t kStereoSamp = kDmaBufSamp * 2;
    constexpr size_t kBufBytes   = kStereoSamp * sizeof(int16_t);

    int16_t* buf = (int16_t*)heap_caps_malloc(kBufBytes, MALLOC_CAP_INTERNAL);
    if (!buf) { _taskRun = false; return; }

    float   smooth = _angleMin;
    int32_t dcAcc  = 0;

    while (_taskRun) {
        size_t bytesRead = 0;
        if (i2s_read(I2S_NUM_1, buf, kBufBytes, &bytesRead, pdMS_TO_TICKS(60))
                != ESP_OK || bytesRead < 4)
            continue;

        size_t mono = bytesRead / (2 * sizeof(int16_t));
        for (size_t i = 0; i < mono; ++i) {
            int32_t s = buf[i * 2];
            dcAcc = dcAcc - (dcAcc >> 8) + s;
            buf[i] = (int16_t)(s - (dcAcc >> 8));
        }

        float db     = _calcAmplitude(buf, mono);
        db           = std::max(kDbFloor, std::min(kDbCeil, db));
        float norm   = (db - kDbFloor) / (kDbCeil - kDbFloor);
        float target = _angleMin + norm * (_angleMax - _angleMin);

        float alpha  = (target > smooth) ? kEmaAttack : kEmaRelease;
        smooth      += alpha * (target - smooth);
        _latestAngle.store(smooth);
    }

    free(buf);
}

} // namespace MOONCAKE::APPS
