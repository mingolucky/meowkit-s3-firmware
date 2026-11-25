#pragma once
#include <mooncake.h>
#include "../../bsp/devices.h"
#include "air_mouse_motion.h"
#include "air_mouse_hid.h"

namespace MOONCAKE::APPS {
class App06 : public mooncake::AppAbility {
public:
    explicit App06(DEVICES* device);
    void onOpen() override;
    void onRunning() override;
    void onClose() override;
private:
    DEVICES* _device;
    air_mouse::Motion _motion;
    air_mouse::Calibration _cal;
    LGFX_Sprite _globe;
    float _bias[3] = {}, _ax = 0, _ay = 0;
    uint8_t _previous_range = 255, _speed = 1;
    bool _imu_ready = false, _calibrating = false, _calibrated = false;
    bool _a_down = false, _b_down = false, _a_armed = false, _b_armed = false;
    bool _touch_down = false, _scroll_active = false, _link_ready = false;
    bool _ui_dirty = true, _ui_scroll = false;
    int _touch_y = -1, _wheel = 0, _last_direction = 0;
    int _bubble_x = -1000, _bubble_y = -1000;
    uint32_t _sample_us = 0, _poll_us = 0, _last_ui = 0, _ignore_until = 0;
    uint32_t _cal_start = 0, _still_since = 0, _last_gyro = 0, _wheel_at = 0;
    uint32_t _b_since = 0;
    const char* _last_status = nullptr;
    void _drawStaticUI();
    void _calibrate(uint32_t now);
    void _sample(uint32_t now);
    void _inputs(uint32_t now);
    void _updateUI();
    void _handleTouch(uint32_t now);
    void _setSpeed(uint8_t speed);
};
}
