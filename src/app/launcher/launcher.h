/**
 * @file launcher.h
 * @brief MeowKit Launcher — persistent UI, two-state exclusive switching
 *
 * Architecture (v2 — no destroy/rebuild):
 *   LVGL screens (home + apps_menu) are created ONCE at boot and kept alive.
 *   Entering an app pauses LVGL rendering; exiting invalidates and resumes.
 *
 * State GUI:  LVGL fully active (home / apps_menu screens)
 * State APP:  LVGL paused, mooncake drives the app
 *             - Legacy apps: draw directly via LovyanGFX (LVGL idle)
 *             - LvAppUI apps: create their own LVGL screen + call lv_timer_handler()
 *
 * Enter app  → pause LVGL, open app via mooncake
 * Exit app   → close app, invalidate + reload LVGL screen (no rebuild)
 */
#pragma once

#include <mooncake.h>
#include "../../bsp/devices.h"
#include "../../system/mk_events.h"
#include "../../ui/ui.h"
#include <string>

class Launcher
{
public:
    explicit Launcher(DEVICES* device);
    ~Launcher() = default;

    void onCreate();
    void onLoop();

private:
    DEVICES* _device;
    mooncake::Mooncake _mooncake;

    /* state */
    bool _sd_ready       = false;
    bool _lvgl_inited    = false;   /* lv_init + drivers done (one-time) */
    bool _ui_ready       = false;   /* UI screens exist (stays true after buildUI) */
    bool _app_running    = false;
    int  _running_app_id = -1;

    /* SD card (mounted via SD_MMC native API) */

    /* sub-routines */
    void initSD();
    void initLVGL();        /* one-time: lv_init + display + touch + FS */
    void buildUI();         /* create screens + populate menu (called ONCE) */
    void installApps();
    void loadAppsMenu();
    void handleAppSelection();
    void handlePhysicalNav();    /* detect button edges → drive navigation */
    void processNavEvents();     /* consume one external navigation event */
    void handleNavigationEvent(mk_event_t evt);
    void updateStatusBar();
    void returnToUI();      /* invalidate + reload LVGL screen after app exit */
};
