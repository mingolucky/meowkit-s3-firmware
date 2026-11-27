#pragma once
#include <cstddef>
#define MALLOC_CAP_DEFAULT 1
#define MALLOC_CAP_DMA 2
#define MALLOC_CAP_INTERNAL 4
#define MALLOC_CAP_8BIT 8
void* heap_caps_malloc(size_t,unsigned);
