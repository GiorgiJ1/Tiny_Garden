#pragma once
#include <raylib.h>

enum class Sfx { Till, Plant, Water, Harvest, Ribbit, Chirp };

// Every sound is synthesised at startup, so there are no asset files.
// If no audio device is available everything quietly becomes a no-op.
class Audio {
public:
    void init();      // after InitWindow
    void shutdown();  // before CloseWindow
    void play(Sfx s);
    void update(float rain, float night);  // ambient loops: wind + rain
    void toggleMute();
    bool muted() const { return mute; }

private:
    bool ready = false;
    bool mute = false;
    Sound sfx[6]{};
    Sound rainLoop{};
    Sound windLoop{};
};
