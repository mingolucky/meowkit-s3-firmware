/**
 * @file app.h
 * @brief Native app registry — install apps + icon table (co-located so they stay in sync).
 *
 * ADDING AN APP:
 *   1. Add installApp() call in registerAllApps() below.
 *   2. Add the matching icon pointer in APP_BUILTIN_ICONS[] immediately below.
 *   Both tables are in the same file — impossible to add one without seeing the other.
 */
#pragma once

/* ── BSP ── */
#include "../bsp/devices.h"

/* ── LVGL icon declarations ── */
#include "../ui/ui.h"

/* ── Active native app headers (app_01 ~ app_09) ── */
#include "app_01/dino.h"        /* Dino         */
#include "app_02/matrix_rain.h" /* Matrix Rain  */
#include "app_03/vu_meter.h"    /* VU Meter     */
#include "app_04/retro_tv.h"    /* Retro TV     */
#include "app_05/pc_monitor.h"  /* PC Monitor   */
#include "app_06/air_mouse.h"   /* Air Mouse    */
#include "app_07/ble_spam.h"    /* BLE Spam     */
#include "app_08/badusb.h"      /* Bad USB      */
#include "app_09/infrared.h"    /* Infrared     */

#include <mooncake.h>
#include <memory>

/**
 * @brief Register active apps into Mooncake (order determines menu slot index).
 *        Keep in sync with APP_BUILTIN_ICONS below.
 */
inline void registerAllApps(mooncake::Mooncake& mc, DEVICES* dev)
{
    /* Menu visual order: left → right, top → bottom (app_01 … app_09) */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App01>(dev));     /* app_01  Dino         */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App02>(dev));     /* app_02  Matrix Rain  */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App03>(dev));     /* app_03  VU Meter     */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App04>(dev));     /* app_04  Retro TV     */
    mc.installApp(std::make_unique<MOONCAKE::APPS::PCMonitor>(dev)); /* app_05  PC Monitor   */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App06>(dev));     /* app_06  Air Mouse    */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App07>(dev));     /* app_07  BLE Spam     */
    mc.installApp(std::make_unique<MOONCAKE::APPS::AppBadUSB>(dev)); /* app_08  Bad USB      */
    mc.installApp(std::make_unique<MOONCAKE::APPS::App09>(dev));     /* app_09  Infrared     */
}

/**
 * @brief App icons — one entry per installApp() call above, same order.
 *        Launcher reads this array by slot index to populate the apps menu grid.
 *
 * Rule: icon[N] must correspond to the Nth installApp() call in registerAllApps().
 */
static const void* const APP_BUILTIN_ICONS[] = {
    &ui_img_dino_png,        /* app_01  Dino        */
    &ui_img_matrix_rain_png, /* app_02  Matrix Rain */
    &ui_img_vu_meter_png,    /* app_03  VU Meter    */
    &ui_img_retro_tv_png,    /* app_04  Retro TV    */
    &ui_img_pc_montior_png,  /* app_05  PC Monitor  */
    &ui_img_air_mouse_png,   /* app_06  Air Mouse   */
    &ui_img_ble_spam_png,    /* app_07  BLE Spam    */
    &ui_img_badusb_png,      /* app_08  Bad USB     */
    &ui_img_infrared_png,    /* app_09  Infrared    */
};
static const int APP_BUILTIN_ICONS_COUNT =
    (int)(sizeof(APP_BUILTIN_ICONS) / sizeof(APP_BUILTIN_ICONS[0]));
