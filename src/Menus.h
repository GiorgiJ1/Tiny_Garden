#pragma once
#include <raylib.h>

#include <string>

#include "Extras.h"
#include "Garden.h"
#include "Inventory.h"
#include "Player.h"

enum class MenuKind { None, Inventory, Shop, Sell, Land, Wildlife };

// What happened inside a menu this frame (Game turns it into sound / toasts / sparkles)
struct MenuEvent {
    std::string message;
    bool sold = false;
    bool bought = false;
    bool expanded = false;
    int level = 0;                 // new land level after an expansion
    bool creature = false;         // a critter was just added
    bool startPlacement = false;   // a lily pad / tree was bought: go pick a spot
    ExtraKind extra = ExtraKind::LilyPad;
};

void drawCoin(Vector2 center, float radius);

// Immediate-mode panels: run() draws the open menu and handles its clicks in one go.
// Call it between BeginDrawing/EndDrawing.
class Menus {
public:
    void open(MenuKind k) {
        kind = k;
        justOpened = true;
    }
    void close() { kind = MenuKind::None; }
    void toggle(MenuKind k) {
        if (kind == k) close();
        else open(k);
    }
    bool isOpen() const { return kind != MenuKind::None; }

    MenuEvent run(Inventory& inv, PlayerData& player, Garden& garden);

private:
    Rectangle beginPanel(const char* title, float height);
    bool button(Rectangle r, const char* label, bool enabled) const;
    void drawInventory(const Inventory& inv, const PlayerData& player);
    MenuEvent drawShop(Inventory& inv, PlayerData& player);
    MenuEvent drawSell(Inventory& inv, PlayerData& player);
    MenuEvent drawLand(Garden& garden, PlayerData& player);
    MenuEvent drawWildlife(Garden& garden, PlayerData& player);
    void footer(const Rectangle& panel, const PlayerData& player, const char* hint) const;

    MenuKind kind = MenuKind::None;
    bool justOpened = false;  // ignore the click that opened the menu
};
