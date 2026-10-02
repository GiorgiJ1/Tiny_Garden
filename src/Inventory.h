#pragma once
#include "Item.h"

// Counts per (item type, plant type). Seeds and crops never mix.
class Inventory {
public:
    void addItem(const Item& item);
    bool removeItem(ItemType type, PlantType plant, int amount);  // false if not enough
    int amount(ItemType type, PlantType plant) const;
    void set(ItemType type, PlantType plant, int amount);         // used by load
    void clear();

private:
    int counts[2][kPlantTypeCount] = {};
};
