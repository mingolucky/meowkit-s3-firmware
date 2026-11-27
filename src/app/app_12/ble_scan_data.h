#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ble_scan {
#ifndef MEOW_BLE_TARGET_ID
#define MEOW_BLE_TARGET_ID ""
#endif
inline constexpr const char* TargetId=MEOW_BLE_TARGET_ID;
struct Observation {
    static constexpr unsigned MaxData=62;
    uint8_t address[6]{};
    uint8_t addressType=0, advLength=0, responseLength=0;
    int16_t rssi=0;
    uint32_t at=0;
    uint8_t data[MaxData]{};
};
struct Device {
    uint8_t address[6]{}, addressType=0;
    char name[33]{}, palId[16]{};
    bool palService=false, palName=false, completeName=false;
    int16_t rssi=0;
    uint32_t lastSeen=0, packets=0;
    uint8_t adv[31]{}, response[31]{}, advLength=0, responseLength=0;
    bool isPal() const { return palService || palName || palId[0]; }
    bool matchesTarget() const { return TargetId[0] && std::strcmp(palId,TargetId)==0; }
    bool sameAddress(const uint8_t* a,uint8_t type) const {
        return type==addressType && std::memcmp(address,a,6)==0;
    }
};
inline char upper(uint8_t c) { return c>='a' && c<='z' ? char(c-'a'+'A') : char(c); }
inline bool digit(uint8_t c) { return c>='0' && c<='9'; }
inline void findPalId(Device& out,const uint8_t* p,size_t n,bool allowBinary) {
    for(size_t i=0;i+2<n;++i) {
        const char a=upper(p[i]), b=upper(p[i+1]);
        const bool binary=(a=='4' && b=='G') || (a=='B' && b=='W');
        if(!binary && !((a=='B' || a=='A') && b=='T')) continue;
        bool text=i+11<=n;
        for(size_t j=2;text && j<11;++j) text=digit(p[i+j]);
        if(text && (i+11==n || !digit(p[i+11]))) {
            out.palId[0]=a; out.palId[1]=b;
            std::memcpy(out.palId+2,p+i+2,9); out.palId[11]=0;
            return;
        }
        if(allowBinary && binary && i+6<=n) {
            const uint32_t serial=uint32_t(p[i+2]) | (uint32_t(p[i+3])<<8) |
                (uint32_t(p[i+4])<<16) | (uint32_t(p[i+5])<<24);
            if(serial>=100000000 && serial<=999999999) {
                std::snprintf(out.palId,sizeof(out.palId),"%c%c%lu",a,b,
                              static_cast<unsigned long>(serial));
                return;
            }
        }
    }
}
inline void parseAd(Device& out,const uint8_t* bytes,size_t size) {
    static constexpr uint8_t palUuid[]={0x00,0x12,0x9b,0x5f,0x80,0x00,0x00,0x80,
        0x00,0x10,0x00,0x00,0x10,0xaf,0x23,0x9c};
    for(size_t at=0;at<size;) {
        const size_t length=bytes[at++];
        if(!length || length>size-at) break;
        const uint8_t type=bytes[at];
        const uint8_t* p=bytes+at+1;
        const size_t n=length-1;
        if(type==0x08 || type==0x09) {
            // Prefer a complete name to a shortened name within one report.
            if(type==0x09 || !out.name[0]) {
                const size_t count=std::min(n,sizeof(out.name)-1);
                for(size_t i=0;i<count;++i) out.name[i]=p[i]>=32 && p[i]<127 ? char(p[i]) : '?';
                out.name[count]=0;
                out.completeName=type==0x09;
            }
        }
        if(type==0x06 || type==0x07 || type==0x21) {
            const size_t limit=type==0x21 ? std::min(n,size_t(16)) : n;
            for(size_t i=0;i+16<=limit;i+=16)
                if(std::memcmp(p+i,palUuid,16)==0) out.palService=true;
        }
        if(type==0x08 || type==0x09) findPalId(out,p,n,false);
        if(type==0xff || type==0x16 || type==0x21) {
            const size_t header=type==0x21 ? 16 : 2;
            if(n>header) findPalId(out,p+header,n-header,true);
        }
        at+=length;
    }
}
inline Device decode(const Observation& r) {
    Device d{};
    std::memcpy(d.address,r.address,6); d.addressType=r.addressType;
    d.rssi=r.rssi; d.lastSeen=r.at; d.packets=1;
    if(r.advLength>31 || r.responseLength>31) return d;
    parseAd(d,r.data,r.advLength);
    parseAd(d,r.data+r.advLength,r.responseLength);
    d.advLength=r.advLength; d.responseLength=r.responseLength;
    std::memcpy(d.adv,r.data,r.advLength);
    std::memcpy(d.response,r.data+r.advLength,r.responseLength);
    char name[sizeof(d.name)];
    for(size_t i=0;i<sizeof(name);++i) name[i]=upper(static_cast<uint8_t>(d.name[i]));
    d.palName=std::strstr(name,"PALGATE") || std::strstr(name,"PAL GATE") ||
        std::strncmp(name,"PAL-",4)==0 || std::strncmp(name,"PAL ",4)==0;
    return d;
}
class Table {
public:
    static constexpr unsigned Capacity=96;
    unsigned count() const { return count_; }
    const Device& operator[](unsigned i) const { return devices_[i]; }
    void clear() { count_=0; }
    unsigned palCount() const {
        unsigned n=0;
        for(unsigned i=0;i<count_;++i) n+=devices_[i].isPal();
        return n;
    }
    void observe(const Observation& r) {
        if(r.advLength>31 || r.responseLength>31) return;
        Device next=decode(r);
        for(unsigned i=0;i<count_;++i) {
            Device& old=devices_[i];
            if(!old.sameAddress(r.address,r.addressType)) continue;
            old.rssi=next.rssi; old.lastSeen=r.at; ++old.packets;
            if(next.name[0] && (next.completeName || !old.completeName)) {
                std::memcpy(old.name,next.name,sizeof(old.name)); old.completeName=next.completeName;
            }
            if(next.palId[0]) std::memcpy(old.palId,next.palId,sizeof(old.palId));
            old.palService|=next.palService; old.palName|=next.palName;
            if(r.advLength) { old.advLength=r.advLength; std::memcpy(old.adv,next.adv,r.advLength); }
            if(r.responseLength) { old.responseLength=r.responseLength; std::memcpy(old.response,next.response,r.responseLength); }
            return;
        }
        if(count_<Capacity) { devices_[count_++]=next; return; }
        // Prefer replacing old generic observations to losing a gate candidate.
        unsigned replace=Capacity;
        uint32_t oldestAge=0;
        for(unsigned i=0;i<count_;++i) {
            if(devices_[i].isPal()) continue;
            const uint32_t age=r.at-devices_[i].lastSeen;
            if(replace==Capacity || age>oldestAge) { replace=i; oldestAge=age; }
        }
        if(replace==Capacity) {
            if(!next.isPal()) return;
            replace=0;
            for(unsigned i=1;i<count_;++i)
                if(r.at-devices_[i].lastSeen>r.at-devices_[replace].lastSeen) replace=i;
        }
        devices_[replace]=next;
    }
private:
    std::array<Device,Capacity> devices_{};
    unsigned count_=0;
};
inline void addressText(const uint8_t* a,char (&text)[18]) {
    std::snprintf(text,sizeof(text),"%02X:%02X:%02X:%02X:%02X:%02X",a[0],a[1],a[2],a[3],a[4],a[5]);
}
} // namespace ble_scan
