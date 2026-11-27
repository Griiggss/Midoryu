/*
 * Midoryu dragon face: a BMO-style pixel face with dragon parts.
 * See dragon_face.h
 */
#include "dragon_face.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace stackchan::avatar {

namespace {

// ---------------------------------------------------------------------------------------------
// Bitmaps
// ---------------------------------------------------------------------------------------------

// Kanji tattoo (zou / tsukuru: "make, build"), 16x16, from a 16px bitmap font
const char* const kKanji[16] = {
    ".#.....#..#.....", "..#...#########.", "...#..#...#.....", ".....#....#.....",
    "..........#.....", ".###.###########", "...#............", "...#...#######..",
    "...#...#.....#..", "...#...#.....#..", "...#...#.....#..", "...#...#######..",
    "..#.#...........", ".#...###########", "................", "................"};

const char* const kHeart[6] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
const char* const kZ[7]     = {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"};
const char* const kQuest[7] = {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."};

// ---------------------------------------------------------------------------------------------
// Mood table (the same numbers the browser prototype uses)
// ---------------------------------------------------------------------------------------------

enum Deco : uint16_t {
    D_SPARK  = 1 << 0,
    D_VEIN   = 1 << 1,
    D_SMOKE  = 1 << 2,
    D_TEAR   = 1 << 3,
    D_QMARK  = 1 << 4,
    D_SWEAT  = 1 << 5,
    D_ZZZ    = 1 << 6,
    D_HEARTS = 1 << 7,
    D_BLUSH  = 1 << 8,
    D_STARS  = 1 << 9,
    D_GLITCH = 1 << 10,
    D_DOTS   = 1 << 11,
    D_WAVE   = 1 << 12,
};

enum Pupil : uint8_t { PK_OVAL = 0, PK_DOT, PK_HEART, PK_SPIRAL, PK_X };
enum Iris : uint8_t { IR_MAIN = 0, IR_RED, IR_BLUE, IR_PINK, IR_AMBER, IR_MAG };

struct MoodDef {
    float open, slope, arch, pw, brow, smile, mopen, fang, ear, size, asym, tilt;
    uint8_t iris;
    uint8_t pupil;
    uint16_t deco;
    bool fire;
    float lockX, lockY;  // fixed gaze offset (screen px)
};

//                      open  slope arch  pw   brow  smile mopen fang  ear   size asym  tilt
const MoodDef kMoods[DragonFace::MoodCount] = {
    /* Neutral   */ {.85f, 0.f, 0.f, 5.f, 0.f, .4f, 0.f, .4f, 0.f, 1.f, 0.f, 0.f, IR_MAIN, PK_OVAL, 0, false, 0, 0},
    /* Happy     */ {.9f, 0.f, 1.f, 5.f, .4f, 1.f, .25f, .7f, 1.f, 1.f, 0.f, 0.f, IR_MAIN, PK_OVAL, D_SPARK, false, 0, 0},
    /* Angry     */ {.75f, 1.f, 0.f, 3.f, -.4f, -.5f, .15f, 1.f, -.7f, 1.f, 0.f, 0.f, IR_RED, PK_OVAL, D_VEIN | D_SMOKE, false, 0, 0},
    /* Sad       */ {.7f, -1.f, 0.f, 9.f, .6f, -1.f, 0.f, 0.f, -1.f, 1.f, 0.f, 0.f, IR_BLUE, PK_OVAL, D_TEAR, false, 0, 0},
    /* Doubt     */ {.9f, .25f, 0.f, 4.f, .3f, -.1f, 0.f, .2f, 0.f, 1.f, .7f, 1.f, IR_MAIN, PK_OVAL, D_QMARK | D_SWEAT, false, 0, 0},
    /* Sleepy    */ {.1f, 0.f, 0.f, 5.f, -.4f, 0.f, 0.f, 0.f, -.6f, 1.f, 0.f, 0.f, IR_MAIN, PK_OVAL, D_ZZZ, false, 0, 0},
    /* Love      */ {1.05f, 0.f, 0.f, 5.f, .3f, .9f, .15f, .2f, .8f, 1.05f, 0.f, 0.f, IR_PINK, PK_HEART, D_HEARTS, false, 0, 0},
    /* Shy       */ {.7f, -.4f, 0.f, 8.f, .5f, .3f, 0.f, 0.f, -.5f, 1.f, 0.f, -.3f, IR_PINK, PK_OVAL, D_BLUSH, false, 0, 0},
    /* Dizzy     */ {.9f, 0.f, 0.f, 5.f, 0.f, -.2f, 0.f, .4f, -.3f, 1.f, 0.f, 1.f, IR_AMBER, PK_SPIRAL, D_STARS, false, 0, 0},
    /* Unhinged  */ {1.15f, .5f, 0.f, 2.f, 0.f, 1.f, .35f, 1.f, .9f, 1.05f, 1.f, 0.f, IR_MAG, PK_OVAL, D_SPARK | D_GLITCH | D_SMOKE, false, 0, 0},
    /* Smug      */ {.55f, .3f, 0.f, 4.f, -.2f, .6f, 0.f, .7f, .2f, 1.f, .5f, -1.f, IR_MAIN, PK_OVAL, 0, false, 0, 0},
    /* Fire      */ {1.f, .9f, 0.f, 2.f, -.3f, .4f, 1.f, 1.f, .6f, 1.f, 0.f, 0.f, IR_AMBER, PK_OVAL, 0, true, 0, 0},
    /* Thinking  */ {.8f, .15f, 0.f, 4.f, .2f, 0.f, 0.f, 0.f, 0.f, 1.f, .3f, 0.f, IR_MAIN, PK_DOT, D_DOTS, false, -6, -5},
    /* Listening */ {1.1f, 0.f, 0.f, 7.f, .5f, .1f, 0.f, .4f, 1.f, 1.05f, 0.f, 0.f, IR_MAIN, PK_OVAL, D_WAVE, false, 0, 0},
    /* Error     */ {.9f, 0.f, 0.f, 5.f, 0.f, -.3f, 0.f, .4f, -1.f, 1.f, 0.f, 0.f, IR_RED, PK_X, D_GLITCH, false, 0, 0},
};

const float kIris[6][3] = {{57, 255, 136}, {255, 77, 94}, {61, 180, 255}, {255, 95, 160}, {255, 176, 32}, {255, 43, 214}};

// Screen colours (Midori palette)
const float kMain[3] = {57, 255, 136};
const float kDim[3]  = {15, 107, 60};

inline float mixf(float a, float b, float t)
{
    return a + (b - a) * t;
}
inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

}  // namespace

// ---------------------------------------------------------------------------------------------

DragonFace::DragonFace()
{
    std::memset(_buf, 0, sizeof(_buf));
    std::memset(_parts, 0, sizeof(_parts));
    _in = Input();
}

float DragonFace::rnd()
{
    _rngState ^= _rngState << 13;
    _rngState ^= _rngState >> 17;
    _rngState ^= _rngState << 5;
    return (_rngState & 0xFFFFFF) / 16777216.0f;
}

// ---------------------------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------------------------

void DragonFace::px(int x, int y, uint8_t c)
{
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
        return;
    }
    _buf[y][x] = c;
}

static inline int rnd2i(float v)
{
    return (int)std::floor(v + 0.5f);
}

void DragonFace::fillRect(float x, float y, float w, float h, uint8_t c)
{
    int x0 = rnd2i(x * .5f), x1 = rnd2i((x + w) * .5f);
    int y0 = rnd2i(y * .5f), y1 = rnd2i((y + h) * .5f);
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++) px(i, j, c);
}

