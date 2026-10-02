#pragma once
#include <raylib.h>

#include <string>

enum class PlantType { Carrot, Tulip, Sunflower, Strawberry };

constexpr int kPlantTypeCount = 4;

struct Plant {
    PlantType type = PlantType::Carrot;
    Vector2 position{};       // world px, bottom-centre of the plant
    int growthStage = 0;      // 0 seed, 1 sprout, 2 medium, 3 fully grown
    float growthTimer = 0.0f; // seconds spent in the current stage
    bool watered = false;     // wet for the current stage only
    bool mature = false;
    float pop = 0.0f;         // seconds left of the little "pop" animation (cosmetic)
};

// All per-plant-type numbers live here
struct PlantData {
    PlantType type;
    std::string name;
    int seedPrice;   // coins to buy one seed
    int sellPrice;   // coins per harvested crop
    float growTime;  // seconds from seed to mature (unwatered)
};

inline const PlantData& plantData(PlantType t) {
    static const PlantData kData[kPlantTypeCount] = {
        {PlantType::Carrot, "Carrot", 5, 12, 30.0f},
        {PlantType::Tulip, "Tulip", 10, 24, 40.0f},
        {PlantType::Sunflower, "Sunflower", 20, 50, 60.0f},
        {PlantType::Strawberry, "Strawberry", 15, 35, 50.0f},
    };
    return kData[int(t)];
}

// Seconds each of the 3 growth stages takes without watering
inline float stageDuration(PlantType t) { return plantData(t).growTime / 3.0f; }

// Growth speed multiplier while a plant is watered
constexpr float kWateredGrowthBoost = 2.5f;

// Colour used for harvest particles
inline Color plantColor(PlantType t) {
    switch (t) {
        case PlantType::Carrot: return {240, 140, 40, 255};
        case PlantType::Tulip: return {236, 90, 130, 255};
        case PlantType::Sunflower: return {250, 206, 52, 255};
        case PlantType::Strawberry: return {220, 50, 60, 255};
    }
    return WHITE;
}

const char* plantName(PlantType t);
void drawPlant(const Plant& p, float time);
