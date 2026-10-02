#include "Garden.h"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <random>

#include "ColorUtil.h"

namespace {

uint32_t hash2(uint32_t a, uint32_t b) {
    uint32_t h = a * 374761393u + b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

unsigned char clampByte(int v) { return (unsigned char)std::clamp(v, 0, 255); }

Color shade(Color c, int d) {
    return {clampByte(c.r + d), clampByte(c.g + d), clampByte(c.b + d), c.a};
}

// Squash-and-overshoot when a plant is planted or grows a stage (pop: 0.4 -> 0)
float popScale(float pop) {
    if (pop <= 0.0f) return 1.0f;
    const float u = 1.0f - pop / 0.4f;
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    const float e = 1.0f + c3 * std::pow(u - 1.0f, 3.0f) + c1 * std::pow(u - 1.0f, 2.0f);
    return 0.6f + 0.4f * e;
}

const Color kFlowerColors[] = {
    {240, 110, 130, 255},  // pink
    {250, 250, 240, 255},  // white
    {250, 200, 70, 255},   // yellow
    {170, 130, 230, 255},  // lavender
    {240, 150, 80, 255},   // orange
};

const Color kStemGreen = {70, 130, 60, 255};
const Color kSand = {224, 204, 146, 255};
const Color kLampLight = {255, 205, 120, 255};

}  // namespace

Garden::Garden(int width, int height, unsigned seed) : w(width), h(height) {
    generate(seed);
}

void Garden::generate(unsigned seed) {
    tiles.assign(w * h, Tile{});
    blocked.assign(w * h, false);
    plants.assign(w * h, std::optional<Plant>{});
    decor.clear();
    growthEvents.clear();

    const Vector2 ws = worldSize();
    const float T = float(TILE_SIZE);
    lanterns = {{0, 0}, {ws.x, 0}, {0, ws.y}, {ws.x, ws.y}};

    std::mt19937 rng(seed);
    auto rnd = [&](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };
    auto rndf = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            at(x, y).variant = hash2(x + seed * 31u, y) % 4;

    // Centre area kept clear for the farming plot
    const int plotX0 = w / 2 - 4, plotX1 = w / 2 + 4;
    const int plotY0 = h / 2 - 2, plotY1 = h / 2 + 2;
    auto inPlot = [&](int x, int y) {
        return x >= plotX0 && x <= plotX1 && y >= plotY0 && y <= plotY1;
    };

    // Shipping box left of the plot, seed shop stall right of it
    shippingTile = {plotX0 - 2, h / 2};
    shopTile = {plotX1 + 2, h / 2};
    auto addStructure = [&](std::pair<int, int> t, DecorType type) {
        blocked[t.second * w + t.first] = true;
        decor.push_back({type, {t.first * T + T / 2, (t.second + 1) * T - 4}, 1.0f, 0.0f, WHITE, 0});
    };
    addStructure(shippingTile, DecorType::ShippingBox);
    addStructure(shopTile, DecorType::ShopStall);

    // Pond (bottom-right blob)
    const int pcx = w - 5 + rnd(-1, 0), pcy = h - 4 + rnd(-1, 0);
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            float nx = dx / 2.4f, ny = dy / 1.5f;
            float d = nx * nx + ny * ny + rndf(-0.25f, 0.25f);
            int x = pcx + dx, y = pcy + dy;
            if (d <= 1.0f && inBounds(x, y) && !inPlot(x, y)) {
                at(x, y).type = TileType::Water;
                blocked[y * w + x] = true;
            }
        }
    }

    auto pickTile = [&](int& x, int& y, bool ring) {
        for (int i = 0; i < 80; i++) {
            x = rnd(0, w - 1);
            y = rnd(0, h - 1);
            if (ring && !(x < 3 || x >= w - 3 || y < 2 || y >= h - 2)) continue;
            if (blocked[y * w + x] || inPlot(x, y)) continue;
            return true;
        }
        return false;
    };

    int x, y;

    // Trees, hugging the edges
    for (int i = 0; i < 10; i++) {
        if (!pickTile(x, y, true)) continue;
        blocked[y * w + x] = true;
        Color crown = {(unsigned char)(70 + rnd(0, 15)), (unsigned char)(140 + rnd(0, 25)), 70, 255};
        decor.push_back({DecorType::Tree, {x * T + T / 2 + rndf(-4, 4), (y + 1) * T - 3},
                         rndf(0.9f, 1.15f), rndf(0, 6.28f), crown, 0});
    }

    // Rocks
    for (int i = 0; i < 6; i++) {
        if (!pickTile(x, y, false)) continue;
        blocked[y * w + x] = true;
        Color c = shade({128, 128, 136, 255}, rnd(-10, 12));
        decor.push_back({DecorType::Rock, {x * T + T / 2, y * T + T - 6},
                         rndf(0.7f, 1.2f), 0, c, rnd(0, 1)});
    }

    // Flowers
    for (int i = 0; i < 30; i++) {
        if (!pickTile(x, y, false)) continue;
        decor.push_back({DecorType::Flower, {x * T + rndf(6, T - 6), y * T + rndf(14, T - 4)},
                         1.0f, rndf(0, 6.28f), kFlowerColors[rnd(0, 4)], 0});
    }

    // Sticks
    for (int i = 0; i < 8; i++) {
        if (!pickTile(x, y, false)) continue;
        decor.push_back({DecorType::Stick, {x * T + rndf(6, T - 12), y * T + rndf(10, T - 8)},
                         rndf(0.8f, 1.2f), rndf(0, 6.28f), {120, 88, 58, 255}, 0});
    }

    // Draw back-to-front
    std::sort(decor.begin(), decor.end(),
              [](const Decoration& a, const Decoration& b) { return a.pos.y < b.pos.y; });

    // Snail starts on a random free tile
    Vector2 start = {T * 1.5f, T * 1.5f};
    for (int i = 0; i < 200; i++) {
        int sx = rnd(0, w - 1), sy = rnd(0, h - 1);
        if (walkable(sx, sy)) {
            start = {sx * T + T / 2, sy * T + T * 0.65f};
            break;
        }
    }
    snail.init(start, seed);
}

