#pragma once
// included by ClockState.h

// ---- the background effects of the clock faces, same as effects.js in the web-ui ----
// every effect is a function of the time t (seconds), the pixel and the settings: no state from frame to frame,
// the drops, flakes, fireflies... have properties from a hash of their number and their place from the time.
// the settings: color, size (1-64, what it means is in effects.js's FX_LIST), speed (1-10), pulse (all of it
// brightens and dims, only Border, Slide and Aurora), angle (degrees, the direction Slide moves in)
// the callers give t as the seconds of the hour (millis() wraps, and a float of the seconds since the start
// loses its fractions after a few hours)

enum SpecialEffect {
  FX_NONE = 0,
  FX_BORDER = 1,     // a border that fades to black towards the middle
  FX_SLIDE = 2,      // a band that fades to black on its sides and sweeps from side to side
  FX_TWINKLE = 3,    // stars that fade in and out
  FX_FIREFLIES = 4,  // soft dots that wander and glow
  FX_SKY_REMOVED = 5, // was Sky, a face saved with it shows no effect
  FX_AURORA = 6,     // soft waving bands
  FX_RAIN = 7,       // drops with tails
  FX_SNOW = 8,       // flakes that sway down
  FX_FIRE = 9,       // flames from the bottom
  FX_RIPPLES = 10,   // rings that grow and fade
  FX_RADAR = 11,     // a sweep around the middle
  FX_PLASMA = 12,    // flowing hues
  FX_WARP = 13,      // stars flying out of the middle
  FX_LAVA = 14,      // soft blobs that float, merge and split
  FX_TUNNEL = 15,    // flying into a twisting tunnel
  FX_CUBE = 16,      // a wireframe cube that tumbles and bounces off the sides
  FX_COUNT
};

const int FX_SIZE_MAX = 64;

// the pulse setting is only for these, the others have their own movement
inline bool fx_can_pulse(int fx)
{
    return fx == FX_BORDER || fx == FX_SLIDE || fx == FX_AURORA || fx == FX_TUNNEL;
}

// it moves, it's drawn again ~25 times a second
inline bool fx_animated(int fx, bool pulse)
{
    if (fx == FX_NONE || fx == FX_SKY_REMOVED)
        return false;
    if (fx == FX_BORDER)
        return pulse;
    return true;
}

// the brightness of an effect (0-1) as the leds should light it to look like the web-ui's preview.
// a screen shows dim colors much darker than they are (its gamma), leds light them as they are: the faint end of
// a fade, that is almost black on the screen, was clearly lit on the panel and stopped at once instead of fading
// to black. only for the effects, the panel's gamma pref is for everything
const float EFFECT_GAMMA = 2.2f;
inline float led_level(float a)
{
    return a <= 0 ? 0 : powf(a, EFFECT_GAMMA);
}

const float FX_TAU = 6.28318530718f;
const int FX_SIN_N = 1024;

struct FxTables {
    float sin[FX_SIN_N];
    uint8_t gamma8[256];  // a color channel as the leds should light it, like led_level()
    uint8_t* radar = nullptr;  // the angle of each pixel from the middle, in 1/256 turns
    int radar_w = 0, radar_h = 0;
    uint16_t* depth = nullptr; // the tunnel's depth of each pixel, 1/64 steps
    uint8_t* fog = nullptr;    // the tunnel's brightness of each pixel 0-255, dark in the middle (far away)
    uint8_t* tangle = nullptr; // the tunnel's angle of each pixel. these three are twice the display each way
    int depth_w = 0, depth_h = 0;
    uint8_t* lines = nullptr;  // Cube's lines before they're drawn, 0 where there's none
    int lines_n = 0;
    bool made = false;

    void make() {
        if (made)
            return;
        for (int i = 0; i < FX_SIN_N; ++i)
            sin[i] = sinf(i * FX_TAU / FX_SIN_N);
        for (int i = 0; i < 256; ++i)
            gamma8[i] = (uint8_t)(powf(i / 255.0f, EFFECT_GAMMA) * 255.0f + 0.5f);
        made = true;
    }
};
// ~4K, in PSRAM: the internal RAM is for the display's buffer and WiFi
inline FxTables& fx_tables() {
    static FxTables* t = nullptr;
    if (t == nullptr) {
        t = (FxTables*)heap_caps_calloc(1, sizeof(FxTables), MALLOC_CAP_SPIRAM);
        if (t == nullptr)
            t = (FxTables*)calloc(1, sizeof(FxTables));
    }
    t->make();
    return *t;
}

