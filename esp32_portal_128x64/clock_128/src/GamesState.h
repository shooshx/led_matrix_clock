#pragma once
#include <Preferences.h>
#include <vector>
#include <initializer_list>
#include "base_utils.h"
#include "ble_hid.h"

// ---- drawing helpers for the games ----

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// pixel art, one char per pixel, '.' is transparent, the other chars are colors (sprite_color)
struct Sprite {
    uint8_t w;
    uint8_t h;
    const char* const* rows;
};

inline uint16_t sprite_color(char c) {
    switch (c) {
        case 'G': return rgb565(60, 200, 60);    // dino green
        case 'C': return rgb565(20, 150, 40);    // cactus
        case 'P': return rgb565(170, 80, 220);   // pterodactyl
        case 'Y': return rgb565(255, 210, 0);
        case 'O': return rgb565(255, 130, 0);
        case 'R': return rgb565(230, 30, 30);
        case 'B': return rgb565(40, 90, 255);
        case 'M': return rgb565(230, 40, 200);
        case 'N': return rgb565(140, 80, 30);    // brown
        case 'D': return rgb565(80, 80, 95);     // dim gray
        case 'W': return rgb565(255, 255, 255);
        case 'K': return 0;
        default:  return rgb565(255, 255, 255);
    }
}

// color_override >= 0 paints all the pixels in that color except white and black (eyes)
inline void draw_sprite(const Sprite& s, int x, int y, int color_override = -1) {
    for (int r = 0; r < s.h; ++r) {
        const char* row = s.rows[r];
        for (int c = 0; c < s.w; ++c) {
            char ch = row[c];
            if (ch == '.')
                continue;
            uint16_t col = (color_override >= 0 && ch != 'W' && ch != 'K') ? (uint16_t)color_override : sprite_color(ch);
            display.drawPixel(x + c, y + r, col);
        }
    }
}

// the default 5x7 font
inline int text_width(const char* s, int size = 1) {
    int n = strlen(s);
    return n == 0 ? 0 : n * 6 * size - size;
}
inline void draw_text(int x, int y, const char* s, uint16_t color, int size = 1) {
    display.setFont(nullptr);
    display.setTextSize(size);
    display.setTextColor(color);
    display.setCursor(x, y);
    display.print(s);
    display.setTextSize(1);
}
inline void draw_text_center(int y, const char* s, uint16_t color, int size = 1) {
    draw_text((display.width() - text_width(s, size)) / 2, y, s, color, size);
}

// a rainbow color that changes with t
inline uint16_t rainbow(int t) {
    static const uint16_t cols[] = { rgb565(255, 40, 40), rgb565(255, 160, 0), rgb565(255, 240, 0),
                                     rgb565(40, 230, 40), rgb565(40, 160, 255), rgb565(200, 60, 255) };
    return cols[(t / 2) % 6];
}

// ---- a game ----

class Game
{
public:
    virtual ~Game() {}
    virtual const char* name() const = 0;
    virtual const Sprite& icon() const = 0;
    virtual bool portrait() const = 0;        // the orientation it's played in
    virtual bool any_orientation() const { return false; } // or it doesn't matter
    virtual void load(Preferences& pref) = 0; // the high score
    virtual uint32_t high_score() const = 0;
    virtual bool has_high_score() const { return true; }
    // extra lines in the pause menu (pause_option(i) is null after the last), and their keys:
    // on_pause_key() returns true if it was one of them, then the game continues
    virtual const char* pause_option(int i) const { return nullptr; }
    virtual bool on_pause_key(uint16_t key) { return false; }
    virtual void start() = 0;                 // to its start screen
    virtual void tick() = 0;                  // a frame, FPS per second
    virtual void on_hid(const HidEvent& ev) = 0;
    virtual void draw() = 0;
};

// The games mode: a menu to pick a game with Left/Right and OK, and the game being played.
// in a game all the input goes to the game, Back pauses it: OK continues, Back again goes to the menu.
// when the panel doesn't stand the way the game is played, it waits and asks to turn the panel
class GamesState : public IScreen
{
public:
    static const int FPS = 30;
    static const unsigned long FRAME_MS = 1000 / FPS;

    GamesState(std::initializer_list<Game*> games) : m_games(games) {}

    void load() {
        m_pref.begin("games", false);
        for (Game* g : m_games)
            g->load(m_pref);
    }

    // when the mode is entered
    void enter() {
        m_playing = false;
        m_paused = false;
        m_need_draw = true;
    }

    // returns true if the input was taken. in the menu Up/Down aren't, they change the mode as usual
    bool on_hid(const HidEvent& ev) {
        bool press = ev.type == HidEvent::KEY_DOWN && !ev.repeat;
        if (!m_playing) {
            if (!press)
                return false;
            int n = m_games.size();
            if (ev.key == HID_KEY_RIGHT || ev.key == HID_KEY_LEFT) {
                m_sel = (m_sel + (ev.key == HID_KEY_RIGHT ? 1 : -1) + n) % n;
                m_need_draw = true;
                return true;
            }
            if (ev.key == HID_KEY_ENTER) {
                m_playing = true;
                m_paused = false;
                game()->start();
                m_need_draw = true;
                return true;
            }
            return false;
        }
        if (m_paused) {
            if (press && ev.key == HID_KEY_ENTER)
                m_paused = false;
            else if (press && game()->on_pause_key(ev.key))
                m_paused = false;
            else if (press && ev.key == HID_KEY_BACK)
                m_playing = false;
            m_need_draw = true;
            return true;
        }
        if (press && (ev.key == HID_KEY_BACK || ev.key == HID_KEY_ESC)) {
            m_paused = true;
            m_need_draw = true;
            return true;
        }
        if (orientation_ok())
            game()->on_hid(ev);
        return true;
    }

