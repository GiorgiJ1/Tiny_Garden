#pragma once
#include "Inventory.h"
#include "Player.h"

namespace Economy {

void addMoney(PlayerData& player, int amount);
bool spendMoney(PlayerData& player, int amount);  // false (and no change) if too poor

// Sell every crop of one type; returns coins earned
int sellCrop(PlayerData& player, Inventory& inv, PlantType type);
int sellAllCrops(PlayerData& player, Inventory& inv);

// Total coin value of all crops currently held
int cropWorth(const Inventory& inv);

}  // namespace Economy
