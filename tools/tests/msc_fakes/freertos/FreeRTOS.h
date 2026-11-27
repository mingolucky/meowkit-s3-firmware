#pragma once
#include <mutex>
#include <chrono>
#include <cstdint>
using TickType_t=uint32_t;constexpr int pdTRUE=1;constexpr TickType_t portMAX_DELAY=UINT32_MAX;
#define pdMS_TO_TICKS(n) (n)
struct StaticSemaphore_t {std::timed_mutex mutex;};using SemaphoreHandle_t=StaticSemaphore_t*;
inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* s){return s;}
inline int xSemaphoreTake(SemaphoreHandle_t s,TickType_t wait){if(wait==portMAX_DELAY){s->mutex.lock();return pdTRUE;}return s->mutex.try_lock_for(std::chrono::milliseconds(wait))?pdTRUE:0;}
inline void xSemaphoreGive(SemaphoreHandle_t s){s->mutex.unlock();}
