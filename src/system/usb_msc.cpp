/** Raw SD USB MSC. Filesystem mounts stay closed while the host owns media. */
#include "usb_msc.h"
#include "usb_msc_transfer.h"
#include "sd_clock.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include <USB.h>
#include <USBMSC.h>
#include <atomic>
#include <cstdlib>
#include <initializer_list>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
extern "C" {
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_heap_caps.h"
}
#include "../bsp/config.h"
#include "../bsp/sdmmc/SDMMC_Class.hpp"

// Register MSC before USB.begin(), retaining the existing composite descriptor.
static USBMSC s_msc;
namespace {
struct MscIdentity {
 MscIdentity(){s_msc.vendorID("MeowKit");s_msc.productID("SDCard");s_msc.productRevision("1.1");}
} s_identity; // Inquiry strings must exist before the initial USB enumeration.
}
static sdmmc_card_t* s_card=nullptr;
static uint8_t* s_scratch=nullptr;
static bool s_hostInitialized=false,s_callbacksInstalled=false;
static std::atomic<int> s_running{0};
static std::atomic<bool> s_acceptIO{false};
static std::atomic<bool> s_prepared{false};
static std::atomic<unsigned long> s_bytes{0};
static constexpr size_t kScratchSize=4096;
static constexpr TickType_t kLifecycleWait=pdMS_TO_TICKS(5000);
struct IoStats {
 uint32_t reads=0,writes=0,errors=0,ejects=0;
 uint32_t lba=0,offset=0,size=0,lastLba=0,lastOffset=0,lastSize=0,maxMicros=0;uint64_t failedSector=0;
 int error=0,driverError=0;bool writing=false;
};
static IoStats s_stats;