void DragonFace::fillEllipse(float cx, float cy, float rx, float ry, float rot, uint8_t c)
{
    float bx = cx * .5f, by = cy * .5f;
    rx = std::max(rx * .5f, 0.6f);
    ry = std::max(ry * .5f, 0.6f);
    float cs = std::cos(rot), sn = std::sin(rot);
    float R = std::max(rx, ry) + 1.f;
    int i0 = (int)std::floor(bx - R), i1 = (int)std::ceil(bx + R);
    int j0 = (int)std::floor(by - R), j1 = (int)std::ceil(by + R);
    for (int j = j0; j <= j1; j++) {
        for (int i = i0; i <= i1; i++) {
            float dx = i + .5f - bx, dy = j + .5f - by;
            float u = dx * cs + dy * sn, v = -dx * sn + dy * cs;
            if ((u * u) / (rx * rx) + (v * v) / (ry * ry) <= 1.f) px(i, j, c);
        }
    }
}

template <typename Fn> void DragonFace::fillPolyFn(const float* pts, int n, Fn fn)
{
    if (n < 3) return;
    float miny = 1e9f, maxy = -1e9f;
    for (int i = 0; i < n; i++) {
        float y = pts[i * 2 + 1] * .5f;
        miny    = std::min(miny, y);
        maxy    = std::max(maxy, y);
    }
    int j0 = (int)std::ceil(miny - .5f), j1 = (int)std::floor(maxy - .5f);
    float xs[64];
    for (int j = j0; j <= j1; j++) {
        float yc = j + .5f;
        int k    = 0;
        for (int i = 0; i < n && k < 62; i++) {
            int a    = i, b = (i + 1) % n;
            float ya = pts[a * 2 + 1] * .5f, yb = pts[b * 2 + 1] * .5f;
            if ((ya <= yc && yb > yc) || (yb <= yc && ya > yc)) {
                float t = (yc - ya) / (yb - ya);
                xs[k++] = (pts[a * 2] + (pts[b * 2] - pts[a * 2]) * t) * .5f;
            }
        }
        std::sort(xs, xs + k);
        for (int m = 0; m + 1 < k; m += 2) {
            int ia = (int)std::ceil(xs[m] - .5f), ib = (int)std::floor(xs[m + 1] - .5f);
            for (int i = ia; i <= ib; i++) px(i, j, fn(i, j));
        }
    }
}

