#include "Economy.h"

namespace Economy {

void addMoney(PlayerData& player, int amount) { player.money += amount; }

bool spendMoney(PlayerData& player, int amount) {
    if (amount < 0 || player.money < amount) return false;
    player.money -= amount;
    return true;
}

int sellCrop(PlayerData& player, Inventory& inv, PlantType type) {
    const int n = inv.amount(ItemType::Crop, type);
    if (n <= 0) return 0;
    inv.removeItem(ItemType::Crop, type, n);
    const int earned = n * plantData(type).sellPrice;
    addMoney(player, earned);
    return earned;
}

int sellAllCrops(PlayerData& player, Inventory& inv) {
    int total = 0;
    for (int i = 0; i < kPlantTypeCount; i++) total += sellCrop(player, inv, PlantType(i));
    return total;
}

int cropWorth(const Inventory& inv) {
    int total = 0;
    for (int i = 0; i < kPlantTypeCount; i++)
        total += inv.amount(ItemType::Crop, PlantType(i)) * plantData(PlantType(i)).sellPrice;
    return total;
}

}  // namespace Economy
