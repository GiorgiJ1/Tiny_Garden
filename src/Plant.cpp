#include "Plant.h"

#include <cmath>

namespace {

const Color kLeaf = {92, 164, 72, 255};
const Color kLeafDark = {66, 130, 58, 255};
const Color kStem = {78, 142, 62, 255};

void leaf(Vector2 base, Vector2 tip, float r, Color c) {
    DrawLineEx(base, tip, 1.6f, kStem);
    DrawCircleV(tip, r, c);
}

void drawSeed(float x, float y) {
    DrawEllipse(int(x), int(y - 2), 7, 3.5f, {104, 72, 48, 255});
    DrawCircleV({x - 2, y - 3}, 1.1f, {62, 42, 30, 255});
    DrawCircleV({x + 2, y - 2}, 1.1f, {62, 42, 30, 255});
}

void drawSprout(float x, float y, float sway) {
    Vector2 top = {x + sway * 0.8f, y - 8};
    DrawLineEx({x, y}, top, 1.6f, kStem);
    DrawCircleV({top.x - 3, top.y + 1}, 2.4f, kLeaf);
    DrawCircleV({top.x + 3, top.y - 1}, 2.4f, kLeaf);
}

void drawCarrot(float x, float y, float sway, int stage) {
    float h = stage == 2 ? 12.0f : 16.0f;
    if (stage == 3) DrawEllipse(int(x), int(y - 3), 4.5f, 2.5f, {240, 140, 40, 255});
    leaf({x, y - 3}, {x - 5 + sway, y - h + 2}, 2.3f, kLeaf);
    leaf({x, y - 3}, {x + sway, y - h}, 2.6f, kLeafDark);
    leaf({x, y - 3}, {x + 5 + sway, y - h + 2}, 2.3f, kLeaf);
}

void drawTulip(float x, float y, float sway, int stage) {
    float h = stage == 2 ? 14.0f : 17.0f;
    Vector2 head = {x + sway, y - h};
    leaf({x, y}, {x - 5, y - 7}, 2.2f, kLeaf);
    leaf({x, y}, {x + 5, y - 6}, 2.2f, kLeaf);
    DrawLineEx({x, y}, head, 1.8f, kStem);
    if (stage == 2) {
        DrawEllipse(int(head.x), int(head.y), 2.8f, 3.6f, {170, 200, 120, 255});
    } else {
        DrawCircleV(head, 4.6f, {226, 72, 112, 255});
        DrawCircleV({head.x - 2.6f, head.y - 2}, 3.0f, {242, 110, 140, 255});
        DrawCircleV({head.x + 2.6f, head.y - 2}, 3.0f, {242, 110, 140, 255});
        DrawCircleV({head.x, head.y - 3}, 3.0f, {250, 150, 172, 255});
    }
}

void drawSunflower(float x, float y, float sway, int stage) {
    float h = stage == 2 ? 18.0f : 26.0f;
    Vector2 head = {x + sway * 1.2f, y - h};
    DrawLineEx({x, y}, head, 2.2f, kStem);
    leaf({x, y - h * 0.35f}, {x - 6, y - h * 0.35f - 3}, 2.6f, kLeaf);
    leaf({x, y - h * 0.55f}, {x + 6, y - h * 0.55f - 3}, 2.6f, kLeaf);
    if (stage == 2) {
        DrawCircleV(head, 3.4f, kLeafDark);
        return;
    }
    for (int i = 0; i < 10; i++) {
        float a = i * 0.6283f;
        DrawCircleV({head.x + std::cos(a) * 7.0f, head.y + std::sin(a) * 7.0f}, 3.2f, {250, 206, 52, 255});
    }
    DrawCircleV(head, 5.8f, {112, 72, 36, 255});
    DrawCircleV({head.x - 1.5f, head.y - 1.5f}, 2.0f, {140, 92, 48, 255});
}

void drawStrawberry(float x, float y, float sway, int stage) {
    float r = stage == 2 ? 4.2f : 5.2f;
    DrawCircleV({x - r * 0.9f + sway * 0.3f, y - r}, r, kLeafDark);
    DrawCircleV({x + r * 0.9f + sway * 0.3f, y - r}, r, kLeafDark);
    DrawCircleV({x + sway * 0.5f, y - r * 1.7f}, r, kLeaf);
    if (stage == 2) {
        DrawCircleV({x + sway * 0.5f, y - r * 2.4f}, 1.6f, {250, 248, 235, 255});
    } else {
        const Color berry = {220, 50, 60, 255};
        DrawCircleV({x - r * 1.1f, y - r * 0.6f}, 2.1f, berry);
        DrawCircleV({x + r * 1.2f, y - r * 0.9f}, 2.1f, berry);
        DrawCircleV({x + 0.5f, y - r * 1.4f}, 2.1f, berry);
    }
}

}

const char* plantName(PlantType t) {
    switch (t) {
        case PlantType::Carrot: return "Carrot";
        case PlantType::Tulip: return "Tulip";
        case PlantType::Sunflower: return "Sunflower";
        case PlantType::Strawberry: return "Strawberry";
    }
    return "";
}

void drawPlant(const Plant& p, float time) {
    const float x = p.position.x, y = p.position.y;
    const float sway = std::sin(time * 1.7f + x * 0.11f + y * 0.07f) * 1.5f;

    if (p.growthStage == 0) {
        drawSeed(x, y);
        return;
    }
    DrawEllipse(int(x), int(y), 5, 2, Fade(BLACK, 0.18f));
    if (p.growthStage == 1) {
        drawSprout(x, y, sway);
        return;
    }
    switch (p.type) {
        case PlantType::Carrot: drawCarrot(x, y, sway, p.growthStage); break;
        case PlantType::Tulip: drawTulip(x, y, sway, p.growthStage); break;
        case PlantType::Sunflower: drawSunflower(x, y, sway, p.growthStage); break;
        case PlantType::Strawberry: drawStrawberry(x, y, sway, p.growthStage); break;
    }
}
