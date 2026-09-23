/**
 * @file MeowKit.cpp
 * @brief MeowKit application entry — manages device lifecycle
 */
#include "MeowKit.h"
#include "splash/splash_screen.h"
#include "system/settings_bridge.h"
#include "system/system_sound.h"
#include "system/system_sound_assets.h"

bool MeowKit::Setup()
{
    _device = std::make_unique<DEVICES>();
    if (!_device) {
        printf("[MeowKit] BSP create failed\n");
        return false;
    }

    _device->init();
    settings_init();
    sys_settings_bridge_attach(_device.get());
    system_sound_init(_device.get());
    settings_load_all();
    system_sound_play_boot(boot_animation_duration_ms);
    SplashScreen::show(_device->Lcd);
    system_sound_stop();

    _launcher = std::make_unique<Launcher>(_device.get());
    _launcher->onCreate();

    return true;
}

void MeowKit::Loop()
{
    if (_launcher) {
        _launcher->onLoop();
    }
}
