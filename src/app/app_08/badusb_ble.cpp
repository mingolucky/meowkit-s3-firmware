// Static composite HID GATT lifecycle: authenticated PIN pairing, bounded report
// pipeline, and explicit application-owned state. No BLEServer objects retained.
#include "badusb_ble.h"
#ifdef BADUSB_BLE_HOST_TEST
#include "../../../test/badusb/stubs/ble_platform.h"
#else
#include "../../system/bluetooth_manager.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_device.h>
#include <esp_system.h>
#endif
#include "badusb_keyboard.h"
namespace keyboard_transport { bool ready(); }
#include <atomic>
#include <cstring>
#include <algorithm>

namespace {
constexpr uint16_t APP_ID = 0x800;
std::atomic<bool> active{false}, linked{false}, secure{false}, subscribed{false}, error{false};
std::atomic<bool> bootSubscribed{false};
std::atomic<bool> congested{false}, suspended{false};
std::atomic<unsigned> epoch{0};
std::atomic<bool> sendFault{false};
std::atomic<bool> mouseSubscribed{false}, mediaSubscribed{false};
std::atomic<uint8_t> leds{0}, disconnectReason{0};
std::atomic<int> lastError{0};
std::atomic<uint16_t> intervalMs{30};
std::atomic<BuBlePairing> pairing{BuBlePairing::None};
std::atomic<uint32_t> passkey{0};
std::atomic<unsigned> pendingReports{0};
enum class TxPhase : uint8_t { Idle, Pending, Accepted, RetryRejected, Fatal };
enum class TxFailure : uint8_t { None, BusyTimeout, CompletionUnknown, LinkLost, ProtocolChanged, Cancelled, AsyncError };
std::atomic<TxPhase> txPhase{TxPhase::Idle};
std::atomic<TxFailure> txFailure{TxFailure::None};
std::atomic<uint16_t> txHandle{0};
std::atomic<unsigned> txSession{0};
uint8_t txBytes[8]={};
uint16_t txSize=0;
std::atomic<bool> recovering{false};
std::atomic<uint32_t> recoveredCongestions{0}, rejectedRetries{0}, cooldownUntil{0}, readinessChanged{0};
BuBleWaitCallback waitCallback=nullptr;
void* waitContext=nullptr;
uint32_t lastWaitPump=0, lastAcceptedAt=0;
constexpr uint32_t RECOVERY_MS=3000;
constexpr uint32_t READY_SETTLE_MS=150;

esp_gatt_if_t iface = ESP_GATT_IF_NONE;
bluetooth_manager::Lease lease=0;
uint16_t conn = 0, handles[2][29] = {};
uint8_t peer[6] = {};
std::atomic<unsigned> services{0};
unsigned advReady = 0;
constexpr unsigned REPORT_WINDOW = 4;
uint32_t startedAt = 0, reportTimes[REPORT_WINDOW] = {};
unsigned windowReports = 0, nextReport = 0, scheduleEpoch = 0;

uint16_t serviceUuid[] = {0x1812, 0x180A};
uint16_t primary = 0x2800, declaration = 0x2803;
uint16_t infoUuid = 0x2A4A, mapUuid = 0x2A4B, controlUuid = 0x2A4C;
uint16_t protocolUuid = 0x2A4E, reportUuid = 0x2A4D, cccUuid = 0x2902, refUuid = 0x2908;
uint16_t bootUuid = 0x2A22, bootOutUuid=0x2A32;
uint8_t outputProp=ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_WRITE|ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t keyboardOut=0, outputRef[]={1,2}, mediaRef[]={2,1}, media[2]={}, mediaCcc[2]={};
uint8_t mouse[4]={}, mouseCcc[2]={}, mouseRef[]={3,1};
uint16_t manufacturerUuid = 0x2A29, pnpUuid = 0x2A50;
uint8_t readProp = ESP_GATT_CHAR_PROP_BIT_READ;
uint8_t writeProp = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t protocolProp = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t reportProp = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
uint8_t info[] = {0x11, 0x01, 0, 2}, control = 0, protocol = 1;
std::atomic<uint8_t> protocolMode{1};
uint8_t report[8] = {}, ccc[2] = {}, ref[] = {1, 1};
uint8_t bootReport[8] = {}, bootCcc[2] = {};
uint8_t manufacturer[] = "MeowKit", pnp[] = {2, 0xAC, 0x05, 0x0A, 0x82, 0x10, 0x02};
// Battery level is not exposed: the board has no validated percentage source.
// Report IDs: keyboard=1 (8 bytes), consumer=2 (16-bit usage), mouse=3 (4 bytes).
// Keep the keyboard part boot-compatible. Consumer matches USB's generic usage.
uint8_t reportMap[] = {
    0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,
    0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,0x25,0x01,
    0x75,0x01,0x95,0x08,0x81,0x02,
    0x95,0x01,0x75,0x08,0x81,0x01,
    0x95,0x05,0x75,0x01,0x05,0x08,0x19,0x01,0x29,0x05,0x91,0x02,
    0x95,0x01,0x75,0x03,0x91,0x01,
    0x95,0x06,0x75,0x08,0x15,0x00,0x26,0xDF,0x00,
    0x05,0x07,0x19,0x00,0x29,0xDF,0x81,0x00,0xC0,
    0x05,0x0C,0x09,0x01,0xA1,0x01,0x85,0x02,
    0x15,0x00,0x26,0xFF,0x03,0x19,0x00,0x2A,0xFF,0x03,
    0x75,0x10,0x95,0x01,0x81,0x00,0xC0,
    0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x03,0x09,0x01,0xA1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x03,0x15,0x00,0x25,0x01,
    0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,0x81,0x01,
    0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,
    0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x03,0x81,0x06,0xC0,0xC0
};
esp_gatts_attr_db_t hidDb[29], disDb[5];
esp_ble_adv_params_t adv = {};

void attribute(esp_gatts_attr_db_t& a, uint16_t& uuid, uint16_t perm,
               uint16_t len, uint8_t* value) {
    a = {};
    a.attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    a.att_desc = {ESP_UUID_LEN_16, reinterpret_cast<uint8_t*>(&uuid), perm, len, len, value};
}
void databases() {
    attribute(hidDb[0], primary, ESP_GATT_PERM_READ, 2, reinterpret_cast<uint8_t*>(&serviceUuid[0]));
    attribute(hidDb[1], declaration, ESP_GATT_PERM_READ, 1, &readProp);
    attribute(hidDb[2], infoUuid, ESP_GATT_PERM_READ, sizeof(info), info);
    attribute(hidDb[3], declaration, ESP_GATT_PERM_READ, 1, &readProp);
    attribute(hidDb[4], mapUuid, ESP_GATT_PERM_READ, sizeof(reportMap), reportMap);
    attribute(hidDb[5], declaration, ESP_GATT_PERM_READ, 1, &writeProp);
    attribute(hidDb[6], controlUuid, ESP_GATT_PERM_WRITE_ENC_MITM, 1, &control);
    attribute(hidDb[7], declaration, ESP_GATT_PERM_READ, 1, &protocolProp);
    attribute(hidDb[8], protocolUuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENC_MITM, 1, &protocol);
    attribute(hidDb[9], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[10], reportUuid, ESP_GATT_PERM_READ_ENC_MITM, sizeof(report), report);
    attribute(hidDb[11], cccUuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENC_MITM, 2, ccc);
    attribute(hidDb[12], refUuid, ESP_GATT_PERM_READ, 2, ref);
    attribute(hidDb[13], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[14], bootUuid, ESP_GATT_PERM_READ_ENC_MITM, 8, bootReport);
    attribute(hidDb[15], cccUuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENC_MITM, 2, bootCcc);
    attribute(hidDb[16], declaration, ESP_GATT_PERM_READ, 1, &outputProp);
    attribute(hidDb[17], reportUuid, ESP_GATT_PERM_READ_ENC_MITM|ESP_GATT_PERM_WRITE_ENC_MITM, 1, &keyboardOut);
    attribute(hidDb[18], refUuid, ESP_GATT_PERM_READ, 2, outputRef);
    attribute(hidDb[19], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[20], reportUuid, ESP_GATT_PERM_READ_ENC_MITM, 2, media);
    attribute(hidDb[21], cccUuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE_ENC_MITM, 2, mediaCcc);
    attribute(hidDb[22], refUuid, ESP_GATT_PERM_READ, 2, mediaRef);
    attribute(hidDb[23], declaration, ESP_GATT_PERM_READ, 1, &outputProp);
    attribute(hidDb[24], bootOutUuid, ESP_GATT_PERM_READ_ENC_MITM|ESP_GATT_PERM_WRITE_ENC_MITM, 1, &keyboardOut);

    attribute(hidDb[25], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[26], reportUuid, ESP_GATT_PERM_READ_ENC_MITM, sizeof(mouse), mouse);
    attribute(hidDb[27], cccUuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE_ENC_MITM, 2, mouseCcc);
    attribute(hidDb[28], refUuid, ESP_GATT_PERM_READ, 2, mouseRef);

    attribute(disDb[0], primary, ESP_GATT_PERM_READ, 2, reinterpret_cast<uint8_t*>(&serviceUuid[1]));
    attribute(disDb[1], declaration, ESP_GATT_PERM_READ, 1, &readProp);
    attribute(disDb[2], manufacturerUuid, ESP_GATT_PERM_READ, sizeof(manufacturer)-1, manufacturer);
    attribute(disDb[3], declaration, ESP_GATT_PERM_READ, 1, &readProp);
    attribute(disDb[4], pnpUuid, ESP_GATT_PERM_READ, sizeof(pnp), pnp);
}
void advertise() {
    if (active && services == 2 && advReady == 3 && !linked) {
        if (esp_ble_gap_start_advertising(&adv) != ESP_OK) error = true;
    }
}
void gap(esp_gap_ble_cb_event_t e, esp_ble_gap_cb_param_t* p) {
    if (!active) return;
    switch(e) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        if (p->adv_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) { error=true; break; }
        advReady |= 1; advertise(); break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        if (p->scan_rsp_data_raw_cmpl.status != ESP_BT_STATUS_SUCCESS) { error=true; break; }
        advReady |= 2; advertise(); break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (p->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) error=true;
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        if (p->update_conn_params.status==ESP_BT_STATUS_SUCCESS)
            intervalMs=uint16_t((p->update_conn_params.conn_int*5+3)/4);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT: esp_ble_gap_security_rsp(p->ble_security.ble_req.bd_addr, true); break;
    case ESP_GAP_BLE_NC_REQ_EVT:
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        if (!linked || memcmp(peer,p->ble_security.key_notif.bd_addr,6)) break;
        passkey=p->ble_security.key_notif.passkey;
        pairing=e==ESP_GAP_BLE_NC_REQ_EVT ? BuBlePairing::Compare : BuBlePairing::Display;
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:
        // This device displays the PIN. Never answer an input request with a fixed PIN.
        esp_ble_passkey_reply(p->ble_security.ble_req.bd_addr,false,0);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (!linked || memcmp(peer,p->ble_security.auth_cmpl.bd_addr,6)) break;
        pairing=BuBlePairing::None;
        readinessChanged=millis();
        secure = p->ble_security.auth_cmpl.success &&
            (p->ble_security.auth_cmpl.auth_mode & ESP_LE_AUTH_REQ_MITM);
        if (!secure) esp_ble_gap_disconnect(p->ble_security.auth_cmpl.bd_addr);
        break;
    default: break;
    }
}
void gatts(esp_gatts_cb_event_t e, esp_gatt_if_t i, esp_ble_gatts_cb_param_t* p) {
    if (!active) return;
    if (e == ESP_GATTS_REG_EVT) {
        if (p->reg.app_id != APP_ID) return;
        if (p->reg.status != ESP_GATT_OK) { error=true; return; }
        iface=i;
        if (esp_ble_gatts_create_attr_tab(hidDb, i, 29, 0) != ESP_OK ||
            esp_ble_gatts_create_attr_tab(disDb, i, 5, 1) != ESP_OK) error=true;
        return;
    }
    if (i != iface && i != ESP_GATT_IF_NONE) return;
    switch(e) {
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        unsigned id=p->add_attr_tab.svc_inst_id;
        if (p->add_attr_tab.status != ESP_GATT_OK || id>1 || p->add_attr_tab.num_handle != (id ? 5 : 29)) { error=true; break; }
        memcpy(handles[id], p->add_attr_tab.handles, p->add_attr_tab.num_handle*sizeof(uint16_t));
        if (esp_ble_gatts_start_service(handles[id][0]) != ESP_OK) error=true;
        break;
    }
    case ESP_GATTS_START_EVT:
        if (p->start.status != ESP_GATT_OK) { error=true; break; }
        ++services; advertise(); break;
    case ESP_GATTS_CONNECT_EVT: {
        conn=p->connect.conn_id; memcpy(peer,p->connect.remote_bda,6);
        linked=true; secure=false; subscribed=false; bootSubscribed=false; mediaSubscribed=false; mouseSubscribed=false; suspended=false; congested=false; sendFault=false; disconnectReason=0; ++epoch;
        pairing=BuBlePairing::None; pendingReports=0;txPhase=TxPhase::Idle;txFailure=TxFailure::None;readinessChanged=millis();
        memset(report,0,sizeof(report)); memset(ccc,0,sizeof(ccc)); protocol=1; protocolMode=1; leds=0; intervalMs=30;
        esp_ble_gatts_set_attr_value(handles[0][11],2,ccc);
        esp_ble_gatts_set_attr_value(handles[0][15],2,ccc);
        esp_ble_gatts_set_attr_value(handles[0][21],2,ccc);
        esp_ble_gatts_set_attr_value(handles[0][27],2,ccc);
        esp_ble_gatts_set_attr_value(handles[0][8],1,&protocol);
        esp_ble_set_encryption(peer, ESP_BLE_SEC_ENCRYPT_MITM);
        esp_ble_conn_update_params_t params = {};
        memcpy(params.bda,peer,6); params.min_int=6; params.max_int=12;
        params.latency=0; params.timeout=400; esp_ble_gap_update_conn_params(&params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        linked=false; secure=false; subscribed=false; bootSubscribed=false;
        pairing=BuBlePairing::None; pendingReports=0;txPhase=TxPhase::Idle;txFailure=TxFailure::None;readinessChanged=millis();
        mediaSubscribed=false; mouseSubscribed=false;
        disconnectReason=p->disconnect.reason;
        ++epoch; sendFault=true;txFailure=TxFailure::LinkLost;txPhase=TxPhase::Idle;
        advertise(); // Reconnect the link, never replay the interrupted script.
        break;
    case ESP_GATTS_CONGEST_EVT: congested=p->congest.congested; break;
    case ESP_GATTS_CONF_EVT: {
        if (!linked || p->conf.conn_id!=conn || txPhase!=TxPhase::Pending || txSession!=epoch || p->conf.handle!=txHandle) break;
        // Failed notifications carry the rejected/accepted packet. Match it so
        // an unrelated or duplicate callback cannot complete the next key.
        if (p->conf.status!=ESP_GATT_OK && p->conf.len &&
            (p->conf.len!=txSize || !p->conf.value || memcmp(p->conf.value,txBytes,txSize))) break;
        pendingReports=0;
        lastError=p->conf.status;
        if (p->conf.status==ESP_GATT_OK || p->conf.status==ESP_GATT_CONGESTED) {
            // ESP-IDF attp_send_msg_to_l2cap: CONGESTED means accepted by
            // L2CAP, NOT rejected. Replaying it would duplicate a keystroke.
            if (p->conf.status==ESP_GATT_CONGESTED) {
                ++recoveredCongestions;cooldownUntil=millis()+200;
            }
            txPhase=TxPhase::Accepted;
        } else if (p->conf.status==ESP_GATT_NO_RESOURCES) {
            // In GATTS_HandleValueNotification this fails while allocating the
            // ATT message, before L2CAP accepts it. Safe to retry in order.
            ++rejectedRetries;cooldownUntil=millis()+200;
            txPhase=TxPhase::RetryRejected;
        } else {
            txFailure=TxFailure::AsyncError;sendFault=true;txPhase=TxPhase::Fatal;
        }
        break;
    }
    case ESP_GATTS_WRITE_EVT:
        if (p->write.is_prep) break;
        if (p->write.handle==handles[0][11] || p->write.handle==handles[0][15] || p->write.handle==handles[0][8])
            readinessChanged=millis();
        if (p->write.handle == handles[0][11] && p->write.len==2) subscribed=(p->write.value[0]&1)!=0;
        if (p->write.handle == handles[0][15] && p->write.len==2) bootSubscribed=(p->write.value[0]&1)!=0;
        if (p->write.handle == handles[0][21] && p->write.len==2) mediaSubscribed=(p->write.value[0]&1)!=0;
        if (p->write.handle == handles[0][27] && p->write.len==2) mouseSubscribed=(p->write.value[0]&1)!=0;
        if ((p->write.handle == handles[0][17] || p->write.handle == handles[0][24]) && p->write.len==1) leds=p->write.value[0];
        if (p->write.handle == handles[0][6] && p->write.len==1) suspended=p->write.value[0]==0;
        if (p->write.handle == handles[0][8] && p->write.len==1) protocolMode=p->write.value[0];
        break;
    default: break;
    }
}

void stopped() {
    // Try to release held keys even if the preceding report failed.
    if (bluetooth_manager::owns(lease) && linked && secure && !congested) {
        uint8_t neutral[8]={};
        esp_ble_gatts_send_indicate(iface,conn,handles[0][protocolMode==0?14:10],8,neutral,false);
    }
    active=false; linked=false; secure=false; subscribed=false; error=false;
    pairing=BuBlePairing::None; pendingReports=0;txPhase=TxPhase::Idle;txFailure=TxFailure::None;readinessChanged=millis();
}
bool sessionValid() {
    return bluetooth_manager::owns(lease) && active && linked && secure && !error;
}
bool subscribedFor(uint16_t handle) {
    if (handle==handles[0][10]) return protocolMode!=0 && subscribed;
    if (handle==handles[0][14]) return protocolMode==0 && bootSubscribed;
    if (handle==handles[0][20]) return protocolMode!=0 && mediaSubscribed;
    if (handle==handles[0][26]) return protocolMode!=0 && mouseSubscribed;
    return false;
}
bool pumpWait(uint32_t started,bool cleanup) {
    if (millis()-started>=40) recovering=true;
    if (!cleanup && recovering && waitCallback && millis()-lastWaitPump>=10) {
        lastWaitPump=millis();
        if (!waitCallback(waitContext)) {
            txFailure=TxFailure::Cancelled;sendFault=true;recovering=false;return false;
        }
    }
    delay(1);
    return true;
}
bool sendReport(uint16_t handle, const uint8_t* data, uint16_t size, bool /*subscription*/, bool neutral=false) {
    if (!data || size>sizeof(txBytes) || !size || !sessionValid() || (sendFault && !neutral)) return false;
    // An unresolved accepted packet is ambiguous: never queue another key or
    // neutral packet over it. Explicit user reconnect is the safe recovery.
    if (txPhase==TxPhase::Pending) return false;
    const unsigned session=epoch.load();
    if (scheduleEpoch!=session) { scheduleEpoch=session;windowReports=nextReport=0; }
    const uint8_t mode=protocolMode.load();
    const uint32_t started=millis();
    const bool cleanup=sendFault && neutral;
    const uint32_t budget=cleanup ? 100u : RECOVERY_MS;
    uint32_t retryAt=started;
    unsigned backoff=2;
    txPhase=TxPhase::Idle;recovering=false;
    while (millis()-started<budget) {
        if (epoch.load()!=session || !sessionValid()) {
            txFailure=TxFailure::LinkLost;break;
        }
        if (protocolMode!=mode) { txFailure=TxFailure::ProtocolChanged;break; }
        const auto phase=txPhase.load();
        if (phase==TxPhase::Accepted) {
            txPhase=TxPhase::Idle;recovering=false;return true;
        }
        if (phase==TxPhase::Fatal || (sendFault && !cleanup)) break;
        if (phase==TxPhase::RetryRejected) {
            txPhase=TxPhase::Idle;retryAt=millis()+backoff;backoff=std::min(32u,backoff*2);
        }
        if (txPhase!=TxPhase::Pending && !suspended && subscribedFor(handle) &&
            uint32_t(millis()-readinessChanged.load())>=READY_SETTLE_MS &&
            int32_t(millis()-retryAt)>=0) {
            const bool cooling=int32_t(cooldownUntil.load()-millis())>0;
            const bool credit=windowReports<REPORT_WINDOW || millis()-reportTimes[nextReport]>=intervalMs.load();
            if (!congested && credit && (!cooling || !windowReports || millis()-lastAcceptedAt>=intervalMs.load())) {
                txHandle=handle;txSession=session;txSize=size;memcpy(txBytes,data,size);
                pendingReports=1;txPhase=TxPhase::Pending;
                const esp_err_t result=esp_ble_gatts_send_indicate(iface,conn,handle,size,const_cast<uint8_t*>(data),false);
                if (result==ESP_OK) {
                    reportTimes[nextReport]=millis();nextReport=(nextReport+1)%REPORT_WINDOW;
                    if(windowReports<REPORT_WINDOW) ++windowReports;
                    lastAcceptedAt=millis();
                    // Wait for this packet's result before its release/next key.
                    // The callback is a local-stack result, not a full radio interval.
                    continue;
                }
                pendingReports=0;txPhase=TxPhase::Idle;lastError=result;
                if (result==ESP_ERR_INVALID_STATE || result==ESP_ERR_INVALID_ARG) {
                    txFailure=TxFailure::AsyncError;break;
                }
                ++rejectedRetries;cooldownUntil=millis()+200;
                retryAt=millis()+backoff;backoff=std::min(32u,backoff*2);
            }
        }
        if (!pumpWait(started,cleanup)) return false;
    }
    if (txPhase==TxPhase::Accepted && epoch.load()==session && sessionValid() && protocolMode==mode) {
        txPhase=TxPhase::Idle;recovering=false;return true;
    }
    if (txPhase==TxPhase::Pending) txFailure=TxFailure::CompletionUnknown;
    else if (txFailure==TxFailure::None) txFailure=TxFailure::BusyTimeout;
    sendFault=true;recovering=false;return false;
}

}
namespace keyboard_transport {
bool ready() {
    return bluetooth_manager::owns(lease) && active && linked && secure &&
        (protocolMode==0?bootSubscribed.load():subscribed.load()) && !suspended && !error &&
        uint32_t(millis()-readinessChanged.load())>=READY_SETTLE_MS;
}
}
bool bu_ble_begin() {
    if (bluetooth_manager::owns(lease)) return !error;
    lease=bluetooth_manager::acquire(bluetooth_manager::Owner::BadUsb,"MeowKit BadUSB",gap,gatts,stopped);
    if (!lease) return false;
    active=true; linked=false; secure=false; subscribed=false; bootSubscribed=false; error=false;
    suspended=false; congested=false; sendFault=false; mediaSubscribed=false; mouseSubscribed=false;
    leds=0; lastError=0; disconnectReason=0; intervalMs=30;
    services=advReady=0; iface=ESP_GATT_IF_NONE; startedAt=millis();windowReports=nextReport=0;scheduleEpoch=epoch.load();
    pairing=BuBlePairing::None;pendingReports=0;txPhase=TxPhase::Idle;txFailure=TxFailure::None;
    recovering=false;recoveredCongestions=0;rejectedRetries=0;cooldownUntil=0;readinessChanged=millis();
    databases();
    adv={}; adv.adv_int_min=0x30; adv.adv_int_max=0x60;
    // A dedicated, deterministic static-random identity avoids the PC reusing
    // the Air Mouse profile's cached GATT handles/report map for BadUSB.
    uint8_t address[6];
    esp_read_mac(address,ESP_MAC_BT); address[0]|=0xC0; address[5]^=0xB8;
    if (esp_ble_gap_set_rand_addr(address)!=ESP_OK) { error=true; bu_ble_end(); return false; }
    adv.adv_type=ADV_TYPE_IND; adv.own_addr_type=BLE_ADDR_TYPE_RANDOM;
    adv.channel_map=ADV_CHNL_ALL; adv.adv_filter_policy=ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    uint8_t auth=ESP_LE_AUTH_REQ_SC_MITM_BOND, io=ESP_IO_CAP_IO, mask=ESP_BLE_ENC_KEY_MASK|ESP_BLE_ID_KEY_MASK, size=16;
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,&auth,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,&io,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,&size,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,&mask,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,&mask,1)!=ESP_OK) {
        bu_ble_end();return false;
    }
    static uint8_t ad[]={2,1,6,3,3,0x12,0x18,3,0x19,0xC1,3};
    static uint8_t name[]={15,9,'M','e','o','w','K','i','t',' ','B','a','d','U','S','B'};
    if (esp_ble_gap_config_adv_data_raw(ad,sizeof(ad))!=ESP_OK ||
        esp_ble_gap_config_scan_rsp_data_raw(name,sizeof(name))!=ESP_OK ||
        esp_ble_gatts_app_register(APP_ID)!=ESP_OK) error=true;
    if (error) { bu_ble_end(); return false; }
    return true;
}
void bu_ble_end() {
    waitCallback=nullptr;waitContext=nullptr;recovering=false;
    bluetooth_manager::release(lease); lease=0; active=false;
}
bool bu_ble_connected() { return keyboard_transport::ready(); }
bool bu_ble_failed() {
    return error || sendFault || bluetooth_manager::failed() ||
        (active && services!=2 && millis()-startedAt>5000);
}
bool bu_ble_keyboard_report(const uint8_t data[8]) {
    bool neutral=true; for(unsigned n=0;n<8;++n) if(data[n]) neutral=false;
    return sendReport(handles[0][protocolMode==0?14:10],data,8,
        protocolMode==0?bootSubscribed.load():subscribed.load(),neutral);
}
bool bu_ble_mouse_report(uint8_t buttons,int8_t x,int8_t y,int8_t wheel) {
    const uint8_t data[4]={buttons,uint8_t(x),uint8_t(y),uint8_t(wheel)};
    return sendReport(handles[0][26],data,4,protocolMode!=0 && mouseSubscribed,!buttons&&!x&&!y&&!wheel);
}
bool bu_ble_consumer_report(uint16_t usage) {
    const uint8_t data[2]={uint8_t(usage),uint8_t(usage>>8)};
    return sendReport(handles[0][20],data,2,protocolMode!=0 && mediaSubscribed,!usage);
}
uint8_t bu_ble_leds() { return leds.load(); }
const char* bu_ble_error() {
    if (error || bluetooth_manager::failed()) return "BLE setup failed. A to retry.";
    if (!linked) return "BLE link lost. Pair, then Run.";
    if (!secure) return "BLE pairing/encryption not ready";
    if (suspended) return "Host suspended Bluetooth HID";
    if (txFailure==TxFailure::CompletionUnknown) return "BLE completion unknown. Retry link.";
    if (txFailure==TxFailure::BusyTimeout) return "BLE busy too long. A Retry.";
    if (txFailure==TxFailure::Cancelled) return "BLE stopped";
    if (txFailure==TxFailure::AsyncError) return "BLE HID rejected report. A Retry.";
    if (txFailure==TxFailure::ProtocolChanged) return "BLE protocol changed. A Retry.";
    if (sendFault) return "BLE TX stopped. A to retry.";
    return "Host HID subscription not ready";
}
bool bu_ble_clear_error() {
    if (!keyboard_transport::ready() || txPhase==TxPhase::Pending) return false;
    sendFault=false; lastError=0;txFailure=TxFailure::None;txPhase=TxPhase::Idle;recovering=false;
    return true;
}

