#pragma once


struct PixelDat {
  uint8_t x;
  uint8_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};


// pixels drawn in the web-ui draw tab, given to set_pixel(x, y, rgb565)
template<typename F>
void parse_pixel_cmd(const std::vector<uint8_t>& v, F set_pixel)
{
  //Serial.printf("parse_pixel %d\n", v.size());
  if (v.size() < 6) {
    Serial.printf("pixel_cmd unexpected size %d\n", v.size());
    return;
  }
  uint32_t sz = 0;
  memcpy(&sz, &v[2], sizeof(uint32_t));
  if (v.size() != sz*5 + 6 + 1) { // +1 for null term that's added on parsing
    Serial.printf("pixel_cmd unexpected size2 %d, %d\n", v.size(), sz);
    return;
  }

  int offset = 6;
  for(int i = 0; i < sz; ++i) {
    PixelDat* p = (PixelDat*)&v[offset];
    offset += 5;
    set_pixel(p->x, p->y, ((p->r & 0xF8) << 8) | ((p->g & 0xFC) << 3) | (p->b >> 3));
  }
}

// the image of the web-ui image tab, given to set_pixel(x, y, rgb565) in the current orientation
template<typename F>
void parse_img_cmd(const std::vector<uint8_t>& v, F set_pixel)
{
  if (v.size() < 6) {
    Serial.printf("img_cmd unexpected size %d\n", v.size());
    return;
  }
  uint32_t sz = 0;
  memcpy(&sz, &v[2], sizeof(uint32_t));
  if (v.size() != sz*3 + 6 + 1) { // +1 for null term that's added on parsing
    Serial.printf("img_cmd unexpected size2 %d, %d\n", v.size(), sz);
    return;
  }
  if (sz != display.width() * display.height()) {
    Serial.printf("img_cmd wrong size3 %d, %d, %d\n", sz, (int)display.width(), (int)display.height());
    return;
  }
  int offset = 6;
  int x = 0, y = 0;
  for(int i = 0; i < sz; ++i) {
    uint8_t r = v[offset], g = v[offset+1], b = v[offset+2];
    set_pixel(x, y, ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    offset += 3;
    ++x;
    if (x >= display.width()) {
      x = 0;
      ++y;
    }
  }
}



// the image mode: the last image from the web-ui image tab, kept so that it can be drawn again
// (with a new brightness, after another mode). panel coordinates, so it turns with the panel like the drawing
class ImageState : public IScreen
{
public:
    static const int W = DISPLAY_WIDTH;
    static const int H = DISPLAY_HEIGHT;

    // called from the web server task
    void set_pixel(int x, int y, uint16_t c) {
        if (x < 0 || y < 0 || x >= display.width() || y >= display.height())
            return;
        int16_t px = x, py = y;
        display.to_physical(px, py);
        m_img[py][px] = c;
        m_need_draw = true;
    }

    // called in loop, returns true if need to draw
    bool update() {
        bool r = m_need_draw;
        m_need_draw = false;
        return r;
    }

    void draw() override {
        display.setDrawer(&matrix_drawer);
        display.clearDisplay();
        uint8_t rotation = display.getRotation();
        display.setRotation(0);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (m_img[y][x] != 0)
                    display.drawPixel(x, y, m_img[y][x]);
        display.setRotation(rotation);
    }

private:
    uint16_t m_img[H][W] = {};
    volatile bool m_need_draw = false;
};
