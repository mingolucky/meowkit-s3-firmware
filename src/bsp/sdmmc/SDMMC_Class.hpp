/**
 * @file SDMMC_Class.hpp
 * @brief BSP SDMMC — auto-frequency negotiation for broad card compatibility.
 *
 * begin() probes an existing mount, or retries 10 / 5 / 1 MHz without formatting.
 * This covers SD / SDHC / SDXC / MMC and cheap generic cards that refuse
 * high-speed clocks.
 *
 * Usage:
 *   devices.sd.begin();
 *   if (devices.sd.isReady()) { ... }
 *   devices.sd.isPresent();    // hot-plug probe (opens "/")
 */
#pragma once
#include <SD_MMC.h>
#include <stdint.h>

class SDMMC_Class {
public:
    enum class CardType { NONE, MMC, SD, SDHC, SDXC };

    /* ── Mount / Unmount ─────────────────────────────────────── */

    /**
     * Mount SD card with automatic frequency fallback.
     * @param mode4bit  Use 4-bit bus (faster, needs D1-D3 wired). Default: false (1-bit).
     * @return true if mounted at any frequency.
     */
    bool begin(bool mode4bit = false);

    /** Unmount and release the SD_MMC driver. */
    void end();

    /* ── State ───────────────────────────────────────────────── */

    bool     isReady()      const { return _mounted; }
    CardType cardType()     const { return _type; }
    uint32_t freqKHz()      const { return _freq_khz; }
    const char* cardTypeStr() const;

    /* ── Capacity (valid after begin()) ─────────────────────── */
    uint64_t cardSize()    const;
    uint64_t totalBytes()  const;
    uint64_t usedBytes()   const;
    uint64_t freeBytes()   const;

    /**
     * Active hot-plug detection: opens "/" on SD_MMC.
     * More reliable than cardType() which caches state.
     */
    bool isPresent();

private:
    bool     _mounted   = false;
    CardType _type      = CardType::NONE;
    uint32_t _freq_khz  = 0;

    bool _tryMount(uint32_t freq_khz, bool mode4bit);
    CardType _mapCardType(uint8_t raw);
};

/** Shared mounting entry point for C bridges and apps without a DEVICES handle. */
bool sdmmcEnsureReady();
