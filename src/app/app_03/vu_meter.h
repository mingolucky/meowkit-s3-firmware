/**
 * @file app_03.h
 * @brief App03 — Classic Analog VU Meter
 *
 * Render pipeline:
 *   1. memcpy bgPatch → workSpr   (fresh BG each frame)
 *   2. needle → workSpr           (chroma-keyed rotate/zoom)
 *   3. workSpr → LCD              (single push, no ghost trails)
 *
 * PSRAM budget:
 *   _bgPatch      220×180×2 ≈  79 KB
 *   _workSpr      220×180×2 ≈  79 KB
 *   _needleSprite  12×(kNeedleLen+8)×2 ≈  3 KB
 */
#pragma once
#include <mooncake.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2s.h"
#include "../../bsp/devices.h"
#include "../../bsp/audio/ES7210_Class.hpp"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    class App03 : public AppAbility {
    public:
        App03(DEVICES* device);
        void onOpen()    override;
        void onRunning() override;
        void onClose()   override;

    private:
        DEVICES* _device = nullptr;

        /* ── Deferred-init state machine ── */
        enum class InitState : uint8_t { Idle, Loading, Ready, Failed };
        InitState _initState = InitState::Idle;
        void _doHeavyInit();

        /* ── Audio capture (FreeRTOS task, core 0, priority 2) ── */
        ES7210_Class       _codec;
        TaskHandle_t       _captureTask = nullptr;
        volatile bool      _taskRun     = false;
        std::atomic<float> _latestAngle {0.0f};

        /* ── Render state ── */
        uint32_t _lastFrameMs     = 0;
        float    _lastRenderAngle = -999.0f;

        LGFX_Sprite* _bgPatch      = nullptr;
        LGFX_Sprite* _workSpr      = nullptr;
        LGFX_Sprite* _needleSprite = nullptr;

        /* ── Tuning ─────────────────────────────────────────────────── */
        static constexpr float ANGLE_MIN_DEFAULT = -45.0f;  // fond d'origine
        static constexpr float ANGLE_MAX_DEFAULT =  35.0f;

        static constexpr int32_t kNeedleLen = 150;   // tip-to-pivot (px)

        static constexpr int32_t kPatchX = 50;
        static constexpr int32_t kPatchY = 60;
        static constexpr int32_t kPatchW = 220;
        static constexpr int32_t kPatchH = 180;
        static constexpr int32_t kWorkH  = 180;

        /* Chroma key for needle sprite (magenta) */
        static constexpr uint32_t kChroma24 = 0xFF00FFu;

        /* Needle rotation pivot in screen coordinates — tune to match the BG image */
        static constexpr float kPivotX = 160.0f;
        static constexpr float kPivotY = 239.0f;

        float _pivotX = kPivotX;
        float _pivotY = kPivotY;

        /* Plage réellement balayée. Elle dépend du fond affiché : l'illustration
         * de la carte SD est graduée pour -45°/+35°, la face dessinée pour un
         * balayage symétrique. L'aiguille doit suivre, sinon elle ne pointerait
         * pas ses propres graduations. */
        float _angleMin = ANGLE_MIN_DEFAULT;
        float _angleMax = ANGLE_MAX_DEFAULT;

        /* ── DSP task ── */
        static void _captureTaskEntry(void* arg);
        void        _captureLoop();

        /* ── Helpers ── */
        static float    _calcAmplitude(const int16_t* buf, size_t mono);
        static uint8_t* _readSD(const char* path, size_t& outLen);
    };
}
