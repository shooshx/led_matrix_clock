#pragma once
#include "GamesState.h"
#include "Orientation.h"
#include "RemotePen.h"

extern Orientation orientation;

// Pixel dust: colored sand that falls the way the panel is tilted, from the accelerometer.
// it starts with a row of 7 colored boxes at the bottom, like the classic demo (without its dark gray,
// that is black at a low brightness).
// Home pauses / resumes, and drawing with the remote like in Life, also while paused:
// moving shows the cursor, holding OK pours sand, Left/Right pick the color (the white one after the
// colors erases), Up/Down the pen size. 45 fps like the demo
// in the pause menu Home restarts, Menu clears all the sand.
//
// the physics is the one of Adafruit_PixelDust (Phil Burgess, Adafruit, BSD license), copied here so that
// every grain has its own color and grains can be added and removed (the library has a fixed count).
// the field is in panel coordinates, so it works in any orientation
namespace dust_art {
static const char* const ICON[] = {
    "....Y....",
    ".........",
    "...Y.....",
    ".....O...",
    "....YOY..",
    "..OYYOYO.",
    ".NOYOYYON",
};
static const Sprite S_ICON = { 9, 7, ICON };
}

class DustGame : public Game
{
public:
    static const int W = DISPLAY_WIDTH;
    static const int H = DISPLAY_HEIGHT;
    static const int MAX_GRAINS = W * H;
    static const int N_COLORS = 7;       // of the sand
    static const int ERASER = N_COLORS;  // the white pen after the colors removes sand
    static const int BOX_H = 8;
    static const int SCALE = 1;          // accelerometer scaling, like the demo
    static const int ELASTICITY = 128;   // 0-255 bounce
    static const int FPS = 45;           // like the demo: the sand moves at most a pixel a frame

    DustGame() {
        m_pen.colors = N_COLORS + 1;
        m_pen.color = 3; // yellow
        m_pen.show_frames = 5 * FPS;
    }

    int fps() const override { return FPS; }

    const char* name() const override { return "Sand"; }
    const Sprite& icon() const override { return dust_art::S_ICON; }
    bool portrait() const override { return false; }
    bool any_orientation() const override { return true; }
    bool has_high_score() const override { return false; }
    void load(Preferences& pref) override {}
    uint32_t high_score() const override { return 0; }

    const char* pause_option(int i) const override {
        static const char* const options[] = { "Home: restart", "Menu: clear" };
        return i < 2 ? options[i] : nullptr;
    }
    bool on_pause_key(uint16_t key) override {
        if (key == HID_KEY_REMOTE_HOME) {
            start();
            return true;
        }
        if (key == HID_KEY_MENU) {
            clear();
            return true;
        }
        return false;
    }

    void start() override {
        if (m_grain == nullptr) {
            // 10 bytes a grain, 80K: in PSRAM, the internal RAM is short
            m_grain = (Grain*)heap_caps_malloc(MAX_GRAINS * sizeof(Grain), MALLOC_CAP_SPIRAM);
            m_max = MAX_GRAINS;
            if (m_grain == nullptr) { // no PSRAM, less sand
                m_max = MAX_GRAINS / 8;
                m_grain = (Grain*)malloc(m_max * sizeof(Grain));
                if (m_grain == nullptr)
                    m_max = 0;
            }
            m_rand = esp_random() | 1;
        }
        clear();
        for (int c = 0; c < N_COLORS; ++c) {
            for (int y = H - BOX_H; y < H; ++y)
                for (int x = c * W / N_COLORS; x < (c + 1) * W / N_COLORS; ++x) // the boxes fill the width
                    add(x, y, c);
        }
        m_running = true;
        m_frame = 0;
        m_pen.reset();
    }

    void on_hid(const HidEvent& ev) override {
        if (m_pen.on_hid(ev, [this](int x, int y) {
                if (m_pen.color == ERASER)
                    remove(x, y);
                else
                    add(x, y, m_pen.color);
            }))
            return;
        if (ev.type == HidEvent::KEY_DOWN && !ev.repeat && ev.key == HID_KEY_REMOTE_HOME)
            m_running = !m_running;
    }

    void tick() override {
        ++m_frame;
        m_pen.tick();
        if (!m_running)
            return;
        float ax, ay, az;
        if (!orientation.read(ax, ay, az)) {
            // no accelerometer: down is the bottom of the current orientation
            int dx = 0, dy = 98;
            display.delta_to_physical(dx, dy);
            ax = dx / 10.0f;
            ay = dy / 10.0f;
            az = 0;
        }
        // up to 4G is 39200, more than an int16_t
        auto in16 = [](float a) { return (int16_t)constrain(a * 1000, -32767.0f, 32767.0f); };
        iterate(in16(ax), in16(ay), in16(az));
    }

    void draw() override {
        uint8_t rotation = display.getRotation();
        display.setRotation(0);
        for (int i = 0; i < m_n; ++i)
            display.drawPixel(m_grain[i].x >> 8, m_grain[i].y >> 8, COLORS[m_grain[i].color]);
        m_pen.draw_cursor(COLORS[m_pen.color]);
        display.setRotation(rotation);

        // in the current orientation
        if (!m_running && (m_frame / 15) % 2 == 0) {
            display.fillRect(1, 1, 9, 9, 0);
            display.fillRect(2, 2, 2, 7, rgb565(255, 255, 255));
            display.fillRect(6, 2, 2, 7, rgb565(255, 255, 255));
        }
    }

