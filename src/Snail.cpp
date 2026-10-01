#include "Snail.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "Garden.h"

namespace {

constexpr float kSpeed = 7.0f;  // px/s, it's a snail
const Color kBody = {206, 190, 158, 255};
const Color kBodyDark = {160, 142, 112, 255};
const Color kShell = {176, 112, 66, 255};
const Color kShellLight = {204, 144, 90, 255};
const Color kShellDark = {118, 70, 40, 255};

}

float Snail::rnd(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); }
int Snail::rndInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); }

void Snail::init(Vector2 start, unsigned seed) {
    rng.seed(seed ^ 0x5A17u);
    pos = target = start;
    state = State::Waiting;
    waitTimer = rnd(1.0f, 3.0f);
    facing = 1.0f;
    anim = 0.0f;
}

void Snail::update(float dt, const Garden& garden) {
    if (state == State::Waiting) {
        waitTimer -= dt;
        if (waitTimer <= 0.0f) {
            if (pickTarget(garden)) state = State::Moving;
            else waitTimer = 1.5f;
        }
        return;
    }

    const float dx = target.x - pos.x, dy = target.y - pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float step = kSpeed * dt;
    anim += dt;

    if (dist <= step) {
        pos = target;
        state = State::Waiting;
        waitTimer = rnd(2.5f, 7.0f);
        return;
    }
    pos.x += dx / dist * step;
    pos.y += dy / dist * step;
    if (std::fabs(dx) > 0.5f) facing = dx > 0.0f ? 1.0f : -1.0f;
}

bool Snail::pickTarget(const Garden& garden) {
    const float T = float(TILE_SIZE);
    const int cx = int(pos.x / T), cy = int(pos.y / T);
    const std::vector<std::pair<int, int>> plants = garden.plantTiles();

    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (int attempt = 0; attempt < 16; attempt++) {
        int tx, ty;
        Vector2 cand;

        if (!plants.empty() && rnd(0.0f, 1.0f) < 0.4f) {
            // Stop next to a plant, on the side facing it
            auto [px, py] = plants[std::uniform_int_distribution<size_t>(0, plants.size() - 1)(rng)];
            const int* d = dirs[rndInt(0, 3)];
            tx = px + d[0];
            ty = py + d[1];
            cand = {tx * T + T / 2 - d[0] * 9.0f + rnd(-2.0f, 2.0f), ty * T + T * 0.65f - d[1] * 9.0f};
        } else {
            tx = cx + rndInt(-5, 5);
            ty = cy + rndInt(-4, 4);
            cand = {tx * T + T / 2 + rnd(-8.0f, 8.0f), ty * T + T * 0.65f + rnd(-4.0f, 4.0f)};
        }

        if (!garden.walkable(tx, ty) || !pathClear(garden, cand)) continue;
        target = cand;
        return true;
    }
    return false;
}

bool Snail::pathClear(const Garden& garden, Vector2 to) const {
    const float T = float(TILE_SIZE);
    const int sx = int(pos.x / T), sy = int(pos.y / T);
    const float dx = to.x - pos.x, dy = to.y - pos.y;
    const int n = std::max(1, int(std::sqrt(dx * dx + dy * dy) / 8.0f) + 1);

    for (int i = 1; i <= n; i++) {
        float t = float(i) / float(n);
        int tx = int(std::floor((pos.x + dx * t) / T));
        int ty = int(std::floor((pos.y + dy * t) / T));
        if (tx == sx && ty == sy) continue;
        if (!garden.walkable(tx, ty)) return false;
    }
    return true;
}

void Snail::draw(float time) const {
    const float x = pos.x, y = pos.y, f = facing;
    const bool moving = state == State::Moving;
    const float stretch = moving ? 1.0f + 0.10f * std::sin(anim * 5.0f) : 1.0f;

    DrawEllipse(int(x), int(y), 10.0f, 3.2f, Fade(BLACK, 0.22f));
    DrawEllipse(int(x + f), int(y - 2), 8.0f * stretch, 3.0f, kBody);

    const Vector2 head = {x + f * 7.0f * stretch, y - 4.0f};
    DrawCircleV(head, 2.6f, kBody);
    const float w1 = std::sin(time * 2.2f) * 1.2f;
    const float w2 = std::sin(time * 2.2f + 1.3f) * 1.2f;
    const Vector2 e1 = {head.x + f * 1.8f + w1, head.y - 6.0f};
    const Vector2 e2 = {head.x - f * 0.4f + w2, head.y - 6.5f};
    DrawLineEx({head.x + f * 1.0f, head.y - 1.0f}, e1, 1.2f, kBodyDark);
    DrawLineEx({head.x - f * 0.5f, head.y - 1.0f}, e2, 1.2f, kBodyDark);
    DrawCircleV(e1, 1.1f, {40, 34, 30, 255});
    DrawCircleV(e2, 1.1f, {40, 34, 30, 255});

    const Vector2 sc = {x - f * 1.5f, y - 8.0f};
    DrawCircleV(sc, 6.2f, kShell);
    DrawCircleV({sc.x - 1.2f, sc.y - 1.2f}, 4.2f, kShellLight);
    DrawCircleLines(int(sc.x), int(sc.y), 4.2f, kShellDark);
    DrawCircleLines(int(sc.x), int(sc.y), 2.4f, kShellDark);
    DrawCircleV(sc, 1.0f, kShellDark);
}
