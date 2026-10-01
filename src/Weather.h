#pragma once
#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

// ------------------------------------------------------------ day / night

struct SkyColors {
    Color tint;    // multiplied over the whole scene
    Color top;     // sky gradient
    Color bottom;
    float night;   // 0 = full day, 1 = full night
};

class DayCycle {
public:
    void update(float dt);
    SkyColors sky() const;

    int day() const { return dayNumber; }
    float hourOfDay() const { return hours; }
    int hourInt() const { return int(hours); }
    int minuteInt() const { return int((hours - float(int(hours))) * 60.0f); }

    void set(int day, float hourValue) {  // used by load
        dayNumber = std::max(1, day);
        hours = std::fmod(std::max(0.0f, hourValue), 24.0f);
    }

private:
    float hours = 8.0f;  // 0..24, starts in the morning
    int dayNumber = 1;
};

// ---------------------------------------------------------------- weather

enum class WeatherType { Sunny, Cloudy, Rain };

class Weather {
public:
    Weather();

    void update(float dt);
    void cycle();  // jump to the next weather (testing)

    WeatherType type() const { return current; }
    const char* name() const;
    float cloudiness() const { return cloud; }   // 0..1, smoothed
    float rain() const { return rainAmount; }    // 0..1, smoothed
    float wetness() const { return wet; }        // lingers after rain stops
    float elapsed() const { return timer; }

    void set(WeatherType t, float elapsedSec) {  // used by load, snaps instantly
        current = t;
        duration = std::max(duration, elapsedSec + 30.0f);
        timer = elapsedSec;
        cloud = t == WeatherType::Sunny ? 0.2f : t == WeatherType::Cloudy ? 0.75f : 1.0f;
        rainAmount = t == WeatherType::Rain ? 1.0f : 0.0f;
        wet = t == WeatherType::Rain ? 0.6f : 0.0f;
    }

private:
    void pickNext();

    WeatherType current = WeatherType::Sunny;
    float timer = 0.0f;
    float duration = 120.0f;
    float cloud = 0.2f;
    float rainAmount = 0.0f;
    float wet = 0.0f;
    std::mt19937 rng;
};

// ----------------------------------------------------------------- clouds

class CloudLayer {
public:
    void init(Vector2 worldSize, unsigned seed);  // needs an open window
    void shutdown();                              // call before CloseWindow
    void update(float dt);
    void drawShadows(float cloudiness) const;     // draw clipped to the garden
    void draw(float cloudiness, float rain) const;

private:
    struct Cloud {
        Vector2 pos;
        float scale;
        float speed;
        float threshold;  // becomes visible once cloudiness passes this
    };

    void drawOne(const Cloud& c, Vector2 offset, Color tint) const;

    Vector2 world{};
    std::vector<Cloud> clouds;
    RenderTexture2D tex{};
    bool ready = false;
    std::mt19937 rng;
};
