#include "../../src/app/app_12/ble_scan_data.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace ble_scan;

static Observation report(std::initializer_list<uint8_t> bytes, uint32_t at=100) {
    Observation r{};
    r.address[5]=1;
    r.rssi=-65;
    r.at=at;
    r.advLength=static_cast<uint8_t>(bytes.size());
    assert(bytes.size()<=sizeof(r.data));
    std::copy(bytes.begin(),bytes.end(),r.data);
    return r;
}

int main() {
    auto r=report({2,1,6,5,9,'T','e','s','t'});
    auto d=decode(r);
    assert(std::strcmp(d.name,"Test")==0 && !d.isPal());

    // Exact little-endian PAL GATT service UUID in a valid 128-bit UUID AD field.
    r=report({17,7,0x00,0x12,0x9b,0x5f,0x80,0x00,0x00,0x80,
              0x00,0x10,0x00,0x00,0x10,0xaf,0x23,0x9c});
    d=decode(r);
    assert(d.palService && d.isPal() && !d.palId[0]);

    // PAL's binary 4G serial in manufacturer data, little endian.
    r=report({2,1,6,10,0xff,0,0,'4','G',0x15,0xcd,0x5b,0x07,0});
    d=decode(r);
    assert(std::strcmp(d.palId,"4G123456789")==0);
    assert(d.matchesTarget());

    r=report({12,9,'4','G','1','2','3','4','5','6','7','8','9'});
    d=decode(r);
    assert(std::strcmp(d.palId,"4G123456789")==0 && d.matchesTarget());

    // A name hint must remain distinguishable from a PAL protocol/ID match.
    d=decode(report({8,9,'P','a','l','G','a','t','e'}));
    assert(d.palName && !d.palService && !d.palId[0]);
    d=decode(report({7,9,'4','G','1','2','3','4'}));
    assert(!d.palId[0] && !d.isPal()); // A short text name is not a binary serial.

    // Truncated fields cannot invent names, UUIDs or controller IDs.
    d=decode(report({31,9,'P','a','l'}));
    assert(!d.name[0] && !d.isPal());
    d=decode(report({2,7,0}));
    assert(!d.palService);
    d=decode(report({3,0xff,'4','G'}));
    assert(!d.palId[0]);
    d=decode(report({17,0x21,'4','G','1','2','3','4',0,0,0,0,0,0,0,0,0,0}));
    assert(!d.isPal()); // UUID-only service data has no controller-ID payload.
    d=decode(report({7,0x16,'4','G','1','2','3','4'}));
    assert(!d.isPal()); // The 16-bit service UUID is not payload either.
    d=decode(report({7,0xff,'4','G','1','2','3','4'}));
    assert(!d.isPal()); // Nor is the manufacturer company identifier.
    r=report({0,12,9,'4','G','1','2','3','4','5','6','7','8','9'});
    assert(!decode(r).matchesTarget());

    Table table;
    r=report({5,9,'T','e','s','t'},10);
    table.observe(r);
    r=report({2,1,6},20);
    r.rssi=-42;
    table.observe(r);
    assert(table.count()==1 && table[0].packets==2);
    assert(std::strcmp(table[0].name,"Test")==0);
    assert(table[0].rssi==-42 && table[0].lastSeen==20);
    table.observe(report({3,8,'T','e'},21));
    assert(std::strcmp(table[0].name,"Test")==0); // Complete name survives a shortened report.
    r.addressType=1;
    table.observe(r);
    assert(table.count()==2); // Identical bytes with a different BLE address type.

    // A scan response preserves service evidence already observed on the advertiser.
    table.clear();
    auto pal=report({17,7,0x00,0x12,0x9b,0x5f,0x80,0x00,0x00,0x80,
                    0x00,0x10,0x00,0x00,0x10,0xaf,0x23,0x9c});
    table.observe(pal);
    auto rsp=report({},200);
    rsp.responseLength=5;
    const uint8_t name[]={4,9,'G','a','t'};
    std::memcpy(rsp.data,name,sizeof(name));
    table.observe(rsp);
    assert(table[0].palService && std::strcmp(table[0].name,"Gat")==0);
    assert(table[0].advLength==18 && table[0].responseLength==5);

    // Keep PAL evidence when a crowded area fills the fixed table.
    for (unsigned i=2;i<Table::Capacity+20;++i) {
        r=report({2,1,6},i);
        r.address[4]=static_cast<uint8_t>(i>>8);
        r.address[5]=static_cast<uint8_t>(i);
        table.observe(r);
    }
    assert(table.count()==Table::Capacity);
    bool retained=false;
    for(unsigned i=0;i<table.count();++i) retained|=table[i].palService;
    assert(retained);

    // Exercise every advertised length with arbitrary bytes under ASan/UBSan.
    for (unsigned len=0;len<=Observation::MaxData;++len) {
        for(unsigned n=0;n<256;++n) {
            r={}; r.advLength=static_cast<uint8_t>(len);
            for(unsigned j=0;j<len;++j) r.data[j]=static_cast<uint8_t>(n+j*31);
            (void)decode(r);
        }
    }
    puts("BLE advertisement and bounded-table tests passed");
}
