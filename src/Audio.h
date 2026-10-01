#pragma once
#include <raylib.h>

enum class Sfx { Till, Plant, Water, Harvest };

class Audio {
public:
    void init();
    void shutdown();
    void play(Sfx s);
    void update(float rain, float night);
    void toggleMute();
    bool muted() const { return mute; }

private:
    bool ready = false;
    bool mute = false;
    Sound sfx[4]{};
    Sound rainLoop{};
    Sound windLoop{};
};
