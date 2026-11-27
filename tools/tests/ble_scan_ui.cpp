#include "ble_scan_fakes/sdk.h"
#include "ble_scan_app.h"
#include "../../src/system/bluetooth_manager.h"
#include <cassert>
#include <cstdio>

static esp_gap_ble_cb_t callback=nullptr;
static bluetooth_manager::Stop onStop=nullptr;
static uint32_t liveLease=0;
namespace bluetooth_manager {
bool enabled() { return true; }
bool owns(Lease l) { return l && l==liveLease; }
Lease acquire(Owner,const char*,esp_gap_ble_cb_t gap,esp_gatts_cb_t,Stop stop) {
    callback=gap; onStop=stop; return liveLease=1;
}
void release(Lease l) { if(l && l==liveLease) { onStop(); liveLease=0; } }
}
static bool displays(const DEVICES& d,const char* s) {
    for(const auto& line:d.Lcd.text) if(line.find(s)!=std::string::npos) return true;
    return false;
}
static void tick(MOONCAKE::APPS::App12& app) { fakeNow+=501; app.onRunning(); }
static void emit(unsigned address,int rssi,const char* name) {
    esp_ble_gap_cb_param_t p{};
    p.scan_rst.search_evt=ESP_GAP_SEARCH_INQ_RES_EVT;
    p.scan_rst.bda[5]=static_cast<uint8_t>(address); p.scan_rst.rssi=rssi;
    const unsigned n=static_cast<unsigned>(std::strlen(name));
    p.scan_rst.adv_data_len=static_cast<uint8_t>(n+2);
    p.scan_rst.ble_adv[0]=static_cast<uint8_t>(n+1); p.scan_rst.ble_adv[1]=9;
    std::memcpy(p.scan_rst.ble_adv+2,name,n);
    callback(ESP_GAP_BLE_SCAN_RESULT_EVT,&p);
}
int main() {
    DEVICES d;
    MOONCAKE::APPS::App12 app(&d);
    app.onOpen(); tick(app);
    assert(displays(d,"Not detected is not proof"));
    esp_ble_gap_cb_param_t p{};
    callback(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,&p); tick(app);
    callback(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT,&p); tick(app);
    emit(1,-30,"First"); emit(2,-60,"Second"); tick(app);
    d.button.Down.event=true; tick(app); // Select second, then make it strongest.
    emit(2,-20,"Second"); tick(app);
    d.button.A.event=true; tick(app);
    assert(displays(d,"Name: Second")); // Selection tracks the address, not row index.
    assert(displays(d,"00:00:00:00:00:02"));
    tick(app); d.button.A.event=true; tick(app);
    assert(displays(d,"Advertisement + scan response"));
    tick(app); d.button.B.event=true; tick(app); tick(app);
    d.button.Right.event=true; tick(app);
    assert(displays(d,"No PAL candidates detected"));
    emit(3,-40,"4G123456789"); tick(app);
    d.button.A.event=true; tick(app);
    assert(displays(d,"TARGET ID MATCH"));
    tick(app); d.button.B.event=true; tick(app); tick(app);
    // Tap a row without moving it under the finger when RSSI changes.
    d.ctp.x=30; d.ctp.y=60; d.ctp.touched=true; tick(app);
    emit(4,-10,"PalGate"); tick(app);
    d.ctp.touched=false; tick(app);
    assert(displays(d,"PAL ID: 4G123456789"));
    app.onClose(); assert(queues==0 && !liveLease);

    app.onOpen(); tick(app);
    callback(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,&p); tick(app);
    callback(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT,&p); tick(app);
    emit(1,-30,"First"); emit(2,-60,"Second"); tick(app);
    // No redraw yet: touching the visible first row must still open First.
    emit(2,-10,"Second"); app.onRunning();
    d.ctp.x=30; d.ctp.y=60; d.ctp.touched=true; app.onRunning();
    // While the finger is held, fill the table and evict First's old table slot.
    for(unsigned i=10;i<130;++i) { emit(i,-50,"Crowd"); app.onRunning(); }
    d.ctp.touched=false; app.onRunning();
    assert(displays(d,"Name: First"));
    assert(displays(d,"00:00:00:00:00:01"));
    app.onClose(); assert(queues==0 && !liveLease);

    app.onOpen(); tick(app);
    callback(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT,&p); tick(app);
    callback(ESP_GAP_BLE_SCAN_START_COMPLETE_EVT,&p); tick(app);
    emit(1,-30,"4G700202763"); tick(app);
    d.button.A.event=true; tick(app);
    assert(displays(d,"PAL controller ID advertised"));
    app.onClose(); assert(queues==0 && !liveLease);
    puts("BLE scanner UI and stable-selection tests passed");
}
