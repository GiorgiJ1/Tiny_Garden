#include "Menus.h"

#include <cmath>

#include "Economy.h"
#include "Shop.h"

namespace {

const Color kPaper = {246, 236, 214, 255};
const Color kPaperDark = {236, 222, 194, 255};
const Color kFrame = {120, 88, 60, 255};
const Color kInk = {84, 58, 36, 255};
const Color kInkSoft = {150, 122, 92, 255};
const Color kGreen = {112, 168, 84, 255};
const Color kGreenHi = {136, 194, 104, 255};
const Color kOff = {200, 188, 166, 255};
const Color kStem = {78, 142, 62, 255};
const Color kLeaf = {92, 164, 72, 255};

constexpr float kPanelW = 520.0f;

void textRight(const char* t, float xRight, float y, int size, Color c) {
    DrawText(t, int(xRight - float(MeasureText(t, size))), int(y), size, c);
}

void textCenter(const char* t, float xCenter, float y, int size, Color c) {
    DrawText(t, int(xCenter - float(MeasureText(t, size)) / 2.0f), int(y), size, c);
}

// Little seed pouch with a dot in the plant's colour
void seedIcon(PlantType t, float x, float y) {
    DrawEllipse(int(x), int(y + 3), 10, 6, {196, 164, 118, 255});
    DrawEllipse(int(x - 2), int(y + 1), 4, 2, {222, 196, 150, 255});
    DrawCircleV({x + 1, y + 3}, 3.2f, plantColor(t));
}

void cropIcon(PlantType t, float x, float y) {
    switch (t) {
        case PlantType::Carrot:
            DrawEllipse(int(x), int(y + 3), 5, 10, {240, 140, 40, 255});
            DrawCircleV({x - 2.5f, y - 8}, 3.0f, kStem);
            DrawCircleV({x + 2.5f, y - 9}, 3.0f, kLeaf);
            break;
        case PlantType::Tulip:
            DrawLineEx({x, y + 11}, {x, y}, 2.2f, kStem);
            DrawCircleV({x, y - 3}, 6.5f, {226, 72, 112, 255});
            DrawCircleV({x - 2.5f, y - 6}, 3.5f, {242, 110, 140, 255});
            DrawCircleV({x + 2.5f, y - 6}, 3.5f, {242, 110, 140, 255});
            break;
        case PlantType::Sunflower:
            for (int i = 0; i < 8; i++) {
                float a = float(i) * 0.7854f;
                DrawCircleV({x + std::cos(a) * 7.5f, y + std::sin(a) * 7.5f}, 3.6f, {250, 206, 52, 255});
            }
            DrawCircleV({x, y}, 5.5f, {112, 72, 36, 255});
            break;
        case PlantType::Strawberry:
            DrawCircleV({x, y + 2}, 8.5f, {220, 50, 60, 255});
            DrawCircleV({x, y - 6}, 4.5f, kLeaf);
            DrawCircleV({x - 3, y + 2}, 1.0f, {250, 230, 180, 255});
            DrawCircleV({x + 3, y + 4}, 1.0f, {250, 230, 180, 255});
            DrawCircleV({x, y + 7}, 1.0f, {250, 230, 180, 255});
            break;
    }
}

// Small pictures for the critter / tree shop
void extraIcon(ExtraKind k, float x, float y) {
    const Color trunk = {105, 74, 48, 255};
    switch (k) {
        case ExtraKind::LilyPad:
            DrawEllipse(int(x), int(y + 2), 12, 7, {70, 150, 78, 255});
            DrawEllipse(int(x), int(y + 1), 10, 5.5f, {96, 176, 96, 255});
            DrawLineEx({x, y + 1}, {x + 10, y - 1}, 1.5f, {50, 120, 62, 255});
            DrawCircleV({x - 3, y - 1}, 3.4f, {246, 170, 196, 255});
            DrawCircleV({x - 3, y - 1}, 1.4f, {250, 220, 120, 255});
            break;
        case ExtraKind::Frog:
            DrawEllipse(int(x), int(y + 3), 9, 7, {92, 170, 70, 255});
            for (int i = -1; i <= 1; i += 2) {
                DrawCircleV({x + 4.0f * float(i), y - 4}, 3.6f, {92, 170, 70, 255});
                DrawCircleV({x + 4.0f * float(i), y - 4.3f}, 2.4f, WHITE);
                DrawCircleV({x + 4.0f * float(i), y - 4}, 1.2f, BLACK);
            }
            DrawLineEx({x - 4, y + 3}, {x + 4, y + 3}, 1.4f, {50, 100, 44, 255});
            break;
        case ExtraKind::Turtle:
            DrawCircleV({x + 10, y}, 3.6f, {118, 160, 90, 255});
            DrawCircleV({x - 5, y - 7}, 2.6f, {118, 160, 90, 255});
            DrawCircleV({x - 5, y + 7}, 2.6f, {118, 160, 90, 255});
            DrawCircleV({x + 5, y - 7}, 2.6f, {118, 160, 90, 255});
            DrawCircleV({x + 5, y + 7}, 2.6f, {118, 160, 90, 255});
            DrawCircleV({x, y}, 8.5f, {70, 100, 54, 255});
            DrawCircleV({x, y}, 7.0f, {104, 140, 70, 255});
            DrawLineEx({x - 5, y}, {x + 5, y}, 1.0f, {70, 100, 54, 255});
            DrawLineEx({x, y - 6}, {x, y + 6}, 1.0f, {70, 100, 54, 255});
            break;
        case ExtraKind::Bird:
            DrawLineEx({x - 5, y + 2}, {x - 12, y + 5}, 2.6f, {40, 90, 170, 255});
            DrawEllipse(int(x), int(y + 2), 8, 6.5f, {70, 130, 220, 255});
            DrawEllipse(int(x + 1), int(y + 4), 5, 3.5f, {140, 180, 245, 255});
            DrawCircleV({x + 6, y - 4}, 4.2f, {70, 130, 220, 255});
            DrawLineEx({x + 9.5f, y - 4}, {x + 13, y - 3}, 2.0f, {250, 170, 60, 255});
            DrawCircleV({x + 7.2f, y - 5}, 1.0f, BLACK);
            break;
        case ExtraKind::OakTree:
            DrawRectangleRec({x - 2, y + 2, 4, 10}, trunk);
            DrawCircleV({x - 6, y + 1}, 6.0f, {66, 134, 64, 255});
            DrawCircleV({x + 6, y + 1}, 6.0f, {66, 134, 64, 255});
            DrawCircleV({x, y - 4}, 8.0f, {78, 150, 74, 255});
            break;
        case ExtraKind::PineTree:
            DrawRectangleRec({x - 2, y + 6, 4, 6}, trunk);
            DrawTriangle({x, y - 4}, {x - 10, y + 8}, {x + 10, y + 8}, {40, 98, 70, 255});
            DrawTriangle({x, y - 12}, {x - 8, y + 1}, {x + 8, y + 1}, {44, 108, 76, 255});
            break;
        case ExtraKind::CherryTree:
            DrawRectangleRec({x - 2, y + 2, 4, 10}, trunk);
            DrawCircleV({x - 6, y + 1}, 6.0f, {228, 150, 178, 255});
            DrawCircleV({x + 6, y + 1}, 6.0f, {228, 150, 178, 255});
            DrawCircleV({x, y - 4}, 8.0f, {240, 168, 192, 255});
            DrawCircleV({x - 3, y - 6}, 1.8f, Fade(WHITE, 0.85f));
            DrawCircleV({x + 4, y - 2}, 1.8f, Fade(WHITE, 0.85f));
            break;
    }
}

}  // namespace

