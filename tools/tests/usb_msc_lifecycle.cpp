#include "msc_fakes/Arduino.h"
#include "msc_fakes/USBMSC.h"
extern "C" {
#include "msc_fakes/sdmmc_cmd.h"
#include "msc_fakes/esp_heap_caps.h"
}
#include "../../src/system/usb_msc.h"
#include <future>
#include <array>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
extern "C" bool tud_disconnect(){assert(!msc_fake::callbackThread);if(msc_fake::failDisconnect)return false;msc_fake::usbConnected=false;++msc_fake::disconnects;return true;}
extern "C" bool tud_connect(){assert(!msc_fake::callbackThread);++msc_fake::reconnects;if(msc_fake::failReconnect)return false;msc_fake::usbConnected=true;return true;}
extern "C" esp_err_t sdmmc_host_init(){msc_fake::host=true;return 0;}
extern "C" esp_err_t sdmmc_host_init_slot(int,const sdmmc_slot_config_t*){return 0;}
extern "C" esp_err_t sdmmc_host_set_card_clk(int slot,uint32_t frequency){assert(slot==1);if(msc_fake::failClock || frequency>msc_fake::maximumClock)return 7;msc_fake::clock=frequency;return 0;}
extern "C" esp_err_t sdmmc_host_deinit(){if(msc_fake::failDeinit)return 7;assert(!msc_fake::activeReads);msc_fake::host=false;++msc_fake::deinits;return 0;}
extern "C" esp_err_t sdmmc_card_init(const sdmmc_host_t* host,sdmmc_card_t* card){assert(host->max_freq_khz==10000);msc_fake::clock=400;card->csd.capacity=16000000;card->csd.sector_size=512;return msc_fake::failInit?7:0;}
extern "C" esp_err_t sdmmc_read_sectors(sdmmc_card_t* card,void* out,size_t,size_t count){
 assert(msc_fake::host && card && count && count<=8 && uintptr_t(out)%4==0);++msc_fake::activeReads;
 {std::unique_lock<std::mutex> lock(msc_fake::barrierMutex);if(msc_fake::blockRead){msc_fake::readEntered=true;msc_fake::barrier.notify_all();msc_fake::barrier.wait(lock,[]{return msc_fake::releaseRead;});}}
 memset(out,0x5a,count*512);--msc_fake::activeReads;return 0;
}
extern "C" esp_err_t sdmmc_write_sectors(sdmmc_card_t* card,const void* in,size_t,size_t count){assert(msc_fake::host && card && count && count<=8 && uintptr_t(in)%4==0);return 0;}
extern "C" void* heap_caps_malloc(size_t n,unsigned caps){++msc_fake::allocations;if(caps&MALLOC_CAP_DMA){assert(n==4096 && (caps&MALLOC_CAP_INTERNAL));++msc_fake::dmaAllocations;}return malloc(n);}
bool sdmmcEnsureReady(){assert(!usb_msc_is_active());++msc_fake::remounts;return true;}
int main(){
 assert(usb_msc_enable());assert(usb_msc_is_active() && msc_fake::present && msc_fake::clock==10000 && msc_fake::dmaAllocations==1);
 assert(msc_fake::disconnects==0 && msc_fake::reconnects==0 && msc_fake::usbConnected);
 auto* msc=USBMSC::instance;std::array<uint8_t,65> packet{};assert(msc->read(16,64,packet.data()+1,64)==64);assert(packet[1]==0x5a && usb_msc_bytes_transferred()==64);
 msc_fake::callbackThread=true;
 assert(msc->startStop(0,false,true));assert(!msc_fake::present && usb_msc_is_active() && msc_fake::host);
 assert(msc->read(16,0,packet.data(),64)==-1);
 assert(msc->startStop(0,true,true));assert(msc_fake::present);
 msc_fake::callbackThread=false;
 msc_fake::blockRead=true;
 auto reader=std::async(std::launch::async,[&]{msc_fake::callbackThread=true;return msc->read(16,0,packet.data()+1,64);});
 {std::unique_lock<std::mutex> lock(msc_fake::barrierMutex);msc_fake::barrier.wait(lock,[]{return msc_fake::readEntered;});}
 auto stopping=std::async(std::launch::async,[]{usb_msc_disable();});assert(stopping.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout);
 {std::lock_guard<std::mutex> lock(msc_fake::barrierMutex);msc_fake::releaseRead=true;msc_fake::barrier.notify_all();}
 assert(reader.get()==64);stopping.get();assert(!usb_msc_is_active() && !msc_fake::present && !msc_fake::host);
 assert(msc->read && msc->read(16,0,packet.data(),64)==-1);assert(msc_fake::callbackLogs==0 && msc_fake::ends==0);
 assert(!msc->startStop(0,true,true));assert(!msc_fake::present && !msc_fake::host);
 msc_fake::blockRead=false;msc_fake::failBegin=true;assert(!usb_msc_enable());assert(!msc_fake::present && !msc_fake::host && !usb_msc_is_active());
 msc_fake::failBegin=false;msc_fake::failInit=true;assert(!usb_msc_enable());assert(!msc_fake::present && !msc_fake::host && !usb_msc_is_active());
 msc_fake::failInit=false;msc_fake::failClock=true;assert(!usb_msc_enable());assert(!msc_fake::present && !msc_fake::host && !usb_msc_is_active());
 msc_fake::failClock=false;msc_fake::maximumClock=5000;assert(usb_msc_enable() && msc_fake::clock==5000);usb_msc_disable();
 msc_fake::maximumClock=1000;assert(usb_msc_enable() && msc_fake::clock==1000);usb_msc_disable();
 msc_fake::failReconnect=false;msc_fake::maximumClock=10000;msc_fake::failBegin=true;msc_fake::failDeinit=true;
 assert(!usb_msc_enable());assert(usb_msc_is_active() && msc_fake::host && !msc_fake::present);
 assert(!msc->startStop(0,true,true));assert(msc->read(16,0,packet.data(),64)==-1);
 msc_fake::failBegin=false;msc_fake::failDeinit=false;usb_msc_disable();assert(!usb_msc_is_active() && !msc_fake::host && !msc_fake::present);
 assert(msc_fake::disconnects==0 && msc_fake::reconnects==0);
 puts("USB MSC lifecycle checks passed (clock selection, eject/load, CDC preserved, failure gating)");
}
