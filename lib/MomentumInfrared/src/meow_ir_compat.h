#pragma once
/* MeowKit portability layer; codec algorithms remain upstream C. */
#include <assert.h>
#include <stdlib.h>

/* Furi's allocator returns cleared memory. Several codec constructors rely on
 * this before their first reset; the standard ESP32/host malloc does not. */
#define malloc(size) calloc(1, (size))

#define furi_check(condition) do { if(!(condition)) abort(); } while(0)
#define furi_assert(condition) furi_check(condition)
#define furi_crash(...) abort()
#define COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
