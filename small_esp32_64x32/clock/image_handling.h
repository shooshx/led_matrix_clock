#pragma once


struct PixelDat {
  uint8_t x;
  uint8_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};


void parse_pixel_cmd(const std::vector<uint8_t>& v)
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

  matrix_drawer.copyBuffer();
  
  int offset = 6;
  for(int i = 0; i < sz; ++i) {
    PixelDat* p = (PixelDat*)&v[offset];
    offset += 5;
    display.drawPixel(p->x, p->y, p->r, p->g, p->b);
    //Serial.printf("PP-draw %d,%d, %x-%x-%x\n", p->x, p->y, p->r, p->g, p->b);
  }
  display.finish();
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
  if (sz != DISPLAY_WIDTH * DISPLAY_HEIGHT) {
    Serial.printf("img_cmd wrong size3 %d, %d, %d\n", sz, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    return;
  }
  matrix_drawer.copyBuffer();
  
  int offset = 6;
  int x = 0, y = 0;
  for(int i = 0; i < sz; ++i) {
    display.drawPixel(x, y, v[offset], v[offset+1], v[offset+2]);
    offset += 3;
    ++x;
    if (x >= DISPLAY_WIDTH) {
      x = 0;
      ++y;
    }
  }
  display.finish();
  //Serial.printf("img_cmd done %d\n", sz);
}
