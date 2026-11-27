#include "air_mouse_hid.h"
#include "air_mouse_subscription.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <Preferences.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_device.h>
#include <mbedtls/sha256.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

// The pinned ESP-IDF 4.4 Bluedroid has no public encryption-change callback.
// Run its security query on the BTU task, which owns the security records.
extern "C" bool BTM_GetSecurityFlagsByTransport(uint8_t* address, uint8_t* flags, uint8_t transport);
extern "C" void bta_dmexecutecallback(void (*callback)(void*), void* context);
extern "C" uint8_t smp_get_state();

namespace {
constexpr uint16_t APP_ID = 0x600;
constexpr uint8_t BTM_LE_TRANSPORT = 2;
constexpr uint8_t BTM_ENCRYPTED = 0x04;
constexpr uint8_t SMP_IDLE = 0;
constexpr uint32_t LINK_CHECK_MS = 50;
std::atomic<bool> active{false}, linked{false}, secure{false}, subscribed{false}, error{false};
std::atomic<bool> bootSubscribed{false};
std::atomic<bool> congested{false}, suspended{false};
std::atomic<unsigned> epoch{0};
std::atomic<unsigned> encryptedEpoch{0}, queryEpoch{0};
esp_gatt_if_t iface = ESP_GATT_IF_NONE;
uint16_t conn = 0, handles[2][16] = {};
uint8_t peer[6] = {};
portMUX_TYPE peerMux = portMUX_INITIALIZER_UNLOCKED;
std::mutex sessionMutex;
air_mouse::Subscription notifications;
bool restorePending = true, subscriptionsDirty = false, authenticationFailed = false;
bool newBondKeys = false, pairingComplete = false;
uint32_t lastLinkCheck = 0, lastEncryptionQuery = 0;
unsigned services = 0, advReady = 0;
uint32_t lastSend = 0, startedAt = 0;
unsigned seenEpoch = 0;
uint8_t desiredButtons = 0, sentButtons = 0, queue[16] = {};
unsigned head = 0, tail = 0;
uint16_t serviceUuid[] = {0x1812, 0x180A};
uint16_t primary = 0x2800, declaration = 0x2803;
uint16_t infoUuid = 0x2A4A, mapUuid = 0x2A4B, controlUuid = 0x2A4C;
uint16_t protocolUuid = 0x2A4E, reportUuid = 0x2A4D, cccUuid = 0x2902, refUuid = 0x2908;
uint16_t bootUuid = 0x2A33;
uint16_t manufacturerUuid = 0x2A29, pnpUuid = 0x2A50;
uint8_t readProp = ESP_GATT_CHAR_PROP_BIT_READ;
uint8_t writeProp = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t protocolProp = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
uint8_t reportProp = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
uint8_t info[] = {0x11, 0x01, 0, 2}, control = 0, protocol = 1;
std::atomic<uint8_t> protocolMode{1};
uint8_t report[5] = {}, ccc[2] = {}, ref[] = {0, 1};
uint8_t bootReport[3] = {}, bootCcc[2] = {};
uint8_t manufacturer[] = "MeowKit", pnp[] = {2, 2, 0xE5, 0x11, 0xA1, 0x10, 2};
// Battery level is not exposed: the board has no validated percentage source.
// Report protocol matches the previous five-byte mouse descriptor. Boot
// protocol exposes three-byte pointer reports for hosts that request it.
uint8_t reportMap[] = {
    0x05,0x01,0x09,0x02,0xA1,0x01,0x09,0x01,0xA1,0x00,
    0x05,0x09,0x19,0x01,0x29,0x05,0x15,0x00,0x25,0x01,
    0x95,0x05,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x03,0x81,0x03,
    0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,0x15,0x81,0x25,0x7F,
    0x75,0x08,0x95,0x03,0x81,0x06,0x05,0x0C,0x0A,0x38,0x02,
    0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x01,0x81,0x06,0xC0,0xC0
};
esp_gatts_attr_db_t hidDb[16], disDb[5];
esp_ble_adv_params_t adv = {};

bool linkEncrypted() {
    return linked && !authenticationFailed && encryptedEpoch==epoch;
}

void readLinkEncryption(void* context) {
    const unsigned requested = reinterpret_cast<uintptr_t>(context);
    // Do not acquire the app mutex on the BTU task. Take only a short copy
    // of the peer address, and discard results from a closed connection.
    uint8_t address[sizeof(peer)];
    portENTER_CRITICAL(&peerMux);
    std::memcpy(address, peer, sizeof(address));
    portEXIT_CRITICAL(&peerMux);
    if (active && linked && epoch==requested) {
        uint8_t flags = 0;
        // HCI can report encryption before SMP finishes exchanging a new
        // bond's keys. Wait for SMP to finish before restoring its CCC state.
        const bool encrypted = smp_get_state()==SMP_IDLE &&
            BTM_GetSecurityFlagsByTransport(address, &flags, BTM_LE_TRANSPORT) && (flags & BTM_ENCRYPTED);
        if (active && linked && epoch==requested) encryptedEpoch=encrypted ? requested : 0;
    }
    unsigned pending = requested;
    queryEpoch.compare_exchange_strong(pending, 0);
}

void requestLinkEncryption(uint32_t now, bool encrypted) {
    if (!linked || authenticationFailed || now-lastEncryptionQuery<(encrypted ? 250u : LINK_CHECK_MS)) return;
    const unsigned current = epoch;
    if (queryEpoch==current && now-lastEncryptionQuery<500) return;
    queryEpoch=current;
    lastEncryptionQuery=now;
    bta_dmexecutecallback(readLinkEncryption, reinterpret_cast<void*>(static_cast<uintptr_t>(current)));
}

struct SavedSubscriptions {
    uint8_t bondTag[8] = {};
    uint8_t flags = 0;
};

bool bondKey(char (&key)[14], SavedSubscriptions& state) {
    int count = esp_ble_get_bond_device_num();
    if (count <= 0) return false;
    std::vector<esp_ble_bond_dev_t> bonds(count);
    if (esp_ble_get_bond_device_list(&count, bonds.data()) != ESP_OK) return false;
    for (int i = 0; i < count; ++i) {
        const auto& bond = bonds[i];
        if (std::memcmp(peer, bond.bd_addr, sizeof(peer)) != 0 &&
            (!(bond.bond_key.key_mask & ESP_LE_KEY_PID) ||
             std::memcmp(peer, bond.bond_key.pid_key.static_addr, sizeof(peer)) != 0)) continue;
        std::snprintf(key, sizeof(key), "c%02x%02x%02x%02x%02x%02x",
                      bond.bd_addr[0], bond.bd_addr[1], bond.bd_addr[2],
                      bond.bd_addr[3], bond.bd_addr[4], bond.bd_addr[5]);
        // Bind settings to this bond generation where the stack exposes its
        // peer LTK. NVS stores only a short fingerprint, never the key.
        if (bond.bond_key.key_mask & ESP_LE_KEY_PENC) {
            uint8_t digest[32];
            const auto& ltk = bond.bond_key.penc_key.ltk;
            if (mbedtls_sha256_ret(ltk, sizeof(ltk), digest, 0) != 0) return false;
            std::memcpy(state.bondTag, digest, sizeof(state.bondTag));
        }
        return true;
    }
    return false;
}

void publishSubscriptions() {
    const uint8_t reportValue[] = {static_cast<uint8_t>(notifications.enabled(false)), 0};
    const uint8_t bootValue[] = {static_cast<uint8_t>(notifications.enabled(true)), 0};
    esp_ble_gatts_set_attr_value(handles[0][11], sizeof(reportValue), reportValue);
    esp_ble_gatts_set_attr_value(handles[0][15], sizeof(bootValue), bootValue);
}

// ESP-IDF writes can arrive before the new bond has been saved. Keep pending
// writes until the peer is in the stack's bond store.
void syncSubscriptions(bool updateGatt = true) {
    if (!restorePending && !subscriptionsDirty) return;
    if (newBondKeys && !pairingComplete) return;
    char key[14];
    SavedSubscriptions state;
    if (!bondKey(key, state)) return;
    Preferences prefs;
    if (!prefs.begin("airmouse-ble", !subscriptionsDirty)) {
        if (!subscriptionsDirty) restorePending = false;
        return;
    }
    SavedSubscriptions saved;
    const bool sameBond = !newBondKeys && prefs.getBytesLength(key)==sizeof(saved) &&
        prefs.getBytes(key, &saved, sizeof(saved))==sizeof(saved) &&
        std::memcmp(saved.bondTag, state.bondTag, sizeof(saved.bondTag))==0;
    if (restorePending) {
        notifications.restore(sameBond ? saved.flags : 0);
        if (updateGatt) publishSubscriptions();
        restorePending = false;
        if (sameBond && (saved.flags & air_mouse::Subscription::Mask))
            Serial.println("[Air Mouse] Loaded bonded report subscriptions");
    }
    state.flags = notifications.flags;
    if (subscriptionsDirty && ((sameBond && saved.flags == state.flags) ||
        prefs.putBytes(key, &state, sizeof(state)) == sizeof(state))) {
        subscriptionsDirty = false;
        newBondKeys = false;
    }
    prefs.end();
}

void resetSession() {
    ++epoch;
    encryptedEpoch=0; queryEpoch=0;
    secure=false; subscribed=false; bootSubscribed=false;
    suspended=false; congested=false;
    notifications={}; restorePending=true; subscriptionsDirty=false;
    authenticationFailed=false;
    newBondKeys=false; pairingComplete=false;
}

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
    attribute(hidDb[6], controlUuid, ESP_GATT_PERM_WRITE_ENCRYPTED, 1, &control);
    attribute(hidDb[7], declaration, ESP_GATT_PERM_READ, 1, &protocolProp);
    attribute(hidDb[8], protocolUuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED, 1, &protocol);
    attribute(hidDb[9], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[10], reportUuid, ESP_GATT_PERM_READ_ENCRYPTED, sizeof(report), report);
    attribute(hidDb[11], cccUuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED, 2, ccc);
    attribute(hidDb[12], refUuid, ESP_GATT_PERM_READ, 2, ref);
    attribute(hidDb[13], declaration, ESP_GATT_PERM_READ, 1, &reportProp);
    attribute(hidDb[14], bootUuid, ESP_GATT_PERM_READ_ENCRYPTED, 3, bootReport);
    attribute(hidDb[15], cccUuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED, 2, bootCcc);
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
    const std::lock_guard<std::mutex> lock(sessionMutex);
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
    case ESP_GAP_BLE_SEC_REQ_EVT: esp_ble_gap_security_rsp(p->ble_security.ble_req.bd_addr, true); break;
    case ESP_GAP_BLE_KEY_EVT:
        if (linked && !newBondKeys && std::memcmp(peer, p->ble_security.ble_key.bd_addr, sizeof(peer))==0) {
            // A new key exchange supersedes anything inherited from the old
            // bond, but any CCC writes on this connection still take priority.
            newBondKeys=true; pairingComplete=false;
            notifications.restore(0);
            subscribed=notifications.enabled(false);
            bootSubscribed=notifications.enabled(true);
            subscriptionsDirty=true;
            publishSubscriptions();
        }
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        const auto& auth = p->ble_security.auth_cmpl;
        if (!linked || std::memcmp(peer, auth.bd_addr, sizeof(peer)) != 0) break;
        if (auth.success) {
            pairingComplete=true;
            Serial.println("[Air Mouse] Pairing succeeded");
        } else {
            authenticationFailed=true;
            secure=false;
            Serial.printf("[Air Mouse] Pairing failed: 0x%02x\n", auth.fail_reason);
            esp_ble_gap_disconnect(p->ble_security.auth_cmpl.bd_addr);
        }
        break;
    }
    default: break;
    }
}
void gatts(esp_gatts_cb_event_t e, esp_gatt_if_t i, esp_ble_gatts_cb_param_t* p) {
    const std::lock_guard<std::mutex> lock(sessionMutex);
    if (!active) return;
    if (e == ESP_GATTS_REG_EVT) {
        if (p->reg.app_id != APP_ID) return;
        if (p->reg.status != ESP_GATT_OK) { error=true; return; }
        iface=i;
        if (esp_ble_gatts_create_attr_tab(hidDb, i, 16, 0) != ESP_OK ||
            esp_ble_gatts_create_attr_tab(disDb, i, 5, 1) != ESP_OK) error=true;
        return;
    }
    if (i != iface && i != ESP_GATT_IF_NONE) return;
    switch(e) {
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
        unsigned id=p->add_attr_tab.svc_inst_id;
        if (p->add_attr_tab.status != ESP_GATT_OK || id>1 || p->add_attr_tab.num_handle != (id ? 5 : 16)) { error=true; break; }
        memcpy(handles[id], p->add_attr_tab.handles, p->add_attr_tab.num_handle*sizeof(uint16_t));
        if (esp_ble_gatts_start_service(handles[id][0]) != ESP_OK) error=true;
        break;
    }
    case ESP_GATTS_START_EVT:
        if (p->start.status != ESP_GATT_OK) { error=true; break; }
        ++services; advertise(); break;
    case ESP_GATTS_CONNECT_EVT: {
        if (linked) {
            if (conn != p->connect.conn_id) esp_ble_gap_disconnect(p->connect.remote_bda);
            break;
        }
        resetSession();
        conn=p->connect.conn_id;
        portENTER_CRITICAL(&peerMux);
        memcpy(peer,p->connect.remote_bda,sizeof(peer));
        portEXIT_CRITICAL(&peerMux);
        linked=true;
        protocol=1; protocolMode=1;
        publishSubscriptions();
        esp_ble_gatts_set_attr_value(handles[0][8],1,&protocol);
        const uint8_t emptyReport[sizeof(report)] = {};
        esp_ble_gatts_set_attr_value(handles[0][10], sizeof(report), emptyReport);
        esp_ble_gatts_set_attr_value(handles[0][14], sizeof(bootReport), emptyReport);
        // Use the HID example's encryption mode for a device with no PIN I/O.
        // Leave connection timing to the host while the security exchange runs.
        const auto result = esp_ble_set_encryption(peer, ESP_BLE_SEC_ENCRYPT_NO_MITM);
        Serial.printf("[Air Mouse] Link opened, encryption request: 0x%x\n", result);
        if (result != ESP_OK) esp_ble_gap_disconnect(peer);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        if (linked && p->disconnect.conn_id == conn) {
            Serial.printf("[Air Mouse] Link closed: 0x%02x\n", p->disconnect.reason);
            // CCC writes reached this callback only after the GATT layer's
            // encryption check. Save a write even if the host left before tick.
            if (subscriptionsDirty) syncSubscriptions(false);
            linked=false; resetSession(); advertise();
        }
        break;
    case ESP_GATTS_CONGEST_EVT:
        if (linked && p->congest.conn_id == conn) congested=p->congest.congested;
        break;
    case ESP_GATTS_WRITE_EVT:
        if (!linked || p->write.conn_id != conn || p->write.is_prep) break;
        if (p->write.handle == handles[0][11] || p->write.handle == handles[0][15]) {
            if (p->write.len == 2 && p->write.value[0] <= 1 && p->write.value[1] == 0) {
                const bool boot = p->write.handle == handles[0][15];
                notifications.write(boot ? air_mouse::Subscription::Boot : air_mouse::Subscription::Report,
                                    p->write.value[0] == 1);
                subscribed=notifications.enabled(false);
                bootSubscribed=notifications.enabled(true);
                subscriptionsDirty = true;
            } else publishSubscriptions();
        }
        if (p->write.handle == handles[0][6] && p->write.len==1) suspended=p->write.value[0]==0;
        if (p->write.handle == handles[0][8] && p->write.len==1 && p->write.value[0]<=1)
            protocolMode=p->write.value[0];
        break;
    default: break;
    }
}
bool send(uint8_t buttons, int8_t x=0, int8_t y=0, int8_t wheel=0) {
    const std::lock_guard<std::mutex> lock(sessionMutex);
    if (!air_mouse_hid::ready() || congested || !linkEncrypted()) return false;
    uint8_t data[5]={buttons, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(wheel),0};
    const bool boot=protocolMode==0;
    if (boot && wheel) return false;
    if(boot) data[0]&=7;
    const uint16_t handle=handles[0][boot?14:10];
    const uint16_t length=boot?3:5;
    const bool ok=esp_ble_gatts_send_indicate(iface,conn,handle,length,data,false)==ESP_OK;
    if(ok) esp_ble_gatts_set_attr_value(handle,length,data);
    return ok;
}
void enqueue(uint8_t b) {
    const unsigned next=(tail+1)%16;
    if (next==head) { head=tail=0; queue[tail++]=0; desiredButtons=0; return; }
    queue[tail]=b; tail=next;
}
}
namespace air_mouse_hid {
bool begin() {
    if (active) return !error;
    // Applications run exclusively. Reinitialize Arduino's stack bookkeeping,
    // then install app-owned callbacks and static attributes.
    BLEDevice::deinit(false);
    BLEDevice::init("MeowKit AirMouse");
    linked=false; resetSession(); error=false;
    services=advReady=0; iface=ESP_GATT_IF_NONE; head=tail=0;
    desiredButtons=sentButtons=0; startedAt=millis(); lastSend=startedAt;
    lastLinkCheck=lastEncryptionQuery=startedAt;
    seenEpoch=epoch.load(); databases();
    adv={}; adv.adv_int_min=0x30; adv.adv_int_max=0x60;
    adv.adv_type=ADV_TYPE_IND; adv.own_addr_type=BLE_ADDR_TYPE_RANDOM;
    adv.channel_map=ADV_CHNL_ALL; adv.adv_filter_policy=ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    uint8_t auth=ESP_LE_AUTH_BOND, io=ESP_IO_CAP_NONE, keys=ESP_BLE_ENC_KEY_MASK|ESP_BLE_ID_KEY_MASK, size=16;
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,&auth,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,&io,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,&size,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,&keys,1)!=ESP_OK ||
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,&keys,1)!=ESP_OK) {
        error=true;
        BLEDevice::deinit(false);
        return false;
    }
    static uint8_t ad[]={2,1,6,3,3,0x12,0x18,3,0x19,0xC2,3};
    static uint8_t name[]={17,9,'M','e','o','w','K','i','t',' ','A','i','r','M','o','u','s','e'};
    // A stable, app-specific static random identity prevents host HID caches
    // from confusing the mouse with another firmware Bluetooth profile.
    const uint8_t* baseAddress=esp_bt_dev_get_address();
    if(!baseAddress) { error=true; BLEDevice::deinit(false); return false; }
    uint8_t address[6]; memcpy(address,baseAddress,6);
    address[0]|=0xC0; address[5]^=0x06;
    active=true;
    if (esp_ble_gap_register_callback(gap)!=ESP_OK || esp_ble_gatts_register_callback(gatts)!=ESP_OK ||
        esp_ble_gap_set_rand_addr(address)!=ESP_OK ||
        esp_ble_gap_config_adv_data_raw(ad,sizeof(ad))!=ESP_OK ||
        esp_ble_gap_config_scan_rsp_data_raw(name,sizeof(name))!=ESP_OK ||
        esp_ble_gatts_app_register(APP_ID)!=ESP_OK) error=true;
    return !error;
}
void end() {
    if (!active) return;
    send(0); // Best effort; disconnect also clears HID button state on the host.
    {
        const std::lock_guard<std::mutex> lock(sessionMutex);
        if (subscriptionsDirty) syncSubscriptions(false);
        active=false;
        esp_ble_gap_stop_advertising();
        if (linked) esp_ble_gap_disconnect(peer);
    }
    BLEDevice::deinit(false); // Retain controller memory so another app can start BLE.
    linked=false; resetSession(); iface=ESP_GATT_IF_NONE;
    head=tail=0; desiredButtons=sentButtons=0;
}
bool connected() { return active && linked; }
bool ready() { return connected() && secure && (protocolMode==0?bootSubscribed.load():subscribed.load()) && !suspended && !error; }
bool scrollReady() { return ready() && protocolMode==1; }
bool failed() { return error; }
void buttons(uint8_t b) { if (b!=desiredButtons) { desiredButtons=b; enqueue(b); } }
void clickRight() { enqueue(desiredButtons|2); enqueue(desiredButtons); }
void tick() {
    if (seenEpoch!=epoch.load()) { seenEpoch=epoch.load(); head=tail=0; desiredButtons=sentButtons=0; }
    const uint32_t now = millis();
    if (active && now-lastLinkCheck>=LINK_CHECK_MS) {
        const std::lock_guard<std::mutex> lock(sessionMutex);
        lastLinkCheck=now;
        if (services!=2 && now-startedAt>5000) error=true;
        const bool encrypted = linkEncrypted();
        requestLinkEncryption(now, encrypted);
        if (encrypted && !secure) Serial.println("[Air Mouse] Link encrypted");
        secure=encrypted;
        if (encrypted) syncSubscriptions();
        subscribed=notifications.enabled(false);
        bootSubscribed=notifications.enabled(true);
    }
    if (!ready()) return;
    if (head!=tail && millis()-lastSend>=10 && send(queue[head])) {
        sentButtons=queue[head]; head=(head+1)%16; lastSend=millis();
    }
}
bool move(int8_t x,int8_t y,int8_t wheel) {
    if (head!=tail || millis()-lastSend<10) return false;
    if (!send(sentButtons,x,y,wheel)) return false;
    lastSend=millis(); return true;
}
}
