#pragma once
#include <raylib.h>

#include <vector>

enum class ParticleKind {
    Rain,
    Splash,
    Leaf,
    Firefly,
    Bit,      // small square with gravity: dirt, droplets, harvest crumbs
    Sparkle,  // twinkling star that drifts up (growth, harvest)
};

struct Particle {
    ParticleKind kind = ParticleKind::Rain;
    Vector2 position{};
    Vector2 velocity{};
    float lifetime = 1.0f;  // seconds left
    float maxLife = 1.0f;
    float size = 1.0f;
    Color color = WHITE;

    float rotation = 0.0f;  // leaves
    float spin = 0.0f;
    float seed = 0.0f;      // per-particle phase for wobble / twinkle
    float ground = 0.0f;    // leaves: y where they land
    bool landed = false;
    bool splash = false;    // rain: make a splash on impact
};

class ParticleSystem {
public:
    void spawn(const Particle& p) { particles.push_back(p); }
    void update(float dt);
    void draw() const;      // normal blending
    void drawGlow() const;  // call under additive blending
    int count(ParticleKind kind) const;
    void clear() { particles.clear(); }

private:
    std::vector<Particle> particles;
};