void DragonFace::fillPoly(const float* pts, int n, uint8_t c)
{
    fillPolyFn(pts, n, [c](int, int) { return c; });
}

void DragonFace::strokeLine(float x0, float y0, float x1, float y1, int thick, uint8_t c)
{
    float bx0 = x0 * .5f, by0 = y0 * .5f, bx1 = x1 * .5f, by1 = y1 * .5f;
    float L   = std::sqrt((bx1 - bx0) * (bx1 - bx0) + (by1 - by0) * (by1 - by0));
    int steps = std::max(1, (int)(L * 2.f));
    for (int s = 0; s <= steps; s++) {
        float t  = (float)s / steps;
        float cx = bx0 + (bx1 - bx0) * t, cy = by0 + (by1 - by0) * t;
        if (thick <= 1) {
            px((int)std::floor(cx), (int)std::floor(cy), c);
        } else if (thick == 2) {
            int ix = rnd2i(cx), iy = rnd2i(cy);
            px(ix - 1, iy - 1, c);
            px(ix, iy - 1, c);
            px(ix - 1, iy, c);
            px(ix, iy, c);
        } else {
            fillEllipse(cx * 2.f, cy * 2.f, thick, thick, 0.f, c);
        }
    }
}

void DragonFace::strokePoly(const float* pts, int n, int thick, uint8_t c, bool closed)
{
    for (int i = 0; i + 1 < n; i++) strokeLine(pts[i * 2], pts[i * 2 + 1], pts[i * 2 + 2], pts[i * 2 + 3], thick, c);
    if (closed && n > 2) strokeLine(pts[(n - 1) * 2], pts[(n - 1) * 2 + 1], pts[0], pts[1], thick, c);
}

void DragonFace::blit(const char* const* rows, int w, int h, float cx, float cy, float scale, uint8_t c, bool centered)
{
    int dw = std::max(1, rnd2i(w * scale)), dh = std::max(1, rnd2i(h * scale));
    int ox = centered ? rnd2i(cx * .5f - dw * .5f) : rnd2i(cx * .5f);
    int oy = centered ? rnd2i(cy * .5f - dh * .5f) : rnd2i(cy * .5f);
    for (int j = 0; j < dh; j++) {
        int sy = std::min(h - 1, j * h / dh);
        for (int i = 0; i < dw; i++) {
            int sx = std::min(w - 1, i * w / dw);
            if (rows[sy][sx] == '#') px(ox + i, oy + j, c);
        }
    }
}

// quadratic bezier into out[] (design coordinates); returns number of points appended
static int quad(float* out, int n0, float x0, float y0, float cx, float cy, float x1, float y1, int segs, bool skipFirst)
{
    int n = n0;
    for (int i = skipFirst ? 1 : 0; i <= segs; i++) {
        float t = (float)i / segs, u = 1.f - t;
        out[n * 2]     = u * u * x0 + 2 * u * t * cx + t * t * x1;
        out[n * 2 + 1] = u * u * y0 + 2 * u * t * cy + t * t * y1;
        n++;
    }
    return n;
}

// ---------------------------------------------------------------------------------------------
// Face parts
// ---------------------------------------------------------------------------------------------

void DragonFace::drawEars(int side)
{
    const Params& p = _cur;
    float x0 = 160.f + side * 118.f, y0 = 100.f;
    for (int i = 0; i < 3; i++) {
        float th = .2f + p.ear * .6f + (1 - i) * .35f;
        float L  = 40.f - i * 7.f;
        float by = y0 + i * 15.f, bx = x0 - side * (i * 2.f);
        float dx = side * std::cos(th), dy = -std::sin(th);
        float tri[6] = {bx, by - 6, bx + dx * L, by + dy * L, bx, by + 6};
        fillPoly(tri, 3, DARK);
        strokePoly(tri, 3, 1, INK, true);
    }
}

void DragonFace::drawHorn(int side)
{
    auto X = [side](float x) { return side < 0 ? x : 320.f - x; };
    float pts[40];
    int n = 0;
    n     = quad(pts, n, X(92), 50, X(80), 28, X(64), 4, 8, false);
    n     = quad(pts, n, X(64), 4, X(104), 14, X(122), 46, 8, true);
    fillPoly(pts, n, DARK);
    strokePoly(pts, n, 1, INK, true);
}

void DragonFace::drawTattoo()
{
    blit(kKanji, 16, 16, 248.f, 160.f, 1.f, TATTOO, true);
}