void Garden::update(float dt) {
    time += dt;
    updatePlants(dt);
    snail.update(dt, *this);
}

// ---------------------------------------------------------------- growth

void Garden::updatePlants(float dt) {
    for (auto& slot : plants) {
        if (!slot) continue;
        Plant& p = *slot;

        p.pop = std::max(0.0f, p.pop - dt);
        if (p.mature) continue;

        p.growthTimer += dt * (p.watered ? kWateredGrowthBoost : 1.0f);
        if (p.growthTimer < stageDuration(p.type)) continue;

        // Next stage: the water gets used up
        p.growthTimer = 0.0f;
        p.watered = false;
        p.growthStage++;
        p.pop = 0.4f;
        if (p.growthStage >= 3) {
            p.growthStage = 3;
            p.mature = true;
        }
        growthEvents.push_back({{p.position.x, p.position.y - 8.0f}, p.mature});
    }
}

std::vector<GrowthEvent> Garden::takeGrowthEvents() {
    std::vector<GrowthEvent> out = std::move(growthEvents);
    growthEvents.clear();
    return out;
}

// ---------------------------------------------------------------- queries

Plant* Garden::plantAt(int x, int y) {
    if (!inBounds(x, y)) return nullptr;
    auto& p = plants[y * w + x];
    return p ? &*p : nullptr;
}

const Plant* Garden::plantAt(int x, int y) const {
    if (!inBounds(x, y)) return nullptr;
    const auto& p = plants[y * w + x];
    return p ? &*p : nullptr;
}

Structure Garden::structureAt(int x, int y) const {
    const std::pair<int, int> t = {x, y};
    if (t == shippingTile) return Structure::Shipping;
    if (t == shopTile) return Structure::SeedShop;
    return Structure::None;
}

bool Garden::walkable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    const TileType t = at(x, y).type;
    return (t == TileType::Grass || t == TileType::Soil) && !blocked[y * w + x] && !plantAt(x, y);
}

std::vector<std::pair<int, int>> Garden::plantTiles() const {
    std::vector<std::pair<int, int>> out;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (plants[y * w + x]) out.emplace_back(x, y);
    return out;
}

// ---------------------------------------------------------------- actions

