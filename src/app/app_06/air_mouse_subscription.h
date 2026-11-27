#pragma once

#include <cstdint>

namespace air_mouse {
struct Subscription {
    static constexpr uint8_t Report = 1;
    static constexpr uint8_t Boot = 2;
    static constexpr uint8_t Mask = Report | Boot;

    uint8_t flags = 0;
    uint8_t written = 0;

    bool enabled(bool boot) const { return flags & (boot ? Boot : Report); }

    void restore(uint8_t saved) {
        flags = (flags & written) | (saved & Mask & ~written);
    }

    void write(uint8_t report, bool enabled) {
        written |= report;
        if (enabled) flags |= report;
        else flags &= ~report;
    }
};
}
