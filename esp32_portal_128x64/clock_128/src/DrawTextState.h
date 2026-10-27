#pragma once
#include <LittleFS.h>
#include "base_utils.h"
#include "ModeIcons.h"
#include "ble_hid.h"
#include "TypeState.h"

// an IDraw that writes into an RGB565 buffer in panel coordinates, for rendering into memory
class BufferDrawer : public IDraw
{
public:
    BufferDrawer(uint16_t* buf, int w, int h) : m_buf(buf), m_w(w), m_h(h) {}
    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override {
        int16_t sx = (int16_t)x, sy = (int16_t)y;
        if (sx < 0 || sy < 0 || sx >= m_w || sy >= m_h)
            return;
        m_buf[sy * m_w + sx] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    void clear() override { memset(m_buf, 0, m_w * m_h * sizeof(uint16_t)); }
    void flush() override {}
private:
    uint16_t* m_buf;
    int m_w, m_h;
};

// The drawing and text mode. a drawing with the typed text (TypeState) always on top of it.
// the drawing is made by the web-ui draw tab and by the remote: its movement moves a circle cursor,
// holding Enter draws. Left/Right (or PgUp/PgDn) select the color, which is also the typing color,
// Volume+/- the pen size. Back asks to clear everything, the drawing and the text: a red question mark is shown,
// OK (Enter) clears, Back again doesn't. the remote cursor hides after 5 seconds without movement.
// the drawing is in panel coordinates, so it turns with the panel, the remote movement and the web-ui
// are in the coordinates of the current orientation. it's saved in LittleFS (so uploadfs erases it)
class DrawTextState : public IScreen
{
public:
    static const int W = DISPLAY_WIDTH;
    static const int H = DISPLAY_HEIGHT;
    static constexpr float SPEED = 0.25f; // display pixels per unit of device movement
    static const int PEN_SIZES = 3;       // pen radius 0, 1, 2: 1, 3, 5 pixels wide
    static const int CURSOR_GAP = 2;      // cursor circle radius is the pen radius + this
    static const unsigned long CURSOR_HIDE_MS = 5000;
    static const unsigned long SAVE_DELAY_MS = 3000; // don't write the flash on every move
    static const unsigned long RENDER_MS = 300;      // how often the web-ui copy is updated while things change

    TypeState m_text;
    int m_pen_r = 0;

private:
    uint16_t m_canvas[H][W];   // the drawing, RGB565 in panel coordinates, 0 is black
    uint16_t m_rendered[H * W]; // the drawing and the text as shown, without cursors, for the web-ui
    float m_x = W / 2;
    float m_y = H / 2;
    bool m_pen_down = false;
    bool m_confirm_clear = false; // the question mark is shown
    bool m_cursor_visible = false;
    unsigned long m_cursor_until = 0;
    bool m_need_draw = true;
    unsigned long m_save_time = 0; // when to save, 0 if nothing to save
    volatile uint32_t m_canvas_version = 0;
    uint32_t m_rendered_version = 0xFFFFFFFF;
    unsigned long m_render_time = 0;

    // saved format: header and then the canvas rows
    struct SaveHeader {
        uint32_t magic;
        uint16_t w, h;
        uint8_t pen_r;
        uint8_t reserved[3];
    };
    static const uint32_t SAVE_MAGIC = 0x32575244; // "DRW2"
    static constexpr const char* SAVE_PATH = "/drawing.bin";

public:
    DrawTextState() {
        memset(m_canvas, 0, sizeof(m_canvas));
        memset(m_rendered, 0, sizeof(m_rendered));
    }

    // changes when the drawing or the text change
    uint32_t version() const { return m_canvas_version + m_text.m_version; }
    // the drawing with the text, panel coordinates
    const uint16_t* rendered() const { return m_rendered; }
    bool cursor_visible() const { return m_cursor_visible && !m_text.m_editing; }
    uint8_t color() const { return m_text.m_color; }

    void load() {
        m_text.load();
        File f = LittleFS.open(SAVE_PATH, "r");
        if (!f)
            return;  // nothing saved yet
        SaveHeader hdr;
        if (f.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr) || hdr.magic != SAVE_MAGIC || hdr.w != W || hdr.h != H)
            return;
        m_pen_r = min((int)hdr.pen_r, PEN_SIZES - 1);
        if (f.read((uint8_t*)m_canvas, sizeof(m_canvas)) != sizeof(m_canvas))
            memset(m_canvas, 0, sizeof(m_canvas));
    }

    void save() {
        m_save_time = 0;
        File f = LittleFS.open(SAVE_PATH, "w");
        if (!f) {
            Serial.println("can't write the drawing");
            return;
        }
        SaveHeader hdr = { SAVE_MAGIC, W, H, (uint8_t)m_pen_r, {0, 0, 0} };
        f.write((const uint8_t*)&hdr, sizeof(hdr));
        f.write((const uint8_t*)m_canvas, sizeof(m_canvas));
    }

    void request_draw() {
        m_need_draw = true;
    }

    // called when the mode is entered
    void reset_pen() {
        m_pen_down = false;
        m_confirm_clear = false;
    }

    // a pixel from the web-ui draw tab, in the coordinates of the current orientation.
    // called from the web server task
    void set_pixel(int x, int y, uint16_t c) {
        if (x < 0 || y < 0 || x >= display.width() || y >= display.height())
            return;
        int16_t px = x, py = y;
        display.to_physical(px, py);
        m_canvas[py][px] = c;
        drawing_changed();
    }

    // the drawing and the text
    void clear_all() {
        memset(m_canvas, 0, sizeof(m_canvas));
        m_text.clear_text();
        drawing_changed();
    }

    // Back from the remote, typing or not
    void ask_clear() {
        m_confirm_clear = true;
        m_need_draw = true;
    }

