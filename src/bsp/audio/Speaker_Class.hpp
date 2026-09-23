/**
 * @file Speaker_Class.hpp
 * @brief Speaker playback class for MeowKit (ESP32-S3 + ES8311 + NS4150B)
 *
 * Hardware chain: ESP32-S3 I2S TX → ES8311 DAC → NS4150B Class-D Amp → 18mm Speaker (1W)
 *
 * Design referenced from M5Atomic-EchoBase, adapted to MeowKit BSP:
 * - Reuses the existing es8311 C driver for codec register control
 * - Uses ESP-IDF legacy I2S driver (driver/i2s.h) for audio data TX path
 * - FreeRTOS background task for non-blocking playback
 * - Supports: buffer playback, tone generation, volume/mute control
 *
 * I2S port assignment:
 *   I2S_NUM_0 = Speaker TX  (this class)
 *   I2S_NUM_1 = Mic RX      (Mic_Class)
 */
#pragma once

#include "../config.h"
#include "ES8311_Class.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include "driver/i2s.h"
#include <cstdint>
#include <cstddef>

// ES8311 default I2C address (CE pin low)
#define SPK_ES8311_ADDR   ES8311_ADDRRES_0   // 0x18

/* NS4150B + 18mm 1W speaker: amplifier output power exceeds speaker rated power
 * at high DAC levels. Validated safe ceiling: volume=16/100 keeps output ≤1W.
 * All callers (settings_bridge, mp3 apps) must not exceed this value. */
#define SPK_VOLUME_MAX    75

/**
 * @brief Speaker configuration.
 */
struct speaker_config_t
{
    // ── I2S pin mapping (defaults from config.h) ──
    int pin_bclk     = BCLKPIN;      ///< Bit clock (shared with mic)
    int pin_ws       = WSPIN;        ///< Word select / LRCK (shared with mic)
    int pin_data_out = DOPIN;        ///< Data out (to ES8311 SDIN)
    int pin_mclk     = MCLKPIN;       ///< Master clock (to ES8311 CCLK)

    // ── Audio parameters ──
    uint32_t sample_rate    = 44100; ///< Default sample rate in Hz
    uint8_t  bits_per_sample = 16;   ///< 16 or 32 bits

    // ── DMA tuning ──
    size_t   dma_buf_len    = 256;   ///< Samples per DMA buffer
    size_t   dma_buf_count  = 8;     ///< Number of DMA buffers

    // ── Background task ──
    uint8_t  task_priority     = 2;
    uint8_t  task_pinned_core  = 1;  ///< Pin to core 1
    size_t   task_stack_size   = 4096;

    // ── I2S port ──
    i2s_port_t i2s_port = I2S_NUM_0; ///< Default to port 0 (port 1 used by mic)
};

/**
 * @brief Speaker playback class.
 *
 * Usage:
 * @code
 *   Speaker_Class spk;
 *   spk.config().sample_rate = 16000;
 *   spk.begin();
 *   spk.setVolume(60);
 *
 *   // Blocking buffer playback
 *   int16_t buf[1024];
 *   spk.play(buf, 1024);
 *
 *   // Tone
 *   spk.tone(1000, 500);  // 1kHz for 500ms
 *
 *   spk.end();
 * @endcode
 */
class Speaker_Class
{
public:
    Speaker_Class() = default;
    ~Speaker_Class() { end(); }

    /// Access configuration (modify before begin())
    speaker_config_t& config() { return _cfg; }
    const speaker_config_t& config() const { return _cfg; }

    /**
     * @brief Initialize ES8311 codec and I2S driver.
     *
     * Configures ES8311 as DAC slave, sets up I2S TX.
     * @param i2c  I2C_Class pointer for ES8311 I2C (default &In_I2C)
     * @return true on success
     */
    bool begin(I2C_Class* i2c = &In_I2C);

    /**
     * @brief Stop playback and release resources.
     */
    void end();

    /// Is the speaker initialized?
    bool isEnabled() const { return _initialized; }

    /// Is playback currently in progress?
    bool isPlaying() const { return _is_playing; }

    // ── Volume & Mute ──

    /**
     * @brief Set output volume.
     * @param volume 0 ~ 100
     * @return true on success
     */
    bool setVolume(int volume);

    /**
     * @brief Get current volume.
     * @return volume 0 ~ 100, or -1 on error
     */
    int  getVolume();

    /**
     * @brief Mute / unmute DAC output.
     * @param enable true = mute, false = unmute
     * @return true on success
     */
    bool setMute(bool enable);

    // ── Playback ──

    /**
     * @brief Play 16-bit PCM buffer (blocking).
     *
     * Writes samples to I2S TX in chunks. Blocks until all data is written.
     * @param data   PCM sample buffer (signed 16-bit, mono or stereo interleaved)
     * @param length Number of samples
     * @param sample_rate  Playback rate (0 = use current config rate)
     * @return true on success
     */
    bool play(const int16_t* data, size_t length, uint32_t sample_rate = 0);

    /**
     * @brief Play raw byte buffer (blocking).
     * @param data   Raw audio bytes
     * @param size   Size in bytes
     * @return true on success
     */
    bool play(const uint8_t* data, size_t size);
    bool playPcm(const int16_t* data, size_t sample_count, uint8_t channels,
                 uint32_t sample_rate);

    /**
     * @brief Play a sine-wave tone (blocking).
     * @param freq_hz   Tone frequency in Hz
     * @param duration_ms Duration in milliseconds
     * @param volume     Override volume (0~100, 0 = use current)
     */
    void tone(uint32_t freq_hz, uint32_t duration_ms, int volume = 0);

    /**
     * @brief Stop any ongoing playback and clear DMA buffer.
     */
    void stop();

    // ── Sample rate ──

    /**
     * @brief Change sample rate at runtime.
     *
     * Reconfigures I2S clock and ES8311 sample frequency.
     * @param rate New sample rate in Hz
     * @return true on success
     */
    bool setSampleRate(uint32_t rate);

    /// Get current sample rate
    uint32_t getSampleRate() const { return _cfg.sample_rate; }

private:
    speaker_config_t _cfg;

    // ── ES8311 codec ──
    es8311_handle_t  _es_handle    = nullptr;
    I2C_Class*       _i2c          = nullptr;

    // ── State ──
    bool     _initialized  = false;
    bool     _i2s_installed = false;
    volatile bool _is_playing = false;
    int      _volume       = 50;      ///< Cached volume 0~100

    // ── Internal ──
    bool _init_codec(uint32_t sample_rate);
    bool _init_i2s();
    void _uninstall_i2s();

    /// Write PCM data to I2S TX (blocking)
    bool _i2s_write(const void* data, size_t size_bytes);
};
