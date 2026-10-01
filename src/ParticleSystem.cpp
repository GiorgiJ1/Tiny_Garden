#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kBitGravity = 320.0f;

// Fade in over `t` seconds after birth, fade out over `t` before death
float edgeFade(const Particle& p, float t) {
    float age = p.maxLife - p.lifetime;
    return std::clamp(std::min(age / t, p.lifetime / t), 0.0f, 1.0f);
}

}  // namespace

void ParticleSystem::update(float dt) {
    std::vector<Particle> born;

    for (Particle& p : particles) {
        p.lifetime -= dt;
        const float age = p.maxLife - p.lifetime;

        switch (p.kind) {
            case ParticleKind::Rain:
                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;
                if (p.lifetime <= 0.0f && p.splash) {
                    Particle s;
                    s.kind = ParticleKind::Splash;
                    s.position = p.position;
                    s.lifetime = s.maxLife = 0.35f;
                    s.size = 4.5f;
                    s.color = p.color;
                    born.push_back(s);
                }
                break;

            case ParticleKind::Splash:
                break;

            case ParticleKind::Leaf:
                if (p.landed) break;
                p.position.x += (p.velocity.x + std::sin(age * 2.0f + p.seed) * 14.0f) * dt;
                p.position.y += p.velocity.y * dt;
                p.rotation += p.spin * dt;
                if (p.position.y >= p.ground) {
                    p.position.y = p.ground;
                    p.landed = true;
                    p.lifetime = std::min(p.lifetime, 5.0f);  // rest on the ground, then fade
                }
                break;

            case ParticleKind::Firefly:
                p.position.x += 12.0f * std::cos(age * 0.9f + p.seed) * dt;
                p.position.y += 12.0f * std::sin(age * 1.3f + p.seed * 1.7f) * dt;
                break;

            case ParticleKind::Bit:
                p.velocity.y += kBitGravity * dt;
                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;
                break;

            case ParticleKind::Sparkle:
                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;
                break;
        }
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.lifetime <= 0.0f; }),
                    particles.end());
    particles.insert(particles.end(), born.begin(), born.end());
}

void ParticleSystem::draw() const {
    for (const Particle& p : particles) {
        switch (p.kind) {
            case ParticleKind::Rain: {
                float sp = std::sqrt(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
                Vector2 tail = {p.position.x - p.velocity.x / sp * p.size,
                                p.position.y - p.velocity.y / sp * p.size};
                DrawLineEx(tail, p.position, 1.2f, p.color);
            } break;

            case ParticleKind::Splash: {
                float t = 1.0f - p.lifetime / p.maxLife;
                float r = 1.0f + t * p.size;
                DrawEllipseLines(int(p.position.x), int(p.position.y), r, r * 0.5f,
                                 Fade(p.color, (1.0f - t) * 0.8f));
            } break;

            case ParticleKind::Leaf: {
                float a = edgeFade(p, 1.0f);
                float thickness = 1.6f + 1.8f * std::fabs(std::sin(p.rotation));
                Vector2 dir = {std::cos(p.rotation) * p.size, std::sin(p.rotation) * p.size};
                DrawLineEx({p.position.x - dir.x, p.position.y - dir.y},
                           {p.position.x + dir.x, p.position.y + dir.y}, thickness, Fade(p.color, a));
            } break;

            case ParticleKind::Bit: {
                float a = std::clamp(p.lifetime / (p.maxLife * 0.6f), 0.0f, 1.0f);
                DrawRectangleRec({p.position.x - p.size / 2, p.position.y - p.size / 2, p.size, p.size},
                                 Fade(p.color, a));
            } break;

            case ParticleKind::Firefly:
            case ParticleKind::Sparkle:
                break;  // drawn in the glow pass
        }
    }
}

void ParticleSystem::drawGlow() const {
    for (const Particle& p : particles) {
        const float age = p.maxLife - p.lifetime;

        if (p.kind == ParticleKind::Firefly) {
            float a = edgeFade(p, 1.5f) * (0.65f + 0.35f * std::sin(age * 3.0f + p.seed * 10.0f));
            DrawCircleGradient(int(p.position.x), int(p.position.y), 10.0f, Fade(p.color, 0.5f * a),
                               Fade(p.color, 0.0f));
            DrawCircleV(p.position, 1.5f, Fade({255, 252, 215, 255}, a));
        } else if (p.kind == ParticleKind::Sparkle) {
            float a = edgeFade(p, 0.25f);
            float s = p.size * (0.7f + 0.3f * std::sin(age * 12.0f + p.seed));
            DrawLineEx({p.position.x - s, p.position.y}, {p.position.x + s, p.position.y}, 1.5f,
                       Fade(p.color, a));
            DrawLineEx({p.position.x, p.position.y - s}, {p.position.x, p.position.y + s}, 1.5f,
                       Fade(p.color, a));
            DrawCircleV(p.position, 1.2f, Fade(WHITE, a));
        }
    }
}

int ParticleSystem::count(ParticleKind kind) const {
    return int(std::count_if(particles.begin(), particles.end(),
                             [&](const Particle& p) { return p.kind == kind; }));
}
