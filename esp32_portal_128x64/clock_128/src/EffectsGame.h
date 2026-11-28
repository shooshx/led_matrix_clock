#pragma once
#include "GamesState.h"

// not a game: one of the clock's background effects (Effects.h) on the whole panel, to look at.
// Up/Down pick the effect, Left/Right the color (12 hues at full saturation), Volume +/- its size setting
// (what it is depends on the effect, like the Size of a clock face), OK its speed (1-10, then 1 again),
// Home turns Slide by 15 degrees.
// what changed shows at the bottom for a moment. the settings are remembered
namespace effects_art {
static const char* const ICON[] = {
    "....Y....",
    "...YWY...",
    "Y..OYO..M",
    ".........",
    "..B...R..",
    ".BWB.RWR.",
    "..B...R..",
};
static const Sprite S_ICON = { 9, 7, ICON };
}

class EffectsGame : public Game
{
public:
    static const int HUES = 12;
    static const int ANGLE_STEP = 15;
    static const unsigned long LABEL_MS = 1500;  // what changed shows this long
    static const unsigned long SAVE_MS = 2000;   // saved when the changes stop for this long

    const char* name() const override { return "Effects"; }
    const Sprite& icon() const override { return effects_art::S_ICON; }
    bool portrait() const override { return false; }
    bool any_orientation() const override { return true; }
    bool has_high_score() const override { return false; }
    uint32_t high_score() const override { return 0; }
    bool uses_volume_keys() const override { return true; }  // the size
    int fps() const override { return 25; }  // like the clock's effects

    void load(Preferences& pref) override {
        m_pref = &pref;
        m_fx = constrain((int)pref.getUChar("fxg_fx", 0), 0, COUNT - 1);
        m_hue = constrain((int)pref.getUChar("fxg_hue", 0), 0, HUES - 1);
        m_size = constrain((int)pref.getUChar("fxg_size", 16), 1, FX_SIZE_MAX);
        m_speed = constrain((int)pref.getUChar("fxg_speed", 5), 1, 10);
        m_angle = constrain((int)pref.getUChar("fxg_angle", 0), 0, 179);
    }

    void start() override {
        show_label();
    }

    void tick() override {
        if (m_dirty && millis() - m_changed >= SAVE_MS && m_pref != nullptr) {
            m_dirty = false;
            m_pref->putUChar("fxg_fx", m_fx);
            m_pref->putUChar("fxg_hue", m_hue);
            m_pref->putUChar("fxg_size", m_size);
            m_pref->putUChar("fxg_speed", m_speed);
            m_pref->putUChar("fxg_angle", m_angle);
        }
    }

    void on_hid(const HidEvent& ev) override {
        if (ev.type != HidEvent::KEY_DOWN)
            return;
        switch (ev.key) {
            case HID_KEY_UP:       m_fx = (m_fx + COUNT - 1) % COUNT; break;
            case HID_KEY_DOWN:     m_fx = (m_fx + 1) % COUNT; break;
            case HID_KEY_LEFT:     m_hue = (m_hue + HUES - 1) % HUES; break;
            case HID_KEY_RIGHT:    m_hue = (m_hue + 1) % HUES; break;
            case HID_KEY_VOL_UP:   m_size = min(FX_SIZE_MAX, m_size + size_step()); break;
            case HID_KEY_VOL_DOWN: m_size = max(1, m_size - size_step()); break;
            case HID_KEY_ENTER:
                if (ev.repeat)
                    return;
                m_speed = m_speed % 10 + 1;
                break;
            case HID_KEY_REMOTE_HOME:
            case HID_KEY_HOME:
                if (ev.repeat || LIST[m_fx].fx != FX_SLIDE)
                    return;
                m_angle = (m_angle + ANGLE_STEP) % 180;
                break;
            default: return;
        }
        m_dirty = true;
        m_changed = millis();
        show_label();
    }

    void draw() override {
        uint8_t r, g, b;
        hsv_rgb(m_hue * (360 / HUES), 1, 1, r, g, b);
        uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        draw_effect(LIST[m_fx].fx, color, m_size, m_speed, false, m_angle, fx_time());
        if (millis() - m_label_at < LABEL_MS) {
            char buf[32];
            if (LIST[m_fx].fx == FX_SLIDE)
                snprintf(buf, sizeof(buf), "%s %d sp%d %d'", LIST[m_fx].name, m_size, m_speed, m_angle);
            else
                snprintf(buf, sizeof(buf), "%s %d sp%d", LIST[m_fx].name, m_size, m_speed);
            if (text_width(buf) > display.width())  // portrait: without the speed
                snprintf(buf, sizeof(buf), "%s %d", LIST[m_fx].name, m_size);
            int h = display.height();
            int tw = text_width(buf);
            int x = max(0, (display.width() - tw) / 2);
            display.fillRect(x - 1, h - 9, tw + 2, 9, 0);
            draw_text(x, h - 8, buf, color);
        }
    }

private:
    struct Entry { int fx; const char* name; };
    static constexpr Entry LIST[] = {
        {FX_BORDER, "Border"}, {FX_SLIDE, "Slide"}, {FX_TWINKLE, "Twinkle"}, {FX_FIREFLIES, "Fireflies"},
        {FX_AURORA, "Aurora"}, {FX_RAIN, "Rain"}, {FX_SNOW, "Snow"}, {FX_FIRE, "Fire"}, {FX_RIPPLES, "Ripples"},
        {FX_RADAR, "Radar"}, {FX_PLASMA, "Plasma"}, {FX_WARP, "Warp"}, {FX_LAVA, "Lava"}, {FX_TUNNEL, "Tunnel"},
        {FX_CUBE, "Cube"},
    };
    static const int COUNT = sizeof(LIST) / sizeof(LIST[0]);

    Preferences* m_pref = nullptr;
    int m_fx = 0;
    int m_hue = 0;
    int m_size = 16;
    int m_speed = 5;
    int m_angle = 0;  // Slide's, degrees
    bool m_dirty = false;
    unsigned long m_changed = 0;
    unsigned long m_label_at = 0;

    // finer steps where small sizes differ a lot
    int size_step() const { return m_size < 8 ? 1 : m_size < 24 ? 2 : 4; }
    void show_label() { m_label_at = millis(); }
};
constexpr EffectsGame::Entry EffectsGame::LIST[];
