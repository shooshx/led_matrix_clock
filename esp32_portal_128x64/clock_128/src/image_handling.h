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

void parse_img_cmd(const std::vector<uint8_t>& v)
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
  // no copyBuffer() needed, the Protomatter canvas always holds the last frame
  
  int offset = 6;
  int x = 0, y = 0;
  for(int i = 0; i < sz; ++i) {
    display.drawPixel(x, y, v[offset], v[offset+1], v[offset+2]);
    offset += 3;
    ++x;
    if (x >= display.width()) {
      x = 0;
      ++y;
    }
  }
  display.finish();
  //Serial.printf("img_cmd done %d\n", sz);
}
