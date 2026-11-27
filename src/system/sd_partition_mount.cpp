#ifdef ARDUINO
#include "sd_partition_mount.h"
#include "sd_partition.h"
#include "sd_recovery.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include "ff.h"
#include "diskio.h"
namespace {
bool blocked=false;
constexpr const char* kRoot="/sdcard";
constexpr const char* kPending=".meowkit-sd-recovery-pending-v1";
constexpr const char* kComplete=".meowkit-sd-recovery-complete-v1";
void printHex(const uint8_t* data,size_t size) {
 static const char digits[]="0123456789ABCDEF";char out[257];
 while(size){size_t n=size>128?128:size;for(size_t i=0;i<n;++i){out[i*2]=digits[data[i]>>4];out[i*2+1]=digits[data[i]&15];}out[n*2]='\0';Serial.print(out);data+=n;size-=n;}
}
void printPath(const char* path){printHex(reinterpret_cast<const uint8_t*>(path),strlen(path));}
bool switchVolume(FATFS* fs,uint8_t partition,uint32_t expected) {
 // VFS owns this FATFS allocation. Reuse it; never register a stack FATFS.
 if(f_mount(nullptr,"0:",0)!=FR_OK)return false;
 VolToPart[0].pt=partition;
 return f_mount(fs,"0:",1)==FR_OK && fs->volbase==expected;
}
bool recover() {
 FF_DIR directory{};if(f_opendir(&directory,"0:/")!=FR_OK)return false;
 FATFS* fs=directory.obj.fs;const uint8_t drive=fs->pdrv;
 if(f_closedir(&directory)!=FR_OK || drive!=VolToPart[0].pd)return false;
 uint8_t sector0[512];if(disk_read(drive,sector0,0,1)!=RES_OK)return false;
 const auto selection=meow_storage::selectPartition(sector0,SD_MMC.cardSize()/512,[&](uint32_t sector,uint8_t* out){return disk_read(drive,out,sector,1)==RES_OK;});
 if(selection.kind==meow_storage::PartitionKind::Reject){Serial.println("[SD RECOVERY] REFUSED invalid/unsupported partition geometry");return false;}
 if(fs->volbase!=selection.start && !switchVolume(fs,selection.index,selection.start))return false;
 Serial.printf("[SD PARTITION] selected=%u start=%lu sectors=%lu legacy=%u\n",selection.index,(unsigned long)selection.start,(unsigned long)selection.sectors,selection.legacy);
 Serial.printf("[SD FAT] volbase=%lu fatbase=%lu fsize=%lu n_fats=%u\n",(unsigned long)fs->volbase,(unsigned long)fs->fatbase,(unsigned long)fs->fsize,unsigned(fs->n_fats));
 for(unsigned fat=0;fat<fs->n_fats;++fat){
  const uint64_t sector=uint64_t(fs->fatbase)+uint64_t(fat)*fs->fsize;
  uint8_t head[512];
  if(sector>=SD_MMC.cardSize()/512 || disk_read(drive,head,DWORD(sector),1)!=RES_OK){Serial.printf("[SD FAT] FAT%u read failed at sector=%llu\n",fat+1,(unsigned long long)sector);return false;}
  Serial.printf("[SD FAT] FAT%u sector=%llu entries=%08lX,%08lX,%08lX\n",fat+1,(unsigned long long)sector,(unsigned long)meow_storage::sdLE32(head),(unsigned long)meow_storage::sdLE32(head+4),(unsigned long)meow_storage::sdLE32(head+8));
 }
 if(!selection.legacy)return true;
 char identity[128];snprintf(identity,sizeof(identity),"MEOWKIT SD RECOVERY v1\npartition=%u start=%lu sectors=%lu\n",selection.index,(unsigned long)selection.start,(unsigned long)selection.sectors);
 const int complete=meow_storage::readRecoveryMarker(kRoot,kComplete,identity),pending=meow_storage::readRecoveryMarker(kRoot,kPending,identity);
 if(complete==1){Serial.println("[SD RECOVERY] Previously verified recovery; legacy view stays closed");return true;}
 if(complete<0 || pending!=0){Serial.println("[SD RECOVERY] REFUSED interrupted/unrecognized recovery marker; restore PC backup before explicit repair");return false;}
 if(!switchVolume(fs,0,0))return false;
 Serial.println("[SD INVENTORY] begin");unsigned files=0,directories=0;uint64_t bytes=0;bool unknown=false;
 const bool inventoried=meow_storage::inventoryLegacy(kRoot,[&](const char* path,bool directory,uint64_t size){
  Serial.print("[SD INVENTORY] path_hex=");printPath(path);Serial.printf(" kind=%s size=%llu known=%u\n",directory?"directory":"file",(unsigned long long)size,meow_storage::knownRecoveryPath(path));
  if(directory)++directories;else {++files;bytes+=size;if(size && !meow_storage::knownRecoveryPath(path))unknown=true;}
 });
 Serial.printf("[SD INVENTORY] end ok=%u files=%u directories=%u bytes=%llu unknown_nonempty=%u\n",inventoried,files,directories,(unsigned long long)bytes,unknown);
 meow_storage::RecoverySnapshot snapshot;
 const bool captured=snapshot.capture(kRoot);
 if(captured){
  Serial.printf("[SD BACKUP] begin files=%u memory_bytes=%u\n",snapshot.fileCount(),unsigned(snapshot.memoryBytes()));unsigned id=0;
  snapshot.emit([&](const char* path,const uint8_t* data,size_t size){
   if(!data)return;
   Serial.printf("[SD BACKUP] file=%u path_hex=",id);printPath(path);Serial.printf(" size=%u crc32=%08lX\n",unsigned(size),(unsigned long)meow_storage::recoveryCRC32(data,size));
   for(size_t offset=0;offset<size;offset+=128){size_t n=size-offset;if(n>128)n=128;Serial.printf("[SD BACKUP] file=%u offset=%u hex=",id,unsigned(offset));printHex(data+offset,n);Serial.println();delay(1);}
   Serial.printf("[SD BACKUP] file=%u end\n",id++);
  });
  Serial.printf("[SD BACKUP] end files=%u\n",snapshot.fileCount());Serial.flush();
 }
 // Close legacy before deciding whether writes can proceed. Its contents are
 // never changed, even if traversal, allocation, or snapshot bounds fail.
 if(!switchVolume(fs,selection.index,selection.start))return false;
 if(!inventoried || !captured || unknown){Serial.printf("[SD RECOVERY] REFUSED inventory=%u snapshot=%u unknown=%u; no migration writes\n",inventoried,captured,unknown);return false;}
 // Serial output is an export, not proof that the host durably received it.
 // Keep BOTH overlapping views unchanged until the host verifies its backup,
 // restores through explicit raw MSC, and writes the complete marker last.
 Serial.printf("[SD RECOVERY] NEEDS_HOST_RESTORE files=%u; verify durable PC backup, restore canonical partition through raw USB MSC, then write COMPLETE marker last; no migration writes\n",snapshot.fileCount());
 return false;
}
}
void sdPartitionSelectInitial(bool canonical){VolToPart[0].pt=canonical?1:0;}
bool sdPartitionBlocked(){return blocked;}
bool sdPartitionRecoverMounted(){if(blocked)return false;if(recover())return true;blocked=true;Serial.println("[SD RECOVERY] Storage disabled for this boot to prevent unbacked writes");return false;}
#endif
