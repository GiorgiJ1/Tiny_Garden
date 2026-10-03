#include "Critters.h"

#include <rlgl.h>

#include <algorithm>
#include <cmath>

#include "Garden.h"

namespace {

constexpr float kPi = 3.14159265f;

float rnd(std::mt19937& g, float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(g);
}

int rndInt(std::mt19937& g, int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(g);
}

float dist(Vector2 a, Vector2 b) { return std::hypot(b.x - a.x, b.y - a.y); }

float wrapAngle(float a) {
    while (a > kPi) a -= 2.0f * kPi;
    while (a < -kPi) a += 2.0f * kPi;
    return a;
}

Color shade(Color c, int d) {
    auto cl = [](int v) { return (unsigned char)std::clamp(v, 0, 255); };
    return {cl(c.r + d), cl(c.g + d), cl(c.b + d), c.a};
}

}  // namespace

// ------------------------------------------------------------------- frog

void Frog::init(Vector2 pad, unsigned seed) {
    rng.seed(seed);
    pos = from = to = pad;
    t = 1.0f;
    wait = rnd(rng, 1.0f, 4.0f);
    facing = rnd(rng, 0.0f, 1.0f) < 0.5f ? 1.0f : -1.0f;
}

void Frog::update(float dt, const Garden& garden, std::vector<CritterSound>& sounds) {
    if (t < 1.0f) {
        t = std::min(1.0f, t + dt / 0.45f);
        const float e = t * t * (3.0f - 2.0f * t);
        pos = {from.x + (to.x - from.x) * e, from.y + (to.y - from.y) * e};
        hopHeight = std::sin(t * kPi) * std::clamp(6.0f + dist(from, to) * 0.15f, 6.0f, 14.0f);
        if (t >= 1.0f) {
            hopHeight = 0.0f;
            wait = rnd(rng, 2.5f, 7.0f);
            if (rnd(rng, 0.0f, 1.0f) < 0.35f) sounds.push_back(CritterSound::Ribbit);
        }
        return;
    }

    wait -= dt;
    if (wait > 0.0f) return;

    // Hop to another pad within reach (or just hop on the spot if there's none)
    std::vector<Vector2> candidates;
    for (const LilyPad& p : garden.lilyPads()) {
        const float d = dist(p.pos, pos);
        if (d > 6.0f && d < 110.0f) candidates.push_back(p.pos);
    }
    from = pos;
    to = candidates.empty() ? pos : candidates[rng() % candidates.size()];
    if (to.x > from.x + 1.0f) facing = 1.0f;
    else if (to.x < from.x - 1.0f) facing = -1.0f;
    t = 0.0f;
}

void Frog::draw() const {
    const float f = facing;
    const float x = pos.x, y = pos.y - hopHeight;
    const bool hopping = t < 1.0f;
    const Color body = {92, 170, 70, 255};
    const Color dark = {60, 124, 52, 255};
    const Color belly = {206, 228, 150, 255};

    if (hopping) DrawEllipse(int(pos.x), int(pos.y), 5.0f, 2.0f, Fade(BLACK, 0.18f));

    if (hopping) {
        DrawLineEx({x - f * 2.0f, y - 3.0f}, {x - f * 9.0f, y - 1.0f}, 2.2f, dark);  // stretched back leg
        DrawLineEx({x + f * 3.0f, y - 3.0f}, {x + f * 8.0f, y - 1.0f}, 1.8f, dark);  // reaching front leg
    } else {
        DrawEllipse(int(x - f * 3.0f), int(y - 2.0f), 3.2f, 2.2f, dark);              // folded back leg
    }

    DrawEllipse(int(x), int(y - 4.0f), hopping ? 7.0f : 5.5f, hopping ? 3.2f : 4.0f, body);
    DrawEllipse(int(x + f), int(y - 3.0f), hopping ? 4.0f : 3.0f, 2.0f, belly);
    DrawCircleV({x + f * 4.0f, y - 6.5f}, 3.2f, body);

    for (int i = 0; i < 2; i++) {
        const float ex = x + f * (3.0f + 2.2f * float(i));
        DrawCircleV({ex, y - 9.2f}, 1.7f, body);
        DrawCircleV({ex, y - 9.4f}, 1.2f, WHITE);
        DrawCircleV({ex + f * 0.3f, y - 9.4f}, 0.6f, BLACK);
    }
}

