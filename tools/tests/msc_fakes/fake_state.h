#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <condition_variable>
namespace msc_fake {
inline bool usbConnected=true,failDisconnect=false,failReconnect=false;
inline unsigned disconnects=0,reconnects=0;
inline bool failDeinit=false;
inline bool failClock=false;inline unsigned maximumClock=10000;
inline bool host=false,present=false,begun=false,failBegin=false,failInit=false;
inline unsigned clock=0,allocations=0,deinits=0,ends=0,remounts=0,dmaAllocations=0;
inline std::atomic<unsigned> activeReads{0},callbackLogs{0};
inline thread_local bool callbackThread=false;
inline std::mutex barrierMutex;inline std::condition_variable barrier;
inline bool blockRead=false,readEntered=false,releaseRead=false;
}