void DragonFace::drawEye(float cx, float cy, int dir, float sc, float browRaise, float openIn, float offx, float offy)
{
    const Params& p = _cur;
    const MoodDef& m = kMoods[_mood];
    float ex = cx + offx, ey = cy + offy;

    // spiky brow ridge
    float by0 = cy - 38.f - browRaise * 8.f;
    float bo = by0 - p.slope * 9.f, bi = by0 + p.slope * 13.f;
    float ox = cx - dir * 22.f, ix = cx + dir * 22.f;
    strokeLine(ox, bo, ix, bi, 3, INK);
    for (float u : {.3f, .65f}) {
        float X = ox + (ix - ox) * u, Y = bo + (bi - bo) * u;
        float tri[6] = {X - 4, Y - 2, X + 4, Y - 2, X + dir * 1.f, Y - 10};
        fillPoly(tri, 3, INK);
    }

    if (p.arch > .5f) {
        // happy ^ eye
        float pts[24];
        int n = quad(pts, 0, ex - 17, ey + 8, ex, ey - 26, ex + 17, ey + 8, 8, false);
        strokePoly(pts, n, 3, INK, false);
        return;
    }

    float ob = std::max(.03f, p.open * openIn);
    float rx = (8.f + p.pw * .65f) * sc;
    float ry = std::max(2.2f, 16.f * sc * ob / .85f);
    float rot = dir * p.slope * .4f;

    switch (m.pupil) {
        case PK_HEART:
            blit(kHeart, 7, 6, ex, ey + 2, 1.7f + (ry < 6 ? -0.5f : 0.f), HEART, true);
            break;
        case PK_SPIRAL: {
            float pts[90];
            int n = 0;
            for (float a = 0; a < 11.f && n < 44; a += .25f) {
                float r = a * 1.3f;
                pts[n * 2]     = ex + std::cos(a + _time * 8.f) * r;
                pts[n * 2 + 1] = ey + std::sin(a + _time * 8.f) * r;
                n++;
            }
            strokePoly(pts, n, 1, INK, false);
            break;
        }
        case PK_X:
            strokeLine(ex - 12, ey - 12, ex + 12, ey + 12, 2, INK);
            strokeLine(ex + 12, ey - 12, ex - 12, ey + 12, 2, INK);
            break;
        case PK_DOT:
            fillEllipse(ex, ey, 7.f, 7.f * std::min(1.f, openIn * 1.2f + .1f), 0.f, INK);
            break;
        default:
            fillEllipse(ex, ey, rx, ry, rot, INK);
            if (ry > 7.f) {
                float hx = -rx * .45f, hy = -ry * .6f;
                float cs = std::cos(rot), sn = std::sin(rot);
                float wx = ex + hx * cs - hy * sn, wy = ey + hx * sn + hy * cs;
                fillRect(wx - 2, wy - 2, 4, 4, WHITE);
            }
            break;
    }
}

void DragonFace::drawEyeOnly(float cx, float cy, int dir, float sc, float openIn, float offx, float offy)
{
    const Params& p  = _cur;
    const MoodDef& m = kMoods[_mood];
    float ex = cx + offx, ey = cy + offy;

    if (p.arch > .5f) {
        // happy ^ eye with glow
        float pts[24];
        int n = quad(pts, 0, ex - 26, ey + 12, ex, ey - 34, ex + 26, ey + 12, 8, false);
        strokePoly(pts, n, 9, SMOKE, false);
        strokePoly(pts, n, 5, INK, false);
        return;
    }

    float ob  = std::max(.05f, p.open * openIn);
    float rx  = (24.f + p.pw * .8f) * sc;
    float ry  = std::max(3.f, 36.f * sc * ob / .85f);
    float rot = dir * p.slope * .4f;

    switch (m.pupil) {
        case PK_HEART:
            fillEllipse(ex, ey, 30.f, 28.f, 0.f, SMOKE);
            blit(kHeart, 7, 6, ex, ey + 2, 5.f, HEART, true);
            break;
        case PK_SPIRAL: {
            float pts[130];
            int n = 0;
            for (float a = 0; a < 15.f && n < 62; a += .25f) {
                float r = a * 2.4f;
                pts[n * 2]     = ex + std::cos(a + _time * 8.f) * r;
                pts[n * 2 + 1] = ey + std::sin(a + _time * 8.f) * r;
                n++;
            }
            strokePoly(pts, n, 2, INK, false);
            break;
        }
        case PK_X:
            strokeLine(ex - 20, ey - 20, ex + 20, ey + 20, 3, INK);
            strokeLine(ex + 20, ey - 20, ex - 20, ey + 20, 3, INK);
            break;
        case PK_DOT:
            fillEllipse(ex, ey, 20.f, 20.f * std::min(1.f, openIn * 1.2f + .1f), 0.f, SMOKE);
            fillEllipse(ex, ey, 12.f, 12.f * std::min(1.f, openIn * 1.2f + .1f), 0.f, INK);
            break;
        default:
            fillEllipse(ex, ey, rx + 9.f, ry + 9.f, rot, SMOKE);
            fillEllipse(ex, ey, rx + 4.f, ry + 4.f, rot, DARK);
            fillEllipse(ex, ey, rx, ry, rot, INK);
            if (ry > 8.f) {
                fillEllipse(ex, ey, rx * .62f, ry * .62f, rot, TATTOO);
                // dragon slit pupil
                fillEllipse(ex, ey, std::max(3.f, rx * .13f), ry * .9f, rot, S);
                float hx = -rx * .45f, hy = -ry * .55f;
                float cs = std::cos(rot), sn = std::sin(rot);
                fillRect(ex + hx * cs - hy * sn - 3, ey + hx * sn + hy * cs - 3, 6, 6, WHITE);
            }
            break;
    }
}