// ----------------------------------------------------------------- turtle

void Turtle::init(Vector2 start, unsigned seed) {
    rng.seed(seed);
    pos = target = start;
    angle = rnd(rng, -kPi, kPi);
    wait = rnd(rng, 1.0f, 4.0f);
    moving = false;
}

bool Turtle::pickTarget(const Garden& garden) {
    const float T = float(TILE_SIZE);
    const int cx = int(std::floor(pos.x / T)), cy = int(std::floor(pos.y / T));

    for (int i = 0; i < 16; i++) {
        const int tx = cx + rndInt(rng, -4, 4), ty = cy + rndInt(rng, -3, 3);
        if (!garden.isWater(tx, ty)) continue;

        const Vector2 cand = {tx * T + rnd(rng, 8.0f, 24.0f), ty * T + rnd(rng, 8.0f, 24.0f)};

        // The straight path has to stay on water
        const int n = std::max(1, int(dist(pos, cand) / 8.0f));
        bool clear = true;
        for (int s = 1; s <= n && clear; s++) {
            const float u = float(s) / float(n);
            clear = garden.isWater(int(std::floor((pos.x + (cand.x - pos.x) * u) / T)),
                                   int(std::floor((pos.y + (cand.y - pos.y) * u) / T)));
        }
        if (!clear) continue;

        target = cand;
        return true;
    }
    return false;
}

void Turtle::update(float dt, const Garden& garden) {
    swim += dt * (moving ? 4.0f : 1.0f);

    if (!moving) {
        wait -= dt;
        if (wait <= 0.0f) {
            if (pickTarget(garden)) moving = true;
            else wait = 2.0f;
        }
        return;
    }

    const float dx = target.x - pos.x, dy = target.y - pos.y;
    const float d = std::hypot(dx, dy);
    const float step = 12.0f * dt;

    // Turn smoothly toward where we're heading
    angle += wrapAngle(std::atan2(dy, dx) - angle) * std::min(1.0f, dt * 3.0f);

    if (d <= step) {
        pos = target;
        moving = false;
        wait = rnd(rng, 3.0f, 9.0f);
    } else {
        pos.x += dx / d * step;
        pos.y += dy / d * step;
    }
}

void Turtle::draw() const {
    const Color skin = {118, 160, 90, 255};
    const Color shellDark = {70, 100, 54, 255};
    const Color shell = {104, 140, 70, 255};

    // Soft ripple around it
    const float r = 12.0f + std::sin(swim * 0.7f);
    DrawEllipseLines(int(pos.x), int(pos.y), r, r * 0.7f, Fade(WHITE, moving ? 0.28f : 0.14f));

    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, 0.0f);
    rlRotatef(angle * 57.29578f, 0.0f, 0.0f, 1.0f);  // local +x = the way it's facing

    const float paddle = std::sin(swim * 3.0f) * (moving ? 1.8f : 0.5f);
    DrawCircleV({5.0f, -7.5f + paddle}, 2.6f, skin);   // front flippers
    DrawCircleV({5.0f, 7.5f - paddle}, 2.6f, skin);
    DrawCircleV({-6.0f, -6.0f - paddle * 0.6f}, 2.1f, skin);  // back flippers
    DrawCircleV({-6.0f, 6.0f + paddle * 0.6f}, 2.1f, skin);
    DrawCircleV({-10.5f, 0.0f}, 1.5f, skin);            // tail
    DrawCircleV({10.5f, 0.0f}, 3.4f, skin);             // head
    DrawCircleV({11.8f, -1.6f}, 0.8f, BLACK);
    DrawCircleV({11.8f, 1.6f}, 0.8f, BLACK);

    rlPushMatrix();
    rlScalef(1.2f, 1.0f, 1.0f);  // squash a circle into an oval shell
    DrawCircleV({0.0f, 0.0f}, 7.2f, shellDark);
    DrawCircleV({0.0f, 0.0f}, 6.0f, shell);
    rlPopMatrix();

    DrawLineEx({-5.0f, 0.0f}, {5.0f, 0.0f}, 1.0f, shellDark);  // shell pattern
    DrawLineEx({-3.5f, -4.5f}, {-3.5f, 4.5f}, 1.0f, shellDark);
    DrawLineEx({3.5f, -4.5f}, {3.5f, 4.5f}, 1.0f, shellDark);

    rlPopMatrix();
}

