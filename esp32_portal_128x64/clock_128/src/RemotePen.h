#pragma once
#include "GamesState.h"

// Drawing with the remote in the games, like in the draw mode: moving the remote shows the cursor and moves it,
// holding OK draws, Left/Right pick one of the colors, Up/Down one of PEN_SIZES pen sizes.
// the position is in panel coordinates (the games' fields turn with the panel), the movement is in the
// coordinates of the current orientation. the cursor shows for a while after something changes
class RemotePen
{
public:
    int colors = 6;  // how many colors the game has
    static const int PEN_SIZES = 3;   // radius 0, 1, 2
    static constexpr float SPEED = 0.25f; // display pixels per unit of device movement, like the draw mode
    static const int CURSOR_SHOW_FRAMES = 5 * GamesState::FPS;

    float x = DISPLAY_WIDTH / 2;
    float y = DISPLAY_HEIGHT / 2;
    int color = 0;   // index of the game's colors
    int radius = 0;
    bool down = false;
    int cursor_frames = 0;
    int show_frames = CURSOR_SHOW_FRAMES; // for a game at another frame rate

    // when the game starts: hidden, not drawing
    void reset() {
        down = false;
        cursor_frames = 0;
    }

    // returns true if the event was for the pen. plot(px, py) is called for every pixel drawn
    template<typename F>
    bool on_hid(const HidEvent& ev, F plot) {
        if (ev.type == HidEvent::MOVE) {
            show();
            move(ev.dx, ev.dy, plot);
            return true;
        }
        if (ev.key == HID_KEY_ENTER) {
            if (ev.repeat)
                return true;
            // only a press shows the cursor: the release of the OK that started the game or closed
            // the pause menu comes here too, without its press
            if (ev.type == HidEvent::KEY_DOWN) {
                down = true;
                show();
                plot_pen(cx(), cy(), plot);
            }
            else
                down = false;
            return true;
        }
        if (ev.type != HidEvent::KEY_DOWN)
            return false;
        switch (ev.key) {
            case HID_KEY_RIGHT: color = (color + 1) % colors; break;
            case HID_KEY_LEFT:  color = (color + colors - 1) % colors; break;
            case HID_KEY_UP:    if (radius < PEN_SIZES - 1) ++radius; break;
            case HID_KEY_DOWN:  if (radius > 0) --radius; break;
            default: return false;
        }
        show();
        return true;
    }

    void tick() {
        if (cursor_frames > 0)
            --cursor_frames;
    }

    // with the display in rotation 0
    void draw_cursor(uint16_t c) const {
        if (cursor_frames > 0)
            display.drawCircle(cx(), cy(), radius + 2, c);
    }

    int cx() const { return (int)(x + 0.5f); }
    int cy() const { return (int)(y + 0.5f); }

private:
    void show() {
        cursor_frames = show_frames;
    }

    template<typename F>
    void move(int dx, int dy, F plot) {
        display.delta_to_physical(dx, dy);
        int px = cx(), py = cy();
        x = constrain(x + dx * SPEED, 0.0f, (float)(DISPLAY_WIDTH - 1));
        y = constrain(y + dy * SPEED, 0.0f, (float)(DISPLAY_HEIGHT - 1));
        if (down && (cx() != px || cy() != py))
            line(px, py, cx(), cy(), plot);
    }

    // a disc of the pen size, inside the panel
    template<typename F>
    void plot_pen(int px, int py, F plot) {
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx * dx + dy * dy > radius * radius + radius)
                    continue;
                int qx = px + dx, qy = py + dy;
                if (qx >= 0 && qy >= 0 && qx < DISPLAY_WIDTH && qy < DISPLAY_HEIGHT)
                    plot(qx, qy);
            }
    }

    // Bresenham
    template<typename F>
    void line(int x0, int y0, int x1, int y1, F plot) {
        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            plot_pen(x0, y0, plot);
            if (x0 == x1 && y0 == y1)
                break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
};