void DragonFace::drawMouth(float openIn, float offx, float offy)
{
    const Params& p = _cur;
    float my        = 158.f + offy;
    float mx        = 160.f + offx;
    float mw        = 84.f + p.smile * 10.f;
    float lx = mx - mw / 2, rx = mx + mw / 2;
    float open = std::max(p.mopen, openIn * .9f);
    float cl = my - p.smile * 10.f + p.tilt * 6.f, cr = my - p.smile * 10.f - p.tilt * 6.f;
    float cu = my + p.smile * 22.f;
    float cL = cu + open * 56.f;

    float upper[28], lower[28], poly[56];
    int nu = quad(upper, 0, lx, cl, mx, cu, rx, cr, 12, false);
    if (open > .04f) {
        int nl = quad(lower, 0, rx, cr, mx, cL, lx, cl, 12, false);
        int n  = 0;
        for (int i = 0; i < nu; i++) { poly[n * 2] = upper[i * 2]; poly[n * 2 + 1] = upper[i * 2 + 1]; n++; }
        for (int i = 1; i < nl; i++) { poly[n * 2] = lower[i * 2]; poly[n * 2 + 1] = lower[i * 2 + 1]; n++; }
        float lowMid = (cl + cr) / 4.f + cL / 2.f;
        float tcx = mx * .5f, tcy = (lowMid - 4.f) * .5f;
        fillPolyFn(poly, n, [&](int i, int j) {
            float dx = (i + .5f - tcx) / 11.f, dy = (j + .5f - tcy) / 4.5f;
            return (dx * dx + dy * dy <= 1.f) ? (uint8_t)TONGUE : (uint8_t)INK;
        });
        strokePoly(upper, nu, 2, INK, false);
        strokePoly(lower, nl, 2, INK, false);
        // lower fangs
        if (open > .45f) {
            for (int s : {-1, 1}) {
                float x = mx + s * mw * .24f, u = (x - lx) / (rx - lx);
                float yy = (1 - u) * (1 - u) * cl + 2 * u * (1 - u) * cL + u * u * cr;
                float tri[6] = {x - 4, yy + 1, x + 4, yy + 1, x, yy - 5 - open * 3};
                fillPoly(tri, 3, WHITE);
            }
        }
    } else {
        strokePoly(upper, nu, 2, INK, false);
    }
    // upper fangs
    float fh = p.fang * 13.f + open * 4.f;
    if (fh > 1.5f) {
        for (int s : {-1, 1}) {
            float x = mx + s * mw * .28f, u = (x - lx) / (rx - lx);
            float yy = (1 - u) * (1 - u) * cl + 2 * u * (1 - u) * cu + u * u * cr;
            float tri[6] = {x - 4, yy + 1.5f, x + 4, yy + 1.5f, x, yy + fh};
            fillPoly(tri, 3, WHITE);
        }
    }
}