void drawCoin(Vector2 c, float r) {
    DrawCircleV({c.x + 1, c.y + 1.5f}, r, Fade(BLACK, 0.2f));
    DrawCircleV(c, r, {232, 176, 36, 255});
    DrawCircleV(c, r * 0.78f, {250, 210, 70, 255});
    DrawCircleV({c.x - r * 0.25f, c.y - r * 0.25f}, r * 0.25f, {255, 236, 150, 255});
}

// ---------------------------------------------------------------- helpers

bool Menus::button(Rectangle r, const char* label, bool enabled) const {
    const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), r);
    const Color fill = !enabled ? kOff : hover ? kGreenHi : kGreen;
    DrawRectangleRounded({r.x, r.y + 2, r.width, r.height}, 0.4f, 6, Fade(BLACK, 0.18f));
    DrawRectangleRounded(r, 0.4f, 6, fill);
    textCenter(label, r.x + r.width / 2.0f, r.y + (r.height - 18.0f) / 2.0f, 18, enabled ? WHITE : kInkSoft);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !justOpened;
}

Rectangle Menus::beginPanel(const char* title, float h) {
    const float W = float(GetScreenWidth()), H = float(GetScreenHeight());
    DrawRectangle(0, 0, int(W), int(H), Fade(BLACK, 0.4f));

    const Rectangle p = {W / 2.0f - kPanelW / 2.0f, H / 2.0f - h / 2.0f - 10.0f, kPanelW, h};
    DrawRectangleRounded({p.x + 4, p.y + 6, p.width, p.height}, 0.05f, 8, Fade(BLACK, 0.3f));
    DrawRectangleRounded({p.x - 3, p.y - 3, p.width + 6, p.height + 6}, 0.05f, 8, kFrame);
    DrawRectangleRounded(p, 0.05f, 8, kPaper);

    textCenter(title, p.x + p.width / 2.0f, p.y + 16.0f, 28, kInk);
    DrawRectangle(int(p.x + 24), int(p.y + 52), int(p.width - 48), 2, kPaperDark);

    if (button({p.x + p.width - 42, p.y + 12, 28, 28}, "x", true)) close();
    if (!justOpened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), p))
        close();
    return p;
}

