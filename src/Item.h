#pragma once
#include "Plant.h"

enum class ItemType { Seed, Crop };

// One stack: "Carrot seeds x3" or "Carrot crops x15". Same struct for everything.
struct Item {
    ItemType itemType = ItemType::Crop;
    PlantType plantType = PlantType::Carrot;
    int amount = 0;
};