    // called in loop, returns true if need to draw
    bool update() {
        unsigned long now = millis();
        bool frame = now - m_last_frame >= FRAME_MS;
        if (frame)
            m_last_frame = now;
        if (m_playing && !m_paused && orientation_ok()) {
            if (frame) {
                game()->tick();
                m_need_draw = true;
            }
        }
        else if (now - m_last_anim >= 250) { // the blinking in the menu and messages
            m_last_anim = now;
            ++m_anim;
            m_need_draw = true;
        }
        bool r = m_need_draw;
        m_need_draw = false;
        return r;
    }

    void draw() override {
        display.setDrawer(&matrix_drawer);
        display.clearDisplay();
        if (!m_playing)
            draw_menu();
        else if (!orientation_ok())
            draw_turn();
        else {
            game()->draw();
            if (m_paused)
                draw_paused();
        }
    }

private:
    std::vector<Game*> m_games;
    int m_sel = 0;
    bool m_playing = false;
    bool m_paused = false;
    bool m_need_draw = true;
    unsigned long m_last_frame = 0;
    unsigned long m_last_anim = 0;
    int m_anim = 0;
    Preferences m_pref;

    Game* game() { return m_games[m_sel]; }
    bool orientation_ok() { return game()->any_orientation() || display.is_portrait() == game()->portrait(); }
    bool blink() const { return (m_anim & 2) == 0; }

    void draw_menu() {
        int w = display.width(), h = display.height();
        draw_text_center(1, "GAMES", rgb565(255, 210, 0));
        const Sprite& ic = game()->icon();
        int iy = h / 2 - ic.h / 2 - 6;
        draw_sprite(ic, (w - ic.w) / 2, iy);
        draw_text_center(iy + ic.h + 3, game()->name(), rgb565(255, 255, 255));
        if (blink()) {
            uint16_t ac = rgb565(120, 200, 255);
            draw_text(1, h / 2 - 7, "<", ac);
            draw_text(w - 6, h / 2 - 7, ">", ac);
        }
        if (game()->has_high_score()) {
            char buf[16];
            snprintf(buf, sizeof(buf), "HI %u", (unsigned)game()->high_score());
            draw_text_center(h - 18, buf, rgb565(120, 120, 140));
        }
        if (blink())
            draw_text_center(h - 9, "OK to play", rgb565(60, 220, 60));
    }

    void draw_turn() {
        int h = display.height();
        uint16_t c = rgb565(255, 210, 0);
        if (game()->portrait()) {
            draw_text_center(h / 2 - 14, "Stand the", c);
            draw_text_center(h / 2 - 4, "panel", c);
            draw_text_center(h / 2 + 6, "upright", c);
        }
        else {
            draw_text_center(h / 2 - 14, "Lay the", c);
            draw_text_center(h / 2 - 4, "panel", c);
            draw_text_center(h / 2 + 6, "sideways", c);
        }
    }

    // the lines in a frame as wide as the longest one, as wide as the display at most.
    // a line that doesn't fit is split after its ':' into two lines
    void draw_paused() {
        int w = display.width(), h = display.height();
        struct Line { char text[24]; uint16_t color; };
        Line lines[10];
        int n = 0;
        auto add = [&](const char* s, uint16_t color) {
            const char* colon = strchr(s, ':');
            if (text_width(s) > w - 6 && colon != nullptr && n < 9) {
                snprintf(lines[n].text, sizeof(lines[n].text), "%.*s", (int)(colon - s + 1), s);
                lines[n++].color = color;
                s = colon + 1;
                while (*s == ' ')
                    ++s;
            }
            if (n < 10) {
                snprintf(lines[n].text, sizeof(lines[n].text), "%s", s);
                lines[n++].color = color;
            }
        };
        add("PAUSED", rgb565(255, 210, 0));
        add("OK: play", rgb565(60, 220, 60));
        add("Back: menu", rgb565(230, 80, 80));
        for (int i = 0; game()->pause_option(i) != nullptr; ++i)
            add(game()->pause_option(i), rgb565(120, 200, 255));

        int tw = 0;
        for (int i = 0; i < n; ++i)
            tw = max(tw, text_width(lines[i].text));
        int bw = min(w, tw + 6), bh = n * 9 + 5;
        int x = (w - bw) / 2, y = (h - bh) / 2;
        display.fillRect(x, y, bw, bh, 0);
        display.drawRect(x, y, bw, bh, rgb565(120, 200, 255));
        for (int i = 0; i < n; ++i)
            draw_text_center(y + 3 + i * 9, lines[i].text, lines[i].color);
    }
};