void Menus::footer(const Rectangle& p, const PlayerData& player, const char* hint) const {
    drawCoin({p.x + 40, p.y + p.height - 32}, 11.0f);
    DrawText(TextFormat("Coins: %d", player.money), int(p.x + 58), int(p.y + p.height - 43), 22, kInk);
    textRight(hint, p.x + p.width - 28, p.y + p.height - 38, 16, kInkSoft);
}

MenuEvent Menus::run(Inventory& inv, PlayerData& player, Garden& garden) {
    MenuEvent ev;
    switch (kind) {
        case MenuKind::None: break;
        case MenuKind::Inventory: drawInventory(inv, player); break;
        case MenuKind::Shop: ev = drawShop(inv, player); break;
        case MenuKind::Sell: ev = drawSell(inv, player); break;
        case MenuKind::Land: ev = drawLand(garden, player); break;
        case MenuKind::Wildlife: ev = drawWildlife(garden, player); break;
    }
    justOpened = false;
    return ev;
}

// -------------------------------------------------------------- inventory

void Menus::drawInventory(const Inventory& inv, const PlayerData& player) {
    const Rectangle p = beginPanel("INVENTORY", 430.0f);
    float y = p.y + 64.0f;

    auto section = [&](const char* name, ItemType type) {
        DrawText(name, int(p.x + 28), int(y), 18, kInkSoft);
        y += 26.0f;
        for (int i = 0; i < kPlantTypeCount; i++) {
            const PlantType t = PlantType(i);
            const int n = inv.amount(type, t);
            const Color col = n > 0 ? kInk : kInkSoft;

            if (type == ItemType::Seed) seedIcon(t, p.x + 52, y + 15);
            else cropIcon(t, p.x + 52, y + 15);

            const std::string label = plantData(t).name + (type == ItemType::Seed ? " seeds" : "");
            DrawText(label.c_str(), int(p.x + 80), int(y + 5), 20, col);
            textRight(TextFormat("x%d", n), p.x + p.width - 32, y + 5, 20, col);
            y += 30.0f;
        }
    };

    section("Seeds", ItemType::Seed);
    y += 12.0f;
    section("Crops", ItemType::Crop);

    footer(p, player, "TAB / ESC to close");
}

// ------------------------------------------------------------------- shop

