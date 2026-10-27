#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_Protomatter.h>
#include "drawer.h"

inline void color888(uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
    g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
    b = (((color & 0x1F) * 527) + 23) >> 6;
}

// Adafruit_GFX front that forwards all pixels to an IDraw, so drawing can go
// through effects like HaloDraw before it reaches the matrix.
// handles setRotation(): drawing is in rotated (logical) coordinates, the drawers get panel (physical) coordinates
class WrapGFX : public Adafruit_GFX {
public:
    WrapGFX(uint16_t width, uint16_t height) : Adafruit_GFX(width, height)
    {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (m_drawer == nullptr)
            return;
        uint8_t r, g, b;
        color888(color, r, g, b);
        to_physical(x, y);
        m_drawer->setPixel(x, y, r, g, b);
    }

    void drawPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        if (m_drawer == nullptr)
            return;
        to_physical(x, y);
        m_drawer->setPixel(x, y, r, g, b);
    }

    // same transform as GFXcanvas16::drawPixel
    void to_physical(int16_t& x, int16_t& y) const {
        int16_t t;
        switch (getRotation()) {
        case 1: t = x; x = WIDTH - 1 - y; y = t; break;
        case 2: x = WIDTH - 1 - x; y = HEIGHT - 1 - y; break;
        case 3: t = x; x = y; y = HEIGHT - 1 - t; break;
        }
    }
    // a movement in logical coordinates to panel coordinates
    void delta_to_physical(int& dx, int& dy) const {
        int t;
        switch (getRotation()) {
        case 1: t = dx; dx = -dy; dy = t; break;
        case 2: dx = -dx; dy = -dy; break;
        case 3: t = dx; dx = dy; dy = -t; break;
        }
    }
    bool is_portrait() const {
        return getRotation() & 1;
    }

    void clearDisplay() {
        if (m_drawer == nullptr)
            return;
        m_drawer->clear();
    }

    void finish() {
        if (m_drawer == nullptr)
            return;
        m_drawer->flush();
    }

    void setDrawer(IDraw* drawer) {
        m_drawer = drawer;
    }

    // x position of the cursor after printing the string from x=0
    // (the old project had this as an extra x_after output of getTextBounds())
    int16_t getTextAdvance(const char* s) {
        int16_t x = 0, y = 0;
        int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1;
        unsigned char c;
        while ((c = *s++))
            charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
        return x;
    }

private:
    IDraw* m_drawer = nullptr;
};

// IDraw on top of the Protomatter matrix.
// Protomatter keeps its own GFXcanvas16 frame buffer that is copied to the panel on show(),
// so the canvas always holds the last frame (no need for the old copyBuffer())
class MatrixDrawer : public IDraw
{
public:
    MatrixDrawer(Adafruit_Protomatter* matrix) : m_matrix(matrix)
    {}

    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override {
        if (m_brightness != 255) {
            r = (uint16_t(r) * (m_brightness + 1)) >> 8;
            g = (uint16_t(g) * (m_brightness + 1)) >> 8;
            b = (uint16_t(b) * (m_brightness + 1)) >> 8;
        }
        // cast so that negative coords that wrapped in uint16_t get clipped by the canvas
        m_matrix->drawPixel((int16_t)x, (int16_t)y, m_matrix->color565(r, g, b));
    }
    void clear() override {
        m_matrix->fillScreen(0);
    }
    void flush() override {
        m_matrix->show();
    }

    // Protomatter has no brightness control so it's done by scaling the colors
    // takes effect on the next draw
    void setBrightness(uint8_t v) {
        m_brightness = v;
    }
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return m_matrix->color565(r, g, b);
    }

private:
    Adafruit_Protomatter* m_matrix;
    uint8_t m_brightness = 255;
};
