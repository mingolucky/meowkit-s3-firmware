#pragma once
#include <cstdio>
inline void delay(unsigned) {}
inline struct SerialType {
    template<class... A> void printf(const char*,A...) {}
    void println(const char*) {}
} Serial;
