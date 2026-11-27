/**
 * @file SDMMC_Class.cpp
 * @brief SDMMC implementation with multi-frequency auto-negotiation.
 *
 * Frequency ladder (1-bit): 10 MHz, 5 MHz, then 1 MHz.
 * Arduino's begin() accepts kHz. Healthy mounts remain open; stale mounts
 * are released before retrying, and USB MSC keeps exclusive raw ownership.
 */
#include "SDMMC_Class.hpp"
#include "../config.h"
#include <Arduino.h>
#include "../../system/usb_msc.h"
#include "../../system/sd_partition_mount.h"
#ifdef ARDUINO
#include "../../system/sd_clock.h"
#include "driver/sdmmc_host.h"
#include "esp_heap_caps.h"
#include "ff.h"
#include "diskio.h"
#endif

/* Frequency ladder — descending, most compatible last */
static constexpr uint32_t kFreqTable[] = { 10000, 5000, 1000 };
static constexpr int       kFreqCount  = (int)(sizeof(kFreqTable) / sizeof(kFreqTable[0]));

/* ── Mount / Unmount ──────────────────────────────────────────── */

bool SDMMC_Class::begin(bool mode4bit)
{
    if (usb_msc_is_active()) return false; // PC owns raw SD sectors exclusively
#ifdef ARDUINO
    if (sdPartitionBlocked()) return false;
#endif
    // cardType() is cached by Arduino, so probe the existing mount before
    // trusting it. Adopt healthy mounts without interrupting open app files.
    if (SD_MMC.cardType()!=CARD_NONE) {
        File root=SD_MMC.open("/");
        const bool readable=root && root.isDirectory();
        root.close();
        if (readable) {
            _mounted=true; _type=_mapCardType(SD_MMC.cardType()); return true;
        }
        SD_MMC.end();
    }
    _mounted=false; _type=CardType::NONE; _freq_khz=0;
    if (!SD_MMC.setPins(HAL_PIN_SD_CLK, HAL_PIN_SD_CMD, HAL_PIN_SD_D0)) return false;

    for (int i = 0; i < kFreqCount; i++) {
        if (_tryMount(kFreqTable[i], mode4bit)) {
            Serial.printf("[SDMMC] Mounted at %u kHz (%s, %s)\n",
                          _freq_khz,
                          cardTypeStr(),
                          mode4bit ? "4-bit" : "1-bit");
            return true;
        }
#ifdef ARDUINO
        if (sdPartitionBlocked()) return false;
#endif
        /* Brief settle before next attempt */
        delay(20);
    }

    Serial.println("[SDMMC] Mount failed at all frequencies");
    return false;
}

void SDMMC_Class::end()
{
    SD_MMC.end();
    _mounted  = false;
    _type     = CardType::NONE;
    _freq_khz = 0;
}

/* ── Capacity ─────────────────────────────────────────────────── */

uint64_t SDMMC_Class::cardSize()   const { return _mounted ? SD_MMC.cardSize()   : 0; }
uint64_t SDMMC_Class::totalBytes() const { return _mounted ? SD_MMC.totalBytes() : 0; }
uint64_t SDMMC_Class::usedBytes()  const { return _mounted ? SD_MMC.usedBytes()  : 0; }
uint64_t SDMMC_Class::freeBytes()  const { return _mounted ? (totalBytes() - usedBytes()) : 0; }

/* ── Active hot-plug probe ────────────────────────────────────── */

bool SDMMC_Class::isPresent()
{
    if (usb_msc_is_active()) return false;
#ifdef ARDUINO
    if (sdPartitionBlocked()) return false;
#endif
    if (SD_MMC.cardType()==CARD_NONE) { _mounted=false; return false; }
    File f = SD_MMC.open("/");
    bool ok = f && f.isDirectory();
    if (f) f.close();
    if (!ok) {
        /* Card was removed; update internal state */
        end();
    } else {
        _mounted=true; _type=_mapCardType(SD_MMC.cardType());
    }
    return ok;
}

/* ── String helpers ───────────────────────────────────────────── */

const char* SDMMC_Class::cardTypeStr() const
{
    switch (_type) {
        case CardType::MMC:  return "MMC";
        case CardType::SD:   return "SD";
        case CardType::SDHC: return "SDHC";
        case CardType::SDXC: return "SDXC";
        default:             return "NONE";
    }
}

/* ── Private ──────────────────────────────────────────────────── */

bool SDMMC_Class::_tryMount(uint32_t freq_khz, bool mode4bit)
{
    /* Arduino passes this value directly to host.max_freq_khz. Never format. */
#ifdef ARDUINO
    // Force the first MBR partition before FatFs can mistake an old sector-0
    // BPB for the active volume. Auto remains the superfloppy fallback.
    sdPartitionSelectInitial(true);
#endif
    bool ok = SD_MMC.begin("/sdcard", !mode4bit, false, freq_khz);
#ifdef ARDUINO
    if (!ok) {
        sdPartitionSelectInitial(false);
        ok=SD_MMC.begin("/sdcard", !mode4bit, false, freq_khz);
    }
    if (ok) {
        auto* dma=static_cast<uint8_t*>(heap_caps_malloc(512,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA));
        uint8_t baseline[512];
        bool verified=dma && disk_read(VolToPart[0].pd,dma,0,1)==RES_OK;
        if(verified) {
            memcpy(baseline,dma,sizeof(baseline));
            verified=meow_storage::verifySDClock(freq_khz,dma,baseline,
                [](uint8_t* data){return int(disk_read(VolToPart[0].pd,data,0,1));},
                [](uint32_t clock){return int(sdmmc_host_set_card_clk(SDMMC_HOST_SLOT_1,clock));});
        }
        free(dma);
        if(!verified){Serial.printf("[SDMMC] Clock/read verification failed at %u kHz\n",freq_khz);SD_MMC.end();return false;}
        if(!sdPartitionRecoverMounted()){SD_MMC.end();return false;}
    }
#endif
    if (!ok) return false;

    uint8_t ct = SD_MMC.cardType();
    File root=ct!=CARD_NONE?SD_MMC.open("/"):File();
    const bool readable=root && root.isDirectory(); root.close();
    if (ct == CARD_NONE || !readable) {
        SD_MMC.end();
        return false;
    }

    _mounted  = true;
    _freq_khz = freq_khz;
    _type     = _mapCardType(ct);
    return true;
}

bool sdmmcEnsureReady() {
    static SDMMC_Class storage;
    return storage.begin();
}

SDMMC_Class::CardType SDMMC_Class::_mapCardType(uint8_t raw)
{
    switch (raw) {
        case CARD_MMC:  return CardType::MMC;
        case CARD_SD:   return CardType::SD;
        case CARD_SDHC: {
            /* SDHC ≤ 32 GB, SDXC > 32 GB — distinguish by size */
            uint64_t sz = SD_MMC.cardSize();
            return (sz > (uint64_t)32 * 1024 * 1024 * 1024) ? CardType::SDXC : CardType::SDHC;
        }
        default: return CardType::NONE;
    }
}
