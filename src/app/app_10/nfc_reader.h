#pragma once
#include "nfc_data.h"
#include <atomic>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace meow_nfc {
enum class ReaderMode : uint8_t { Idle, Scan, Field, Debug };
enum class ReaderState : uint8_t { Idle, Initializing, Scanning, Reading, Found, Field, Debug, Error };
struct ReaderSnapshot {
    ReaderState state = ReaderState::Idle;
    uint8_t chipId = 0;
    uint16_t errorCode = 0;
    uint16_t supplyMv = 0;
    uint32_t cycles = 0, ioErrors = 0;
    bool externalField = false, fieldOn = false;
    char message[96]{};
    Tag tag;
};
class Reader {
public:
    Reader();
    ~Reader();
    bool begin();
    void end();
    // filter: 0=all, 1=A, 2=B, 3=F, 4=V.
    void request(ReaderMode mode, uint8_t filter = 0);
    bool snapshot(ReaderSnapshot& out) const;
private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
    SemaphoreHandle_t _mutex = nullptr;
    TaskHandle_t _task = nullptr;
    std::unique_ptr<ReaderSnapshot> _snapshot;
    std::atomic<bool> _stop{false}, _running{false};
    std::atomic<uint8_t> _mode{0}, _filter{0};
    std::atomic<uint32_t> _generation{0};
    static void entry(void* context);
    static bool transfer(void* context, uint8_t address, const uint8_t* tx,
                         size_t txLength, uint8_t* rx, size_t rxLength);
    void worker();
    void publish(ReaderState state, const char* text, uint16_t code = 0, const Tag* tag = nullptr);
    bool capture(uint32_t generation);
};
} // namespace meow_nfc
