#pragma once
#include <raylib.h>

#include <random>
#include <string>
#include <vector>

#include "Audio.h"
#include "Garden.h"
#include "Inventory.h"
#include "Menus.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "SaveSystem.h"
#include "Weather.h"

enum class Tool { Hoe, Seed, Water, Harvest };

// "+1 Carrot" style text that floats up from a world position
struct FloatText {
    Vector2 pos;
    std::string text;
    float life;
    Color color;
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void update(float dt);
    void updateCamera(float dt);
    void updateAmbient(float dt);
    void handleInput();
    void useTool(int x, int y);
    void handleMenuEvent(const MenuEvent& ev);

    void spawnRain();
    void spawnLeaf();
    void spawnFirefly();
    void spawnBurst(Vector2 c, int n, Color color, float speed, float lift, float size);
    void spawnSparkles(Vector2 c, int n, Color color);
    void spawnFloatText(Vector2 world, const std::string& text, Color color);
    void applyGrowthEvents();
    Vector2 tileCenter(int x, int y) const;

    void saveToDisk(bool announce);
    void loadFromDisk(bool announce);
    void showToast(const std::string& msg);

    void draw();
    void drawSky() const;
    void drawStars() const;
    void applyTint() const;
    void drawHighlight() const;
    void drawToolbar() const;
    void drawHud() const;
    void drawMoney() const;
    void drawTooltip() const;
    void drawFloatTexts() const;
    void drawToast() const;
    void drawWeatherIcon(float cx, float cy) const;
    Rectangle toolbarRect() const;

    Garden garden;
    unsigned seed;
    bool quit = false;

    // Camera (smoothed)
    Camera2D camera{};
    float time = 0.0f;
    float userZoom = 1.0f;
    float zoomTarget = 1.0f;
    Vector2 pan = {0, 0};
    Vector2 panTarget = {0, 0};
    Vector2 parallax = {0, 0};

    // Tools / input
    Tool tool = Tool::Hoe;
    PlantType seedType = PlantType::Carrot;
    bool hovering = false;
    int hoverX = 0, hoverY = 0;
    int lastX = -1, lastY = -1;   // last tile a drag-action was applied to
    bool clickBlocked = false;    // swallow a held click after a menu closes

    // Economy
    Inventory inventory;
    PlayerData player;
    Menus menus;

    // UI polish
    float toolLift[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    Vector2 highlightPos = {0, 0};
    bool wasHovering = false;
    std::string toast;
    float toastTimer = 0.0f;
    std::vector<FloatText> floatTexts;

    // Living world
    DayCycle dayCycle;
    Weather weather;
    CloudLayer clouds;
    ParticleSystem particles;
    Audio audio;
    SkyColors sky{};
    std::mt19937 rng{1234};
    float rainAccum = 0.0f;
    float leafTimer = 3.0f;
    float fireflyAccum = 0.0f;
};