bool Garden::till(int x, int y) {
    if (!inBounds(x, y)) return false;
    if (at(x, y).type != TileType::Grass || blocked[y * w + x]) return false;

    at(x, y).type = TileType::Soil;

    // Clear small decorations growing on this tile
    const Rectangle r = {float(x * TILE_SIZE), float(y * TILE_SIZE), float(TILE_SIZE), float(TILE_SIZE)};
    decor.erase(std::remove_if(decor.begin(), decor.end(),
                               [&](const Decoration& d) {
                                   return (d.type == DecorType::Flower || d.type == DecorType::Stick) &&
                                          CheckCollisionPointRec(d.pos, r);
                               }),
                decor.end());
    return true;
}

bool Garden::sow(int x, int y, PlantType type) {
    if (!inBounds(x, y)) return false;
    if (at(x, y).type != TileType::Soil || plantAt(x, y)) return false;

    Plant p;
    p.type = type;
    p.position = {x * float(TILE_SIZE) + TILE_SIZE / 2.0f, y * float(TILE_SIZE) + TILE_SIZE - 5.0f};
    p.pop = 0.4f;
    plants[y * w + x] = p;
    return true;
}

bool Garden::water(int x, int y) {
    Plant* p = plantAt(x, y);
    if (!p || p->mature || p->watered) return false;
    p->watered = true;
    return true;
}

bool Garden::harvest(int x, int y) {
    Plant* p = plantAt(x, y);
    if (!p || !p->mature) return false;
    plants[y * w + x].reset();
    return true;
}

void Garden::waterAll() {
    for (auto& slot : plants)
        if (slot && !slot->mature) slot->watered = true;
}

// ---------------------------------------------------------------- drawing

void Garden::draw() const {
    const Vector2 ws = worldSize();

    // Soft shadow + wooden frame
    DrawRectangleRounded({-6, -2, ws.x + 16, ws.y + 16}, 0.03f, 6, Fade(BLACK, 0.25f));
    DrawRectangleRounded({-8, -8, ws.x + 16, ws.y + 16}, 0.02f, 6, {110, 78, 52, 255});

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) drawTile(x, y);

    // Plants row by row so lower rows overlap higher ones
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const Plant* p = plantAt(x, y);
            if (!p) continue;

            const float s = popScale(p->pop);
            if (s == 1.0f) {
                drawPlant(*p, time);
                continue;
            }
            // Scale around the plant's base
            rlPushMatrix();
            rlTranslatef(p->position.x, p->position.y, 0.0f);
            rlScalef(s, s, 1.0f);
            rlTranslatef(-p->position.x, -p->position.y, 0.0f);
            drawPlant(*p, time);
            rlPopMatrix();
        }
    }

    // Decorations are sorted by y; slot the snail in at its own depth
    bool snailDrawn = false;
    for (const Decoration& d : decor) {
        if (!snailDrawn && d.pos.y > snail.y()) {
            snail.draw(time);
            snailDrawn = true;
        }
        drawDecoration(d);
    }
    if (!snailDrawn) snail.draw(time);

    drawLanterns();
}

void Garden::drawLanterns() const {
    for (const Vector2& l : lanterns) {
        DrawRectangleRec({l.x - 1.5f, l.y - 12.0f, 3.0f, 14.0f}, {70, 50, 36, 255});
        DrawCircleV({l.x, l.y - 14.0f}, 4.0f, {70, 50, 36, 255});
        DrawCircleV({l.x, l.y - 14.0f}, 3.0f, kLampLight);
    }
}

void Garden::drawGlow(float glow) const {
    if (glow <= 0.01f) return;
    for (const Vector2& l : lanterns) {
        DrawCircleGradient(int(l.x), int(l.y - 14), 80.0f, Fade(kLampLight, 0.5f * glow),
                           Fade(kLampLight, 0.0f));
    }
}

void Garden::drawTile(int x, int y) const {
    switch (at(x, y).type) {
        case TileType::Grass: drawGrass(x, y); break;
        case TileType::Soil: drawSoil(x, y); break;
        case TileType::Water: drawWater(x, y); break;
        case TileType::Stone:
            DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, {140, 140, 146, 255});
            break;
    }
}

