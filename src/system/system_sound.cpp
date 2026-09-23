#include "system_sound.h"
#include "system_sound_assets.h"
#include "../bsp/devices.h"
#include "../bsp/config.h"
#include "../../lib/ESP32-audioI2S/src/mp3_decoder/mp3_decoder.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace {
enum class SoundId : uint8_t { Stop, Boot, Button };
struct SoundCommand { SoundId id; uint32_t limit_ms; };
DEVICES* s_dev = nullptr;
QueueHandle_t s_queue = nullptr;
TaskHandle_t s_task = nullptr;
volatile bool s_stop = false;
volatile bool s_ready = false;
volatile bool s_suspended = false;
volatile bool s_playing = false;
volatile bool s_key_enabled = true;
volatile int s_volume = 50;

size_t skip_id3(const uint8_t* p, size_t n) {
    if (n < 10 || p[0] != 'I' || p[1] != 'D' || p[2] != '3') return 0;
    size_t z = ((size_t)(p[6] & 0x7f) << 21) | ((size_t)(p[7] & 0x7f) << 14) |
               ((size_t)(p[8] & 0x7f) << 7) | (size_t)(p[9] & 0x7f);
    z += 10;
    return z < n ? z : n;
}

void amp(bool on) {
    if (!s_dev) return;
    if (on) {
        s_dev->speaker.setMute(true);
        s_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, HIGH);
        delay(12);
        s_dev->speaker.setMute(false);
    } else {
        s_dev->speaker.setMute(true);
        delay(4);
        s_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    }
}

bool play_mp3(const uint8_t* data, size_t len, uint32_t limit_ms) {
    if (!s_ready || s_suspended || !data || len < 4 || s_volume == 0) return false;
    if (!MP3Decoder_AllocateBuffers()) return false;
    MP3Decoder_ClearBuffer();
    int16_t* pcm = static_cast<int16_t*>(heap_caps_malloc(
        2304 * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!pcm) { MP3Decoder_FreeBuffers(); return false; }

    s_stop = false;
    s_playing = true;
    amp(true);
    uint32_t deadline = limit_ms ? millis() + limit_ms : 0;
    size_t off = skip_id3(data, len);
    bool played = false;
    while (!s_stop && off + 4 < len) {
        if (deadline && (int32_t)(millis() - deadline) >= 0) break;
        int sync = MP3FindSyncWord(const_cast<unsigned char*>(data + off),
                                   static_cast<int>(len - off));
        if (sync < 0) break;
        off += (size_t)sync;
        int before = (int)(len - off);
        int left = before;
        int err = MP3Decode(const_cast<unsigned char*>(data + off), &left, pcm, 0);
        int used = before - left;
        if (used <= 0) { ++off; continue; }
        off += (size_t)used;
        if (err == 0) {
            int rate = MP3GetSampRate();
            int channels = MP3GetChannels();
            int samples = MP3GetOutputSamps();
            if (rate > 0 && samples > 0 && (channels == 1 || channels == 2)) {
                if (!s_dev->speaker.playPcm(pcm, (size_t)samples,
                                            (uint8_t)channels, (uint32_t)rate)) break;
                played = true;
            }
        }
        taskYIELD();
    }
    s_dev->speaker.stop();
    amp(false);
    free(pcm);
    MP3Decoder_FreeBuffers();
    s_playing = false;
    return played;
}

void task(void*) {
    SoundCommand c{};
    for (;;) {
        if (xQueueReceive(s_queue, &c, portMAX_DELAY) != pdTRUE) continue;
        if (c.id == SoundId::Stop) { s_stop = true; continue; }
        if (s_suspended) continue;
        if (c.id == SoundId::Button && !s_key_enabled) continue;
        play_mp3(c.id == SoundId::Boot ? boot_sound_mp3_data : button_sound_mp3_data,
                 c.id == SoundId::Boot ? boot_sound_mp3_len : button_sound_mp3_len,
                 c.limit_ms);
    }
}

void enqueue(SoundId id, uint32_t limit) {
    if (!s_ready || !s_queue || (s_suspended && id != SoundId::Stop)) return;
    s_stop = true;
    SoundCommand c{id, limit};
    xQueueOverwrite(s_queue, &c);
}
}

bool system_sound_init(DEVICES* dev) {
    if (s_ready) return true;
    if (!dev) return false;
    s_dev = dev;
    s_dev->speaker.config().sample_rate = 44100;
    if (!s_dev->speaker.begin(&In_I2C)) return false;
    s_dev->speaker.setMute(true);
    s_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    s_queue = xQueueCreate(1, sizeof(SoundCommand));
    if (!s_queue) { s_dev->speaker.end(); return false; }
    s_ready = true;
    if (xTaskCreatePinnedToCore(task, "system_sound", 6144, nullptr, 3,
                                &s_task, 1) != pdPASS) {
        s_ready = false; vQueueDelete(s_queue); s_queue = nullptr;
        s_dev->speaker.end(); return false;
    }
    return true;
}
void system_sound_set_volume(int p) {
    if (p < 0) p = 0; if (p > 100) p = 100; s_volume = p;
    if (s_dev && s_dev->speaker.isEnabled())
        s_dev->speaker.setVolume(p * SPK_VOLUME_MAX / 100);
}
void system_sound_set_key_enabled(bool on) { s_key_enabled = on; }
void system_sound_play_boot(uint32_t ms) { enqueue(SoundId::Boot, ms); }
void system_sound_play_button(void) { enqueue(SoundId::Button, 0); }
void system_sound_stop(void) { enqueue(SoundId::Stop, 0); }

void system_sound_suspend(void) {
    if (!s_ready || s_suspended) return;
    s_suspended = true;
    s_stop = true;
    if (s_queue) xQueueReset(s_queue);

    /* Wait until the decoder has stopped touching I2S before releasing it. */
    uint32_t deadline = millis() + 1000;
    while (s_playing && (int32_t)(millis() - deadline) < 0)
        vTaskDelay(pdMS_TO_TICKS(2));

    s_dev->speaker.setMute(true);
    s_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    if (!s_playing) s_dev->speaker.end();
}

bool system_sound_resume(void) {
    if (!s_ready) return false;
    if (!s_suspended) return true;
    if (s_playing) return false;

    s_dev->speaker.config().sample_rate = 44100;
    if (!s_dev->speaker.begin(&In_I2C)) return false;
    s_dev->speaker.setVolume(s_volume * SPK_VOLUME_MAX / 100);
    s_dev->speaker.setMute(true);
    s_dev->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    s_stop = false;
    s_suspended = false;
    return true;
}