void DragonFace::drawDecor()
{
    uint16_t d = kMoods[_mood].deco;
    float t    = _time;
    if ((d & D_BLUSH) && !_eyesOnly) {
        for (int i = 0; i < 3; i++) fillRect(74.f + i * 10.f, 146.f, 6.f, 2.f, INK);
    }
    if (d & D_VEIN) {
        float s = 1.5f + .2f * std::sin(t * 10.f);
        for (int a : {-1, 1}) {
            for (int b : {-1, 1}) {
                float pts[6] = {262 + a * 3 * s, 52 + b * 9 * s, 262 + a * 3 * s, 52 + b * 3 * s, 262 + a * 9 * s, 52 + b * 3 * s};
                strokePoly(pts, 3, 1, RED, false);
            }
        }
    }
    if (d & D_SWEAT) {
        float y = 74.f + std::fmod(t * 22.f, 30.f);
        fillEllipse(262, y + 4, 4, 4, 0, BLUE);
        float tri[6] = {258, y + 3, 262, y - 6, 266, y + 3};
        fillPoly(tri, 3, BLUE);
    }
    if (d & D_QMARK) {
        blit(kQuest, 5, 7, 278.f, 50.f + std::sin(t * 3.f) * 3.f, 2.f, AMBER, true);
    }
    if (d & D_DOTS) {
        int lit = ((int)(t * 2.5f)) % 4;
        for (int i = 0; i < 3; i++) fillRect(148.f + i * 12.f, 18.f, 7.f, 7.f, lit > i ? INK : DARK);
    }
    if (d & D_STARS) {
        for (int i = 0; i < 3; i++) {
            float a = t * 4.f + i * 2.1f;
            int bx = rnd2i((160.f + std::cos(a) * 92.f) * .5f), by = rnd2i((58.f + std::sin(a) * 14.f) * .5f);
            for (int k = -2; k <= 2; k++) { px(bx + k, by, AMBER); px(bx, by + k, AMBER); }
            px(bx + 1, by, AMBER); px(bx, by + 1, AMBER);
        }
    }
    if (d & D_WAVE) {
        for (int i = 0; i < 3; i++) {
            float ph = std::fmod(t * 1.6f + i * .33f, 1.f);
            if (ph > .85f) continue;
            float r = 8.f + ph * 26.f;
            for (int s : {-1, 1}) {
                float pts[18];
                int n = 0;
                for (int k = 0; k < 8; k++) {
                    float a = -1.1f + 2.2f * k / 7.f;
                    pts[n * 2]     = (s < 0 ? 30.f : 290.f) + (s < 0 ? 1 : -1) * std::cos(a) * r;
                    pts[n * 2 + 1] = 112.f + std::sin(a) * r;
                    n++;
                }
                strokePoly(pts, n, 1, INK, false);
            }
        }
    }
    if (d & D_SPARK) {
        static const float sp[4][2] = {{34, 48}, {286, 58}, {248, 26}, {56, 170}};
        for (int i = 0; i < 4; i++) {
            float s = .5f + .5f * std::sin(t * 4.f + i * 1.7f);
            if (s < .2f) continue;
            int r  = 1 + (int)(s * 3.f);
            int bx = rnd2i(sp[i][0] * .5f), by = rnd2i(sp[i][1] * .5f);
            for (int k = -r; k <= r; k++) { px(bx + k, by, ORANGE); px(bx, by + k, ORANGE); }
        }
    }
}

void DragonFace::drawParticles()
{
    for (const Particle& q : _parts) {
        if (!q.kind) continue;
        float u = q.t / q.max;
        switch (q.kind) {
            case 1:  // heart
                blit(kHeart, 7, 6, q.x, q.y, q.s > 2.5f ? 1.5f : 1.f, HEART, true);
                break;
            case 2: {  // Z
                float sc = 1.f + u * 0.9f;
                blit(kZ, 5, 7, q.x, q.y, sc, INK, true);
                break;
            }
            case 3: {  // smoke
                float s = q.s + u * 7.f;
                fillRect(q.x - s / 2, q.y - s / 2, s, s, SMOKE);
                break;
            }
            case 4:  // tear
                fillRect(q.x, q.y, 4.f, 8.f, BLUE);
                break;
            case 5: {  // fire
                if (u > .85f) break;
                uint8_t col = u < .2f ? YELLOW : (u < .5f ? ORANGE : (u < .75f ? AMBER : RED));
                float s     = q.s * (1.f - u * .5f);
                fillRect(q.x - s / 2, q.y - s / 2, s, s, col);
                break;
            }
        }
    }
}

void DragonFace::spawn(uint8_t kind, float x, float y, float vx, float vy, float max, float s)
{
    for (Particle& q : _parts) {
        if (!q.kind) {
            q = Particle{x, y, vx, vy, 0.f, max, s, kind};
            return;
        }
    }
}

void DragonFace::emit(float dt, int slot, float rate, uint8_t kind, float x, float y, float rx, float vx, float vy,
                      float max, float s)
{
    _spawnAcc[slot] += dt * rate;
    while (_spawnAcc[slot] >= 1.f) {
        _spawnAcc[slot] -= 1.f;
        spawn(kind, x + (rnd() - .5f) * rx, y, vx, vy, max, s);
    }
}

// ---------------------------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------------------------

