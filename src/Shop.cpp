#include "Shop.h"

#include "Economy.h"

namespace Shop {

bool buySeed(PlayerData& player, Inventory& inv, PlantType type, int qty) {
    if (qty <= 0) return false;
    if (!Economy::spendMoney(player, plantData(type).seedPrice * qty)) return false;
    inv.addItem({ItemType::Seed, type, qty});
    return true;
}

}  // namespace Shop
