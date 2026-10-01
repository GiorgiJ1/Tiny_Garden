#include "SaveSystem.h"

#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>

bool saveGame(const std::string& path, const Garden& garden, const SaveMeta& meta) {
    std::ofstream f(path);
    if (!f) return false;

    f << "TINYGARDEN 1\n";
    f << "seed " << meta.seed << "\n";
    f << "day " << meta.day << "\n";
    f << "hours " << meta.hours << "\n";
    f << "weather " << meta.weather << " " << meta.weatherTimer << "\n";

    std::vector<std::pair<int, int>> soil;
    std::vector<std::pair<int, int>> plants;
    for (int y = 0; y < garden.height(); y++) {
        for (int x = 0; x < garden.width(); x++) {
            if (garden.at(x, y).type == TileType::Soil) soil.emplace_back(x, y);
            if (garden.plantAt(x, y)) plants.emplace_back(x, y);
        }
    }

    f << "soil " << soil.size() << "\n";
    for (auto [x, y] : soil) f << x << " " << y << "\n";

    f << "plants " << plants.size() << "\n";
    for (auto [x, y] : plants) {
        const Plant* p = garden.plantAt(x, y);
        f << x << " " << y << " " << int(p->type) << " " << p->growthStage << " " << p->growthTimer << " "
          << int(p->watered) << " " << int(p->mature) << "\n";
    }
    return bool(f);
}

bool loadGame(const std::string& path, Garden& garden, SaveMeta& meta) {
    std::ifstream f(path);
    if (!f) return false;

    std::string tag, key;
    int version = 0;
    f >> tag >> version;
    if (tag != "TINYGARDEN" || version != 1) return false;

    SaveMeta m;
    f >> key >> m.seed >> key >> m.day >> key >> m.hours >> key >> m.weather >> m.weatherTimer;

    const size_t maxTiles = size_t(garden.width()) * size_t(garden.height());

    size_t n = 0;
    f >> key >> n;
    if (!f || n > maxTiles) return false;
    std::vector<std::pair<int, int>> soil(n);
    for (auto& s : soil) f >> s.first >> s.second;

    struct SavedPlant {
        int x, y, type, stage;
        float timer;
        int watered, mature;
    };
    f >> key >> n;
    if (!f || n > maxTiles) return false;
    std::vector<SavedPlant> plants(n);
    for (auto& p : plants) f >> p.x >> p.y >> p.type >> p.stage >> p.timer >> p.watered >> p.mature;
    if (!f) return false;  // parsed everything before touching the garden

    m.weather = std::clamp(m.weather, 0, 2);
    garden.generate(m.seed);
    for (auto [x, y] : soil) garden.till(x, y);
    for (const SavedPlant& sp : plants) {
        if (!garden.sow(sp.x, sp.y, PlantType(std::clamp(sp.type, 0, kPlantTypeCount - 1)))) continue;
        Plant* p = garden.plantAt(sp.x, sp.y);
        p->growthStage = std::clamp(sp.stage, 0, 3);
        p->growthTimer = sp.timer;
        p->watered = sp.watered != 0;
        p->mature = sp.mature != 0 || p->growthStage == 3;
        p->pop = 0.0f;
    }

    meta = m;
    return true;
}