    // while asking to clear, all the input comes here: OK clears, Back doesn't, the rest is ignored.
    // returns true if the input was taken
    bool on_confirm_hid(const HidEvent& ev) {
        if (!m_confirm_clear)
            return false;
        if (ev.type == HidEvent::KEY_DOWN && !ev.repeat) {
            if (ev.key == HID_KEY_ENTER) {
                clear_all();
                m_confirm_clear = false;
            }
            else if (ev.key == HID_KEY_BACK) {
                m_confirm_clear = false;
                m_need_draw = true;
            }
        }
        return true;
    }

    // called in loop, returns true if need to draw
    bool update() {
        unsigned long now = millis();
        if (m_save_time != 0 && (long)(now - m_save_time) >= 0)
            save();
        if (m_cursor_visible && (long)(now - m_cursor_until) >= 0) {
            m_cursor_visible = false;
            m_need_draw = true;
        }
        if (version() != m_rendered_version && now - m_render_time >= RENDER_MS)
            render();
        bool text_need_draw = m_text.update(); // cursor blink
        bool r = m_need_draw || text_need_draw;
        m_need_draw = false;
        return r;
    }

    // input when not typing
    void on_hid(const HidEvent& ev) {
        if (ev.type == HidEvent::MOVE) {
            show_cursor();
            move(ev.dx, ev.dy);
            return;
        }
        if (ev.key == HID_KEY_ENTER && !ev.repeat) {
            m_pen_down = (ev.type == HidEvent::KEY_DOWN);
            show_cursor();
            if (m_pen_down)
                plot_pen(cur_x(), cur_y());
            return;
        }
        if (ev.type != HidEvent::KEY_DOWN)
            return;
        if (ev.key == HID_KEY_RIGHT || ev.key == HID_KEY_PGUP)
            m_text.set_color(palette_step(m_text.m_color, 1));
        else if (ev.key == HID_KEY_LEFT || ev.key == HID_KEY_PGDN)
            m_text.set_color(palette_step(m_text.m_color, -1));
        else if (ev.key == HID_KEY_VOL_UP)
            set_pen(m_pen_r + 1);
        else if (ev.key == HID_KEY_VOL_DOWN)
            set_pen(m_pen_r - 1);
        else
            return;
        m_need_draw = true;
    }

    void draw() override {
        display.setDrawer(&matrix_drawer);
        display.clearDisplay();
        uint8_t rotation = display.getRotation();
        display.setRotation(0);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (m_canvas[y][x] != 0)
                    display.drawPixel(x, y, m_canvas[y][x]);
        display.setRotation(rotation);

        m_text.draw(); // on top, with its cursor while typing

        if (cursor_visible()) {
            display.setRotation(0);
            display.drawCircle(cur_x(), cur_y(), m_pen_r + CURSOR_GAP, hid_palette[m_text.m_color]);
            display.setRotation(rotation);
        }

        if (m_confirm_clear) {
            // bold: the 2x default font drawn twice, 1 pixel apart, on a black box at the bottom middle
            const int w = 12, h = 14;
            int x = (display.width() - w) / 2, y = display.height() - h - 1;
            display.fillRect(x - 2, y - 2, w + 5, h + 3, 0);
            display.setFont(nullptr);
            display.setTextSize(2);
            display.setTextColor(0xF800);
            for (int dx = 0; dx < 2; ++dx) {
                display.setCursor(x + dx, y);
                display.print('?');
            }
            display.setTextSize(1);
        }
    }

private:
    int cur_x() const { return (int)(m_x + 0.5f); }
    int cur_y() const { return (int)(m_y + 0.5f); }

    void drawing_changed() {
        m_need_draw = true;
        m_save_time = millis() + SAVE_DELAY_MS;
        ++m_canvas_version;
    }

    void show_cursor() {
        if (!m_cursor_visible)
            m_need_draw = true;
        m_cursor_visible = true;
        m_cursor_until = millis() + CURSOR_HIDE_MS;
    }

    void set_pen(int r) {
        m_pen_r = constrain(r, 0, PEN_SIZES - 1);
        m_save_time = millis() + SAVE_DELAY_MS; // saved with the drawing
        show_cursor();
    }

    // the copy for the web-ui: the drawing and the text drawn over it into memory
    void render() {
        uint32_t v = version(); // before, in case the web-ui changes it meanwhile
        memcpy(m_rendered, m_canvas, sizeof(m_rendered));
        BufferDrawer bd(m_rendered, W, H);
        display.setDrawer(&bd);
        m_text.draw_text(false);
        display.setDrawer(&matrix_drawer);
        m_rendered_version = v;
        m_render_time = millis();
    }

    void move(int dx, int dy) {
        display.delta_to_physical(dx, dy);
        int px = cur_x(), py = cur_y();
        m_x = constrain(m_x + dx * SPEED, 0.0f, (float)(W - 1));
        m_y = constrain(m_y + dy * SPEED, 0.0f, (float)(H - 1));
        if (cur_x() == px && cur_y() == py)
            return;
        if (m_pen_down)
            line(px, py, cur_x(), cur_y());
        m_need_draw = true;
    }

    void plot(int x, int y) {
        if (x >= 0 && y >= 0 && x < W && y < H)
            m_canvas[y][x] = hid_palette[m_text.m_color];
    }

    // a disc of the pen size
    void plot_pen(int x, int y) {
        for (int dy = -m_pen_r; dy <= m_pen_r; ++dy)
            for (int dx = -m_pen_r; dx <= m_pen_r; ++dx)
                if (dx * dx + dy * dy <= m_pen_r * m_pen_r + m_pen_r)
                    plot(x + dx, y + dy);
        drawing_changed();
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
