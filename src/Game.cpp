#include "Game.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "ColorUtil.h"

namespace {

constexpr float kBtnW = 118.0f;
constexpr float kBtnH = 44.0f;
constexpr float kBtnGap = 8.0f;
const char* kToolLabels[] = {"[1] Hoe", "[2] Seed", "[3] Water", "[4] Harvest"};
const char* kSavePath = "tiny_garden_save.txt";

float rnd(std::mt19937& g, float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(g);
}

uint32_t hashU(uint32_t a) {
    a ^= a >> 16;
    a *= 0x7feb352dU;
    a ^= a >> 15;
    a *= 0x846ca68bU;
    a ^= a >> 16;
    return a;
}

void textShadow(const char* t, int x, int y, int size, Color c) {
    DrawText(t, x + 1, y + 1, size, Fade(BLACK, 0.45f));
    DrawText(t, x, y, size, c);
}

// Frame-rate independent smoothing factor
float smooth(float rate, float dt) { return 1.0f - std::exp(-rate * dt); }

}  // namespace

Game::Game() : garden(24, 16, 12345), seed(12345) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1280, 800, "Tiny Garden");
    SetTargetFPS(60);
    camera.zoom = 1.0f;

    clouds.init(garden.worldSize(), 777);  // needs the window (render texture)
    audio.init();
    sky = dayCycle.sky();

    loadFromDisk(false);  // pick up where we left off, if there's a save
}

Game::~Game() {
    audio.shutdown();
    clouds.shutdown();
    CloseWindow();
}

void Game::run() {
    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }
    saveToDisk(false);  // autosave on close
}

// ------------------------------------------------------------------ update

void Game::update(float dt) {
    time += dt;

    if (IsKeyPressed(KEY_N)) {  // new garden, new seed
        seed++;
        garden.generate(seed);
        particles.clear();
        showToast("New garden");
    }
    if (IsKeyPressed(KEY_R)) {  // reset view
        panTarget = {0, 0};
        zoomTarget = 1.0f;
    }
    if (IsKeyPressed(KEY_T)) weather.cycle();  // testing
    if (IsKeyPressed(KEY_F5)) saveToDisk(true);
    if (IsKeyPressed(KEY_F9)) loadFromDisk(true);
    if (IsKeyPressed(KEY_M)) {
        audio.toggleMute();
        showToast(audio.muted() ? "Sound off" : "Sound on");
    }

    dayCycle.update(dt * (IsKeyDown(KEY_F) ? 20.0f : 1.0f));  // hold F to fast-forward the clock
    sky = dayCycle.sky();
    weather.update(dt);

    garden.update(dt);
    garden.setWetness(weather.wetness());
    if (weather.rain() > 0.5f) garden.waterAll();
    applyGrowthEvents();

    clouds.update(dt);
    updateAmbient(dt);
    particles.update(dt);
    audio.update(weather.rain(), sky.night);
    updateCamera(dt);

    Vector2 mouse = GetMousePosition();
    Vector2 m = GetScreenToWorld2D(mouse, camera);
    hoverX = int(std::floor(m.x / TILE_SIZE));
    hoverY = int(std::floor(m.y / TILE_SIZE));
    hovering = garden.inBounds(hoverX, hoverY) && !CheckCollisionPointRec(mouse, toolbarRect());

    // Highlight glides between tiles instead of snapping
    if (hovering) {
        Vector2 t = {float(hoverX * TILE_SIZE), float(hoverY * TILE_SIZE)};
        if (!wasHovering) {
            highlightPos = t;
        } else {
            float k = smooth(28.0f, dt);
            highlightPos.x += (t.x - highlightPos.x) * k;
            highlightPos.y += (t.y - highlightPos.y) * k;
        }
    }
    wasHovering = hovering;

    handleInput();

    // Toolbar buttons ease up/down
    for (int i = 0; i < 4; i++)
        toolLift[i] += ((i == int(tool) ? 1.0f : 0.0f) - toolLift[i]) * smooth(16.0f, dt);

    if (toastTimer > 0.0f) toastTimer -= dt;
}

void Game::updateAmbient(float dt) {
    // Rain
    const float rain = weather.rain();
    if (rain > 0.05f) {
        rainAccum += 300.0f * rain * dt;
        while (rainAccum >= 1.0f) {
            spawnRain();
            rainAccum -= 1.0f;
        }
    } else {
        rainAccum = 0.0f;
    }

    // Falling leaves
    leafTimer -= dt;
    if (leafTimer <= 0.0f) {
        if (rain < 0.2f) spawnLeaf();
        leafTimer = rnd(rng, 3.0f, 8.0f);
    }

    // Fireflies at night
    const float want = std::clamp((sky.night - 0.5f) * 2.0f, 0.0f, 1.0f);
    const int target = int(24.0f * want);
    if (particles.count(ParticleKind::Firefly) < target) {
        fireflyAccum += dt * 5.0f;
        while (fireflyAccum >= 1.0f) {
            spawnFirefly();
            fireflyAccum -= 1.0f;
        }
    } else {
        fireflyAccum = 0.0f;
    }
}

