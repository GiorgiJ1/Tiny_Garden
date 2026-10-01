#pragma once
#include <string>

#include "Garden.h"

struct SaveMeta {
    unsigned seed = 12345;
    int day = 1;
    float hours = 8.0f;
    int weather = 0;  // WeatherType as int
    float weatherTimer = 0.0f;
};

// Plain-text save: seed, clock, weather, tilled tiles, plants.
// Decorations are rebuilt from the seed, so they aren't stored.
bool saveGame(const std::string& path, const Garden& garden, const SaveMeta& meta);
bool loadGame(const std::string& path, Garden& garden, SaveMeta& meta);
