#include "ble_scan_fakes/sdk.h"
#include "../../src/system/bluetooth_manager.h"
#include "../../src/app/app_12/ble_scanner.h"
#include <cassert>
#include <cstdio>

static esp_gap_ble_cb_t callback=nullptr;
static bluetooth_manager::Stop onStop=nullptr;
static bool allowed=true;
static uint32_t liveLease=0;
namespace bluetooth_manager {
bool enabled() { return allowed; }
bool owns(Lease l) { return l && l==liveLease; }
Lease acquire(Owner,const char*,esp_gap_ble_cb_t gap,esp_gatts_cb_t gatts,Stop stop) {
    assert(gatts==nullptr); // Scanner never registers a GATT application.
    if(!allowed || liveLease) return 0;
    callback=gap; onStop=stop; return liveLease=1;
}
void release(Lease l) { if(l && l==liveLease) { ++releases; if(onStop) onStop(); liveLease=0; } }
}
static void event(esp_gap_ble_cb_event_t e,int status=0) {
    esp_ble_gap_cb_param_t p{};
    p.scan_param_cmpl.status=p.scan_start_cmpl.status=p.scan_stop_cmpl.status=status;
    callback(e,&p);
}
static void confirmDuringClock() { ++fakeNow; event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); }
static void beginScan(ble_scan::Scanner& s) {
    assert(s.begin());
    assert(s.state()==ble_scan::Scanner::State::Configuring);
    event(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT); s.poll();
    assert(s.state()==ble_scan::Scanner::State::Starting);
    event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); s.poll();
    assert(s.state()==ble_scan::Scanner::State::Scanning);
}
int main() {
    using State=ble_scan::Scanner::State;
    ble_scan::Scanner s;
    allowed=false;
    assert(!s.begin() && queues==0 && s.state()==State::Error);
    allowed=true;
    beginScan(s);
    esp_ble_gap_cb_param_t p{};
    p.scan_rst.search_evt=ESP_GAP_SEARCH_INQ_RES_EVT;
    p.scan_rst.bda[5]=1;
    p.scan_rst.adv_data_len=5;
    const uint8_t name[]={4,9,'G','a','t'};
    std::memcpy(p.scan_rst.ble_adv,name,sizeof(name));
    for(unsigned i=0;i<45;++i) callback(ESP_GAP_BLE_SCAN_RESULT_EVT,&p);
    s.poll();
    assert(s.table().count()==1 && s.table()[0].packets==32);
    assert(s.dropped()==13);
    s.setPaused(true); s.poll();
    assert(s.state()==State::Stopping);
    const unsigned before=starts;
    s.setPaused(false); s.poll();
    assert(starts==before); // Resume waits for the previous stop completion.
    event(ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT); s.poll();
    assert(starts==before+1);
    event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); s.poll();
    p.scan_rst.search_evt=ESP_GAP_SEARCH_INQ_CMPL_EVT;
    callback(ESP_GAP_BLE_SCAN_RESULT_EVT,&p); s.poll();
    assert(s.state()==State::Starting); // Bounded discovery rounds continue.
    event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); s.poll();
    s.setPaused(true); s.poll();
    event(ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT); s.poll();
    assert(!liveLease && queues==0 && s.state()==State::Paused);
    const unsigned observations=s.table().count();
    s.setPaused(false); s.poll();
    assert(s.state()==State::Configuring && s.table().count()==observations);
    s.end(); assert(queues==0 && !liveLease);
    event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); // Late callback after close is ignored.
    assert(s.state()==State::Idle);

    assert(s.begin()); // Closing while parameters are pending still releases resources.
    s.end(); assert(queues==0 && !liveLease);
    assert(s.begin());
    fakeNow+=6000; s.poll();
    assert(s.state()==State::Error && !liveLease && queues==0);

    setupRc=ESP_FAIL;
    assert(!s.begin() && s.state()==State::Error && !liveLease && queues==0);
    setupRc=0;
    assert(s.begin());
    event(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT); s.poll();
    fakeNow+=4000;
    event(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT); s.poll();
    fakeNow+=7500; s.poll();
    assert(s.state()==State::Scanning); // Watchdog begins at start confirmation.
    s.end();
    assert(s.begin());
    event(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT); s.poll();
    fakeNow+=5001; clockHook=confirmDuringClock; s.poll();
    assert(s.state()==State::Scanning); // A start confirmation wins a stale timeout check.
    fakeNow+=7500; s.poll(); assert(s.state()==State::Scanning);
    s.end();
    assert(s.begin()); s.setPaused(true); s.poll();
    assert(s.state()==State::Paused && !liveLease && queues==0);
    s.end();
    failQueue=true;
    assert(!s.begin() && !liveLease && queues==0);
    failQueue=false;
    beginScan(s);
    event(ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT,ESP_FAIL); // Unrelated completion must not change running state.
    assert(s.state()==State::Scanning);
    allowed=false;
    bluetooth_manager::release(liveLease);
    s.poll();
    assert(s.state()==State::Error && queues==0);
    allowed=true;
    for(unsigned n=0;n<20;++n) { beginScan(s); s.end(); }
    assert(queues==0 && !liveLease);
    puts("BLE scanner asynchronous lifecycle tests passed");
}
