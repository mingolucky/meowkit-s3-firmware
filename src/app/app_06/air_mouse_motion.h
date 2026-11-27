#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace air_mouse {
struct Report { int x = 0, y = 0; };

// Keep sub-pixel movement and unsent displacement; bound backlog to one extra
// report so a stalled link cannot keep moving the cursor long after a gesture.
struct Motion {
    float x = 0, y = 0;
    void clear() { x = y = 0; }
    void add(float wx, float wy, float dt, float gain) {
        if (!std::isfinite(wx) || !std::isfinite(wy) || dt <= 0 || dt > .05f) {
            clear(); return;
        }
        auto smooth = [](float v) {
            constexpr float noise = .15f;
            return std::fabs(v) <= noise ? 0.f : v - std::copysign(noise, v);
        };
        x += smooth(wx) * dt * gain;
        y += smooth(wy) * dt * gain;
        const float scale = std::max(std::fabs(x), std::fabs(y)) / 254.f;
        if (scale > 1) { x /= scale; y /= scale; }
    }
    Report pending() const {
        const float scale = std::max(1.f, std::max(std::fabs(x), std::fabs(y)) / 127.f);
        // Only send whole counts. Rounding +0.5 to +1 leaves -0.5, which
        // would send -1 on the next tick and oscillate even at rest.
        return {static_cast<int>(x / scale), static_cast<int>(y / scale)};
    }
    void sent(Report r) { x -= r.x; y -= r.y; }
};

struct Calibration {
    unsigned count = 0;
    float mean[3] = {}, m2[3] = {};
    void reset() { *this = Calibration{}; }
    bool add(float x, float y, float z, float acc) {
        const float v[3] = {x, y, z};
        if (!std::isfinite(acc) || !std::isfinite(x) || !std::isfinite(y) ||
            !std::isfinite(z) || std::fabs(acc - 1.f) > .08f ||
            x*x + y*y + z*z > 100.f) { reset(); return false; }
        // Estimate the zero-rate offset from stable samples, not proximity to
        // zero: rejecting offsets above 0.8 dps prevents learning that bias.
        ++count;
        for (unsigned i=0; i<3; ++i) {
            const float d = v[i] - mean[i];
            mean[i] += d / count;
            m2[i] += d * (v[i] - mean[i]);
        }
        if (count < 150) return false;
        for (unsigned i=0; i<3; ++i) {
            if (m2[i] / (count - 1) > .01f) { reset(); return false; }
        }
        return true;
    }
};
} // namespace air_mouse
