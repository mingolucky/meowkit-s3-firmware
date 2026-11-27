#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_NO_MEM=0x101, ESP_ERR_INVALID_STATE=0x103, ESP_ERR_TIMEOUT=0x107;
enum esp_gap_ble_cb_event_t { ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,
    ESP_GAP_BLE_SCAN_START_COMPLETE_EVT, ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT,
    ESP_GAP_BLE_SCAN_RESULT_EVT };
enum {ESP_GAP_SEARCH_INQ_RES_EVT,ESP_GAP_SEARCH_INQ_CMPL_EVT};
struct esp_ble_gap_cb_param_t {
    struct { int status=0; } scan_param_cmpl,scan_start_cmpl,scan_stop_cmpl;
    struct { int search_evt=0; uint8_t bda[6]{},ble_adv[62]{};
        uint8_t ble_addr_type=0,adv_data_len=0,scan_rsp_len=0; int rssi=-60; } scan_rst;
};
using esp_gap_ble_cb_t=void(*)(esp_gap_ble_cb_event_t,esp_ble_gap_cb_param_t*);
using esp_gatts_cb_t=void(*)(int,int,void*);
enum {BLE_SCAN_TYPE_ACTIVE, BLE_ADDR_TYPE_PUBLIC, BLE_SCAN_FILTER_ALLOW_ALL, BLE_SCAN_DUPLICATE_DISABLE};
struct esp_ble_scan_params_t { int scan_type=0,own_addr_type=0,scan_filter_policy=0;
    uint16_t scan_interval=0,scan_window=0; int scan_duplicate=0; };
using TickType_t=unsigned;
constexpr bool pdTRUE=true;
struct FakeQueue { unsigned capacity,size; std::deque<std::vector<uint8_t>> rows; };
using QueueHandle_t=FakeQueue*;
inline uint32_t fakeNow=0;
inline unsigned queues=0,starts=0,stops=0,releases=0;
inline bool failQueue=false;
inline int setupRc=0,startRc=0,stopRc=0;
inline void (*clockHook)()=nullptr;
inline uint32_t millis() {
    const uint32_t sampled=fakeNow;
    if(clockHook) { auto hook=clockHook; clockHook=nullptr; hook(); }
    return sampled;
}
inline void delay(unsigned ms) { fakeNow+=ms; }
inline QueueHandle_t xQueueCreate(unsigned capacity,unsigned size) {
    if(failQueue) return nullptr;
    ++queues; return new FakeQueue{capacity,size,{}};
}
inline void vQueueDelete(QueueHandle_t q) { --queues; delete q; }
inline bool xQueueSend(QueueHandle_t q,const void* data,unsigned) {
    if(q->rows.size()>=q->capacity) return false;
    auto p=static_cast<const uint8_t*>(data);
    q->rows.emplace_back(p,p+q->size); return true;
}
inline bool xQueueReceive(QueueHandle_t q,void* data,unsigned) {
    if(q->rows.empty()) return false;
    std::memcpy(data,q->rows.front().data(),q->size); q->rows.pop_front(); return true;
}
inline int esp_ble_gap_set_scan_params(esp_ble_scan_params_t*) { return setupRc; }
inline int esp_ble_gap_start_scanning(uint32_t seconds) { ++starts; return seconds==8 ? startRc : ESP_FAIL; }
inline int esp_ble_gap_stop_scanning() { ++stops; return stopRc; }
