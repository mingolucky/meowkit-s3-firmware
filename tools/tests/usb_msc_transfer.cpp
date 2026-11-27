#include "../../src/system/usb_msc_transfer.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>
using namespace meow_storage;
struct Disk {
 std::map<uint64_t,std::array<uint8_t,512>> sectors;
 unsigned reads=0,writes=0;int readFailure=0,writeFailure=0;uint64_t lastSector=0;size_t lastCount=0;
 std::array<uint8_t,512>& sector(uint64_t lba) {auto pair=sectors.emplace(lba,std::array<uint8_t,512>{});if(pair.second)for(unsigned i=0;i<512;++i)pair.first->second[i]=uint8_t(lba+i);return pair.first->second;}
 int read(uint8_t* out,uint64_t lba,size_t count){assert(count && count<=8 && uintptr_t(out)%4==0);++reads;lastSector=lba;lastCount=count;if(readFailure)return readFailure;for(size_t n=0;n<count;++n)memcpy(out+n*512,sector(lba+n).data(),512);return 0;}
 int write(const uint8_t* in,uint64_t lba,size_t count){assert(count && count<=8 && uintptr_t(in)%4==0);++writes;lastSector=lba;lastCount=count;if(writeFailure)return writeFailure;for(size_t n=0;n<count;++n)memcpy(sector(lba+n).data(),in+n*512,512);return 0;}
};
int main(){
 Disk disk;alignas(4) uint8_t scratch[4096];std::array<uint8_t,10001> buffer{};
 auto transfer=[&](bool write,uint32_t lba,uint32_t offset,uint32_t size,uint64_t capacity=16000000){return mscTransfer(write,lba,offset,buffer.data()+1,size,capacity,scratch,sizeof(scratch),[&](uint8_t* out,uint64_t sector,size_t n){return disk.read(out,sector,n);},[&](const uint8_t* in,uint64_t sector,size_t n){return disk.write(in,sector,n);});};
 assert(transfer(false,16,64,64).bytes==64);for(unsigned i=0;i<64;++i)assert(buffer[i+1]==disk.sector(16)[64+i]);assert(disk.lastCount==1);
 assert(transfer(false,16,500,64).bytes==64);for(unsigned i=0;i<64;++i)assert(buffer[i+1]==disk.sector(16+(500+i)/512)[(500+i)%512]);
 assert(transfer(false,16,1024+32,64).bytes==64);assert(disk.lastSector==18);assert(buffer[1]==disk.sector(18)[32]);
 auto original=disk.sector(3);std::fill(buffer.begin(),buffer.end(),0xa5);assert(transfer(true,3,111,64).bytes==64);for(unsigned i=0;i<512;++i)assert(disk.sector(3)[i]==(i>=111 && i<175?0xa5:original[i]));
 auto beforeA=disk.sector(4),beforeB=disk.sector(5);assert(transfer(true,4,500,64).bytes==64);for(unsigned i=0;i<512;++i){assert(disk.sector(4)[i]==(i>=500?0xa5:beforeA[i]));assert(disk.sector(5)[i]==(i<52?0xa5:beforeB[i]));}
 auto reads=disk.reads;assert(transfer(true,100,0,8192).bytes==8192 && disk.reads==reads && disk.lastCount==8);assert(transfer(false,100,0,8192).bytes==8192);for(unsigned i=0;i<8192;++i)assert(buffer[i+1]==0xa5);
 assert(transfer(false,8388607,500,64).bytes==64 && disk.lastSector==8388608); // crosses 4GiB byte boundary
 assert(transfer(false,99,500,12,100).bytes==12);assert(transfer(false,99,500,13,100).error==MscTransferError::OutOfRange);
 assert(transfer(false,100,0,0,100).bytes==0);reads=disk.reads;assert(transfer(false,0,0,0).bytes==0 && disk.reads==reads);
 assert(transfer(false,UINT32_MAX,UINT32_MAX,64).error==MscTransferError::OutOfRange);assert(transfer(false,0,0,64,UINT64_MAX).error==MscTransferError::InvalidArgument);
 assert(transfer(false,0,0,UINT32_MAX).error==MscTransferError::InvalidArgument);
 disk.readFailure=7;auto failed=transfer(false,0,0,64);assert(failed.bytes==-1 && failed.error==MscTransferError::Read && failed.ioError==7);
 auto writes=disk.writes;assert(transfer(true,0,1,64).error==MscTransferError::Read && disk.writes==writes);disk.readFailure=0;
 disk.writeFailure=8;failed=transfer(true,0,0,512);assert(failed.bytes==-1 && failed.error==MscTransferError::Write && failed.ioError==8);
 puts("USB MSC byte transfer checks passed");
}
