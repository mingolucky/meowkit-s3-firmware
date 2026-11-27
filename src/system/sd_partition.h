#pragma once
#include <cstdint>
namespace meow_storage {
enum class PartitionKind { Reject, Superfloppy, Partition };
struct PartitionSelection { PartitionKind kind=PartitionKind::Reject; uint8_t index=0; uint32_t start=0,sectors=0; bool legacy=false; };
inline uint16_t sdLE16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1])<<8; }
inline uint32_t sdLE32(const uint8_t* p) { return uint32_t(sdLE16(p)) | uint32_t(sdLE16(p+2))<<16; }
// Validate FAT12/16/32 geometry without trusting a type label or doing any writes.
inline bool fatBootSector(const uint8_t* b,uint64_t available,bool enforceBounds=true) {
 if (b[510]!=0x55 || b[511]!=0xaa || (b[0]!=0xeb && b[0]!=0xe9) || sdLE16(b+11)!=512) return false;
 const uint32_t cluster=b[13],reserved=sdLE16(b+14),fats=b[16];
 const uint32_t total=sdLE16(b+19)?sdLE16(b+19):sdLE32(b+32);
 const uint32_t fat=sdLE16(b+22)?sdLE16(b+22):sdLE32(b+36);
 const uint64_t overhead=uint64_t(reserved)+uint64_t(fats)*fat+(uint32_t(sdLE16(b+17))*32+511)/512;
 if (!cluster || cluster>128 || (cluster&(cluster-1)) || !reserved || !fats || fats>2 || !fat || total<=overhead || (enforceBounds && total>available)) return false;
 const uint64_t clusters=(total-overhead)/cluster;
 if (!clusters || clusters>=0x0ffffff5) return false;
 const unsigned bits=clusters<4085?12:clusters<65525?16:32;
 if (uint64_t(fat)*512*8 < (clusters+2)*bits) return false;
 if (bits==32 && (sdLE16(b+17) || sdLE16(b+22) || sdLE32(b+44)<2 || sdLE32(b+44)>=clusters+2)) return false;
 return true;
}
inline bool fatPartitionType(uint8_t type) {
 return type==0x01 || type==0x04 || type==0x06 || type==0x0b || type==0x0c || type==0x0e;
}
template<class Read> PartitionSelection selectPartition(const uint8_t* m,uint64_t capacity,Read read) {
 if (m[510]!=0x55 || m[511]!=0xaa) return {};
 bool table=false;
 for(unsigned i=0;i<4;++i) { const auto* e=m+446+16*i; // Recognizable FAT entries claim a table even with corrupt flags or zero geometry.
 // Ordinary boot code containing no such entry keeps superfloppy fallback.
 if(fatPartitionType(e[4]) || (e[4] && (e[0]==0 || e[0]==0x80) && sdLE32(e+8) && sdLE32(e+12))) table=true; }
 if (!table) return fatBootSector(m,capacity)?PartitionSelection{PartitionKind::Superfloppy,0,0,uint32_t(capacity),false}:PartitionSelection{};
 // Refuse malformed, overlapping, or out-of-card tables before choosing a partition.
 for(unsigned i=0;i<4;++i) {
  const auto* e=m+446+16*i; if(!e[4]) { if(sdLE32(e+8)||sdLE32(e+12))return {}; continue; }
  const uint64_t start=sdLE32(e+8),length=sdLE32(e+12);
  if((e[0]!=0 && e[0]!=0x80)||!start||!length||start+length>capacity)return {};
  for(unsigned j=0;j<i;++j) {const auto* f=m+446+16*j;if(f[4] && start<uint64_t(sdLE32(f+8))+sdLE32(f+12) && sdLE32(f+8)<start+length)return {};}
 }
 for(unsigned i=0;i<4;++i) {
  const auto* e=m+446+16*i; const uint8_t t=e[4];
  if(!fatPartitionType(t))continue;
  uint8_t boot[512]; const uint32_t start=sdLE32(e+8),length=sdLE32(e+12);
  if(!read(start,boot) || !fatBootSector(boot,length))return {};
  return {PartitionKind::Partition,uint8_t(i+1),start,length,fatBootSector(m,capacity,false)};
 }
 return {};
}
}
