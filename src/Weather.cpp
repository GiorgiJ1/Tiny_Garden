#include "Weather.h"

#include <algorithm>
#include <cmath>

#include "ColorUtil.h"

namespace {

struct Key {
    float hour;
    SkyColors c;
};

const SkyColors kNight = {{88, 98, 155, 255}, {34, 44, 100, 255}, {70, 86, 150, 255}, 1.0f};

const Key kKeys[] = {
    {0.0f, kNight},
    {5.0f, kNight},
    {6.5f, {{255, 190, 165, 255}, {120, 150, 215, 255}, {255, 200, 160, 255}, 0.35f}},
    {8.0f, {{255, 244, 224, 255}, {120, 180, 235, 255}, {200, 225, 245, 255}, 0.0f}},
    {14.0f, {{255, 255, 252, 255}, {96, 170, 240, 255}, {180, 215, 245, 255}, 0.0f}},
    {17.5f, {{255, 240, 215, 255}, {100, 165, 235, 255}, {200, 215, 235, 255}, 0.0f}},
    {19.0f, {{255, 172, 125, 255}, {100, 112, 195, 255}, {255, 170, 120, 255}, 0.25f}},
    {20.5f, {{150, 125, 175, 255}, {62, 72, 150, 255}, {190, 120, 150, 255}, 0.75f}},
    {22.0f, kNight},
    {24.0f, kNight},
};

constexpr float kDayLengthSec = 300.0f;

}

void DayCycle::update(float dt) {
    hours += dt * 24.0f / kDayLengthSec;
    if (hours >= 24.0f) {
        hours -= 24.0f;
        dayNumber++;
    }
}

SkyColors DayCycle::sky() const {
    const int n = int(sizeof(kKeys) / sizeof(kKeys[0]));
    for (int i = 0; i < n - 1; i++) {
        const Key& a = kKeys[i];
        const Key& b = kKeys[i + 1];
        if (hours > b.hour) continue;

        float t = (hours - a.hour) / (b.hour - a.hour);
        t = t * t * (3.0f - 2.0f * t);
        return {mixColor(a.c.tint, b.c.tint, t), mixColor(a.c.top, b.c.top, t),
                mixColor(a.c.bottom, b.c.bottom, t), a.c.night + (b.c.night - a.c.night) * t};
    }
    return kNight;
}

Weather::Weather() : rng(std::random_device{}()) {}

const char* Weather::name() const {
    switch (current) {
        case WeatherType::Sunny: return "Sunny";
        case WeatherType::Cloudy: return "Cloudy";
        case WeatherType::Rain: return "Rain";
    }
    return "";
}

void Weather::update(float dt) {
    timer += dt;
    if (timer >= duration) pickNext();

    const float cloudTarget = current == WeatherType::Sunny ? 0.2f : current == WeatherType::Cloudy ? 0.75f : 1.0f;
    const float rainTarget = current == WeatherType::Rain ? 1.0f : 0.0f;

    const float k = 1.0f - std::exp(-dt * 0.6f);
    cloud += (cloudTarget - cloud) * k;
    rainAmount += (rainTarget - rainAmount) * k;

    if (rainAmount > 0.3f) wet = std::min(1.0f, wet + dt * 0.2f);
    else wet = std::max(0.0f, wet - dt * 0.02f);
}

void Weather::pickNext() {
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    switch (current) {
        case WeatherType::Sunny: current = WeatherType::Cloudy; break;
        case WeatherType::Cloudy: current = u(rng) < 0.5f ? WeatherType::Sunny : WeatherType::Rain; break;
        case WeatherType::Rain: current = WeatherType::Cloudy; break;
    }
    timer = 0.0f;
    duration = 90.0f + u(rng) * 90.0f;
}

void Weather::cycle() {
    current = WeatherType((int(current) + 1) % 3);
    timer = 0.0f;
}

void CloudLayer::init(Vector2 worldSize, unsigned seed) {
    world = worldSize;
    rng.seed(seed);

    tex = LoadRenderTexture(256, 128);
    SetTextureFilter(tex.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(tex);
    ClearBackground({255, 255, 255, 0});
    auto puffs = [](float dy, Color c) {
        DrawCircleV({62, 88 + dy}, 28, c);
        DrawCircleV({100, 70 + dy}, 40, c);
        DrawCircleV({150, 62 + dy}, 46, c);
        DrawCircleV({198, 82 + dy}, 34, c);
        DrawRectangleRounded({50, 84 + dy, 170, 34}, 1.0f, 8, c);
    };
    puffs(8.0f, {214, 222, 236, 255});
    puffs(0.0f, WHITE);
    EndTextureMode();
    ready = true;

    auto rnd = [&](float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng); };
    clouds.clear();
    for (int i = 0; i < 12; i++) {
        clouds.push_back({{rnd(-150.0f, world.x + 150.0f), rnd(-80.0f, world.y + 40.0f)},
                          rnd(0.9f, 1.7f), rnd(5.0f, 14.0f), i * 0.07f});
    }
}

void CloudLayer::shutdown() {
    if (!ready) return;
    UnloadRenderTexture(tex);
    ready = false;
}

void CloudLayer::update(float dt) {
    for (Cloud& c : clouds) {
        c.pos.x += c.speed * dt;
        if (c.pos.x > world.x + 220.0f) {
            c.pos.x = -220.0f;
            c.pos.y = std::uniform_real_distribution<float>(-80.0f, world.y + 40.0f)(rng);
        }
    }
}

void CloudLayer::drawOne(const Cloud& c, Vector2 offset, Color tint) const {
    const float w = 150.0f * c.scale, h = 75.0f * c.scale;
    const Rectangle src = {0, 0, 256, -128};
    const Rectangle dst = {c.pos.x + offset.x, c.pos.y + offset.y, w, h};
    DrawTexturePro(tex.texture, src, dst, {w / 2, h / 2}, 0.0f, tint);
}

void CloudLayer::drawShadows(float cloudiness) const {
    if (!ready) return;
    for (const Cloud& c : clouds) {
        float vis = std::clamp((cloudiness - c.threshold) * 5.0f, 0.0f, 1.0f);
        if (vis > 0.01f) drawOne(c, {40.0f, 70.0f}, Fade(BLACK, 0.12f * vis));
    }
}

void CloudLayer::draw(float cloudiness, float rain) const {
    if (!ready) return;
    const Color col = mixColor(WHITE, {135, 142, 156, 255}, rain);
    for (const Cloud& c : clouds) {
        float vis = std::clamp((cloudiness - c.threshold) * 5.0f, 0.0f, 1.0f);
        if (vis > 0.01f) drawOne(c, {0, 0}, Fade(col, 0.55f * vis));
    }
}
