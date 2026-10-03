#pragma once
#include <raylib.h>

#include <random>
#include <vector>

class Garden;

// Sounds critters ask the game to play
enum class CritterSound { Ribbit, Chirp };

// Sits on a lily pad, now and then hops to a nearby one
class Frog {
public:
    void init(Vector2 pad, unsigned seed);
    void update(float dt, const Garden& garden, std::vector<CritterSound>& sounds);
    void draw() const;

private:
    Vector2 pos{}, from{}, to{};
    float t = 1.0f;  // hop progress, 1 = sitting
    float wait = 2.0f;
    float facing = 1.0f;
    float hopHeight = 0.0f;
    std::mt19937 rng;
};

// Paddles around the pond, resting between trips
class Turtle {
public:
    void init(Vector2 start, unsigned seed);
    void update(float dt, const Garden& garden);
    void draw() const;

private:
    bool pickTarget(const Garden& garden);

    Vector2 pos{}, target{};
    float angle = 0.0f;
    float wait = 2.0f;
    float swim = 0.0f;
    bool moving = false;
    std::mt19937 rng;
};

// Perches in trees, sometimes flutters down to peck at the ground, sleeps at night
class Bird {
public:
    void init(const Garden& garden, Color color, unsigned seed);
    void update(float dt, const Garden& garden, float night, std::vector<CritterSound>& sounds);
    void draw() const;

private:
    enum class State { Perched, Flying, Foraging };

    bool pickPerch(const Garden& garden, Vector2& p, float& groundY);
    bool pickGround(const Garden& garden, Vector2& p, float& groundY);
    void takeOff(Vector2 dest, float destGround, State next);

    State state = State::Perched;
    State after = State::Perched;
    Vector2 pos{}, from{}, to{};       // pos = feet (perched / foraging) or body (flying)
    float ground = 0.0f, fromGround = 0.0f, toGround = 0.0f;  // y of the shadow
    float t = 0.0f, dur = 1.0f, arc = 0.0f;
    float wait = 3.0f;
    float facing = 1.0f;
    float bob = 0.0f;
    bool asleep = false;
    Color color = WHITE;
    std::mt19937 rng;
};
