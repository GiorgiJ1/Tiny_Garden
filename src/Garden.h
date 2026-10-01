#pragma once
#include <raylib.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Plant.h"
#include "Snail.h"

constexpr int TILE_SIZE = 32;

enum class TileType { Grass, Soil, Water, Stone };

struct Tile {
    TileType type = TileType::Grass;
    uint8_t variant = 0;  // subtle shade variation
};

enum class DecorType { Rock, Flower, Tree, Stick };

struct Decoration {
    DecorType type;
    Vector2 pos;   // ground contact point, world px
    float size;
    float phase;   // sway offset (or rotation for sticks)
    Color color;
    int variant;
};

// A plant moved to its next growth stage (Game turns these into sparkles)
struct GrowthEvent {
    Vector2 pos;
    bool matured;
};

class Garden {
public:
    Garden(int width, int height, unsigned seed);

    void generate(unsigned seed);
    void update(float dt);
    void draw() const;
    void drawGlow(float glow) const;  // lantern light, call under additive blending

    // Player actions. Each returns true if something actually happened.
    bool till(int x, int y);
    bool sow(int x, int y, PlantType type);
    bool water(int x, int y);
    bool harvest(int x, int y);

    void waterAll();                           // rain
    void setWetness(float w) { wetness = w; }  // darker soil + puddles

    std::vector<GrowthEvent> takeGrowthEvents();

    Plant* plantAt(int x, int y);
    const Plant* plantAt(int x, int y) const;
    const std::vector<Decoration>& decorations() const { return decor; }

    // For the snail: free of water, trees, rocks and plants
    bool walkable(int x, int y) const;
    std::vector<std::pair<int, int>> plantTiles() const;

    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    Tile& at(int x, int y) { return tiles[y * w + x]; }
    const Tile& at(int x, int y) const { return tiles[y * w + x]; }

    int width() const { return w; }
    int height() const { return h; }
    Vector2 worldSize() const { return {float(w * TILE_SIZE), float(h * TILE_SIZE)}; }

private:
    void updatePlants(float dt);
    void drawTile(int x, int y) const;
    void drawGrass(int x, int y) const;
    void drawSoil(int x, int y) const;
    void drawWater(int x, int y) const;
    void drawDecoration(const Decoration& d) const;
    void drawLanterns() const;

    int w, h;
    float time = 0.0f;
    float wetness = 0.0f;
    std::vector<Tile> tiles;
    std::vector<bool> blocked;                 // trees, rocks, water: can't be tilled
    std::vector<std::optional<Plant>> plants;  // parallel to tiles
    std::vector<Decoration> decor;
    std::vector<Vector2> lanterns;             // frame corners
    std::vector<GrowthEvent> growthEvents;
    Snail snail;
};