void Garden::drawGrass(int x, int y) const {
    const int px = x * TILE_SIZE, py = y * TILE_SIZE;
    const int v = at(x, y).variant;
    const Color base = {clampByte(104 + v * 4), clampByte(168 + v * 4), clampByte(76 + v * 2), 255};
    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, base);

    // Puddles after rain
    if (wetness > 0.3f && hash2(x * 3 + 1, y * 5 + 2) % 6 == 0) {
        float a = (wetness - 0.3f) / 0.7f;
        DrawEllipse(px + 16, py + 19, 10, 5, Fade({110, 165, 215, 255}, 0.45f * a));
        DrawEllipse(px + 14, py + 18, 4, 2, Fade(WHITE, 0.25f * a));
    }

    // A few swaying grass tufts per tile
    const Color blade = shade(base, -28);
    const Color bladeLight = shade(base, 14);
    for (int i = 0; i < 3; i++) {
        uint32_t hv = hash2(x * 7 + i, y * 13 + i * 5);
        float bx = px + 3 + float(hv % 26);
        float by = py + 8 + float((hv >> 6) % 22);
        float hgt = 5.0f + float((hv >> 12) % 4);
        float sway = std::sin(time * 1.6f + x * 0.35f + y * 0.2f + i) * 1.8f;
        DrawLineEx({bx, by}, {bx + sway, by - hgt}, 1.5f, blade);
        DrawLineEx({bx + 2, by}, {bx + 2 + sway * 0.8f, by - hgt * 0.7f}, 1.5f, bladeLight);
    }
}

void Garden::drawSoil(int x, int y) const {
    const int px = x * TILE_SIZE, py = y * TILE_SIZE;
    const Plant* p = plantAt(x, y);
    const float wet = (p && p->watered) ? 1.0f : wetness * 0.6f;

    const Color base = mixColor({124, 88, 60, 255}, {92, 62, 44, 255}, wet);
    const Color furrow = shade(base, -16);
    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, base);

    // Furrows
    for (int i = 0; i < 3; i++)
        DrawRectangle(px + 3, py + 7 + i * 9, TILE_SIZE - 6, 2, furrow);

    // Specks
    for (int i = 0; i < 4; i++) {
        uint32_t hv = hash2(x * 5 + i, y * 11 + i);
        DrawRectangle(px + 2 + int(hv % 28), py + 2 + int((hv >> 7) % 28), 2, 2, shade(base, 14));
    }
}

void Garden::drawWater(int x, int y) const {
    const int px = x * TILE_SIZE, py = y * TILE_SIZE;
    DrawRectangle(px, py, TILE_SIZE, TILE_SIZE, {72, 140, 200, 255});

    // Sand edge wherever the neighbour isn't water
    auto notWater = [&](int nx, int ny) { return inBounds(nx, ny) && at(nx, ny).type != TileType::Water; };
    if (notWater(x, y - 1)) DrawRectangle(px, py, TILE_SIZE, 3, kSand);
    if (notWater(x, y + 1)) DrawRectangle(px, py + TILE_SIZE - 3, TILE_SIZE, 3, kSand);
    if (notWater(x - 1, y)) DrawRectangle(px, py, 3, TILE_SIZE, kSand);
    if (notWater(x + 1, y)) DrawRectangle(px + TILE_SIZE - 3, py, 3, TILE_SIZE, kSand);

    // Expanding ripple
    uint32_t hv = hash2(x, y);
    float t = std::fmod(time * 0.35f + (hv % 100) / 100.0f, 1.0f);
    float r = t * 13.0f;
    DrawEllipseLines(px + 8 + int(hv % 16), py + 10 + int((hv >> 8) % 12), r, r * 0.5f,
                     Fade(WHITE, (1.0f - t) * 0.5f));
}