void DragonFace::computePalette()
{
    float iris[3] = {_cur.ir, _cur.ig, _cur.ib};
    float dist    = std::sqrt((iris[0] - kMain[0]) * (iris[0] - kMain[0]) + (iris[1] - kMain[1]) * (iris[1] - kMain[1]) +
                           (iris[2] - kMain[2]) * (iris[2] - kMain[2]));
    float tint    = std::min(.5f, dist / 260.f);
    float k       = std::min(1.f, tint * 2.4f);
    float base[3], tgt[3], scr[3], ink[3], dk[3], tat[3], smk[3];
    static const float kWhiteish[3] = {225, 248, 238}, kGreyMint[3] = {120, 190, 165};
    for (int i = 0; i < 3; i++) {
        float a = mixf(kMain[i], kWhiteish[i], .58f);
        base[i] = mixf(a, kGreyMint[i], .15f);
        tgt[i]  = mixf(iris[i], 255.f, .5f);
        scr[i]  = mixf(base[i], tgt[i], k);
        ink[i]  = mixf(kDim[i], i == 0 ? 0.f : (i == 1 ? 12.f : 10.f), .72f);
        dk[i]   = mixf(scr[i], ink[i], .28f);
        tat[i]  = mixf(ink[i], scr[i], .06f);
        smk[i]  = mixf(scr[i], ink[i], .3f);
    }
    auto set = [this](int idx, float r, float g, float b) {
        _pal[idx][0] = (uint8_t)clampf(r, 0, 255);
        _pal[idx][1] = (uint8_t)clampf(g, 0, 255);
        _pal[idx][2] = (uint8_t)clampf(b, 0, 255);
    };
    if (_eyesOnly) {
        set(S, 3, 6, 6);
        set(INK, iris[0], iris[1], iris[2]);
        set(DARK, iris[0] * .5f, iris[1] * .5f, iris[2] * .5f);
        set(TATTOO, mixf(iris[0], 255.f, .6f), mixf(iris[1], 255.f, .6f), mixf(iris[2], 255.f, .6f));
    } else {
        set(S, scr[0], scr[1], scr[2]);
        set(INK, ink[0], ink[1], ink[2]);
        set(DARK, dk[0], dk[1], dk[2]);
        set(TATTOO, tat[0], tat[1], tat[2]);
    }
    set(WHITE, 255, 255, 255);
    set(TONGUE, 255, 105, 135);
    set(HEART, 255, 70, 140);
    set(RED, 255, 77, 94);
    set(AMBER, 255, 176, 32);
    set(BLUE, 61, 180, 255);
    set(MAG, 255, 43, 214);
    set(YELLOW, 255, 245, 140);
    set(ORANGE, 255, 130, 0);
    if (_eyesOnly) {
        set(SMOKE, iris[0] * .16f, iris[1] * .16f, iris[2] * .16f);
    } else {
        set(SMOKE, smk[0], smk[1], smk[2]);
    }
}

// ---------------------------------------------------------------------------------------------
// Update / render
// ---------------------------------------------------------------------------------------------

uint32_t DragonFace::hashState() const
{
    uint32_t h = 2166136261u;
    auto add   = [&h](int v) { h = (h ^ (uint32_t)v) * 16777619u; };
    const float* f = &_cur.open;
    for (int i = 0; i < 15; i++) add((int)(f[i] * 8.f));
    add(_mood);
    add((int)(_in.eyeOpenL * 20)); add((int)(_in.eyeOpenR * 20));
    add((int)_in.eyeOffLX); add((int)_in.eyeOffLY); add((int)_in.eyeOffRX); add((int)_in.eyeOffRY);
    add((int)(_in.eyeSizeL * 10)); add((int)(_in.eyeSizeR * 10));
    add((int)(_in.mouthOpen * 20)); add((int)_in.mouthOffX); add((int)_in.mouthOffY);
    add(_in.eyeVisibleL); add(_in.eyeVisibleR); add(_in.mouthVisible);
    return h;
}

bool DragonFace::update(const Input& in, uint32_t nowMs)
{
    if (_first) {
        _lastMs = nowMs;
    }
    float dt = clampf((nowMs - _lastMs) / 1000.f, 0.f, .1f);
    _lastMs  = nowMs;
    _time += dt;

    int mood = in.mood;
    if (mood < 0 || mood >= MoodCount) mood = Neutral;
    const MoodDef& m = kMoods[mood];

    // targets
    _tgt = Params{m.open, m.slope, m.arch, m.pw, m.brow, m.smile, m.mopen, m.fang, m.ear, m.size, m.asym, m.tilt,
                  kIris[m.iris][0], kIris[m.iris][1], kIris[m.iris][2]};
    if (_first) {
        _cur   = _tgt;
        _first = false;
    }
    _mood = mood;
    _in   = in;
    // tween (exp smoothing)
    float k    = 1.f - std::exp(-dt * 9.f);
    float* c   = &_cur.open;
    float* tg  = &_tgt.open;
    for (int i = 0; i < 15; i++) c[i] += (tg[i] - c[i]) * k;

    // particles
    bool animated = m.deco != 0 || m.fire || m.pupil == PK_SPIRAL;
    for (Particle& q : _parts) {
        if (!q.kind) continue;
        animated = true;
        q.t += dt;
        q.x += q.vx * dt;
        q.y += q.vy * dt;
        if (q.kind == 4) q.vy += 60.f * dt;
        if (q.kind == 1) q.x += std::sin(q.t * 4.f) * .4f;
        if (q.t >= q.max) q.kind = 0;
    }
    if (m.deco & D_HEARTS) emit(dt, 0, 3.f, 1, 160.f, 140.f, 100.f, (rnd() - .5f) * 12.f, -26.f - rnd() * 20.f, 2.2f, rnd() > .5f ? 3.f : 2.f);
    if (m.deco & D_ZZZ) emit(dt, 1, 1.1f, 2, 226.f, 78.f, 0.f, 9.f, -14.f, 2.6f, 10.f);
    if ((m.deco & D_SMOKE) && !m.fire) emit(dt, 2, 3.5f, 3, rnd() > .5f ? 172.f : 148.f, 128.f, 0.f, (rnd() - .5f) * 10.f, -22.f, 1.6f, 3.f);
    if (m.deco & D_TEAR) emit(dt, 3, 1.4f, 4, rnd() > .5f ? 78.f : 242.f, 122.f, 0.f, 0.f, 10.f, 1.1f, 2.f);
    if (m.fire) {
        _spawnAcc[4] += dt * 55.f;
        while (_spawnAcc[4] >= 1.f) {
            _spawnAcc[4] -= 1.f;
            float a  = 3.14159f * (.08f + rnd() * .84f);
            float sp = 70.f + rnd() * 90.f;
            float vx = std::cos(a) * sp * (rnd() > .5f ? 1.f : -1.f) * .9f;
            float vy = std::sin(a) * sp * .55f + 10.f;
            spawn(5, 160.f + (rnd() - .5f) * 26.f, 176.f, vx, vy, .35f + rnd() * .35f, 6.f + rnd() * 6.f);
        }
    }

    // glitch strips
    if (m.deco & D_GLITCH) {
        if (nowMs >= _glitchAt) {
            _glitchAt    = nowMs + 80 + (uint32_t)(rnd() * 500);
            _glitchCount = rnd() < .35f ? 0 : 1 + (int)(rnd() * 2.99f);
            for (int i = 0; i < _glitchCount; i++) {
                _glitchRows[i][0] = (int)(rnd() * 108.f);
                _glitchRows[i][1] = 2 + (int)(rnd() * 8.f);
                _glitchRows[i][2] = (rnd() > .5f ? 1 : -1) * (1 + (int)(rnd() * 6.f));
            }
        }
        animated = true;
    } else {
        _glitchCount = 0;
    }

    uint32_t h   = hashState();
    bool changed = animated || h != _lastHash;
    _lastHash    = h;
    return changed;
}