// the same 32 bit hash as fx_hash() in the web-ui
inline uint32_t fx_hash(uint32_t n)
{
    n ^= n >> 16;
    n *= 0x7feb352du;
    n ^= n >> 15;
    n *= 0x846ca68bu;
    n ^= n >> 16;
    return n;
}
inline float fx_rnd(uint32_t n)  // 0..1
{
    return fx_hash(n) / 4294967296.0f;
}
// sine from a table, thousands of sinf() a frame are too slow
inline float fsin(float x)
{
    return fx_tables().sin[((int)floorf(x * (FX_SIN_N / FX_TAU))) & (FX_SIN_N - 1)];
}
inline float fx_frac(float x)
{
    return x - floorf(x);
}
inline float fx_clamp01(float x)
{
    return x < 0 ? 0 : x > 1 ? 1 : x;
}

// smooth noise 0..1 on a grid of random values
inline float fx_noise(float x, float y)
{
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - xi, fy = y - yi;
    float u = fx * fx * (3 - 2 * fx), v = fy * fy * (3 - 2 * fy);
    auto at = [](int i, int j) { return fx_rnd((uint32_t)(i & 0xffff) | ((uint32_t)(j & 0xffff) << 16)); };
    float a = at(xi, yi), b = at(xi + 1, yi), c = at(xi, yi + 1), d = at(xi + 1, yi + 1);
    float ab = a + (b - a) * u, cd = c + (d - c) * u;
    return ab + (cd - ab) * v;
}

