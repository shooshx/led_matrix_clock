#pragma once
#include "GamesState.h"

// Color game of life: every live cell has a hue (0-255). it starts with half the cells alive in random hues,
// a new cell gets the average hue of its 3 parents. the edges wrap around.
// Home pauses / resumes, Volume+/- the speed (up to a generation every frame), and drawing with the remote
// like in the draw mode, also while paused: moving shows the cursor, holding OK draws live cells,
// Left/Right pick the color, Up/Down the pen size.
// the field is in panel coordinates, so it works in any orientation and turns with the panel
namespace life_art {
static const char* const ICON[] = {
    "..R......",
    "...Y.....",
    ".GBM.....",
    ".........",
    ".....O...",
    "......B..",
    "....RGY..",
};
static const Sprite S_ICON = { 9, 7, ICON };
}

class LifeGame : public Game
{
public:
    static const int W = DISPLAY_WIDTH;
    static const int H = DISPLAY_HEIGHT;

    LifeGame() {
        // hue -> RGB565, full saturation and value. made once, no hue math while running
        for (int h = 0; h < 256; ++h) {
            int region = h / 43, rem = (h - region * 43) * 6;
            uint8_t q = 255 - rem, t = rem, r, g, b;
            switch (region) {
                case 0:  r = 255; g = t;   b = 0;   break;
                case 1:  r = q;   g = 255; b = 0;   break;
                case 2:  r = 0;   g = 255; b = t;   break;
                case 3:  r = 0;   g = q;   b = 255; break;
                case 4:  r = t;   g = 0;   b = 255; break;
                default: r = 255; g = 0;   b = q;   break;
            }
            m_hue_rgb[h] = rgb565(r, g, b);
        }
    }

    const char* name() const override { return "Life"; }
    bool uses_volume_keys() const override { return true; }  // the speed
    const Sprite& icon() const override { return life_art::S_ICON; }
    bool portrait() const override { return false; }
    bool any_orientation() const override { return true; }
    bool has_high_score() const override { return false; }
    void load(Preferences& pref) override {}

    // in the pause menu Home fills the field again with random cells, Menu clears it
    const char* pause_option(int i) const override {
        static const char* const options[] = { "Home: random", "Menu: clear" };
        return i < 2 ? options[i] : nullptr;
    }
    bool on_pause_key(uint16_t key) override {
        if (key == HID_KEY_REMOTE_HOME) {
            start();
            return true;
        }
        if (key == HID_KEY_MENU) {
            memset(m_alive[m_cur], 0, sizeof(m_alive[m_cur]));
            return true;
        }
        return false;
    }
    uint32_t high_score() const override { return 0; }

    void start() override {
        m_cur = 0;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                m_alive[0][y][x] = random(0, 2);
                m_hue[0][y][x] = random(0, 256);
            }
        m_running = true;
        m_frame = 0;
        m_pen_down = false;
        m_cursor_frames = 0;
        m_speed_frames = 0;
    }

    void on_hid(const HidEvent& ev) override {
        if (ev.type == HidEvent::MOVE) {
            show_cursor();
            move(ev.dx, ev.dy);
            return;
        }
        if (ev.key == HID_KEY_ENTER && !ev.repeat) {
            // only a press shows the cursor: the release of the OK that started the game or closed
            // the pause menu comes here too, without its press
            if (ev.type == HidEvent::KEY_DOWN) {
                m_pen_down = true;
                show_cursor();
                plot_pen(cur_x(), cur_y());
            }
            else
                m_pen_down = false;
            return;
        }
        if (ev.type != HidEvent::KEY_DOWN)
            return;
        switch (ev.key) {
            case HID_KEY_REMOTE_HOME:
                if (!ev.repeat)
                    m_running = !m_running;
                break;
            case HID_KEY_VOL_UP:   set_speed(m_speed + 1); break;
            case HID_KEY_VOL_DOWN: set_speed(m_speed - 1); break;
            case HID_KEY_RIGHT:    m_color = (m_color + 1) % COLORS; show_cursor(); break;
            case HID_KEY_LEFT:     m_color = (m_color + COLORS - 1) % COLORS; show_cursor(); break;
            case HID_KEY_UP:       if (m_pen_r < PEN_SIZES - 1) ++m_pen_r; show_cursor(); break;
            case HID_KEY_DOWN:     if (m_pen_r > 0) --m_pen_r; show_cursor(); break;
        }
    }

    void tick() override {
        ++m_frame;
        if (m_cursor_frames > 0)
            --m_cursor_frames;
        if (m_speed_frames > 0)
            --m_speed_frames;
        if (m_running && m_frame % FRAMES_PER_GEN[m_speed] == 0)
            step(); // at most one generation per frame
    }

    void draw() override {
        uint8_t rotation = display.getRotation();
        display.setRotation(0);
        const uint8_t (&alive)[H][W] = m_alive[m_cur];
        const uint8_t (&hue)[H][W] = m_hue[m_cur];
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (alive[y][x])
                    display.drawPixel(x, y, m_hue_rgb[hue[y][x]]);
        if (m_cursor_frames > 0)
            display.drawCircle(cur_x(), cur_y(), m_pen_r + 2, m_hue_rgb[COLOR_HUES[m_color]]);
        display.setRotation(rotation);

        // in the current orientation
        if (!m_running && (m_frame / 15) % 2 == 0) {
            display.fillRect(1, 1, 9, 9, 0);
            display.fillRect(2, 2, 2, 7, rgb565(255, 255, 255));
            display.fillRect(6, 2, 2, 7, rgb565(255, 255, 255));
        }
        if (m_speed_frames > 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d gen/s", GamesState::FPS / FRAMES_PER_GEN[m_speed]);
            int w = text_width(buf);
            int x = (display.width() - w) / 2, y = display.height() / 2 - 4;
            display.fillRect(x - 2, y - 2, w + 4, 11, 0);
            draw_text(x, y, buf, rgb565(255, 255, 255));
        }
    }

