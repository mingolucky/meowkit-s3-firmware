/**
 * @file ui_sd_bridge.h
 * @brief C ABI bridge so the SquareLine-generated C UI screens can talk
 *        to the Arduino C++ SD_MMC driver.
 */
#ifndef UI_SD_BRIDGE_H
#define UI_SD_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Called once per directory entry while ui_sd_list_dir() walks it.
 *  name   - basename of the entry (no path), valid only during the call.
 *  is_dir - 1 if directory, 0 if regular file.
 *  user   - opaque pointer passed to ui_sd_list_dir().
 */
typedef void (*ui_sd_visit_cb)(const char* name, int is_dir, void* user);

/* Active SD probe — opens "/" and verifies it is a directory.
 * Returns 1 if accessible, 0 otherwise. Safe to call ~1 Hz. */
int ui_sd_present(void);
/* Allows explicit USB raw-card recovery when filesystem writes are gated. */
int ui_sd_recovery_blocked(void);

/* Iterate a directory on the SD card.
 *  path : absolute path starting with "/", e.g. "/", "/config".
 * Returns 0 on success, -1 on error (open failed / not a directory). */
int ui_sd_list_dir(const char* path, ui_sd_visit_cb cb, void* user);

/* Storage usage — both return 0 if the card is not mounted. */
uint64_t ui_sd_total_bytes(void);
uint64_t ui_sd_used_bytes(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UI_SD_BRIDGE_H */