MenuEvent Menus::drawShop(Inventory& inv, PlayerData& player) {
    MenuEvent ev;
    const Rectangle p = beginPanel("SEED SHOP", 368.0f);
    float y = p.y + 66.0f;

    for (int i = 0; i < kPlantTypeCount; i++) {
        const PlantType t = PlantType(i);
        const PlantData& d = plantData(t);
        const Rectangle row = {p.x + 20, y, p.width - 40, 52};

        DrawRectangleRounded(row, 0.2f, 6, kPaperDark);
        seedIcon(t, row.x + 30, row.y + 26);
        DrawText((d.name + " seed").c_str(), int(row.x + 58), int(row.y + 7), 20, kInk);
        DrawText(TextFormat("You have %d   -   sells for %d", inv.amount(ItemType::Seed, t), d.sellPrice),
                 int(row.x + 58), int(row.y + 31), 14, kInkSoft);

        drawCoin({row.x + 268, row.y + 26}, 9.0f);
        DrawText(TextFormat("%d", d.seedPrice), int(row.x + 282), int(row.y + 14), 22, kInk);

        const bool can1 = player.money >= d.seedPrice;
        const bool can5 = player.money >= d.seedPrice * 5;
        if (button({row.x + row.width - 150, row.y + 11, 66, 30}, "Buy", can1) && Shop::buySeed(player, inv, t, 1))
            ev.bought = true;
        if (button({row.x + row.width - 76, row.y + 11, 66, 30}, "x5", can5) && Shop::buySeed(player, inv, t, 5))
            ev.bought = true;

        y += 58.0f;
    }

    footer(p, player, "B / ESC to close");
    return ev;
}

// ------------------------------------------------------------------- sell

MenuEvent Menus::drawSell(Inventory& inv, PlayerData& player) {
    MenuEvent ev;
    const Rectangle p = beginPanel("SHIPPING BOX", 380.0f);
    float y = p.y + 66.0f;

    for (int i = 0; i < kPlantTypeCount; i++) {
        const PlantType t = PlantType(i);
        const PlantData& d = plantData(t);
        const int n = inv.amount(ItemType::Crop, t);
        const Rectangle row = {p.x + 20, y, p.width - 40, 46};

        DrawRectangleRounded(row, 0.2f, 6, kPaperDark);
        cropIcon(t, row.x + 30, row.y + 23);
        DrawText(TextFormat("%s  x%d", d.name.c_str(), n), int(row.x + 58), int(row.y + 5), 20,
                 n > 0 ? kInk : kInkSoft);
        DrawText(TextFormat("%d coins each", d.sellPrice), int(row.x + 58), int(row.y + 28), 14, kInkSoft);

        drawCoin({row.x + 268, row.y + 23}, 9.0f);
        DrawText(TextFormat("%d", n * d.sellPrice), int(row.x + 282), int(row.y + 12), 22, n > 0 ? kInk : kInkSoft);

        if (button({row.x + row.width - 80, row.y + 8, 70, 30}, "Sell", n > 0)) {
            const int earned = Economy::sellCrop(player, inv, t);
            ev.sold = true;
            ev.message = TextFormat("Sold %d %s for %d coins", n, d.name.c_str(), earned);
        }
        y += 52.0f;
    }

    const int worth = Economy::cropWorth(inv);
    if (button({p.x + p.width / 2.0f - 130, y + 6, 260, 40}, TextFormat("Sell all   +%d", worth), worth > 0)) {
        const int earned = Economy::sellAllCrops(player, inv);
        ev.sold = true;
        ev.message = TextFormat("Sold everything for %d coins", earned);
    }

    footer(p, player, "ESC to close");
    return ev;
}

// ------------------------------------------------------------------- land