void DragonFace::render()
{
    computePalette();
    std::memset(_buf, S, sizeof(_buf));

    const MoodDef& m = kMoods[_mood];
    const Params& p  = _cur;

    if (!_eyesOnly) {
        drawEars(-1);
        drawEars(1);
        drawHorn(-1);
        drawHorn(1);
        drawTattoo();
    }

    // gaze lock / dizzy orbit
    float lockX = m.lockX, lockY = m.lockY;
    if (_mood == Dizzy) {
        lockX = std::cos(_time * 5.f) * 5.f;
        lockY = std::sin(_time * 5.f) * 4.f;
    }
    float aL = 1.f + p.asym * .25f, aR = 1.f - p.asym * .25f;
    if (_eyesOnly) {
        if (_in.eyeVisibleL) {
            drawEyeOnly(88.f, 120.f, 1, p.size * aL * (1.f + _in.eyeSizeL * .35f), _in.eyeOpenL, _in.eyeOffLX + lockX,
                        _in.eyeOffLY + lockY);
        }
        if (_in.eyeVisibleR) {
            drawEyeOnly(232.f, 120.f, -1, p.size * aR * (1.f + _in.eyeSizeR * .35f), _in.eyeOpenR,
                        _in.eyeOffRX + lockX, _in.eyeOffRY + lockY);
        }
    } else if (_in.eyeVisibleL) {
        drawEye(96.f, 104.f, 1, p.size * aL * (1.f + _in.eyeSizeL * .35f), p.brow + p.asym * .8f, _in.eyeOpenL,
                _in.eyeOffLX + lockX, _in.eyeOffLY + lockY);
    }
    if (!_eyesOnly && _in.eyeVisibleR) {
        drawEye(224.f, 104.f, -1, p.size * aR * (1.f + _in.eyeSizeR * .35f), p.brow - p.asym * .3f, _in.eyeOpenR,
                _in.eyeOffRX + lockX, _in.eyeOffRY + lockY);
    }
    if (!_eyesOnly) {
        fillRect(150, 124, 4, 4, INK);
        fillRect(166, 124, 4, 4, INK);
    }
    if (_in.mouthVisible && !_eyesOnly) {
        drawMouth(_in.mouthOpen, _in.mouthOffX, _in.mouthOffY);
    }
    drawDecor();
    drawParticles();

    // glitch: shift a few pixel rows sideways
    for (int g = 0; g < _glitchCount; g++) {
        int y = _glitchRows[g][0], hh = _glitchRows[g][1], dx = _glitchRows[g][2];
        for (int j = y; j < y + hh && j < kHeight; j++) {
            uint8_t tmp[kWidth];
            std::memcpy(tmp, _buf[j], kWidth);
            for (int i = 0; i < kWidth; i++) {
                int s = i - dx;
                _buf[j][i] = (s >= 0 && s < kWidth) ? tmp[s] : (uint8_t)S;
            }
        }
    }
}

}  // namespace stackchan::avatar
