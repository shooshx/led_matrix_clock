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
    {
        make_lut(255, 10);
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override {
        if (m_matrix == nullptr) // while it's being replaced
            return;
        r = m_lut[r];
        g = m_lut[g];
        b = m_lut[b];
        // cast so that negative coords that wrapped in uint16_t get clipped by the canvas
        m_matrix->drawPixel((int16_t)x, (int16_t)y, m_matrix->color565(r, g, b));
    }
    void clear() override {
        if (m_matrix != nullptr)
            m_matrix->fillScreen(0);
    }
    void flush() override {
        if (m_matrix != nullptr)
            m_matrix->show();
    }

    // Protomatter has no brightness control so it's done by scaling the colors, with the gamma curve:
    // out = 255 * (v / 255) ^ gamma * brightness. gamma in tenths, 10 is none. takes effect on the next draw
    void setBrightness(uint8_t brightness) {
        if (brightness != m_brightness)
            make_lut(brightness, m_gamma10);
    }
    void setGamma(int gamma10) {
        gamma10 = constrain(gamma10, 5, 40);
        if (gamma10 != m_gamma10)
            make_lut(m_brightness, gamma10);
    }
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    // the matrix is replaced when its bit depth changes
    void setMatrix(Adafruit_Protomatter* matrix) {
        m_matrix = matrix;
    }

private:
    Adafruit_Protomatter* m_matrix;
    uint8_t m_brightness = 0;
    int m_gamma10 = 0;
    uint8_t m_lut[256];

    void make_lut(uint8_t brightness, int gamma10) {
        m_brightness = brightness;
        m_gamma10 = gamma10;
        float gamma = gamma10 / 10.0f;
        for (int i = 0; i < 256; ++i) {
            float v = powf(i / 255.0f, gamma) * 255.0f * (brightness + 1) / 256.0f;
            m_lut[i] = (uint8_t)(v + 0.5f);
        }
    }
};