void Garden::drawDecoration(const Decoration& d) const {
    const float s = d.size;
    const float x = d.pos.x, y = d.pos.y;

    switch (d.type) {
        case DecorType::Tree: {
            float sway = std::sin(time * 0.9f + d.phase) * 2.0f * s;
            DrawEllipse(int(x), int(y), 16 * s, 6 * s, Fade(BLACK, 0.22f));
            DrawRectangleRec({x - 3 * s, y - 16 * s, 6 * s, 16 * s}, {105, 74, 48, 255});
            Vector2 c = {x + sway, y - 26 * s};
            DrawCircleV({c.x - 9 * s, c.y + 4 * s}, 11 * s, shade(d.color, -12));
            DrawCircleV({c.x + 9 * s, c.y + 4 * s}, 11 * s, shade(d.color, -12));
            DrawCircleV(c, 14 * s, d.color);
            DrawCircleV({c.x - 4 * s + sway * 0.3f, c.y - 5 * s}, 7 * s, shade(d.color, 18));
        } break;

        case DecorType::Rock: {
            DrawEllipse(int(x + 1), int(y), 11 * s, 5 * s, Fade(BLACK, 0.22f));
            DrawEllipse(int(x), int(y - 4 * s), 10 * s, 7 * s, d.color);
            DrawEllipse(int(x - 3 * s), int(y - 6 * s), 5 * s, 3 * s, shade(d.color, 25));
            if (d.variant == 1)
                DrawEllipse(int(x + 10 * s), int(y - 1 * s), 5 * s, 3.5f * s, shade(d.color, -10));
        } break;

        case DecorType::Flower: {
            float sway = std::sin(time * 1.8f + d.phase) * 2.2f;
            Vector2 head = {x + sway, y - 8};
            DrawLineEx({x, y}, head, 1.5f, kStemGreen);
            for (int i = 0; i < 5; i++) {
                float a = i * 6.2832f / 5.0f;
                DrawCircleV({head.x + std::cos(a) * 2.6f, head.y + std::sin(a) * 2.6f}, 2.0f, d.color);
            }
            DrawCircleV(head, 1.6f, {255, 220, 90, 255});
        } break;

        case DecorType::Stick: {
            float len = 12 * s;
            Vector2 end = {x + std::cos(d.phase) * len, y + std::sin(d.phase) * len * 0.5f};
            Vector2 mid = {(x + end.x) / 2, (y + end.y) / 2};
            DrawLineEx(d.pos, end, 2.0f, d.color);
            DrawLineEx(mid, {mid.x + 3, mid.y - 3}, 1.5f, d.color);
        } break;

        case DecorType::ShippingBox: {
            DrawEllipse(int(x), int(y), 17, 6, Fade(BLACK, 0.22f));
            DrawRectangleRec({x - 13, y - 17, 26, 17}, {150, 104, 64, 255});  // body
            for (int i = 1; i < 4; i++)                                         // planks
                DrawRectangleRec({x - 13 + i * 6.5f, y - 14, 1.5f, 14}, {124, 84, 52, 255});
            DrawRectangleRec({x - 15, y - 21, 30, 6}, {180, 130, 80, 255});    // lid
            DrawRectangleRec({x - 15, y - 21, 30, 2}, {204, 156, 104, 255});
            DrawCircleV({x, y - 9}, 4.5f, {250, 206, 52, 255});                // coin mark
            DrawCircleV({x, y - 9}, 2.8f, {226, 172, 36, 255});
        } break;

        case DecorType::ShopStall: {
            DrawEllipse(int(x), int(y), 19, 6, Fade(BLACK, 0.22f));
            DrawRectangleRec({x - 15, y - 13, 30, 13}, {140, 98, 60, 255});    // counter
            DrawRectangleRec({x - 15, y - 13, 30, 3}, {176, 128, 80, 255});
            DrawRectangleRec({x - 14, y - 32, 2, 20}, {100, 70, 44, 255});     // posts
            DrawRectangleRec({x + 12, y - 32, 2, 20}, {100, 70, 44, 255});
            DrawRectangleRec({x - 9, y - 20, 6, 7}, {240, 140, 40, 255});      // seed packets
            DrawRectangleRec({x - 1, y - 20, 6, 7}, {236, 90, 130, 255});
            DrawRectangleRec({x + 7, y - 20, 6, 7}, {250, 206, 52, 255});
            for (int i = 0; i < 5; i++)                                          // striped awning
                DrawRectangleRec({x - 16 + i * 6.4f, y - 37, 6.4f, 8},
                                 i % 2 == 0 ? Color{214, 84, 72, 255} : Color{246, 238, 224, 255});
        } break;
    }
}