// rgb (0-255) to hue (0-360), and back with saturation / value 0-1
inline float fx_hue(float r, float g, float b)
{
    float mx = max(r, max(g, b)), mn = min(r, min(g, b)), d = mx - mn;
    if (d == 0)
        return 0;
    float h = mx == r ? fmodf((g - b) / d, 6) : mx == g ? (b - r) / d + 2 : (r - g) / d + 4;
    return fmodf(h * 60 + 360, 360);
}
inline void fx_hsv(float h, float s, float v, float& r, float& g, float& b)
{
    h = fmodf(fmodf(h, 360) + 360, 360);
    float c = v * s, x = c * (1 - fabsf(fmodf(h / 60, 2) - 1)), m = v - c;
    if (h < 60)       { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    r = roundf((r + m) * 255);
    g = roundf((g + m) * 255);
    b = roundf((b + m) * 255);
}

// the radar's angle of each pixel from the middle, in 1/256 turns, for the size of the display now
inline const uint8_t* radar_angles(int w, int h)
{
    FxTables& t = fx_tables();
    if (t.radar == nullptr || t.radar_w != w || t.radar_h != h) {
        if (t.radar == nullptr) {
            // 128 x 64, the same in both orientations
            t.radar = (uint8_t*)heap_caps_calloc(w * h, 1, MALLOC_CAP_SPIRAM);
            if (t.radar == nullptr)
                t.radar = (uint8_t*)calloc(w * h, 1);
        }
        float cx = (w - 1) / 2.0f, cy = (h - 1) / 2.0f;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                t.radar[y * w + x] = (uint8_t)((int)floorf(fx_frac(atan2f(y - cy, x - cx) / FX_TAU) * 256) & 255);
        t.radar_w = w;
        t.radar_h = h;
    }
    return t.radar;
}

// the tunnel's angle (1/256 turns), depth (1 / the distance from the middle, 1/64 steps) and brightness (0-255,
// dark in the middle, far away) of each pixel of an area twice the display's size in each direction, its middle
// in the area's middle. the tunnel's middle moves around by drawing from another place of it (the demoscene way)
const float TUNNEL_DEPTH = 32.0f;
struct TunnelTables { const uint8_t* angle; const uint16_t* depth; const uint8_t* fog; int tw; };
inline bool tunnel_tables(int w, int h, TunnelTables& out)
{
    FxTables& t = fx_tables();
    int tw = 2 * w, th = 2 * h;
    if (t.depth == nullptr) {
        // 128 x 64 the same in both orientations, ~128K
        t.depth = (uint16_t*)heap_caps_calloc(tw * th, sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        t.fog = (uint8_t*)heap_caps_calloc(tw * th, 1, MALLOC_CAP_SPIRAM);
        t.tangle = (uint8_t*)heap_caps_calloc(tw * th, 1, MALLOC_CAP_SPIRAM);
        if (t.depth == nullptr || t.fog == nullptr || t.tangle == nullptr)
            return false;
    }
    if (t.depth_w != w || t.depth_h != h) {
        float cx = w - 0.5f, cy = h - 0.5f;  // at the display's middle when it isn't moved
        float edge = min(w, h) * 0.45f;
        for (int y = 0; y < th; ++y)
            for (int x = 0; x < tw; ++x) {
                int p = y * tw + x;
                float dx = x - cx, dy = y - cy;
                float d = max(0.75f, sqrtf(dx * dx + dy * dy));
                t.depth[p] = (uint16_t)min(65535.0f, TUNNEL_DEPTH / d * 64);
                float f = fx_clamp01(d / edge);
                t.fog[p] = (uint8_t)(f * f * 255);
                t.tangle[p] = (uint8_t)((int)floorf(fx_frac(atan2f(dy, dx) / FX_TAU) * 256) & 255);
            }
        t.depth_w = w;
        t.depth_h = h;
    }
    out.angle = t.tangle;
    out.depth = t.depth;
    out.fog = t.fog;
    out.tw = tw;
    return true;
}

// a circle of radius r around cx, cy (midpoint), set(x, y) for each pixel
template<typename F>
void fx_circle(int cx, int cy, int r, F set)
{
    if (r <= 0) {
        set(cx, cy);
        return;
    }
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        set(cx + x, cy + y); set(cx + y, cy + x); set(cx - y, cy + x); set(cx - x, cy + y);
        set(cx - x, cy - y); set(cx - y, cy - x); set(cx + y, cy - x); set(cx + x, cy - y);
        ++y;
        if (err < 0)
            err += 2 * y + 1;
        else {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

// the background effect, over the clock's background and under the texts. t in seconds, angle in degrees (Slide)
void draw_effect(int fx, uint16_t color, int fx_size, int fx_speed, bool pulse, int angle, float t)
{
    int w = display.width(), h = display.height();
    uint8_t r, g, b;
    color888(color, r, g, b);
    int size = constrain(fx_size, 1, FX_SIZE_MAX);
    int speed = constrain(fx_speed, 1, 10);
    float k = (pulse && fx_can_pulse(fx)) ? 0.6f + 0.4f * sinf(FX_TAU * t * speed / 6.0f) : 1.0f;
    FxTables& tab = fx_tables();
    // the effect's color at a brightness 0..1, nothing when it's ~black (the background stays)
    auto dot = [&](int x, int y, float a) {
        a *= k;
        if (a < 0.004f || x < 0 || y < 0 || x >= w || y >= h)
            return;
        a = led_level(min(1.0f, a));
        display.drawPixel(x, y, (uint8_t)(r * a), (uint8_t)(g * a), (uint8_t)(b * a));
    };
    // a color of its own
    auto rgb = [&](int x, int y, float cr, float cg, float cb) {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;
        display.drawPixel(x, y, tab.gamma8[(int)min(255.0f, cr * k)], tab.gamma8[(int)min(255.0f, cg * k)],
                          tab.gamma8[(int)min(255.0f, cb * k)]);
    };

    if (fx == FX_BORDER) {
        // fades to black towards the middle. the color of each ring once
        uint8_t ring[FX_SIZE_MAX][3];
        for (int d = 0; d < size; ++d) {
            float a = 1.0f - (float)d / size;
            a = led_level(min(1.0f, a * a * k));
            ring[d][0] = r * a;
            ring[d][1] = g * a;
            ring[d][2] = b * a;
        }
        for (int y = 0; y < h; ++y) {
            bool edge_row = y < size || y >= h - size;
            for (int x = 0; x < w; ++x) {
                if (!edge_row && x == size && w - size > x) // skip the middle of the row
                    x = w - size;
                int d = min(min(x, y), min(w - 1 - x, h - 1 - y));
                if (d >= size)
                    continue;
                display.drawPixel(x, y, ring[d][0], ring[d][1], ring[d][2]);
            }
        }
    }
    else if (fx == FX_SLIDE) {
        // a band that sweeps from side to side, across the direction of the angle (0: left - right, 90: up - down)
        float ang = angle * (FX_TAU / 360.0f);
        float ca = cosf(ang), sa = sinf(ang);
        // where the pixels are along the direction: from the panel's corners
        float p0 = 0, p1 = 0;
        for (int c = 0; c < 4; ++c) {
            float p = (c & 1 ? w : 0) * ca + (c & 2 ? h : 0) * sa;
            p0 = c == 0 ? p : min(p0, p);
            p1 = c == 0 ? p : max(p1, p);
        }
        float period = 12.0f / speed;
        float ph = fmodf(t, period) / period;
        float tri = ph < 0.5f ? ph * 2 : 2 - ph * 2;
        float half = size / 2.0f;
        float cp = p0 - half + tri * (p1 - p0 + size);
        for (int y = 0; y < h; ++y) {
            float py = (y + 0.5f) * sa;
            for (int x = 0; x < w; ++x) {
                float a = 1.0f - fabsf((x + 0.5f) * ca + py - cp) / half;
                if (a > 0)
                    dot(x, y, a * a);
            }
        }
    }
    else if (fx == FX_TWINKLE) {
        // stars in fixed places, each fades in and out at its own rate, mostly dark
        int n = size * 6;
        for (int i = 0; i < n; ++i) {
            int x = fx_hash(i * 3 + 1) % w, y = fx_hash(i * 3 + 2) % h;
            float rate = (0.3f + 0.7f * fx_rnd(i * 3 + 3)) * speed / 5;
            float s = fsin(FX_TAU * (t * rate + fx_rnd(i * 7 + 5)));
            if (s > 0)
                dot(x, y, s * s * s);
        }
    }
    else if (fx == FX_FIREFLIES) {
        // soft dots that wander (two slow waves each) and glow
        int n = size;
        float sp5 = speed / 5.0f;
        for (int i = 0; i < n; ++i) {
            float fx_ = 0.05f + 0.1f * fx_rnd(i * 11 + 1), fy_ = 0.05f + 0.1f * fx_rnd(i * 11 + 2);
            int x = (int)roundf((w - 1) / 2.0f + (w - 3) / 2.0f * fsin(FX_TAU * (t * fx_ * sp5 + fx_rnd(i * 11 + 3))));
            int y = (int)roundf((h - 1) / 2.0f + (h - 3) / 2.0f * fsin(FX_TAU * (t * fy_ * sp5 + fx_rnd(i * 11 + 4))));
            float glow = 0.5f + 0.5f * fsin(FX_TAU * (t * (0.3f + 0.5f * fx_rnd(i * 11 + 6)) + fx_rnd(i * 11 + 5)));
            dot(x, y, glow);
            dot(x + 1, y, glow * 0.35f); dot(x - 1, y, glow * 0.35f);
            dot(x, y + 1, glow * 0.35f); dot(x, y - 1, glow * 0.35f);
            dot(x + 1, y + 1, glow * 0.12f); dot(x - 1, y + 1, glow * 0.12f);
            dot(x + 1, y - 1, glow * 0.12f); dot(x - 1, y - 1, glow * 0.12f);
        }
    }
    else if (fx == FX_AURORA) {
        // three soft bands. each is two waves that travel at their own speeds in opposite directions, drifts up and
        // down slowly, gets wider and narrower along it, and a brighter part runs along it
        float wd = max(1.0f, size / 2.0f);
        const float band_shine[3] = {1, 0.7f, 0.5f};
        float sp = speed / 3.0f;
        float drift[3];
        for (int j = 0; j < 3; ++j)
            drift[j] = h * (0.25f + 0.25f * j) + h * 0.1f * fsin(FX_TAU * t * sp * (0.011f + 0.007f * j) + j * 2.1f);
        for (int x = 0; x < w; ++x) {
            float u = (float)x / w;
            float centers[3], shine[3], width[3];
            for (int j = 0; j < 3; ++j) {
                centers[j] = drift[j]
                    + h * 0.13f * fsin(FX_TAU * (u * (0.8f + 0.45f * j) + t * sp * (0.05f + 0.03f * j)) + j * 1.7f)
                    + h * 0.07f * fsin(FX_TAU * (u * (2.1f + 0.6f * j) - t * sp * (0.11f + 0.05f * j)) + j * 4.1f);
                width[j] = wd * (0.7f + 0.3f * fsin(FX_TAU * (u * 1.3f + t * sp * (0.04f + 0.02f * j)) + j * 2.9f));
                float sh = 0.6f + 0.25f * fsin(FX_TAU * (u * 1.7f - t * sp * (0.09f + 0.04f * j)) + j)
                                + 0.15f * fsin(FX_TAU * (u * 3.3f + t * sp * (0.15f - 0.03f * j)) + j * 3.3f);
                shine[j] = sh * band_shine[j];
            }
            for (int y = 0; y < h; ++y) {
                float a = 0;
                for (int j = 0; j < 3; ++j) {
                    float d = 1 - fabsf(y - centers[j]) / width[j];
                    if (d > 0)
                        a += d * d * shine[j];
                }
                if (a > 0)
                    dot(x, y, min(1.0f, a));
            }
        }
    }
    else if (fx == FX_RAIN) {
        // drops falling in columns, a bright head and a fading tail
        int n = size * 2;
        for (int i = 0; i < n; ++i) {
            int x = fx_hash(i * 13 + 1) % w;
            float v = (20 + 40 * fx_rnd(i * 13 + 2)) * speed / 5;
            int len = 3 + fx_hash(i * 13 + 3) % 6;
            float period = h + len;
            int head = (int)floorf(fmodf(fmodf(t * v + fx_rnd(i * 13 + 4) * period, period) + period, period));
            for (int j = 0; j < len; ++j) {
                float a = 1 - (float)j / len;
                dot(x, head - j, a * a);
            }
        }
    }
    else if (fx == FX_SNOW) {
        // flakes falling slowly, swaying a little
        int n = size * 2;
        for (int i = 0; i < n; ++i) {
            float v = (4 + 8 * fx_rnd(i * 17 + 2)) * speed / 5;
            float period = h + 2;
            int y = (int)roundf(fmodf(fmodf(t * v + fx_rnd(i * 17 + 3) * period, period) + period, period) - 1);
            float sway = 1.5f * fsin(FX_TAU * (t * (0.1f + 0.2f * fx_rnd(i * 17 + 4)) + fx_rnd(i * 17 + 5)));
            int x = (((int)roundf(fx_rnd(i * 17 + 1) * w + sway) % w) + w) % w;
            dot(x, y, 0.6f + 0.4f * fx_rnd(i * 17 + 6));
        }
    }
    else if (fx == FX_FIRE) {
        // flames from the bottom: noise that rises, hotter below. the color, then towards white where it's hottest
        int fh = min(h, max(4, size * 2));
        for (int y = h - fh; y < h; ++y) {
            float rel = (float)(h - 1 - y) / fh;
            for (int x = 0; x < w; ++x) {
                float n = 0.65f * fx_noise(x * 0.25f, (y + t * speed * 3) * 0.25f) + 0.35f * fx_noise(x * 0.6f, (y + t * speed * 5) * 0.6f);
                float heat = fx_clamp01(n * 1.4f - rel * 1.2f + 0.05f);
                if (heat < 0.02f)
                    continue;
                if (heat < 0.5f)
                    rgb(x, y, r * heat / 0.5f, g * heat / 0.5f, b * heat / 0.5f);
                else {
                    float wt = (heat - 0.5f) / 0.5f * 0.7f;
                    rgb(x, y, r + (255 - r) * wt, g + (255 - g) * wt, b + (255 - b) * wt);
                }
            }
        }
    }
    else if (fx == FX_RIPPLES) {
        // rings that grow from random places and fade, a few at a time
        int n = max(1, size / 4);
        float life = 4.0f * 5 / speed;
        float max_r = max(w, h) * 0.6f;
        for (int j = 0; j < n; ++j) {
            float s = t / life + (float)j / n;
            int c = (int)floorf(s);
            float age = s - c;
            uint32_t id = (uint32_t)c * 16 + j;
            int cx = fx_hash(id * 2 + 1) % w, cy = fx_hash(id * 2 + 2) % h;
            float a = (1 - age) * (1 - age);
            fx_circle(cx, cy, (int)roundf(age * max_r), [&](int x, int y) { dot(x, y, a); });
        }
    }
    else if (fx == FX_RADAR) {
        // a sweep around the middle with a fading trail, a whole turn at 64
        const uint8_t* angles = radar_angles(w, h);
        if (angles == nullptr)
            return;
        float sweep = fx_frac(t * speed * 0.05f);
        float trail = min(1.0f, max(0.03f, size / (float)FX_SIZE_MAX));
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                float behind = fx_frac(sweep - angles[y * w + x] / 256.0f + 1);
                if (behind < trail) {
                    float a = 1 - behind / trail;
                    dot(x, y, a * a);
                }
            }
        dot((w - 1) / 2, (h - 1) / 2, 1);
    }
    else if (fx == FX_PLASMA) {
        // waves of hues around the color's hue. the place is warped by slow waves (not straight lines), two waves
        // that turn around slowly, and rings from a point that wanders
        // the size: how big the waves are, the bigger ones grow slower (64 is a few blobs, not one)
        float sc = 3.0f / sqrtf(size);
        // a palette that goes around: from the color's hue to 90 degrees each way and back, darker and brighter
        // twice on the way. the plasma's value goes around it twice, every value is colorful
        float base = fx_hue(r, g, b);
        float pal[64][3];
        for (int i = 0; i < 64; ++i) {
            float u = i / 64.0f;
            float tri = 1 - fabsf(2 * u - 1);  // 0..1..0
            fx_hsv(base + (tri - 0.5f) * 180, 1, 0.35f + 0.45f * (0.5f - 0.5f * cosf(FX_TAU * 2 * u)),
                   pal[i][0], pal[i][1], pal[i][2]);
        }
        float ts = t * speed;
        // two directions that turn, one each way
        float a1 = ts * 0.021f, a2 = 2.0f - ts * 0.016f;
        float d1x = cosf(a1) * 0.23f * sc, d1y = sinf(a1) * 0.23f * sc;
        float d2x = cosf(a2) * 0.17f * sc, d2y = sinf(a2) * 0.17f * sc;
        // the rings' middle
        float cx = w * (0.5f + 0.4f * sinf(ts * 0.037f)), cy = h * (0.5f + 0.4f * sinf(ts * 0.029f + 1.3f));
        float t1 = ts * 0.31f, t2 = ts * 0.23f, t3 = ts * 0.27f, t4 = ts * 0.13f;
        float rk = 0.25f * sc;
        // the warp of a row depends on its y, of a column on its x
        float warp_y[128], warp_x[128];
        for (int x = 0; x < w && x < 128; ++x)
            warp_x[x] = 4.0f * fsin(x * 0.11f * sc + t4);
        for (int y = 0; y < h && y < 128; ++y)
            warp_y[y] = 4.0f * fsin(y * 0.13f * sc - t4 * 1.3f);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                float wx = x + warp_y[y & 127], wy = y + warp_x[x & 127];
                float dx = wx - cx, dy = wy - cy;
                float v = fsin(wx * d1x + wy * d1y + t1) + fsin(wx * d2x + wy * d2y - t2)
                        + fsin(sqrtf(dx * dx + dy * dy) * rk - t3) + fsin(wx * 0.07f * sc - wy * 0.11f * sc + t4);
                // around the palette twice from v = -4 to 4, and it drifts slowly
                int i = (int)floorf(fx_frac(v * 0.25f + ts * 0.004f) * 64) & 63;
                rgb(x, y, pal[i][0], pal[i][1], pal[i][2]);
            }
    }
    else if (fx == FX_WARP) {
        // stars that start near the middle in a direction of their own and go faster as they get out, brighter and
        // with a longer streak. when it's out it starts again in another direction
        float cx = (w - 1) / 2.0f, cy = (h - 1) / 2.0f;
        float out = sqrtf(cx * cx + cy * cy) + 2;
        int n = size * 2;
        for (int i = 0; i < n; ++i) {
            float rate = (0.12f + 0.2f * fx_rnd(i * 19 + 1)) * speed / 5;
            float s = t * rate + fx_rnd(i * 19 + 2);
            int c = (int)floorf(s);
            float age = s - c;
            float ang = FX_TAU * fx_rnd((uint32_t)c * 977 + i * 19 + 3);
            float dx = fsin(ang + FX_TAU / 4), dy = fsin(ang);
            float d = 1 + age * age * out;
            float a = min(1.0f, 0.15f + age * 1.3f);
            int len = 1 + (int)(age * age * 8);
            for (int j = 0; j < len; ++j) {
                float dj = d - j;
                if (dj < 0)
                    break;
                dot((int)roundf(cx + dx * dj), (int)roundf(cy + dy * dj), a * (1 - (float)j / len));
            }
        }
    }
    else if (fx == FX_LAVA) {
        // blobs that float on slow waves. where they are near each other they melt together: every pixel adds up
        // how near it is to each, the shape is where that's over a threshold, with a soft edge. hues around the color
        const int nb = 6;
        float rad = 3 + size * 0.35f;
        float r2 = rad * rad;
        float sp = speed / 5.0f;
        float bx[nb], by[nb];
        for (int j = 0; j < nb; ++j) {
            float fx_ = 0.015f + 0.03f * fx_rnd(j * 23 + 1), fy_ = 0.015f + 0.03f * fx_rnd(j * 23 + 2);
            bx[j] = (w - 1) / 2.0f + (w * 0.42f) * fsin(FX_TAU * (t * fx_ * sp + fx_rnd(j * 23 + 3)));
            by[j] = (h - 1) / 2.0f + (h * 0.42f) * fsin(FX_TAU * (t * fy_ * sp + fx_rnd(j * 23 + 4)));
        }
        float base = fx_hue(r, g, b);
        float pal[32][3];
        for (int i = 0; i < 32; ++i)
            fx_hsv(base + (1 - i / 31.0f) * 25, 1, 0.25f + 0.5f * i / 31, pal[i][0], pal[i][1], pal[i][2]);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                float f = 0;
                for (int j = 0; j < nb; ++j) {
                    float dx = x - bx[j], dy = y - by[j];
                    f += r2 / (dx * dx + dy * dy + 1);
                }
                if (f < 0.6f)
                    continue;
                // the edge from 0.6 to 1, then brighter towards the middles
                float edge = fx_clamp01((f - 0.6f) / 0.4f);
                int i = (int)(fx_clamp01(1 - 1 / f) * 31);
                float e = edge * edge;
                rgb(x, y, pal[i][0] * e, pal[i][1] * e, pal[i][2] * e);
            }
    }
    else if (fx == FX_TUNNEL) {
        // stripes in the depth (rings coming out of the middle) and around it (the size: how many), twisting,
        // dark far away in the middle. now and then the tube turns: its far end bends to a side (mostly left or
        // right) while the near part stays, then it straightens again or turns the other way
        TunnelTables tt;
        if (!tunnel_tables(w, h, tt))
            return;
        float fly = t * speed * 0.12f, spin = t * speed * 0.03f;
        float around = FX_TAU * size / 256.0f;  // per 1/256 turn
        // the bend: one for each stretch of time, straight about half the times. it stays for a while, then
        // changes smoothly to the next one
        float seg = 3 + 30.0f / speed;
        float s = t / seg;
        int i = (int)floorf(s);
        float k = fx_clamp01((s - i - 0.55f) / 0.45f);
        k = k * k * (3 - 2 * k);
        auto bend = [&](int n, float& bx, float& by) {
            uint32_t u = (uint32_t)n * 7;
            bool straight = fx_rnd(u + 1) < 0.45f;
            bx = straight ? 0 : (fx_rnd(u + 2) * 2 - 1) * w * 0.4f;
            by = straight ? 0 : (fx_rnd(u + 3) * 2 - 1) * h * 0.15f;
        };
        float ax, ay, bx2, by2;
        bend(i, ax, ay);
        bend(i + 1, bx2, by2);
        float bend_x = ax + (bx2 - ax) * k, bend_y = ay + (by2 - ay) * k;  // where the far end's middle is
        // the middle of the ring at depth z is moved by bend * (1 - BEND_NEAR / z) from the display's middle: the
        // rings at the display's edges (z ~ BEND_NEAR) stay, the deep ones go all the way. it never moves a ring
        // past a nearer one (the bend grows slower than the rings shrink), so every pixel is on one ring, found by
        // going from the depth of the straight tube to the bent one a few times (it gets closer each time)
        const float BEND_NEAR = 0.4f;
        const int BEND_ROUNDS = 5;
        const float near64 = BEND_NEAR * 64;  // in the table's depth units
        int cx = w / 2, cy = h / 2;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int p0 = (y + cy) * tt.tw + x + cx;
                int p = p0;
                for (int it = 0; it < BEND_ROUNDS; ++it) {
                    float g = 1 - near64 / max(near64, (float)tt.depth[p]);
                    p = p0 - (int)floorf(bend_y * g + 0.5f) * tt.tw - (int)floorf(bend_x * g + 0.5f);
                }
                float z = tt.depth[p] / 64.0f;
                float rings = fsin(FX_TAU * (z * 1.5f + fly));
                float spokes = fsin(tt.angle[p] * around + FX_TAU * (z * 0.12f + spin));
                float a = 0.5f + 0.5f * rings * spokes;
                dot(x, y, a * a * tt.fog[p] / 255.0f);
            }
        }
    }
    else if (fx == FX_CUBE) {
        // a wireframe cube that tumbles and bounces off the sides, its far edges dimmer. a straight path that
        // bounces off the sides is a zigzag in x and another in y, so it's from the time too. it bounces where the
        // sphere around its drawing touches the side
        float bound = min(min(w, h) / 2.0f - 1, 4 + size * 0.45f);  // the most its drawing reaches from its middle
        float rad = bound * 0.75f;            // its corners from its middle
        float half = rad / 1.7320508f;        // half its edge
        float cam = rad * 4;                  // the eye's distance: the nearest corner is drawn 4/3 bigger
        float v = speed * 3.0f;               // pixels a second
        // where it is along the zigzag doesn't depend on the size (the room it has): a new size doesn't make it
        // jump to another place, it stays as far along between the sides
        auto zigzag = [&](float lo, float hi, float vv, float ph, float full) {
            float range = hi - lo;
            if (range <= 0)
                return (lo + hi) / 2;
            float u = fx_frac(t * vv / (2 * full) + ph);
            return lo + range * (1 - fabsf(1 - 2 * u));
        };
        float mx = zigzag(bound, w - 1 - bound, v, 0.13f, w - 1);
        float my = zigzag(bound, h - 1 - bound, v * 0.83f, 0.61f, h - 1);
        float ax = t * speed * 0.11f, ay = t * speed * 0.17f, az = t * speed * 0.05f;
        float sxa = sinf(ax), cxa = cosf(ax), sya = sinf(ay), cya = cosf(ay), sza = sinf(az), cza = cosf(az);
        float px[8], py[8], pz[8];
        for (int i = 0; i < 8; ++i) {
            float x = i & 1 ? half : -half, y = i & 2 ? half : -half, z = i & 4 ? half : -half;
            float y1 = y * cxa - z * sxa, z1 = y * sxa + z * cxa;     // around x
            float x2 = x * cya + z1 * sya, z2 = -x * sya + z1 * cya;  // around y
            float x3 = x2 * cza - y1 * sza, y3 = x2 * sza + y1 * cza; // around z
            float s = cam / (cam + z2);  // z towards the eye is negative
            px[i] = mx + x3 * s;
            py[i] = my + y3 * s;
            pz[i] = z2;
        }
        // the lines into a buffer, the brightest where they meet (a corner of a dim line doesn't hide a bright one)
        FxTables& ft = fx_tables();
        if (ft.lines == nullptr) {
            ft.lines = (uint8_t*)heap_caps_calloc(w * h, 1, MALLOC_CAP_SPIRAM);
            if (ft.lines == nullptr)
                return;
            ft.lines_n = w * h;
        }
        if (ft.lines_n < w * h)
            return;
        static std::vector<uint16_t> touched;
        touched.clear();
        auto plot = [&](int x, int y, float a) {
            if (x < 0 || y < 0 || x >= w || y >= h || a <= 0)
                return;
            int p = y * w + x;
            uint8_t c = (uint8_t)min(255.0f, a * 255);
            if (ft.lines[p] == 0)
                touched.push_back(p);
            if (c > ft.lines[p])
                ft.lines[p] = c;
        };
        // anti-aliased: each step along the line lights the two pixels around it, by how near they are
        auto line = [&](float x0, float y0, float x1, float y1, float a) {
            bool steep = fabsf(y1 - y0) > fabsf(x1 - x0);
            if (steep) {
                std::swap(x0, y0);
                std::swap(x1, y1);
            }
            if (x0 > x1) {
                std::swap(x0, x1);
                std::swap(y0, y1);
            }
            float g = x1 - x0 > 0 ? (y1 - y0) / (x1 - x0) : 0;
            for (int x = (int)roundf(x0); x <= (int)roundf(x1); ++x) {
                float y = y0 + (x - x0) * g;
                int yi = (int)floorf(y);
                float f = y - yi;
                if (steep) {
                    plot(yi, x, a * (1 - f));
                    plot(yi + 1, x, a * f);
                }
                else {
                    plot(x, yi, a * (1 - f));
                    plot(x, yi + 1, a * f);
                }
            }
        };
        for (int i = 0; i < 8; ++i)
            for (int bit = 1; bit < 8; bit <<= 1) {
                if (i & bit)
                    continue;
                int j = i | bit;
                float near_ = (1 - (pz[i] + pz[j]) / 2 / rad) / 2;  // 0 far .. 1 near
                line(px[i], py[i], px[j], py[j], 0.3f + 0.7f * near_);
            }
        for (uint16_t p : touched) {
            dot(p % w, p / w, ft.lines[p] / 255.0f);
            ft.lines[p] = 0;
        }
    }
}

// the time for the effects: the seconds of the hour, a float keeps its fractions
inline float fx_time()
{
    return (millis() % 3600000UL) / 1000.0f;
}