// Persistent storage is never freed, including after media removal. Callbacks
// already dispatched by TinyUSB may arrive after teardown and safely reject IO.
static SemaphoreHandle_t ioMutex() {
 static StaticSemaphore_t storage;
 static SemaphoreHandle_t const mutex=xSemaphoreCreateMutexStatic(&storage);
 return mutex;
}
static int32_t transfer(bool writing,uint32_t lba,uint32_t offset,uint8_t* buffer,uint32_t size) {
 if(!s_acceptIO.load() || !s_prepared.load())return -1;
 const auto mutex=ioMutex();if(!mutex || xSemaphoreTake(mutex,kLifecycleWait)!=pdTRUE)return -1;
 if(!s_acceptIO.load() || !s_prepared.load() || !s_card || !s_scratch){xSemaphoreGive(mutex);return -1;}
 if(writing)++s_stats.writes;else ++s_stats.reads;
 s_stats.lastLba=lba;s_stats.lastOffset=offset;s_stats.lastSize=size;const uint32_t started=micros();
 const auto result=meow_storage::mscTransfer(writing,lba,offset,buffer,size,s_card->csd.capacity,s_scratch,kScratchSize,
  [](uint8_t* out,uint64_t sector,size_t count){return int(sdmmc_read_sectors(s_card,out,size_t(sector),count));},
  [](const uint8_t* in,uint64_t sector,size_t count){return int(sdmmc_write_sectors(s_card,in,size_t(sector),count));});
 const uint32_t elapsed=micros()-started;if(elapsed>s_stats.maxMicros)s_stats.maxMicros=elapsed;
 if(result.bytes<0){++s_stats.errors;s_stats.lba=lba;s_stats.offset=offset;s_stats.size=size;s_stats.failedSector=result.sector;s_stats.error=int(result.error);s_stats.driverError=result.ioError;s_stats.writing=writing;}
 else s_bytes.fetch_add(static_cast<unsigned long>(result.bytes));
 xSemaphoreGive(mutex);return result.bytes;
}
static int32_t _msc_read_cb(uint32_t lba,uint32_t offset,void* buffer,uint32_t size){return transfer(false,lba,offset,static_cast<uint8_t*>(buffer),size);}
static int32_t _msc_write_cb(uint32_t lba,uint32_t offset,uint8_t* buffer,uint32_t size){return transfer(true,lba,offset,buffer,size);}
static bool _msc_startstop_cb(uint8_t,bool start,bool loadEject) {
 // Never print from a TinyUSB callback: CDC also needs the TinyUSB task.
 if(!loadEject)return true;
 if(!start){s_acceptIO.store(false);s_msc.mediaPresent(false);}
 const auto mutex=ioMutex();
 if(!mutex || xSemaphoreTake(mutex,0)!=pdTRUE)return !start;
 bool ok=true;
 if(start){
  ok=s_prepared.load() && s_running.load() && s_hostInitialized && s_card && s_scratch;
  if(ok){s_acceptIO.store(true);s_msc.mediaPresent(true);}
 } else ++s_stats.ejects;
 xSemaphoreGive(mutex);
 return ok;
}
// Caller owns ioMutex and has hidden media/rejected new callbacks.
static bool releaseRaw() {
 if(s_hostInitialized){
  const auto error=sdmmc_host_deinit();
  if(error!=ESP_OK){Serial.printf("[MSC] raw host cleanup failed: %d; resources retained\n",int(error));return false;}
  s_hostInitialized=false;
 }
 free(s_card);s_card=nullptr;free(s_scratch);s_scratch=nullptr;return true;
}
static void remountFilesystem() {
 SD_MMC.setPins(HAL_PIN_SD_CLK,HAL_PIN_SD_CMD,HAL_PIN_SD_D0);
 Serial.println(sdmmcEnsureReady()?"[MSC] FS remounted":"[MSC] FS remount failed");
}
int usb_msc_enable(void) {
 const auto mutex=ioMutex();if(!mutex || xSemaphoreTake(mutex,kLifecycleWait)!=pdTRUE)return 0;
 if(s_running.load()){const bool ready=s_acceptIO.load();xSemaphoreGive(mutex);return ready?1:0;}
 s_running.store(1);s_prepared.store(false);s_acceptIO.store(false);s_msc.mediaPresent(false);s_bytes.store(0);s_stats={};
 // Change removable-media readiness without resetting the shared USB device.
 // CDC stays connected while removed-media requests fail promptly in the SDK.
 SD_MMC.end();
 sdmmc_host_t host=SDMMC_HOST_DEFAULT();
 host.max_freq_khz=10000; // Known working filesystem negotiation, rather than 20MHz.
 host.slot=SDMMC_HOST_SLOT_1;host.flags=SDMMC_HOST_FLAG_1BIT;
 sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
 slot.clk=(gpio_num_t)HAL_PIN_SD_CLK;slot.cmd=(gpio_num_t)HAL_PIN_SD_CMD;slot.d0=(gpio_num_t)HAL_PIN_SD_D0;slot.width=1;slot.flags=0;
 bool ok=false;int error=0;uint32_t selectedClock=0;const char* stage="host init";
 do {
  error=sdmmc_host_init();if(error!=ESP_OK)break;s_hostInitialized=true;
  stage="slot init";error=sdmmc_host_init_slot(SDMMC_HOST_SLOT_1,&slot);if(error!=ESP_OK)break;
  stage="card/scratch allocation";
  s_card=static_cast<sdmmc_card_t*>(heap_caps_malloc(sizeof(sdmmc_card_t),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  s_scratch=static_cast<uint8_t*>(heap_caps_malloc(kScratchSize,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA));
  if(!s_card || !s_scratch){error=-1;break;}
  memset(s_card,0,sizeof(*s_card));
  stage="card init";error=sdmmc_card_init(&host,s_card);if(error!=ESP_OK)break;
  stage="clock baseline";error=sdmmc_read_sectors(s_card,s_scratch,0,1);if(error!=ESP_OK)break;
  uint8_t baseline[512];memcpy(baseline,s_scratch,sizeof(baseline));
  stage="clock verification";
  for(uint32_t frequency:{10000u,5000u,1000u}) {
   if(meow_storage::verifySDClock(frequency,s_scratch,baseline,
      [](uint8_t* data){return int(sdmmc_read_sectors(s_card,data,0,1));},
      [](uint32_t clock){return int(sdmmc_host_set_card_clk(SDMMC_HOST_SLOT_1,clock));})) {
    selectedClock=frequency;break;
   }
  }
  if(!selectedClock){error=-1;break;}
  stage="card geometry";if(!s_card->csd.capacity || s_card->csd.sector_size!=512){error=-1;break;}
  // SDK end() clears callback pointers without synchronizing its check/call.
  // Keep pointers installed permanently, and guard readiness/resources here.
  if(!s_callbacksInstalled){s_msc.onRead(_msc_read_cb);s_msc.onWrite(_msc_write_cb);s_msc.onStartStop(_msc_startstop_cb);s_callbacksInstalled=true;}
  s_msc.vendorID("MeowKit");s_msc.productID("SDCard");s_msc.productRevision("1.1");
  stage="USBMSC.begin";if(!s_msc.begin(uint32_t(s_card->csd.capacity),512)){error=-1;break;}
  s_prepared.store(true); // Set only after verification and USBMSC.begin succeed.
  s_acceptIO.store(true); // Card, scratch, callbacks, geometry are all ready.
  s_msc.mediaPresent(true);ok=true;
 } while(false);
 const uint32_t sectors=ok?uint32_t(s_card->csd.capacity):0;
 if(!ok){
  s_acceptIO.store(false);s_msc.mediaPresent(false);
  Serial.printf("[MSC] enable failed at %s: %d\n",stage,error);
  if(releaseRaw()){s_running.store(0);remountFilesystem();}
 }
 xSemaphoreGive(mutex);
 if(ok)Serial.printf("[MSC] enabled — %lu sectors x 512 B, verified clock=%lu kHz, DMA scratch=%u\n",(unsigned long)sectors,(unsigned long)selectedClock,unsigned(kScratchSize));
 return ok?1:0;
}
void usb_msc_disable(void) {
 // Hide media first and stop queued callbacks accepting work. An existing
 // callback retains resources until it releases this same persistent mutex.
 s_prepared.store(false);s_msc.mediaPresent(false);s_acceptIO.store(false);
 const auto mutex=ioMutex();
 if(!mutex || xSemaphoreTake(mutex,kLifecycleWait)!=pdTRUE){Serial.println("[MSC] IO drain timed out; raw resources retained, retry disable");return;}
 s_msc.mediaPresent(false);s_acceptIO.store(false);
 if(!s_running.load()){xSemaphoreGive(mutex);return;}
 if(releaseRaw()){s_running.store(0);remountFilesystem();}
 xSemaphoreGive(mutex);
}
int usb_msc_is_active(void){return s_running.load();}
unsigned long usb_msc_bytes_transferred(void) {
 // Called by the UI/main task. Snapshot under the IO lock, then print outside
 // it so CDC logging can never prevent a TinyUSB callback from finishing.
 static uint32_t loggedErrors=0,loggedEjects=0,loggedReads=0,loggedWrites=0;IoStats stats;bool snapshot=false;
 const auto mutex=ioMutex();if(mutex && xSemaphoreTake(mutex,0)==pdTRUE){stats=s_stats;snapshot=true;xSemaphoreGive(mutex);}
 if(snapshot && (stats.errors!=loggedErrors || stats.ejects!=loggedEjects || uint32_t(stats.reads-loggedReads)>=256 || uint32_t(stats.writes-loggedWrites)>=256)){
  loggedErrors=stats.errors;loggedEjects=stats.ejects;loggedReads=stats.reads;loggedWrites=stats.writes;
  Serial.printf("[MSC RATE] bytes=%lu last_lba=%lu offset=%lu size=%lu max_callback_us=%lu\n",s_bytes.load(),(unsigned long)stats.lastLba,(unsigned long)stats.lastOffset,(unsigned long)stats.lastSize,(unsigned long)stats.maxMicros);
  Serial.printf("[MSC IO] reads=%lu writes=%lu errors=%lu ejects=%lu last=%s lba=%lu offset=%lu size=%lu sector=%llu error=%d sdk=%d\n",(unsigned long)stats.reads,(unsigned long)stats.writes,(unsigned long)stats.errors,(unsigned long)stats.ejects,stats.writing?"write":"read",(unsigned long)stats.lba,(unsigned long)stats.offset,(unsigned long)stats.size,(unsigned long long)stats.failedSector,stats.error,stats.driverError);
 }
 return s_bytes.load();
}