void Game::spawnRain() {
    const Vector2 ws = garden.worldSize();
    Particle p;
    p.kind = ParticleKind::Rain;
    p.position = {rnd(rng, 0.0f, ws.x + 120.0f), rnd(rng, -140.0f, -60.0f)};
    p.velocity = {-70.0f, rnd(rng, 520.0f, 640.0f)};
    const float groundY = rnd(rng, 0.0f, ws.y);
    p.lifetime = p.maxLife = (groundY - p.position.y) / p.velocity.y;
    p.size = rnd(rng, 7.0f, 11.0f);
    p.color = {196, 220, 248, 150};

    const float landX = p.position.x + p.velocity.x * p.lifetime;
    p.splash = landX > 0.0f && landX < ws.x && rnd(rng, 0.0f, 1.0f) < 0.4f;
    particles.spawn(p);
}

void Game::spawnLeaf() {
    std::vector<const Decoration*> trees;
    for (const Decoration& d : garden.decorations())
        if (d.type == DecorType::Tree) trees.push_back(&d);
    if (trees.empty()) return;

    const Decoration& t = *trees[rng() % trees.size()];
    Particle p;
    p.kind = ParticleKind::Leaf;
    p.position = {t.pos.x + rnd(rng, -14.0f, 14.0f), t.pos.y - 26.0f * t.size + rnd(rng, -8.0f, 8.0f)};
    p.velocity = {rnd(rng, 6.0f, 18.0f), rnd(rng, 16.0f, 28.0f)};
    p.ground = std::max(t.pos.y + rnd(rng, -6.0f, 12.0f), p.position.y + 10.0f);
    p.lifetime = p.maxLife = 30.0f;  // cut short once it lands
    p.size = rnd(rng, 2.2f, 3.2f);
    p.rotation = rnd(rng, 0.0f, 6.28f);
    p.spin = rnd(rng, -3.0f, 3.0f);
    p.seed = rnd(rng, 0.0f, 6.28f);
    p.color = rnd(rng, 0.0f, 1.0f) < 0.65f ? Color{120, 175, 70, 255} : Color{225, 175, 60, 255};
    particles.spawn(p);
}

void Game::spawnFirefly() {
    const Vector2 ws = garden.worldSize();
    Particle p;
    p.kind = ParticleKind::Firefly;
    p.position = {rnd(rng, 0.0f, ws.x), rnd(rng, 0.0f, ws.y)};
    p.lifetime = p.maxLife = rnd(rng, 9.0f, 15.0f);
    p.size = 1.0f;
    p.seed = rnd(rng, 0.0f, 6.28f);
    p.color = {255, 238, 140, 255};
    particles.spawn(p);
}

// Little fan of crumbs/droplets thrown upward from c
void Game::spawnBurst(Vector2 c, int n, Color color, float speed, float lift, float size) {
    for (int i = 0; i < n; i++) {
        const float ang = rnd(rng, -2.7f, -0.45f);
        const float sp = speed * rnd(rng, 0.4f, 1.0f);

        Particle p;
        p.kind = ParticleKind::Bit;
        p.position = {c.x + rnd(rng, -6.0f, 6.0f), c.y + rnd(rng, -3.0f, 3.0f)};
        p.velocity = {std::cos(ang) * sp, std::sin(ang) * sp - lift * rnd(rng, 0.3f, 1.0f)};
        p.lifetime = p.maxLife = rnd(rng, 0.35f, 0.7f);
        p.size = rnd(rng, 1.5f, 2.8f) * size;
        p.color = mixColor(color, WHITE, rnd(rng, 0.0f, 0.3f));
        particles.spawn(p);
    }
}

void Game::spawnSparkles(Vector2 c, int n, Color color) {
    for (int i = 0; i < n; i++) {
        Particle p;
        p.kind = ParticleKind::Sparkle;
        p.position = {c.x + rnd(rng, -10.0f, 10.0f), c.y + rnd(rng, -18.0f, 2.0f)};
        p.velocity = {rnd(rng, -6.0f, 6.0f), rnd(rng, -26.0f, -10.0f)};
        p.lifetime = p.maxLife = rnd(rng, 0.7f, 1.2f);
        p.size = rnd(rng, 2.5f, 4.5f);
        p.seed = rnd(rng, 0.0f, 6.28f);
        p.color = color;
        particles.spawn(p);
    }
}

