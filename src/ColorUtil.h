#pragma once
#include <raylib.h>

#include <algorithm>

inline Color mixColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto l = [&](unsigned char x, unsigned char y) {
        return (unsigned char)(x + (y - x) * t + 0.5f);
    };
    return {l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), l(a.a, b.a)};
}
