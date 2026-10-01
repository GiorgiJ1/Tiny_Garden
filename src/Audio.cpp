#include "Audio.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr int kRate = 44100;
constexpr float kTwoPi = 6.2831853f;

float noise() { return float(GetRandomValue(-1000, 1000)) / 1000.0f; }

Sound synth(float seconds, const std::function<float(float)>& gen) {
    const int n = int(seconds * float(kRate));

    Wave w{};
    w.frameCount = unsigned(n);
    w.sampleRate = kRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = MemAlloc(unsigned(n) * sizeof(short));

    short* s = static_cast<short*>(w.data);
    for (int i = 0; i < n; i++) {
        float v = std::clamp(gen(float(i) / float(kRate)), -1.0f, 1.0f);
        s[i] = short(v * 32000.0f);
    }

    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    return snd;
}

}

void Audio::init() {
    InitAudioDevice();
    ready = IsAudioDeviceReady();
    if (!ready) return;

    sfx[int(Sfx::Till)] = synth(0.14f, [lp = 0.0f](float t) mutable -> float {
        lp += (noise() - lp) * 0.18f;
        return lp * std::exp(-t * 22.0f) * 2.2f;
    });

    sfx[int(Sfx::Plant)] = synth(0.16f, [ph = 0.0f](float t) mutable -> float {
        float f = 90.0f + 260.0f * std::exp(-t * 28.0f);
        ph += kTwoPi * f / float(kRate);
        return std::sin(ph) * std::exp(-t * 16.0f) * 0.9f;
    });

    sfx[int(Sfx::Water)] = synth(0.32f, [lp = 0.0f](float t) mutable -> float {
        lp += (noise() - lp) * 0.45f;
        return lp * std::sin(3.14159f * t / 0.32f) * 0.6f;
    });

    sfx[int(Sfx::Harvest)] = synth(0.5f, [](float t) -> float {
        auto note = [&](float f, float t0) -> float {
            if (t < t0) return 0.0f;
            float u = t - t0;
            return (std::sin(kTwoPi * f * u) + 0.3f * std::sin(kTwoPi * f * 2.0f * u)) * std::exp(-u * 8.0f);
        };
        return (note(660.0f, 0.0f) + note(880.0f, 0.09f)) * 0.35f;
    });

    rainLoop = synth(2.0f, [lp = 0.0f](float) mutable -> float {
        float n = noise();
        lp += (n - lp) * 0.25f;
        return (n - lp) * 0.35f;
    });

    windLoop = synth(6.0f, [lp = 0.0f](float t) mutable -> float {
        lp += (noise() - lp) * 0.012f;
        float amp = 0.65f + 0.35f * std::sin(kTwoPi * t / 6.0f);
        float edge = std::min({1.0f, t / 0.05f, (6.0f - t) / 0.05f});
        return lp * 7.0f * amp * edge;
    });

    for (Sound& s : sfx) SetSoundVolume(s, 0.5f);
}

void Audio::shutdown() {
    if (!ready) return;
    for (Sound& s : sfx) UnloadSound(s);
    UnloadSound(rainLoop);
    UnloadSound(windLoop);
    CloseAudioDevice();
    ready = false;
}

void Audio::play(Sfx s) {
    if (ready) PlaySound(sfx[int(s)]);
}

void Audio::update(float rain, float night) {
    if (!ready) return;

    if (!IsSoundPlaying(windLoop)) PlaySound(windLoop);
    SetSoundVolume(windLoop, 0.12f * (1.0f - 0.4f * night));

    const float r = std::clamp(rain, 0.0f, 1.0f);
    if (r > 0.03f) {
        if (!IsSoundPlaying(rainLoop)) PlaySound(rainLoop);
        SetSoundVolume(rainLoop, 0.45f * r);
    } else if (IsSoundPlaying(rainLoop)) {
        StopSound(rainLoop);
    }
}

void Audio::toggleMute() {
    mute = !mute;
    if (ready) SetMasterVolume(mute ? 0.0f : 1.0f);
}