void Game::applyGrowthEvents() {
    for (const GrowthEvent& e : garden.takeGrowthEvents()) {
        if (e.matured) spawnSparkles(e.pos, 8, {255, 226, 120, 255});
        else spawnSparkles(e.pos, 3, {190, 255, 170, 255});
    }
}

Vector2 Game::tileCenter(int x, int y) const {
    return {x * float(TILE_SIZE) + TILE_SIZE / 2.0f, y * float(TILE_SIZE) + TILE_SIZE * 0.6f};
}

void Game::updateCamera(float dt) {
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) zoomTarget = std::clamp(zoomTarget * (1.0f + wheel * 0.1f), 0.5f, 3.0f);
    userZoom += (zoomTarget - userZoom) * smooth(10.0f, dt);

    const Vector2 ws = garden.worldSize();
    const float fit = std::min((GetScreenWidth() - 80.0f) / ws.x, (GetScreenHeight() - 160.0f) / ws.y);
    camera.zoom = fit * userZoom * (1.0f + 0.004f * std::sin(time * 0.35f));  // slow "breathing"
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f - 12.0f};

    const float speed = 400.0f / camera.zoom;
    if (IsKeyDown(KEY_W)) panTarget.y -= speed * dt;
    if (IsKeyDown(KEY_S)) panTarget.y += speed * dt;
    if (IsKeyDown(KEY_A)) panTarget.x -= speed * dt;
    if (IsKeyDown(KEY_D)) panTarget.x += speed * dt;
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 d = GetMouseDelta();
        panTarget.x -= d.x / camera.zoom;
        panTarget.y -= d.y / camera.zoom;
    }
    pan.x += (panTarget.x - pan.x) * smooth(12.0f, dt);
    pan.y += (panTarget.y - pan.y) * smooth(12.0f, dt);

    // Tiny parallax toward the mouse + a lazy idle drift
    const Vector2 mouse = GetMousePosition();
    const Vector2 want = {
        std::clamp((mouse.x - GetScreenWidth() / 2.0f) / (GetScreenWidth() / 2.0f), -1.0f, 1.0f) * 5.0f,
        std::clamp((mouse.y - GetScreenHeight() / 2.0f) / (GetScreenHeight() / 2.0f), -1.0f, 1.0f) * 5.0f};
    parallax.x += (want.x - parallax.x) * smooth(3.0f, dt);
    parallax.y += (want.y - parallax.y) * smooth(3.0f, dt);
    const Vector2 drift = {std::sin(time * 0.17f) * 3.0f, std::cos(time * 0.13f) * 2.5f};

    camera.target = {ws.x / 2.0f + pan.x + parallax.x + drift.x, ws.y / 2.0f + pan.y + parallax.y + drift.y};
}

void Game::handleInput() {
    if (IsKeyPressed(KEY_ONE)) tool = Tool::Hoe;
    if (IsKeyPressed(KEY_TWO)) {
        if (tool == Tool::Seed) seedType = PlantType((int(seedType) + 1) % kPlantTypeCount);
        tool = Tool::Seed;
    }
    if (IsKeyPressed(KEY_THREE)) tool = Tool::Water;
    if (IsKeyPressed(KEY_FOUR)) tool = Tool::Harvest;

    // Click or drag across tiles to apply the tool once per tile
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        lastX = lastY = -1;
        return;
    }
    if (!hovering || (hoverX == lastX && hoverY == lastY)) return;

    useTool(hoverX, hoverY);
    lastX = hoverX;
    lastY = hoverY;
}

void Game::useTool(int x, int y) {
    const Vector2 c = tileCenter(x, y);

    switch (tool) {
        case Tool::Hoe:
            if (garden.till(x, y)) {
                spawnBurst(c, 10, {118, 84, 58, 255}, 55.0f, 30.0f, 1.0f);
                audio.play(Sfx::Till);
            }
            break;

        case Tool::Seed:
            if (garden.sow(x, y, seedType)) {
                spawnBurst(c, 8, {104, 72, 48, 255}, 45.0f, 30.0f, 1.0f);
                audio.play(Sfx::Plant);
            }
            break;

        case Tool::Water:
            if (garden.water(x, y)) {
                spawnBurst(c, 9, {130, 190, 240, 255}, 30.0f, 10.0f, 0.8f);
                audio.play(Sfx::Water);
            }
            break;

        case Tool::Harvest: {
            const Plant* p = garden.plantAt(x, y);
            const Color col = p ? plantColor(p->type) : WHITE;  // copy before the plant is gone
            if (garden.harvest(x, y)) {
                spawnBurst(c, 14, col, 75.0f, 55.0f, 1.2f);
                spawnSparkles(c, 6, {255, 236, 150, 255});
                audio.play(Sfx::Harvest);
            }
        } break;
    }
}

