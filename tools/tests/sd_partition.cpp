#include "../../src/system/sd_partition.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace meow_storage;
static std::array<uint8_t,512> load(const char* p) { std::array<uint8_t,512> s{}; FILE* f=fopen(p,"rb"); assert(f); assert(fread(s.data(),1,512,f)==512); fclose(f); return s; }
int main() {
 auto m=load("tools/tests/sd_fixtures/sector0.bin"),v=load("tools/tests/sd_fixtures/sector16.bin");
 auto read=[&](uint32_t lba,uint8_t* out) { if(lba!=16)return false; memcpy(out,v.data(),512);return true; };
 auto p=selectPartition(m.data(),15495168,read);
 assert(p.kind==PartitionKind::Partition && p.index==1 && p.start==16 && p.sectors==15495152 && p.legacy);
 m[510]=0; assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);m[510]=0x55;
 assert(selectPartition(m.data(),100,read).kind==PartitionKind::Reject);
 m[450]=0x83; assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);m[450]=0x0c;
 v[13]=3; assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);v[13]=8;
 auto original=m;
 // A malformed claimed MBR must not become writable superfloppy fallback just
 // because sector zero also contains a valid, physically bounded FAT BPB.
 for(unsigned byte=0;byte<4;++byte)m[32+byte]=uint8_t(15495168u>>(8*byte));
 assert(fatBootSector(m.data(),15495168));
 const auto bounded=m;
 m[446]=0x7f;
 assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);
 m=bounded;memset(m.data()+454,0,4);
 assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);
 m=bounded;memset(m.data()+458,0,4);
 assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);m=original;
 m[446]=0x7f; assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);m=original;
 assert(selectPartition(m.data(),15495168,[](uint32_t,uint8_t*){return false;}).kind==PartitionKind::Reject);
 memcpy(m.data()+462,m.data()+446,16);assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);m=original;
 memcpy(m.data()+462,m.data()+446,16);memset(m.data()+446,0,16);
 p=selectPartition(m.data(),15495168,read);assert(p.kind==PartitionKind::Partition && p.index==2);m=original;
 v[510]=0;assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);v[510]=0x55;
 memset(m.data()+446,0,64); // real superfloppy remains supported, including old oversized BPB rejection
 assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Reject);
 m=v; // Retain genuine Windows FAT boot code in the partition-table region.
 assert(selectPartition(m.data(),15495168,read).kind==PartitionKind::Superfloppy);
 puts("SD partition checks passed");
}
