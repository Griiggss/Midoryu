/*
 * Midoryu dragon face: a BMO-style pixel face with dragon parts.
 *
 * Pure C++ (no LVGL, no ESP-IDF) so it can be unit-tested and previewed on a PC.
 * It draws into a 160x120 palette-indexed buffer that the caller scales 2x to the 320x240 screen.
 */
#pragma once
#include <cstdint>

namespace stackchan::avatar {

class DragonFace {
public:
    static constexpr int kWidth      = 160;
    static constexpr int kHeight     = 120;
    static constexpr int kPaletteLen = 14;

    // Same order as stackchan::avatar::Emotion
    enum Mood {
        Neutral = 0,
        Happy,
        Angry,
        Sad,
        Doubt,
        Sleepy,
        Love,
        Shy,
        Dizzy,
        Unhinged,
        Smug,
        Fire,
        Thinking,
        Listening,
        Error,
        MoodCount
    };

    // Live controls coming from the avatar elements (blink, speaking, idle motion, ...)
    struct Input {
        int mood         = Neutral;
        float eyeOpenL   = 1.0f;  // 0..1, 1 = fully open
        float eyeOpenR   = 1.0f;
        bool eyeVisibleL = true;
        bool eyeVisibleR = true;
        float eyeOffLX   = 0.0f;  // eye centre offset, in 320x240 screen pixels
        float eyeOffLY   = 0.0f;
        float eyeOffRX   = 0.0f;
        float eyeOffRY   = 0.0f;
        float eyeSizeL   = 0.0f;  // -1..1
        float eyeSizeR   = 0.0f;
        float mouthOpen  = 0.0f;  // 0..1 (speaking)
        float mouthOffX  = 0.0f;
        float mouthOffY  = 0.0f;
        bool mouthVisible = true;
    };

    DragonFace();

    /**
     * @brief Advance animation. Returns true when the picture needs to be redrawn.
     */
    bool update(const Input& in, uint32_t nowMs);

    /**
     * @brief Draw current state into the index buffer
     */
    void render();

    const uint8_t* indices() const
    {
        return &_buf[0][0];
    }
    /** RGB888 palette (kPaletteLen entries) for the current frame */
    const uint8_t (*palette() const)[3]
    {
        return _pal;
    }

private:
    struct Params {
        float open, slope, arch, pw, brow, smile, mopen, fang, ear, size, asym, tilt, ir, ig, ib;
    };
    struct Particle {
        float x, y, vx, vy, t, max, s;
        uint8_t kind;  // 0 none, 1 heart, 2 z, 3 smoke, 4 tear, 5 fire
    };
    enum Pal { S = 0, INK, DARK, TATTOO, WHITE, TONGUE, HEART, RED, AMBER, BLUE, MAG, YELLOW, ORANGE, SMOKE };

    // primitives (design coordinates are the 320x240 screen)
    void px(int x, int y, uint8_t c);
    void fillRect(float x, float y, float w, float h, uint8_t c);
    void fillEllipse(float cx, float cy, float rx, float ry, float rot, uint8_t c);
    template <typename Fn> void fillPolyFn(const float* pts, int n, Fn fn);
    void fillPoly(const float* pts, int n, uint8_t c);
    void strokeLine(float x0, float y0, float x1, float y1, int thick, uint8_t c);
    void strokePoly(const float* pts, int n, int thick, uint8_t c, bool closed);
    void blit(const char* const* rows, int w, int h, float cx, float cy, float scale, uint8_t c, bool centered);

    void drawEars(int side);
    void drawHorn(int side);
    void drawTattoo();
    void drawEye(float cx, float cy, int dir, float sc, float browRaise, float openIn, float offx, float offy);
    void drawMouth(float openIn, float offx, float offy);
    void drawDecor();
    void drawParticles();
    void spawn(uint8_t kind, float x, float y, float vx, float vy, float max, float s);
    void emit(float dt, int slot, float rate, uint8_t kind, float x, float y, float rx, float vx, float vy, float max,
              float s);
    void computePalette();

    uint8_t _buf[kHeight][kWidth];
    uint8_t _pal[kPaletteLen][3];

    Params _cur{};
    Params _tgt{};
    int _mood = -1;
    Input _in{};
    uint32_t _t          = 0;  // ms
    uint32_t _lastMs     = 0;
    bool _first          = true;
    uint32_t _rngState   = 0x1234567;
    Particle _parts[40]{};
    float _spawnAcc[8]{};
    uint32_t _glitchAt   = 0;
    int _glitchRows[3][3]{};  // y, h, dx
    int _glitchCount     = 0;
    uint32_t _lastHash   = 0;
    float _time          = 0;

    float rnd();
    uint32_t hashState() const;
};

}  // namespace stackchan::avatar