BuBleDiagnostics bu_ble_diagnostics() {
    return {linked.load(),secure.load(),protocolMode==0?bootSubscribed.load():subscribed.load(),
        mediaSubscribed.load(),mouseSubscribed.load(),congested.load(),disconnectReason.load(),
        intervalMs.load(),lastError.load(),recoveredCongestions.load(),rejectedRetries.load()};
}

BuBlePin bu_ble_pairing() { return {pairing.load(),passkey.load()}; }
void bu_ble_pairing_reply(bool accept) {
    const auto mode=pairing.exchange(BuBlePairing::None);
    if (!linked || !bluetooth_manager::owns(lease)) return;
    if (mode==BuBlePairing::Compare) {
        if (esp_ble_confirm_reply(peer,accept)!=ESP_OK) esp_ble_gap_disconnect(peer);
    } else if (mode==BuBlePairing::Display && !accept) esp_ble_gap_disconnect(peer);
}
bool bu_ble_wait_idle() {
    const unsigned session=epoch.load();
    const uint32_t started=millis();
    while (pendingReports && millis()-started<RECOVERY_MS) {
        if (epoch.load()!=session || !sessionValid() || sendFault || !pumpWait(started,false)) return false;
    }
    recovering=false;
    if (pendingReports) { txFailure=TxFailure::CompletionUnknown;sendFault=true; }
    return !sendFault && epoch.load()==session && sessionValid();
}
void bu_ble_set_wait_callback(BuBleWaitCallback callback,void* context) { waitCallback=callback;waitContext=context; }
bool bu_ble_recovering() { return recovering.load(); }
bool bu_ble_session_alive() { return sessionValid(); }
bool bu_ble_requires_reconnect() { return txPhase==TxPhase::Pending || txFailure==TxFailure::CompletionUnknown; }
bool bu_ble_reconnect() {
    if (!bluetooth_manager::owns(lease)) return false;
    if (!linked) return true;
    return esp_ble_gap_disconnect(peer)==ESP_OK;
}