// ------------------------------------------------------------------- bird

void Bird::init(const Garden& garden, Color c, unsigned seed) {
    rng.seed(seed);
    color = c;
    state = after = State::Perched;
    asleep = false;
    wait = rnd(rng, 2.0f, 6.0f);
    facing = rnd(rng, 0.0f, 1.0f) < 0.5f ? 1.0f : -1.0f;
    if (!pickPerch(garden, pos, ground)) {
        pos = {64.0f, 64.0f};
        ground = 64.0f;
    }
    from = to = pos;
    fromGround = toGround = ground;
}

bool Bird::pickPerch(const Garden& garden, Vector2& p, float& groundY) {
    std::vector<const Decoration*> trees;
    for (const Decoration& d : garden.decorations())
        if (d.type == DecorType::Tree) trees.push_back(&d);
    if (trees.empty()) return false;

    const Decoration& tree = *trees[rng() % trees.size()];
    p = {tree.pos.x + rnd(rng, -8.0f, 8.0f), tree.pos.y - treeCrownHeight(tree) + rnd(rng, -4.0f, 4.0f)};
    groundY = tree.pos.y;
    return true;
}

bool Bird::pickGround(const Garden& garden, Vector2& p, float& groundY) {
    const float T = float(TILE_SIZE);
    const int cx = int(std::floor(pos.x / T)), cy = int(std::floor(ground / T));
    for (int i = 0; i < 12; i++) {
        const int tx = cx + rndInt(rng, -6, 6), ty = cy + rndInt(rng, -4, 4);
        if (!garden.walkable(tx, ty)) continue;
        p = {tx * T + rnd(rng, 6.0f, 26.0f), ty * T + rnd(rng, 16.0f, 28.0f)};
        groundY = p.y;
        return true;
    }
    return false;
}

void Bird::takeOff(Vector2 dest, float destGround, State next) {
    from = pos;
    fromGround = ground;
    to = dest;
    toGround = destGround;
    after = next;

    const float d = dist(from, to);
    dur = std::max(0.7f, d / 70.0f);
    arc = std::clamp(d * 0.18f, 6.0f, 26.0f);
    facing = to.x >= from.x ? 1.0f : -1.0f;
    t = 0.0f;
    state = State::Flying;
}

void Bird::update(float dt, const Garden& garden, float night, std::vector<CritterSound>& sounds) {
    bob += dt;

    if (state == State::Flying) {
        t += dt / dur;
        const float e = std::min(t, 1.0f);
        const float s = e * e * (3.0f - 2.0f * e);
        pos = {from.x + (to.x - from.x) * s, from.y + (to.y - from.y) * s - std::sin(e * kPi) * arc};
        ground = fromGround + (toGround - fromGround) * s;
        if (t >= 1.0f) {
            pos = to;
            ground = toGround;
            state = after;
            wait = after == State::Foraging ? rnd(rng, 4.0f, 8.0f) : rnd(rng, 5.0f, 14.0f);
            if (rnd(rng, 0.0f, 1.0f) < 0.5f) sounds.push_back(CritterSound::Chirp);
        }
        return;
    }

    // Night: go home to a tree and sleep
    if (night > 0.55f) {
        if (state == State::Foraging) {
            Vector2 p;
            float g;
            if (pickPerch(garden, p, g)) takeOff(p, g, State::Perched);
            return;
        }
        asleep = true;
        return;
    }
    asleep = false;

    wait -= dt;
    if (wait > 0.0f) return;

    Vector2 p;
    float g;
    if (state == State::Perched && rnd(rng, 0.0f, 1.0f) < 0.45f && pickGround(garden, p, g)) {
        takeOff(p, g, State::Foraging);
    } else if (pickPerch(garden, p, g)) {
        takeOff(p, g, State::Perched);
    } else {
        wait = 3.0f;
    }
}