    // for the tests
    int grains() const { return m_n; }
    bool occupied(int x, int y) const { return get(x, y); }

private:
    struct Grain {
        int16_t x, y;   // 256 per pixel
        int16_t vx, vy; // up to 256, a pixel a frame
        uint8_t color;
    };
    static constexpr uint16_t COLORS[N_COLORS + 1] = {
        rgb565(120, 79, 23),   // brown
        rgb565(228, 3, 3),     // red
        rgb565(255, 140, 0),   // orange
        rgb565(255, 237, 0),   // yellow
        rgb565(0, 128, 38),    // green
        rgb565(0, 77, 255),    // blue
        rgb565(117, 7, 135),   // purple
        rgb565(255, 255, 255), // the eraser
    };

    Grain* m_grain = nullptr;
    int m_max = 0;
    int m_n = 0;
    uint8_t m_bitmap[H][W / 8];
    uint32_t m_rand = 1;
    bool m_running = true;
    int m_frame = 0;
    RemotePen m_pen;

    void set(int x, int y)   { m_bitmap[y][x >> 3] |= 0x80 >> (x & 7); }
    void unset(int x, int y) { m_bitmap[y][x >> 3] &= ~(0x80 >> (x & 7)); }
    bool get(int x, int y) const { return m_bitmap[y][x >> 3] & (0x80 >> (x & 7)); }

    void clear() {
        m_n = 0;
        memset(m_bitmap, 0, sizeof(m_bitmap));
    }

    // a grain at a free pixel
    void add(int x, int y, int color) {
        if (m_n >= m_max || get(x, y))
            return;
        Grain& g = m_grain[m_n++];
        g.x = x * 256;
        g.y = y * 256;
        g.vx = g.vy = 0;
        g.color = color;
        set(x, y);
    }

    // the grain at a pixel, if there is one. the last grain takes its place in the list
    void remove(int x, int y) {
        if (!get(x, y))
            return;
        for (int i = 0; i < m_n; ++i)
            if ((m_grain[i].x >> 8) == x && (m_grain[i].y >> 8) == y) {
                m_grain[i] = m_grain[--m_n];
                unset(x, y);
                return;
            }
    }

    // 0 .. n-1, xorshift, faster than random()
    int rnd(int n) {
        m_rand ^= m_rand << 13;
        m_rand ^= m_rand >> 17;
        m_rand ^= m_rand << 5;
        return (int)((m_rand >> 8) % (uint32_t)n);
    }

    static void bounce(int16_t& v) { v = (-v) * ELASTICITY / 256; }

    // a frame of Adafruit_PixelDust::iterate(), without the sorting
    void iterate(int16_t ax, int16_t ay, int16_t az) {
        ax = (int32_t)ax * SCALE / 256;
        ay = (int32_t)ay * SCALE / 256;
        az = abs((int32_t)az * SCALE / 2048);
        // a little random motion so stacks topple, more the more the panel is upright
        az = (az >= 4) ? 1 : 5 - az;
        ax -= az;
        ay -= az;
        int16_t az2 = az * 2 + 1;

        for (int i = 0; i < m_n; ++i) {
            Grain& g = m_grain[i];
            g.vx += ax + rnd(az2);
            g.vy += ay + rnd(az2);
            // at most a pixel a frame (as a 2D vector), so grains don't pass through each other
            int32_t v2 = (int32_t)g.vx * g.vx + (int32_t)g.vy * g.vy;
            if (v2 > 65536) {
                float v = sqrtf((float)v2);
                g.vx = (int)(256.0f * g.vx / v);
                g.vy = (int)(256.0f * g.vy / v);
            }
        }

        const int32_t xMax = W * 256 - 1, yMax = H * 256 - 1;
        for (int i = 0; i < m_n; ++i) {
            Grain& g = m_grain[i];
            int32_t newx = g.x + g.vx, newy = g.y + g.vy;
            if (newx < 0)         { newx = 0;    bounce(g.vx); }
            else if (newx > xMax) { newx = xMax; bounce(g.vx); }
            if (newy < 0)         { newy = 0;    bounce(g.vy); }
            else if (newy > yMax) { newy = yMax; bounce(g.vy); }

            int32_t oldidx = (g.y >> 8) * W + (g.x >> 8);
            int32_t newidx = (newy >> 8) * W + (newx >> 8);
            if (oldidx != newidx && get(newx >> 8, newy >> 8)) {
                int32_t delta = abs(newidx - oldidx);
                if (delta == 1) {          // blocked left or right
                    newx = g.x;
                    bounce(g.vx);
                }
                else if (delta == W) {     // blocked up or down
                    newy = g.y;
                    bounce(g.vy);
                }
                else {                     // diagonal: skid along one axis if it's free, the faster first
                    bool x_first = abs(g.vx) >= abs(g.vy);
                    bool x_free = !get(newx >> 8, g.y >> 8);
                    bool y_free = !get(g.x >> 8, newy >> 8);
                    if (x_first ? x_free : !y_free && x_free) {
                        newy = g.y;
                        bounce(g.vy);
                    }
                    else if (y_free) {
                        newx = g.x;
                        bounce(g.vx);
                    }
                    else {
                        newx = g.x;
                        newy = g.y;
                        bounce(g.vx);
                        bounce(g.vy);
                    }
                }
            }
            unset(g.x >> 8, g.y >> 8);
            g.x = newx;
            g.y = newy;
            set(newx >> 8, newy >> 8);
        }
    }
};

constexpr uint16_t DustGame::COLORS[DustGame::N_COLORS + 1];
