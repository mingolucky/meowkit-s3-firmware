#pragma once
#include "ble_scan_data.h"
#include <atomic>
#include <esp_gap_ble_api.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace ble_scan {
class Scanner {
public:
    enum class State : uint8_t { Idle,Configuring,Ready,Starting,Scanning,Stopping,Paused,Error };
    bool begin();
    void end();
    void poll();
    void setPaused(bool paused) { wanted_=!paused; }
    bool paused() const { return !wanted_; }
    State state() const { return state_.load(); }
    const Table& table() const { return table_; }
    uint32_t dropped() const { return dropped_.load(); }
    int error() const { return error_.load(); }
    const char* message() const;
private:
    static std::atomic<Scanner*> active_;
    static void gap(esp_gap_ble_cb_event_t event,esp_ble_gap_cb_param_t* p);
    static void stopped();
    void fail(int error);
    void cleanup();
    bool startRadio();
    Table table_;
    QueueHandle_t queue_=nullptr;
    uint32_t lease_=0;
    std::atomic<uint32_t> started_{0};
    bool wanted_=false,disabled_=false;
    std::atomic<State> state_{State::Idle};
    std::atomic<bool> accepting_{false};
    std::atomic<int> error_{0};
    std::atomic<uint32_t> dropped_{0};
    esp_ble_scan_params_t params_{};
};
} // namespace ble_scan
