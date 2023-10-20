#include <Adafruit_GFX.h>

#ifndef ARDUINO
  #define PROGMEM
#endif

const uint8_t ShadowBoldBitmap[] PROGMEM = {	

	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x20 C040 ' ' */
	0x00,0x00,0x08,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x21 C041 '!' */
	0x00,0x00,0x21,0x00,0x63,0x00,0x63,0x00,0x7B,0xC0,0x73,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x22 C042 '"' */
	0x31,0x80,0x31,0x80,0x31,0x80,0x40,0x00,0xC0,0x00,0xF3,0x90,0xF3,0x90,0x31,0x80,0x31,0x80,0x40,0x00,0xC0,0x00,0xF3,0x90,0xF3,0x90,0x31,0x80,0x3D,0xE0,0x39,0xC0,0x00,0x00,0x00,0x00,	/* 0x23 C043 '#' */
	0x04,0x00,0x00,0x00,0x20,0x00,0x44,0x00,0xCC,0xA0,0xC4,0xC0,0xC0,0x80,0xF0,0x00,0x7C,0x00,0x1C,0x80,0x0C,0x80,0x40,0x00,0xE0,0x20,0x7C,0xC0,0x3C,0x80,0x0F,0x00,0x0E,0x00,0x00,0x00,	/* 0x24 C044 '$' */
	0x00,0x00,0x20,0x80,0x41,0x80,0xD9,0x00,0xD9,0x00,0xC3,0x00,0xFA,0x00,0x72,0x00,0x06,0x00,0x04,0x40,0x04,0x80,0x0C,0xB0,0x09,0xB0,0x09,0x80,0x1F,0xF0,0x1C,0xE0,0x00,0x00,0x00,0x00,	/* 0x25 C045 '%' */
	0x10,0x00,0x20,0x00,0x66,0x00,0x66,0x00,0x60,0x00,0x70,0x80,0x20,0x00,0x46,0x00,0xCF,0x10,0xC3,0x20,0xC2,0x00,0xC0,0x00,0xE0,0x40,0x7F,0xF0,0x3F,0x60,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x26 C046 '&' */
	0x00,0x00,0x08,0x00,0x18,0x00,0x1C,0x00,0x1C,0x00,0x09,0x00,0x0E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x27 C047 ''' */
	0x00,0x00,0x00,0x00,0x08,0x00,0x11,0x00,0x32,0x00,0x20,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x60,0x00,0x70,0x00,0x30,0x00,0x38,0x00,0x1F,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x28 C050 '(' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x1C,0x00,0x0E,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x04,0x00,0x10,0x00,0x31,0x00,0x3E,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x29 C051 ')' */
	0x00,0x00,0x00,0x00,0x04,0x00,0x08,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xE0,0x60,0x40,0x40,0x04,0x00,0x0E,0x00,0x17,0x00,0x23,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x2A C052 '*' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x18,0x00,0x40,0x00,0xC0,0x00,0xF9,0xC0,0xF9,0x80,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x2B C053 '+' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1C,0x00,0x1C,0x00,0x09,0x00,0x0E,0x00,0x1C,0x00,	/* 0x2C C054 ',' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x2D C055 '-' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x2E C056 '.' */
	0x00,0x00,0x01,0x00,0x03,0x00,0x02,0x00,0x06,0x00,0x06,0x00,0x04,0x00,0x0C,0x00,0x0C,0x00,0x08,0x00,0x18,0x00,0x18,0x00,0x10,0x00,0x30,0x00,0x3C,0x00,0x38,0x00,0x00,0x00,0x00,0x00,	/* 0x2F C057 '/' */
	0x00,0x00,0x00,0x00,0x20,0x00,0x46,0x00,0xCF,0x00,0xC3,0x00,0xC3,0x00,0xC3,0x00,0xC3,0x00,0xC3,0x00,0xC3,0x00,0xC0,0x00,0xE0,0x40,0x7F,0x80,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x30 C060 '0' */
	0x00,0x00,0x10,0x00,0x20,0x00,0x60,0x00,0x78,0x00,0x78,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x20,0x00,0x60,0x00,0x7F,0x80,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x31 C061 '1' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xEE,0x00,0x7F,0x00,0x33,0x00,0x02,0x00,0x00,0x40,0x00,0x80,0x03,0x80,0x06,0x00,0x40,0x00,0xC0,0x00,0xFF,0xE0,0xFF,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x32 C062 '2' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xEE,0x00,0x7F,0x00,0x63,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1F,0x00,0x23,0x00,0x40,0x00,0xF0,0x40,0xFF,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x33 C063 '3' */
	0x00,0x00,0x02,0x00,0x06,0x00,0x46,0x00,0xC6,0x00,0xC6,0x00,0xC6,0x00,0xC0,0x00,0xC0,0x00,0xFE,0x70,0xFE,0x60,0x06,0x00,0x06,0x00,0x07,0x80,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x34 C064 '4' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0xE0,0xCF,0xC0,0xC0,0x00,0xC0,0x00,0xDE,0x00,0xFF,0x00,0xC1,0x80,0x21,0x00,0x40,0x00,0xE0,0x20,0xFF,0xC0,0x3F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x35 C065 '5' */
	0x00,0x00,0x02,0x00,0x04,0x00,0x08,0x00,0x10,0x00,0x20,0x00,0x60,0x00,0x46,0x00,0xCF,0x00,0xC3,0x00,0xC2,0x00,0xE0,0x00,0x70,0x40,0x3F,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x36 C066 '6' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0x00,0xFE,0x10,0x00,0x20,0x00,0x40,0x08,0x80,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x1F,0x00,0x1E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x37 C067 '7' */
	0x00,0x00,0x00,0x00,0x20,0x00,0x47,0x00,0xCF,0x80,0xC1,0x80,0xE0,0x00,0x60,0x00,0x47,0x00,0xCF,0x80,0xC1,0x80,0xC0,0x00,0xE0,0x20,0x7F,0xC0,0x3F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x38 C070 '8' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x00,0x4F,0x80,0xC1,0x80,0xC1,0x00,0xE0,0x00,0x70,0x80,0x3F,0x80,0x1F,0x00,0x00,0x00,0x20,0x20,0x7F,0xC0,0x3F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x39 C071 '9' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x3A C072 ':' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1C,0x00,0x1C,0x00,0x09,0x00,0x0E,0x00,0x1C,0x00,	/* 0x3B C073 ';' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x01,0x80,0x06,0x00,0x00,0x00,0x60,0x00,0xF8,0x00,0x3E,0x00,0x0F,0x80,0x03,0xE0,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x3C C074 '<' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x3D C075 '=' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x7C,0x00,0x1F,0x00,0x04,0x00,0x00,0x30,0x00,0xC0,0x03,0x00,0x0C,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x3E C076 '>' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xEE,0x00,0x77,0x00,0x23,0x00,0x00,0x00,0x08,0x40,0x19,0x80,0x1F,0x00,0x1C,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x3F C077 '?' */
	0x00,0x00,0x10,0x00,0x27,0x80,0x6F,0xC0,0x48,0xE0,0xC2,0x60,0xC4,0x60,0xCB,0x00,0xC9,0x00,0xC8,0x50,0xEF,0xE0,0x67,0x40,0x70,0x00,0x3F,0xC0,0x1F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x40 C100 '@' */
	0x00,0x00,0x02,0x00,0x06,0x00,0x04,0x00,0x0C,0x00,0x08,0x00,0x19,0x00,0x13,0x80,0x30,0x00,0x20,0x00,0x67,0x80,0x4F,0xC0,0xC0,0xC0,0xE0,0x70,0xE0,0x60,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x41 C101 'A' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0x00,0xCF,0x80,0xC1,0x80,0xC0,0x00,0xC0,0x00,0xCF,0x00,0xCF,0x80,0xC1,0x80,0xC0,0x00,0xC0,0x20,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x42 C102 'B' */
	0x00,0x00,0x00,0x00,0x10,0x00,0x23,0x80,0x67,0xF0,0x4C,0xE0,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xE0,0x00,0x60,0x00,0x70,0x20,0x3F,0xC0,0x1F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x43 C103 'C' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0x00,0xCF,0x80,0xC1,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x80,0xC0,0x00,0xC0,0x40,0xFF,0x80,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x44 C104 'D' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0xE0,0xCF,0xC0,0xC0,0x00,0xC0,0x00,0xCF,0x80,0xCF,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xFF,0xE0,0xFF,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x45 C105 'E' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0xE0,0xCF,0xC0,0xC0,0x00,0xC0,0x00,0xCF,0x80,0xCF,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xF0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x46 C106 'F' */
	0x00,0x00,0x00,0x00,0x10,0x00,0x63,0x80,0x67,0xF0,0xC0,0xE0,0xC0,0x00,0xC0,0x00,0xC2,0x00,0xC6,0x00,0xE7,0x80,0x60,0x00,0x70,0x40,0x3F,0xF0,0x1F,0x60,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x47 C107 'G' */
	0x00,0x00,0x40,0x40,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x00,0xC0,0x00,0xCF,0xC0,0xCF,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xF0,0xF0,0xE0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x48 C110 'H' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xF9,0xC0,0xF9,0x80,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x40,0x00,0xC0,0x00,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x49 C111 'I' */
	0x00,0x00,0x10,0x00,0x30,0x00,0x3C,0xC0,0x3C,0x80,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x48,0x00,0xC0,0x00,0xE0,0x80,0x7F,0x00,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4A C112 'J' */
	0x00,0x00,0x40,0x40,0xC0,0x80,0xC1,0x00,0xC2,0x00,0xC4,0x00,0xC8,0x00,0xC0,0x00,0xC0,0x00,0xC2,0x00,0xC7,0x00,0xC3,0x80,0xC1,0xC0,0xF0,0xF0,0xE0,0x60,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4B C113 'K' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xFF,0xE0,0xFF,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4C C114 'L' */
	0x00,0x00,0x41,0x00,0xC1,0x00,0xC2,0x00,0xC0,0x40,0xC8,0x40,0xC8,0xC0,0xCC,0xC0,0xCC,0xC0,0xCE,0xC0,0xC6,0xC0,0xC0,0xC0,0xC0,0xC0,0xF0,0xF0,0xE0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4D C115 'M' */
	0x00,0x00,0x40,0x40,0xC0,0xC0,0xC0,0xC0,0xC8,0xC0,0xC8,0xC0,0xCC,0xC0,0xCC,0x40,0xCE,0x40,0xC6,0x00,0xC7,0x00,0xC3,0x00,0xC3,0x80,0xF1,0xF0,0xE1,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4E C116 'N' */
	0x00,0x00,0x00,0x00,0x10,0x00,0x23,0x00,0x67,0x80,0x49,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xE1,0x80,0x60,0x00,0x70,0x20,0x3F,0xC0,0x1F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x4F C117 'O' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0x00,0xCF,0x80,0xC1,0x80,0xC0,0x00,0xC0,0x40,0xCF,0x80,0xCF,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xF0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x50 C120 'P' */
	0x00,0x00,0x00,0x00,0x10,0x00,0x23,0x00,0x67,0x80,0x4D,0xC0,0xC0,0xC0,0xC0,0x40,0xC0,0x00,0xC1,0x00,0xE3,0x80,0x60,0x00,0x70,0x00,0x3F,0xD0,0x1F,0xE0,0x00,0x40,0x00,0x00,0x00,0x00,	/* 0x51 C121 'Q' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xCF,0x00,0xCF,0x80,0xC1,0x80,0xC0,0x00,0xC0,0x40,0xCE,0x00,0xCF,0x00,0xC3,0x00,0xC3,0x80,0xC1,0x80,0xF1,0xE0,0xE0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x52 C122 'R' */
	0x00,0x00,0x00,0x00,0x20,0x00,0x47,0x00,0xCF,0xA0,0xC1,0xC0,0xE0,0x00,0x7E,0x00,0x3F,0x00,0x03,0x80,0x01,0x00,0x40,0x00,0xE0,0x20,0x7F,0xC0,0x3F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x53 C123 'S' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xFC,0xF0,0xFC,0xE0,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0F,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x54 C124 'T' */
	0x00,0x00,0x00,0x00,0x40,0x40,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xE0,0x80,0x60,0x00,0x70,0x20,0x3F,0xC0,0x1F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x55 C125 'U' */
	0x00,0x00,0x40,0x40,0xC0,0x40,0xC0,0xC0,0xE0,0xC0,0x60,0x80,0x61,0x80,0x71,0x80,0x31,0x00,0x31,0x00,0x38,0x00,0x18,0x00,0x18,0x00,0x1F,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x56 C126 'V' */
	0x00,0x00,0x40,0x40,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC4,0xC0,0xC4,0x40,0xC8,0x40,0xC8,0x00,0xC1,0x00,0xC3,0x00,0xC3,0x80,0xC1,0x80,0xF1,0xF0,0xE0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x57 C127 'W' */
	0x00,0x00,0x40,0x40,0xC0,0x80,0xE0,0x80,0x71,0x00,0x30,0x00,0x38,0x00,0x1C,0x00,0x08,0x00,0x10,0x00,0x11,0x00,0x23,0x80,0x43,0x80,0xF1,0xF0,0xE1,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x58 C130 'X' */
	0x00,0x00,0x40,0x40,0xC0,0x80,0xE1,0x80,0x71,0x00,0x30,0x00,0x38,0x00,0x1C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0F,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x59 C131 'Y' */
	0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0x80,0xFE,0x10,0x00,0x20,0x00,0xC0,0x01,0x80,0x02,0x00,0x04,0x00,0x48,0x00,0xC0,0x00,0xC0,0x00,0xFF,0xF0,0xFF,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x5A C132 'Z' */
	0x10,0x00,0x30,0x00,0x33,0x80,0x33,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x3F,0x80,0x3F,0x00,0x00,0x00,0x00,0x00,	/* 0x5B C133 '[' */
	0x00,0x00,0x20,0x00,0x60,0x00,0x70,0x00,0x30,0x00,0x30,0x00,0x38,0x00,0x18,0x00,0x18,0x00,0x1C,0x00,0x0C,0x00,0x0C,0x00,0x0E,0x00,0x06,0x00,0x07,0x80,0x07,0x00,0x00,0x00,0x00,0x00,	/* 0x5C C134 '\' */
	0x10,0x00,0x30,0x00,0x3E,0x00,0x3E,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x10,0x00,0x30,0x00,0x3F,0x80,0x3F,0x00,0x00,0x00,0x00,0x00,	/* 0x5D C135 ']' */
	0x00,0x00,0x00,0x00,0x02,0x00,0x47,0x00,0xEB,0xA0,0x71,0xC0,0x20,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x5E C136 '^' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xFF,0xF0,0xFF,0xE0,0x00,0x00,	/* 0x5F C137 '_' */
	0x00,0x00,0x09,0x00,0x12,0x00,0x30,0x00,0x30,0x00,0x3E,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x60 C140 '`' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xFE,0x00,0xFF,0x00,0x20,0x00,0x40,0x00,0xCF,0x00,0xCF,0x00,0xC0,0x00,0xE1,0x00,0x7F,0xE0,0x3D,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x61 C141 'a' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC7,0x00,0xCF,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xC0,0x00,0xC8,0x40,0xFF,0x80,0xEF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x62 C142 'b' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x07,0x40,0x4F,0x80,0xC1,0x00,0xC0,0x00,0xC0,0x00,0xE0,0x00,0x70,0x40,0x3F,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x63 C143 'c' */
	0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x80,0x00,0x80,0x00,0x00,0x07,0x00,0x4F,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xE0,0x00,0x70,0x80,0x3F,0xE0,0x1E,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x64 C144 'd' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x00,0x4F,0x80,0xC0,0x00,0xC0,0x00,0xCF,0xE0,0xC7,0x00,0xE0,0x00,0x70,0x40,0x3F,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x65 C145 'e' */
	0x00,0x00,0x00,0x00,0x04,0x00,0x08,0x00,0x19,0xA0,0x19,0xC0,0x18,0x80,0x18,0x00,0x40,0x00,0xC0,0x00,0xF9,0xC0,0xF9,0x80,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x66 C146 'f' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x4F,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xE0,0x00,0x70,0x80,0x3F,0x80,0x1F,0x00,0x00,0x00,0x20,0x20,0x7F,0xC0,0x3F,0x80,	/* 0x67 C147 'g' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC7,0x00,0xCF,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xF1,0xE0,0xE1,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x68 C150 'h' */
	0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x69 C151 'i' */
	0x00,0x00,0x04,0x00,0x0C,0x00,0x0F,0x00,0x0E,0x00,0x00,0x00,0x04,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x40,0x00,0xC0,0x00,0xE0,0x80,0x7F,0x00,0x3E,0x00,	/* 0x6A C152 'j' */
	0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xC1,0x00,0xC2,0x00,0xC4,0x00,0xC8,0x00,0xC0,0x00,0xC0,0x00,0xCC,0x00,0xCE,0x00,0xC7,0x00,0xF3,0xE0,0xE1,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x6B C153 'k' */
	0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x6C C154 'l' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x41,0x00,0xC2,0x00,0xC0,0x00,0xC8,0x80,0xCC,0xC0,0xCC,0xC0,0xCC,0xC0,0xCC,0xC0,0xCC,0xC0,0xFF,0xF0,0xEE,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x6D C155 'm' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xC7,0x00,0xCF,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xF1,0xE0,0xE1,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x6E C156 'n' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x4F,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xE0,0x00,0x70,0x40,0x3F,0x80,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x6F C157 'o' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC7,0x00,0xCF,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xC0,0x00,0xC8,0x40,0xCF,0x80,0xCF,0x00,0xC0,0x00,0xF0,0x00,0xE0,0x00,	/* 0x70 C160 'p' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x07,0x00,0x4F,0x80,0xC3,0x80,0xC1,0x80,0xC1,0x00,0xE0,0x00,0x70,0x80,0x3F,0x80,0x1F,0x80,0x01,0x80,0x01,0xE0,0x01,0xC0,	/* 0x71 C161 'q' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xC7,0x40,0xCF,0x80,0xC1,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xC0,0x00,0xF0,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x72 C162 'r' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x40,0x4F,0x80,0xC1,0x00,0xE0,0x00,0x7E,0x00,0x1F,0x00,0x40,0x00,0xE0,0x40,0x7F,0x80,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x73 C163 's' */
	0x00,0x00,0x00,0x00,0x08,0x00,0x18,0x00,0x20,0x00,0x60,0x00,0x79,0xC0,0x79,0x80,0x18,0x00,0x18,0x00,0x18,0x80,0x18,0x00,0x18,0x40,0x1F,0x80,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x74 C164 't' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xE3,0x00,0x60,0x00,0x70,0x80,0x3F,0xE0,0x1E,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x75 C165 'u' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x41,0x00,0xC3,0x00,0xC3,0x00,0xE2,0x00,0x66,0x00,0x62,0x00,0x72,0x00,0x30,0x00,0x30,0x00,0x3E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x76 C166 'v' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x20,0xC4,0x60,0xCC,0x40,0xC0,0x40,0xE0,0x00,0x61,0x00,0x63,0x00,0x73,0x80,0x31,0x80,0x31,0xC0,0x20,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x77 C167 'w' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x40,0xE0,0x80,0x71,0x00,0x38,0x00,0x1C,0x00,0x08,0x00,0x10,0x00,0x21,0x00,0x43,0x80,0xF1,0xE0,0xE0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x78 C170 'x' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC1,0x80,0xC3,0x00,0xE0,0x00,0x70,0x80,0x3F,0x80,0x1F,0x00,0x00,0x00,0x20,0x20,0x7F,0xC0,0x3F,0x80,	/* 0x79 C171 'y' */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0xC0,0x00,0xFE,0x00,0xF8,0x00,0x00,0xC0,0x03,0x80,0x0E,0x00,0x40,0x00,0xC0,0x00,0xFF,0xC0,0xFF,0x80,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x7A C172 'z' */
	0x00,0x00,0x08,0x00,0x11,0x80,0x33,0x00,0x32,0x00,0x30,0x00,0x20,0x00,0x00,0x00,0xE0,0x00,0x70,0x00,0x30,0x00,0x30,0x00,0x30,0x00,0x38,0x00,0x1F,0x80,0x0F,0x00,0x00,0x00,0x00,0x00,	/* 0x7B C173 '{' */
	0x08,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x1E,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,	/* 0x7C C174 '|' */
	0x10,0x00,0x30,0x00,0x38,0x00,0x3C,0x00,0x0E,0x00,0x06,0x00,0x06,0x00,0x03,0x00,0x00,0x60,0x02,0x40,0x06,0x00,0x04,0x00,0x10,0x00,0x31,0x00,0x3E,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,	/* 0x7D C175 '}' */
	0x00,0x00,0x10,0x20,0x20,0x40,0x46,0x00,0xE7,0x00,0x73,0xE0,0x21,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 0x7E C176 '~' */
};


const GFXglyph ShadowBoldGlyph[] PROGMEM = {

/* offset, bit-width, bit-height, advance cursor, x offset, y offset */
	{ 0, 16, 18, 12, 1, 0 }, /* 0x20 C040 ' ' */
	{ 36, 16, 18, 12, 1, 0 }, /* 0x21 C041 '!' */
	{ 72, 16, 18, 12, 1, 0 }, /* 0x22 C042 '"' */
	{ 108, 16, 18, 12, 1, 0 }, /* 0x23 C043 '#' */
	{ 144, 16, 18, 12, 1, 0 }, /* 0x24 C044 '$' */
	{ 180, 16, 18, 12, 1, 0 }, /* 0x25 C045 '%' */
	{ 216, 16, 18, 12, 1, 0 }, /* 0x26 C046 '&' */
	{ 252, 16, 18, 12, 1, 0 }, /* 0x27 C047 ''' */
	{ 288, 16, 18, 12, 1, 0 }, /* 0x28 C050 '(' */
	{ 324, 16, 18, 12, 1, 0 }, /* 0x29 C051 ')' */
	{ 360, 16, 18, 12, 1, 0 }, /* 0x2A C052 '*' */
	{ 396, 16, 18, 12, 1, 0 }, /* 0x2B C053 '+' */
	{ 432, 16, 18, 12, 1, 0 }, /* 0x2C C054 ',' */
	{ 468, 16, 18, 12, 1, 0 }, /* 0x2D C055 '-' */
	{ 504, 16, 18, 12, 1, 0 }, /* 0x2E C056 '.' */
	{ 540, 16, 18, 12, 1, 0 }, /* 0x2F C057 '/' */
	{ 576, 16, 18, 12, 1, 0 }, /* 0x30 C060 '0' */
	{ 612, 16, 18, 12, 1, 0 }, /* 0x31 C061 '1' */
	{ 648, 16, 18, 12, 1, 0 }, /* 0x32 C062 '2' */
	{ 684, 16, 18, 12, 1, 0 }, /* 0x33 C063 '3' */
	{ 720, 16, 18, 12, 1, 0 }, /* 0x34 C064 '4' */
	{ 756, 16, 18, 12, 1, 0 }, /* 0x35 C065 '5' */
	{ 792, 16, 18, 12, 1, 0 }, /* 0x36 C066 '6' */
	{ 828, 16, 18, 12, 1, 0 }, /* 0x37 C067 '7' */
	{ 864, 16, 18, 12, 1, 0 }, /* 0x38 C070 '8' */
	{ 900, 16, 18, 12, 1, 0 }, /* 0x39 C071 '9' */
	{ 936, 16, 18, 12, 1, 0 }, /* 0x3A C072 ':' */
	{ 972, 16, 18, 12, 1, 0 }, /* 0x3B C073 ';' */
	{ 1008, 16, 18, 12, 1, 0 }, /* 0x3C C074 '<' */
	{ 1044, 16, 18, 12, 1, 0 }, /* 0x3D C075 '=' */
	{ 1080, 16, 18, 12, 1, 0 }, /* 0x3E C076 '>' */
	{ 1116, 16, 18, 12, 1, 0 }, /* 0x3F C077 '?' */
	{ 1152, 16, 18, 12, 1, 0 }, /* 0x40 C100 '@' */
	{ 1188, 16, 18, 12, 1, 0 }, /* 0x41 C101 'A' */
	{ 1224, 16, 18, 12, 1, 0 }, /* 0x42 C102 'B' */
	{ 1260, 16, 18, 12, 1, 0 }, /* 0x43 C103 'C' */
	{ 1296, 16, 18, 12, 1, 0 }, /* 0x44 C104 'D' */
	{ 1332, 16, 18, 12, 1, 0 }, /* 0x45 C105 'E' */
	{ 1368, 16, 18, 12, 1, 0 }, /* 0x46 C106 'F' */
	{ 1404, 16, 18, 12, 1, 0 }, /* 0x47 C107 'G' */
	{ 1440, 16, 18, 12, 1, 0 }, /* 0x48 C110 'H' */
	{ 1476, 16, 18, 12, 1, 0 }, /* 0x49 C111 'I' */
	{ 1512, 16, 18, 12, 1, 0 }, /* 0x4A C112 'J' */
	{ 1548, 16, 18, 12, 1, 0 }, /* 0x4B C113 'K' */
	{ 1584, 16, 18, 12, 1, 0 }, /* 0x4C C114 'L' */
	{ 1620, 16, 18, 12, 1, 0 }, /* 0x4D C115 'M' */
	{ 1656, 16, 18, 12, 1, 0 }, /* 0x4E C116 'N' */
	{ 1692, 16, 18, 12, 1, 0 }, /* 0x4F C117 'O' */
	{ 1728, 16, 18, 12, 1, 0 }, /* 0x50 C120 'P' */
	{ 1764, 16, 18, 12, 1, 0 }, /* 0x51 C121 'Q' */
	{ 1800, 16, 18, 12, 1, 0 }, /* 0x52 C122 'R' */
	{ 1836, 16, 18, 12, 1, 0 }, /* 0x53 C123 'S' */
	{ 1872, 16, 18, 12, 1, 0 }, /* 0x54 C124 'T' */
	{ 1908, 16, 18, 12, 1, 0 }, /* 0x55 C125 'U' */
	{ 1944, 16, 18, 12, 1, 0 }, /* 0x56 C126 'V' */
	{ 1980, 16, 18, 12, 1, 0 }, /* 0x57 C127 'W' */
	{ 2016, 16, 18, 12, 1, 0 }, /* 0x58 C130 'X' */
	{ 2052, 16, 18, 12, 1, 0 }, /* 0x59 C131 'Y' */
	{ 2088, 16, 18, 12, 1, 0 }, /* 0x5A C132 'Z' */
	{ 2124, 16, 18, 12, 1, 0 }, /* 0x5B C133 '[' */
	{ 2160, 16, 18, 12, 1, 0 }, /* 0x5C C134 '\' */
	{ 2196, 16, 18, 12, 1, 0 }, /* 0x5D C135 ']' */
	{ 2232, 16, 18, 12, 1, 0 }, /* 0x5E C136 '^' */
	{ 2268, 16, 18, 12, 1, 0 }, /* 0x5F C137 '_' */
	{ 2304, 16, 18, 12, 1, 0 }, /* 0x60 C140 '`' */
	{ 2340, 16, 18, 12, 1, 0 }, /* 0x61 C141 'a' */
	{ 2376, 16, 18, 12, 1, 0 }, /* 0x62 C142 'b' */
	{ 2412, 16, 18, 12, 1, 0 }, /* 0x63 C143 'c' */
	{ 2448, 16, 18, 12, 1, 0 }, /* 0x64 C144 'd' */
	{ 2484, 16, 18, 12, 1, 0 }, /* 0x65 C145 'e' */
	{ 2520, 16, 18, 12, 1, 0 }, /* 0x66 C146 'f' */
	{ 2556, 16, 18, 12, 1, 0 }, /* 0x67 C147 'g' */
	{ 2592, 16, 18, 12, 1, 0 }, /* 0x68 C150 'h' */
	{ 2628, 16, 18, 12, 1, 0 }, /* 0x69 C151 'i' */
	{ 2664, 16, 18, 12, 1, 0 }, /* 0x6A C152 'j' */
	{ 2700, 16, 18, 12, 1, 0 }, /* 0x6B C153 'k' */
	{ 2736, 16, 18, 12, 1, 0 }, /* 0x6C C154 'l' */
	{ 2772, 16, 18, 12, 1, 0 }, /* 0x6D C155 'm' */
	{ 2808, 16, 18, 12, 1, 0 }, /* 0x6E C156 'n' */
	{ 2844, 16, 18, 12, 1, 0 }, /* 0x6F C157 'o' */
	{ 2880, 16, 18, 12, 1, 0 }, /* 0x70 C160 'p' */
	{ 2916, 16, 18, 12, 1, 0 }, /* 0x71 C161 'q' */
	{ 2952, 16, 18, 12, 1, 0 }, /* 0x72 C162 'r' */
	{ 2988, 16, 18, 12, 1, 0 }, /* 0x73 C163 's' */
	{ 3024, 16, 18, 12, 1, 0 }, /* 0x74 C164 't' */
	{ 3060, 16, 18, 12, 1, 0 }, /* 0x75 C165 'u' */
	{ 3096, 16, 18, 12, 1, 0 }, /* 0x76 C166 'v' */
	{ 3132, 16, 18, 12, 1, 0 }, /* 0x77 C167 'w' */
	{ 3168, 16, 18, 12, 1, 0 }, /* 0x78 C170 'x' */
	{ 3204, 16, 18, 12, 1, 0 }, /* 0x79 C171 'y' */
	{ 3240, 16, 18, 12, 1, 0 }, /* 0x7A C172 'z' */
	{ 3276, 16, 18, 12, 1, 0 }, /* 0x7B C173 '{' */
	{ 3312, 16, 18, 12, 1, 0 }, /* 0x7C C174 '|' */
	{ 3348, 16, 18, 12, 1, 0 }, /* 0x7D C175 '}' */
	{ 3384, 16, 18, 12, 1, 0 }, /* 0x7E C176 '~' */
	{ 3420, 0, 0, 0, 0, 0 }, /* 0x7F empty 'No Bitmap Defined' */
};


const GFXfont ShadowBold PROGMEM = {
	(uint8_t *)ShadowBoldBitmap,
	(GFXglyph *)ShadowBoldGlyph, 
	32, 127, 18 //ASCII start, ASCII stop,y Advance
};
