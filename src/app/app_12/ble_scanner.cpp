#include "ble_scanner.h"
#include "../../system/bluetooth_manager.h"
#include <Arduino.h>

namespace ble_scan {
std::atomic<Scanner*> Scanner::active_{nullptr};

void Scanner::fail(int code) {
    error_=code; accepting_=false; state_=State::Error;
}
void Scanner::stopped() {
    Scanner* s=active_.load();
    if(!s) return;
    s->accepting_=false;
    if(s->state_!=State::Error) s->state_=State::Paused;
}
void Scanner::cleanup() {
    accepting_=false;
    // BLE deinit waits for callbacks before the queue can be freed.
    bluetooth_manager::release(lease_); lease_=0;
    if(active_==this) active_=nullptr;
    if(queue_) { vQueueDelete(queue_); queue_=nullptr; }
}
void Scanner::end() {
    wanted_=false; cleanup(); state_=State::Idle;
}
bool Scanner::begin() {
    end(); table_.clear(); dropped_=0; error_=0;
    wanted_=true;
    return startRadio();
}
bool Scanner::startRadio() {
    disabled_=!bluetooth_manager::enabled();
    if(disabled_) { fail(ESP_ERR_INVALID_STATE); return false; }
    if(active_) { fail(ESP_ERR_INVALID_STATE); return false; }
    queue_=xQueueCreate(32,sizeof(Observation));
    if(!queue_) { fail(ESP_ERR_NO_MEM); return false; }
    active_=this;
    lease_=bluetooth_manager::acquire(bluetooth_manager::Owner::Other,
        "MeowKit BLE Scanner",gap,nullptr,stopped);
    if(!lease_) { fail(ESP_ERR_INVALID_STATE); cleanup(); return false; }
    params_={};
    params_.scan_type=BLE_SCAN_TYPE_ACTIVE;
    params_.own_addr_type=BLE_ADDR_TYPE_PUBLIC;
    params_.scan_filter_policy=BLE_SCAN_FILTER_ALLOW_ALL;
    params_.scan_interval=0x80; params_.scan_window=0x60;
    params_.scan_duplicate=BLE_SCAN_DUPLICATE_DISABLE;
    state_=State::Configuring; started_=millis();
    const int rc=esp_ble_gap_set_scan_params(&params_);
    if(rc!=ESP_OK) { fail(rc); cleanup(); return false; }
    return true;
}
void Scanner::gap(esp_gap_ble_cb_event_t event,esp_ble_gap_cb_param_t* p) {
    Scanner* s=active_.load();
    if(!s || !p) return;
    const State state=s->state_;
    auto complete=[&](State next,int status) {
        State expected=state;
        if(status!=ESP_OK) {
            if(s->state_.compare_exchange_strong(expected,State::Error)) {
                s->error_=status; s->accepting_=false;
            }
        } else s->state_.compare_exchange_strong(expected,next);
    };
    if(event==ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT && state==State::Configuring) {
        complete(State::Ready,p->scan_param_cmpl.status);
    } else if(event==ESP_GAP_BLE_SCAN_START_COMPLETE_EVT && state==State::Starting) {
        if(p->scan_start_cmpl.status==ESP_OK) s->started_=millis();
        complete(State::Scanning,p->scan_start_cmpl.status);
        s->accepting_=s->state_==State::Scanning;
    } else if(event==ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT && state==State::Stopping) {
        s->accepting_=false;
        complete(State::Paused,p->scan_stop_cmpl.status);
    } else if(event==ESP_GAP_BLE_SCAN_RESULT_EVT) {
        const auto& r=p->scan_rst;
        if(r.search_evt==ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            if(state==State::Scanning) { s->accepting_=false; complete(State::Paused,ESP_OK); }
            return;
        }
        if(r.search_evt!=ESP_GAP_SEARCH_INQ_RES_EVT || state!=State::Scanning || !s->accepting_ || !s->queue_ ||
           r.adv_data_len>31 || r.scan_rsp_len>31) return;
        Observation observation{};
        std::memcpy(observation.address,r.bda,6);
        observation.addressType=static_cast<uint8_t>(r.ble_addr_type);
        observation.rssi=static_cast<int16_t>(r.rssi);
        observation.at=millis(); observation.advLength=r.adv_data_len;
        observation.responseLength=r.scan_rsp_len;
        std::memcpy(observation.data,r.ble_adv,size_t(r.adv_data_len)+r.scan_rsp_len);
        if(xQueueSend(s->queue_,&observation,0)!=pdTRUE) ++s->dropped_;
    }
}
void Scanner::poll() {
    if(queue_) {
        Observation observation;
        for(unsigned n=0;n<32 && xQueueReceive(queue_,&observation,0)==pdTRUE;++n)
            table_.observe(observation);
    }
    State state=state_;
    if(lease_ && !bluetooth_manager::owns(lease_) && state!=State::Error) {
        disabled_=!bluetooth_manager::enabled(); fail(ESP_ERR_INVALID_STATE); state=State::Error;
    }
    if(state==State::Error) { if(queue_ || lease_) cleanup(); return; }
    if(!wanted_ && state==State::Configuring) {
        cleanup(); state_=State::Paused; return;
    }
    // A successful start may publish a new timestamp from the BLE task.
    // Sample it before the clock, and only time out the state we inspected.
    const uint32_t started=started_.load();
    const uint32_t now=millis();
    auto timeout=[&]() {
        State expected=state;
        if(state_.compare_exchange_strong(expected,State::Error)) {
            error_=ESP_ERR_TIMEOUT; accepting_=false; cleanup();
        }
    };
    if((state==State::Configuring || state==State::Starting || state==State::Stopping) &&
       now-started>5000) { timeout(); return; }
    if(state==State::Scanning && now-started>11000) { timeout(); return; }
    if(state==State::Ready || state==State::Paused) {
        if(!wanted_) {
            if(queue_ || lease_) cleanup();
            state_=State::Paused; return;
        }
        if(!lease_) { startRadio(); return; }
        state_=State::Starting; started_=now;
        const int rc=esp_ble_gap_start_scanning(8);
        if(rc!=ESP_OK) { fail(rc); cleanup(); }
    } else if(state==State::Scanning && !wanted_) {
        state_=State::Stopping; started_=now;
        const int rc=esp_ble_gap_stop_scanning();
        if(rc!=ESP_OK) { fail(rc); cleanup(); }
    }
}
const char* Scanner::message() const {
    if(state_!=State::Error) return "";
    if(disabled_) return "Enable Bluetooth in Settings";
    if(error_==ESP_ERR_NO_MEM) return "Not enough memory";
    if(error_==ESP_ERR_TIMEOUT) return "Bluetooth timed out. A: retry";
    return "Bluetooth busy/error. A: retry";
}
} // namespace ble_scan
