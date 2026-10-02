#pragma once
#include "Inventory.h"
#include "Player.h"

namespace Shop {

// Spend coins, add seeds. Returns false (and changes nothing) if you can't afford it.
bool buySeed(PlayerData& player, Inventory& inv, PlantType type, int qty = 1);

}  // namespace Shop
