/**
 * @file ui_sd_bridge.cpp
 * @brief SD_MMC ↔ C UI bridge implementation.
 */
#include "ui_sd_bridge.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <string.h>
#include "../bsp/sdmmc/SDMMC_Class.hpp"
#include "../system/usb_msc.h"
#include "../system/sd_partition_mount.h"

extern "C" int ui_sd_recovery_blocked(void) { return sdPartitionBlocked()?1:0; }

extern "C" int ui_sd_present(void)
{
    if (usb_msc_is_active()) return 1;
    return sdmmcEnsureReady()?1:0;
}

extern "C" uint64_t ui_sd_total_bytes(void)
{
    if (usb_msc_is_active()) return 0;
    return SD_MMC.totalBytes();
}

extern "C" uint64_t ui_sd_used_bytes(void)
{
    if (usb_msc_is_active()) return 0;
    return SD_MMC.usedBytes();
}

extern "C" int ui_sd_list_dir(const char* path, ui_sd_visit_cb cb, void* user)
{
    if (!path || !cb || usb_msc_is_active()) return -1;

    File dir = SD_MMC.open(path);
    if (!dir) return -1;
    if (!dir.isDirectory()) { dir.close(); return -1; }

    File f = dir.openNextFile();
    while (f) {
        const char* full = f.name();
        const char* slash = strrchr(full, '/');
        const char* base = slash ? slash + 1 : full;
        cb(base, f.isDirectory() ? 1 : 0, user);

        File next = dir.openNextFile();
        f.close();
        f = next;
    }
    dir.close();
    return 0;
}