private:
    static const int COLORS = 6;
    static constexpr uint8_t COLOR_HUES[COLORS] = { 0, 43, 85, 128, 170, 213 }; // red yellow green cyan blue magenta
    static const int SPEEDS = 6;
    static constexpr uint8_t FRAMES_PER_GEN[SPEEDS] = { 30, 15, 6, 3, 2, 1 }; // 1 2 5 10 15 30 generations / s
    static const int PEN_SIZES = 3;
    static constexpr float CURSOR_SPEED = 0.25f; // like the draw mode
    static const int CURSOR_SHOW_FRAMES = 5 * GamesState::FPS;
    static const int SPEED_SHOW_FRAMES = GamesState::FPS;

    uint8_t m_alive[2][H][W];
    uint8_t m_hue[2][H][W];
    int m_cur = 0;          // the current generation of the 2 buffers
    uint16_t m_hue_rgb[256];
    bool m_running = true;
    int m_frame = 0;
    int m_speed = SPEEDS - 1;
    int m_color = 0;
    int m_pen_r = 0;
    bool m_pen_down = false;
    float m_x = W / 2, m_y = H / 2;
    int m_cursor_frames = 0;
    int m_speed_frames = 0;

    // the hue in the middle of the shorter way around: a difference as int8_t wraps around the 256 hues
    static uint8_t hue_mid(uint8_t a, uint8_t b) {
        return a + (int8_t)(uint8_t)(b - a) / 2;
    }
    // the average of 3 hues: the middle of two, then a third of the way to the third one
    static uint8_t hue_avg3(uint8_t a, uint8_t b, uint8_t c) {
        uint8_t m = hue_mid(a, b);
        return m + (int8_t)(uint8_t)(c - m) / 3;
    }

    void step() {
        const uint8_t (&alive)[H][W] = m_alive[m_cur];
        const uint8_t (&hue)[H][W] = m_hue[m_cur];
        uint8_t (&nalive)[H][W] = m_alive[1 - m_cur];
        uint8_t (&nhue)[H][W] = m_hue[1 - m_cur];
        for (int y = 0; y < H; ++y) {
            int rows[3] = { (y + H - 1) % H, y, (y + 1) % H };
            for (int x = 0; x < W; ++x) {
                int cols[3] = { (x + W - 1) % W, x, (x + 1) % W };
                int n = 0;
                uint8_t parents[3];
                for (int dy = 0; dy < 3; ++dy)
                    for (int dx = 0; dx < 3; ++dx) {
                        if (dy == 1 && dx == 1)
                            continue;
                        int ry = rows[dy], rx = cols[dx];
                        if (alive[ry][rx]) {
                            if (n < 3)
                                parents[n] = hue[ry][rx];
                            ++n;
                        }
                    }
                if (alive[y][x]) {
                    nalive[y][x] = (n == 2 || n == 3);
                    nhue[y][x] = hue[y][x];
                }
                else if (n == 3) { // born
                    nalive[y][x] = 1;
                    nhue[y][x] = hue_avg3(parents[0], parents[1], parents[2]);
                }
                else
                    nalive[y][x] = 0;
            }
        }
        m_cur = 1 - m_cur;
    }

    void set_speed(int s) {
        m_speed = constrain(s, 0, SPEEDS - 1);
        m_speed_frames = SPEED_SHOW_FRAMES;
    }

    void show_cursor() {
        m_cursor_frames = CURSOR_SHOW_FRAMES;
    }

    int cur_x() const { return (int)(m_x + 0.5f); }
    int cur_y() const { return (int)(m_y + 0.5f); }

    void move(int dx, int dy) {
        display.delta_to_physical(dx, dy);
        int px = cur_x(), py = cur_y();
        m_x = constrain(m_x + dx * CURSOR_SPEED, 0.0f, (float)(W - 1));
        m_y = constrain(m_y + dy * CURSOR_SPEED, 0.0f, (float)(H - 1));
        if (m_pen_down && (cur_x() != px || cur_y() != py))
            line(px, py, cur_x(), cur_y());
    }

    // live cells of the current color
    void plot_pen(int x, int y) {
        for (int dy = -m_pen_r; dy <= m_pen_r; ++dy)
            for (int dx = -m_pen_r; dx <= m_pen_r; ++dx) {
                if (dx * dx + dy * dy > m_pen_r * m_pen_r + m_pen_r)
                    continue;
                int px = x + dx, py = y + dy;
                if (px < 0 || py < 0 || px >= W || py >= H)
                    continue;
                m_alive[m_cur][py][px] = 1;
                m_hue[m_cur][py][px] = COLOR_HUES[m_color];
            }
    }

    // Bresenham
    void line(int x0, int y0, int x1, int y1) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            plot_pen(x0, y0);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
};

constexpr uint8_t LifeGame::COLOR_HUES[LifeGame::COLORS];
constexpr uint8_t LifeGame::FRAMES_PER_GEN[LifeGame::SPEEDS];
