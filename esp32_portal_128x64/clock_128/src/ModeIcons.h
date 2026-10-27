#pragma once
#include <stdint.h>

// colors selected with PgUp/PgDn in the keyboard modes (RGB565)
const uint16_t hid_palette[] = {
    0xF800, // red
    0xFFE0, // yellow
    0x07E0, // green
    0x07FF, // cyan
    0x001F, // blue
    0xF81F, // magenta
};
const int HID_PALETTE_SIZE = sizeof(hid_palette) / sizeof(hid_palette[0]);

inline uint8_t palette_step(uint8_t idx, int d) {
    return (idx + d + HID_PALETTE_SIZE) % HID_PALETTE_SIZE;
}

// 8x8 icons shown at the bottom left identifying the current mode, one byte per row, MSB is the left pixel
const int MODE_ICON_SIZE = 8;
const uint16_t MODE_ICON_COLOR = 0x8410; // gray

const uint8_t icon_clock[] = {
    0b00111100,
    0b01000010,
    0b10010001,
    0b10010001,
    0b10011101,
    0b10000001,
    0b01000010,
    0b00111100,
};

const uint8_t icon_stopwatch[] = {
    0b00111000,
    0b00010000,
    0b01111100,
    0b10000010,
    0b10010010,
    0b10001010,
    0b10000010,
    0b01111100,
};

const uint8_t icon_timer[] = { // hourglass
    0b11111110,
    0b01000100,
    0b00101000,
    0b00010000,
    0b00010000,
    0b00101000,
    0b01111100,
    0b11111110,
};

const uint8_t icon_movedraw[] = { // pencil
    0b00000110,
    0b00001011,
    0b00010110,
    0b00101100,
    0b01011000,
    0b01110000,
    0b11000000,
    0b10000000,
};

// typing with caps lock off
const uint8_t icon_type_lower[] = {
    0b00011000,
    0b00011000,
    0b01111110,
    0b01111110,
    0b00011000,
    0b00011000,
    0b00011011,
    0b00001110,
};

// typing with caps lock on
const uint8_t icon_type[] = {
    0b11111111,
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00011000,
};
