#pragma once
#include <raylib.h>

#include <random>

class Garden;

class Snail {
public:
    void init(Vector2 start, unsigned seed);
    void update(float dt, const Garden& garden);
    void draw(float time) const;

    float y() const { return pos.y; }

private:
    enum class State { Waiting, Moving };

    bool pickTarget(const Garden& garden);
    bool pathClear(const Garden& garden, Vector2 to) const;
    float rnd(float lo, float hi);
    int rndInt(int lo, int hi);

    Vector2 pos{};
    Vector2 target{};
    State state = State::Waiting;
    float waitTimer = 2.0f;
    float facing = 1.0f;
    float anim = 0.0f;
    std::mt19937 rng;
};