// -------------------------------------------------------------- save / load

void Game::saveToDisk(bool announce) {
    SaveMeta m;
    m.seed = seed;
    m.day = dayCycle.day();
    m.hours = dayCycle.hourOfDay();
    m.weather = int(weather.type());
    m.weatherTimer = weather.elapsed();

    const bool ok = saveGame(kSavePath, garden, m);
    if (announce) showToast(ok ? "Garden saved" : "Save failed");
}

void Game::loadFromDisk(bool announce) {
    SaveMeta m;
    if (!loadGame(kSavePath, garden, m)) {
        if (announce) showToast("No save found");
        return;
    }

    seed = m.seed;
    dayCycle.set(m.day, m.hours);
    weather.set(WeatherType(m.weather), m.weatherTimer);
    sky = dayCycle.sky();
    particles.clear();
    lastX = lastY = -1;
    if (announce) showToast("Garden loaded");
}

void Game::showToast(const std::string& msg) {
    toast = msg;
    toastTimer = 2.0f;
}

// -------------------------------------------------------------------- draw

void Game::draw() {
    BeginDrawing();
    drawSky();

    BeginMode2D(camera);
    garden.draw();
    if (hovering) drawHighlight();

    // Cloud shadows, clipped to the garden
    const Vector2 a = GetWorldToScreen2D({0, 0}, camera);
    const Vector2 b = GetWorldToScreen2D(garden.worldSize(), camera);
    if (b.x > a.x && b.y > a.y) {
        BeginScissorMode(int(a.x), int(a.y), int(b.x - a.x), int(b.y - a.y));
        clouds.drawShadows(weather.cloudiness());
        EndScissorMode();
    }

    particles.draw();
    clouds.draw(weather.cloudiness(), weather.rain());
    EndMode2D();

    applyTint();

    // Lights and sparkles glow on top of the tint
    const float lanternGlow = std::clamp((sky.night - 0.25f) / 0.55f, 0.0f, 1.0f);
    BeginMode2D(camera);
    BeginBlendMode(BLEND_ADDITIVE);
    garden.drawGlow(lanternGlow);
    particles.drawGlow();
    EndBlendMode();
    EndMode2D();

    drawHud();
    drawToolbar();
    drawToast();
    EndDrawing();
}

void Game::drawSky() const {
    // Overcast pulls the sky toward grey
    const Color overcast = mixColor({135, 145, 158, 255}, {50, 58, 90, 255}, sky.night);
    const float amount = 0.55f * weather.cloudiness();
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                           mixColor(sky.top, overcast, amount), mixColor(sky.bottom, overcast, amount));
    drawStars();
}

void Game::drawStars() const {
    const float vis = sky.night * (1.0f - 0.85f * weather.cloudiness());
    if (vis < 0.02f) return;

    const int W = GetScreenWidth(), H = GetScreenHeight();
    for (uint32_t i = 0; i < 140; i++) {
        uint32_t h = hashU(i * 2654435761u + 7u);
        float fx = float(h & 0xFFFF) / 65535.0f;
        float fy = float((h >> 16) & 0xFFFF) / 65535.0f;
        float twinkle = 0.55f + 0.45f * std::sin(time * (0.8f + float(h % 5) * 0.4f) + float(i));
        int s = (h % 7 == 0) ? 3 : 2;
        DrawRectangle(int(fx * W), int(fy * H), s, s, Fade(WHITE, vis * twinkle));
    }
}

void Game::applyTint() const {
    const float dim = 1.0f - 0.22f * weather.cloudiness() - 0.08f * weather.rain();
    const Color t = {(unsigned char)(sky.tint.r * dim), (unsigned char)(sky.tint.g * dim),
                     (unsigned char)(sky.tint.b * dim), 255};
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), t);
    EndBlendMode();
}

void Game::drawHighlight() const {
    float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
    Rectangle r = {highlightPos.x, highlightPos.y, float(TILE_SIZE), float(TILE_SIZE)};
    DrawRectangleRec(r, Fade(WHITE, 0.14f + 0.10f * pulse));
    DrawRectangleLinesEx(r, 2.0f, Fade({255, 244, 200, 255}, 0.6f + 0.3f * pulse));
}

