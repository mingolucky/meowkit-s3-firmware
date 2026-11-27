#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
namespace meow_storage {
enum class MscTransferError { None, InvalidArgument, OutOfRange, Read, Write };
struct MscTransferResult { int32_t bytes=-1;MscTransferError error=MscTransferError::InvalidArgument;int ioError=0;uint64_t sector=0; };
// TinyUSB offsets and buffers are byte-oriented, including 64-byte packets.
// Every driver call uses the caller-owned aligned DMA scratch, never the USB
// buffer. Partial writes use read/modify/write; aligned spans use bulk sectors.
template<class Read,class Write> MscTransferResult mscTransfer(bool writing,uint32_t lba,uint32_t offset,uint8_t* buffer,uint32_t size,uint64_t capacity,uint8_t* scratch,size_t scratchSize,Read read,Write write) {
 MscTransferResult result;
 if(!capacity || capacity>std::numeric_limits<uint64_t>::max()/512 || (size && !buffer) || !scratch || uintptr_t(scratch)%4 || scratchSize<512 || scratchSize%512 || size>uint32_t(std::numeric_limits<int32_t>::max()))return result;
 const uint64_t start=uint64_t(lba)*512+offset,total=capacity*512;
 if(start>total || uint64_t(size)>total-start){result.error=MscTransferError::OutOfRange;return result;}
 uint64_t position=start;uint32_t remaining=size;
 while(remaining) {
  const uint64_t sector=position/512;const size_t within=size_t(position%512);
  result.sector=sector;size_t count=0;
  if(within || remaining<512) {
   count=512-within;if(count>remaining)count=remaining;
   result.ioError=read(scratch,sector,1);
   if(result.ioError){result.error=MscTransferError::Read;return result;}
   if(writing) {
    memcpy(scratch+within,buffer,count);result.ioError=write(scratch,sector,1);
    if(result.ioError){result.error=MscTransferError::Write;return result;}
   } else memcpy(buffer,scratch+within,count);
  } else {
   size_t sectors=remaining/512;if(sectors>scratchSize/512)sectors=scratchSize/512;
   count=sectors*512; // remaining>=512 and validated scratch ensure sectors>0.
   if(writing) {
    memcpy(scratch,buffer,count);result.ioError=write(scratch,sector,sectors);
    if(result.ioError){result.error=MscTransferError::Write;return result;}
   } else {
    result.ioError=read(scratch,sector,sectors);
    if(result.ioError){result.error=MscTransferError::Read;return result;}
    memcpy(buffer,scratch,count);
   }
  }
  buffer+=count;position+=count;remaining-=uint32_t(count);
 }
 result.bytes=int32_t(size);result.error=MscTransferError::None;return result;
}
}
