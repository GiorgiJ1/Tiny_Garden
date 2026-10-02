#include "Inventory.h"

#include <algorithm>

void Inventory::addItem(const Item& item) {
    if (item.amount <= 0) return;
    counts[int(item.itemType)][int(item.plantType)] += item.amount;
}

bool Inventory::removeItem(ItemType type, PlantType plant, int amount) {
    int& n = counts[int(type)][int(plant)];
    if (amount <= 0 || n < amount) return false;
    n -= amount;
    return true;
}

int Inventory::amount(ItemType type, PlantType plant) const {
    return counts[int(type)][int(plant)];
}

void Inventory::set(ItemType type, PlantType plant, int amount) {
    counts[int(type)][int(plant)] = std::max(0, amount);
}

void Inventory::clear() {
    for (auto& row : counts)
        for (int& n : row) n = 0;
}