void Bird::draw() const {
    const float f = facing;
    const Color light = shade(color, 45);
    const Color wing = shade(color, -45);
    const Color beak = {250, 170, 60, 255};

    if (state != State::Perched) {
        const float h = std::max(0.0f, ground - pos.y);
        DrawEllipse(int(pos.x), int(ground), 4.0f, 1.6f,
                    Fade(BLACK, 0.2f * std::clamp(1.0f - h / 60.0f, 0.2f, 1.0f)));
    }

    if (state == State::Flying) {
        const float x = pos.x, y = pos.y - 3.0f;
        const float flap = std::sin(bob * 30.0f);
        DrawLineEx({x - f * 4.0f, y}, {x - f * 9.0f, y + 1.5f}, 2.0f, wing);                          // tail
        DrawEllipse(int(x), int(y), 5.0f, 3.0f, color);                                               // body
        DrawEllipse(int(x + f), int(y + 1.0f), 3.0f, 1.8f, light);
        DrawLineEx({x - f, y - 1.0f}, {x - f * 3.0f, y - 1.0f - 6.5f * flap}, 2.4f, wing);            // wings
        DrawLineEx({x + f * 0.5f, y - 1.0f}, {x + f * 2.0f, y - 1.0f - 5.0f * std::sin(bob * 30.0f + 0.8f)}, 2.0f, wing);
        DrawCircleV({x + f * 4.5f, y - 1.5f}, 2.5f, color);                                           // head
        DrawLineEx({x + f * 6.5f, y - 1.5f}, {x + f * 9.0f, y - 1.0f}, 1.5f, beak);
        DrawCircleV({x + f * 5.2f, y - 2.3f}, 0.7f, BLACK);
        return;
    }

    const float x = pos.x, y = pos.y;
    const float peck = state == State::Foraging ? std::max(0.0f, std::sin(bob * 5.0f)) * 3.0f : 0.0f;
    const float flick = std::sin(bob * 2.0f) * 0.8f;

    DrawLineEx({x - f * 1.0f, y - 0.5f}, {x - f * 1.0f, y + 1.5f}, 1.0f, beak);  // legs
    DrawLineEx({x + f * 1.5f, y - 0.5f}, {x + f * 1.5f, y + 1.5f}, 1.0f, beak);
    DrawLineEx({x - f * 4.0f, y - 3.0f}, {x - f * 9.0f, y - 2.0f + flick}, 2.0f, wing);  // tail
    DrawEllipse(int(x), int(y - 3.0f), 4.6f, 3.8f, color);                                // body
    DrawEllipse(int(x + f), int(y - 2.0f), 3.0f, 2.4f, light);
    DrawEllipse(int(x - f), int(y - 3.5f), 3.0f, 2.0f, wing);

    const Vector2 head = {x + f * (4.0f + peck * 0.8f), y - 6.0f + peck};
    DrawCircleV(head, 2.7f, color);
    DrawLineEx({head.x + f * 2.2f, head.y + 0.3f}, {head.x + f * 4.6f, head.y + 0.8f + peck * 0.3f}, 1.5f, beak);
    if (asleep) DrawLineEx({head.x + f * 0.6f, head.y - 0.6f}, {head.x + f * 2.0f, head.y - 0.6f}, 1.0f, BLACK);
    else DrawCircleV({head.x + f * 1.2f, head.y - 0.7f}, 0.75f, BLACK);
}