Rectangle Game::toolbarRect() const {
    const float total = 4 * kBtnW + 3 * kBtnGap;
    return {GetScreenWidth() / 2.0f - total / 2.0f, GetScreenHeight() - kBtnH - 18.0f, total, kBtnH};
}

void Game::drawToolbar() const {
    const Rectangle bar = toolbarRect();
    for (int i = 0; i < 4; i++) {
        const float lift = toolLift[i];
        Rectangle b = {bar.x + i * (kBtnW + kBtnGap), bar.y - 6.0f * lift, kBtnW, kBtnH};
        DrawRectangleRounded({b.x, b.y + 3, b.width, b.height}, 0.3f, 6, Fade(BLACK, 0.25f));
        DrawRectangleRounded(b, 0.3f, 6, mixColor({40, 52, 44, 210}, {255, 236, 190, 240}, lift));

        int tw = MeasureText(kToolLabels[i], 18);
        DrawText(kToolLabels[i], int(b.x + (b.width - tw) / 2), int(b.y + (b.height - 18) / 2), 18,
                 mixColor({235, 235, 220, 255}, {90, 60, 36, 255}, lift));
    }

    if (tool == Tool::Seed) {
        const char* msg = TextFormat("%s  (press 2 to change)", plantName(seedType));
        int tw = MeasureText(msg, 16);
        textShadow(msg, int(bar.x + (bar.width - tw) / 2), int(bar.y - 32), 16, Fade(RAYWHITE, 0.95f));
    }
}

void Game::drawHud() const {
    textShadow(TextFormat("Day %d", dayCycle.day()), 16, 12, 22, RAYWHITE);
    textShadow(TextFormat("%02d:%02d", dayCycle.hourInt(), dayCycle.minuteInt()), 16, 38, 20,
               Fade(RAYWHITE, 0.9f));
    drawWeatherIcon(26.0f, 74.0f);
    textShadow(weather.name(), 46, 65, 18, Fade(RAYWHITE, 0.9f));

    const char* line1 = "WASD/MMB pan   Wheel zoom   R reset view   N new garden   T weather   hold F skip time";
    const char* line2 = TextFormat("F5 save   F9 load   M sound   (seed %u)", seed);
    textShadow(line1, GetScreenWidth() - MeasureText(line1, 14) - 16, 14, 14, Fade(RAYWHITE, 0.75f));
    textShadow(line2, GetScreenWidth() - MeasureText(line2, 14) - 16, 32, 14, Fade(RAYWHITE, 0.75f));
}

void Game::drawToast() const {
    if (toastTimer <= 0.0f) return;
    const float a = std::clamp(toastTimer / 0.5f, 0.0f, 1.0f);
    const int tw = MeasureText(toast.c_str(), 18);
    const int x = GetScreenWidth() / 2 - tw / 2, y = 56;
    DrawRectangleRounded({x - 14.0f, y - 6.0f, tw + 28.0f, 30.0f}, 0.5f, 8, Fade(Color{30, 40, 34, 255}, 0.65f * a));
    DrawText(toast.c_str(), x, y, 18, Fade(RAYWHITE, a));
}

void Game::drawWeatherIcon(float cx, float cy) const {
    auto cloudShape = [&](Color c) {
        DrawCircleV({cx - 5, cy + 2}, 5.0f, c);
        DrawCircleV({cx + 2, cy - 2}, 6.5f, c);
        DrawCircleV({cx + 8, cy + 2}, 4.5f, c);
    };

    switch (weather.type()) {
        case WeatherType::Sunny:
            for (int i = 0; i < 8; i++) {
                float a = i * 0.7854f;
                DrawLineEx({cx + std::cos(a) * 9.0f, cy + std::sin(a) * 9.0f},
                           {cx + std::cos(a) * 12.0f, cy + std::sin(a) * 12.0f}, 1.8f, {255, 214, 80, 255});
            }
            DrawCircleV({cx, cy}, 6.0f, {255, 214, 80, 255});
            break;
        case WeatherType::Cloudy:
            cloudShape({225, 230, 240, 255});
            break;
        case WeatherType::Rain:
            cloudShape({170, 180, 196, 255});
            DrawLineEx({cx - 5, cy + 8}, {cx - 7, cy + 13}, 1.6f, {150, 195, 245, 255});
            DrawLineEx({cx + 1, cy + 8}, {cx - 1, cy + 13}, 1.6f, {150, 195, 245, 255});
            DrawLineEx({cx + 7, cy + 8}, {cx + 5, cy + 13}, 1.6f, {150, 195, 245, 255});
            break;
    }
}