MenuEvent Menus::drawLand(Garden& garden, PlayerData& player) {
    MenuEvent ev;
    const Rectangle p = beginPanel("LAND", 372.0f);

    // Mini map: owned land (green) and the next plot (gold outline) inside the whole garden
    const int cell = 8;
    const float mapW = float(garden.width() * cell), mapH = float(garden.height() * cell);
    const float mx = p.x + p.width / 2.0f - mapW / 2.0f, my = p.y + 68.0f;

    DrawRectangle(int(mx) - 4, int(my) - 4, int(mapW) + 8, int(mapH) + 8, kFrame);
    DrawRectangle(int(mx), int(my), int(mapW), int(mapH), {78, 110, 70, 255});

    const TileRect cur = garden.landRect(garden.landLevel());
    DrawRectangle(int(mx) + cur.x0 * cell, int(my) + cur.y0 * cell, cur.width() * cell, cur.height() * cell,
                  {136, 190, 100, 255});

    const float ty = my + mapH + 20.0f;
    if (!garden.canExpand()) {
        textCenter("The whole garden is yours!", p.x + p.width / 2.0f, ty + 14.0f, 22, kInk);
    } else {
        const TileRect next = garden.landRect(garden.landLevel() + 1);
        DrawRectangleLinesEx({mx + float(next.x0 * cell), my + float(next.y0 * cell),
                              float(next.width() * cell), float(next.height() * cell)},
                             2.0f, {250, 206, 52, 255});

        DrawText(TextFormat("Now:   %d x %d tiles", cur.width(), cur.height()), int(p.x + 28), int(ty), 20, kInk);
        DrawText(TextFormat("Next:  %d x %d tiles  (+%d)", next.width(), next.height(), next.tiles() - cur.tiles()),
                 int(p.x + 28), int(ty + 26), 20, kInk);

        const int cost = garden.nextLandCost();
        drawCoin({p.x + 40, ty + 76}, 11.0f);
        DrawText(TextFormat("%d coins", cost), int(p.x + 58), int(ty + 64), 24, kInk);

        if (button({p.x + p.width - 160, ty + 58, 120, 36}, "Expand", player.money >= cost) &&
            Economy::spendMoney(player, cost)) {
            garden.expand();
            ev.expanded = true;
            ev.level = garden.landLevel();
            ev.message = "Land expanded!";
        }
    }

    footer(p, player, "L / ESC to close");
    return ev;
}

// --------------------------------------------------------------- wildlife

MenuEvent Menus::drawWildlife(Garden& garden, PlayerData& player) {
    MenuEvent ev;
    const Rectangle p = beginPanel("CRITTERS & TREES", 482.0f);
    float y = p.y + 64.0f;

    for (int i = 0; i < kExtraKindCount; i++) {
        const ExtraKind k = ExtraKind(i);
        const ExtraData& d = extraData(k);
        const Rectangle row = {p.x + 20, y, p.width - 40, 46};

        const char* why = "";
        const bool available = garden.canAdd(k, &why);
        const bool affordable = player.money >= d.price;

        DrawRectangleRounded(row, 0.2f, 6, kPaperDark);
        extraIcon(k, row.x + 28, row.y + 24);
        DrawText(d.name.c_str(), int(row.x + 56), int(row.y + 4), 18, kInk);
        DrawText(TextFormat("%d/%d", garden.count(k), d.max), int(row.x + 64) + MeasureText(d.name.c_str(), 18),
                 int(row.y + 7), 14, kInkSoft);
        if (available) DrawText(d.blurb.c_str(), int(row.x + 56), int(row.y + 26), 13, kInkSoft);
        else DrawText(why, int(row.x + 56), int(row.y + 26), 13, Color{190, 90, 70, 255});

        drawCoin({row.x + 318, row.y + 23}, 8.0f);
        DrawText(TextFormat("%d", d.price), int(row.x + 331), int(row.y + 12), 20, kInk);

        if (button({row.x + row.width - 76, row.y + 8, 66, 30}, "Buy", available && affordable) &&
            Economy::spendMoney(player, d.price)) {
            if (d.placeable) {
                ev.startPlacement = true;  // Game takes over: pick a spot on the map
                ev.extra = k;
                close();
            } else if (garden.addCreature(k)) {
                ev.creature = true;
                ev.message = TextFormat("A new %s joined your garden!", d.name.c_str());
            } else {
                Economy::addMoney(player, d.price);  // couldn't add it after all, refund
            }
        }
        y += 52.0f;
    }

    footer(p, player, "C / ESC to close");
    return ev;
}
