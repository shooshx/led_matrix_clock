/*********************************************************************
This is a library for Chinese LED matrix displays

Written by Dominic Buchstaller.
BSD license, check license.txt for more information
*********************************************************************/


#ifndef _PxMATRIX_H
#define _PxMATRIX_H

// Color depth per primary color - the more the slower the update
#ifndef PxMATRIX_COLOR_DEPTH
#define PxMATRIX_COLOR_DEPTH 4
#endif

#if PxMATRIX_COLOR_DEPTH > 8 || PxMATRIX_COLOR_DEPTH < 1
#error "PxMATRIX_COLOR_DEPTH must be 1 to 8 bits maximum"
#endif

// Defines how long we display things by default
#ifndef PxMATRIX_DEFAULT_SHOWTIME
#define PxMATRIX_DEFAULT_SHOWTIME 30
#endif

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef PxMATRIX_SPI_FREQUENCY
#define PxMATRIX_SPI_FREQUENCY 20000000
#endif

// Legacy suppport
#ifdef double_buffer
#define PxMATRIX_DOUBLE_BUFFER true
#endif

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#if defined(ESP8266) || defined(ESP32)
  #define SPI_TRANSFER(x,y) SPI.writeBytes(x,y)
  #define SPI_BYTE(x) SPI.write(x)
  #define SPI_2BYTE(x) SPI.write16(x)
#endif


#include "Adafruit_GFX.h"
#include "Arduino.h"
#include <SPI.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <stdlib.h>

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif

#ifdef ESP32
  #define GPIO_REG_SET(val) GPIO.out_w1ts = val
  #define GPIO_REG_CLEAR(val) GPIO.out_w1tc = val
#else
  #error all non ESP32 code was removed  
#endif


#ifdef ESP32

  #include "soc/spi_struct.h"
  #include "esp32-hal-gpio.h"

  struct spi_struct_t {
      spi_dev_t * dev;
  #if !CONFIG_DISABLE_HAL_LOCKS
      xSemaphoreHandle lock;
  #endif
      uint8_t num;
  };
#endif

// HW SPI PINS
#define SPI_BUS_CLK 14
#define SPI_BUS_MOSI 13
#define SPI_BUS_MISO 12
#define SPI_BUS_SS 4

// Specify how the Panel handles row muxing:
// BINARY: Pins A-E map to rows 1-32 via binary decoding (default)
// STRAIGHT: Pins A-D are directly mapped to rows 1-4
// SHIFTREG: A, B, C on Panel are connected to a shift register Clock, /Enable, Data
// SHIFTREG_ABC_BIN_DE: A-C are connected to Shift-Register Clock, Data, /Enable, D-E to binary decoder (crazy shit)
// SHIFTREG_SPI_SE: Like SHIFTREG, but you connect A and C on Panel to its Clock and Data output (and ground B). This will not work with fast_update enabled!
enum mux_patterns {BINARY, STRAIGHT, SHIFTREG_ABC, SHIFTREG_SPI_SE, SHIFTREG_ABC_BIN_DE};

// Specifies what blocking pattern the panel is using 
// |AB|,|DB|
// |CD|,|CA|
// |AB|,|DB|
// |CD|,|CA|
enum block_patterns {ABCD, DBCA};

// This is how the scanning is implemented. LINE just scans it left to right,
// ZIGZAG jumps 4 rows after every byte, ZAGGII alse revereses every second byte
enum scan_patterns {LINE, ZIGZAG,ZZAGG, ZAGGIZ, WZAGZIG, VZAG, ZAGZIG, WZAGZIG2, ZZIAGG};

// Specifies speciffic driver chip. Most panels implement a standard shifted
// register (SHIFT). Other chips/panels may need special treatment in oder to work
enum driver_chips {SHIFT, FM6124, FM6126A};

// Specify the color order
enum color_orders {RRGGBB, RRBBGG, GGRRBB, GGBBRR, BBRRGG, BBGGRR};


#define color_step (256 / PxMATRIX_COLOR_DEPTH)
#define color_half_step (int(color_step / 2))
#define color_third_step (int(color_step / 3))
#define color_two_third_step (int(color_third_step*2))

class PxMATRIX : public Adafruit_GFX {
 public:
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D);
  inline PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D,uint8_t E);

  inline void begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS);
  inline void begin(uint8_t row_pattern);
  inline void begin();

  inline void clearDisplay(void);
  inline void clearDisplay(bool selected_buffer);

  // Updates the display
  inline void display(uint16_t show_time);
  inline void display();

  // Draw pixels
  inline void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);

  inline void drawPixel(int16_t x, int16_t y, uint16_t color);

  inline void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b);

  // Does nothing for now (always returns 0)
  uint8_t getPixel(int8_t x, int8_t y);

  // Converts RGB888 to RGB565
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  // Helpful for debugging (place in display update loop)
  inline void displayTestPattern(uint16_t showtime);

  // Helpful for debugging (place in display update loop)
  inline void displayTestPixel(uint16_t show_time);

  // FLush the buffer of the display
  inline void flushDisplay();

  // Rotate display
  inline void setRotate(bool rotate);

  // Flip display
  inline void setFlip(bool flip);

  // Helps to reduce display update latency on larger displays
  inline void setFastUpdate(bool fast_update);

  // When using double buffering, this displays the draw buffer
  inline void showBuffer();

#ifdef PxMATRIX_DOUBLE_BUFFER
  // This copies the display buffer to the drawing buffer (or reverse)
  inline void copyBuffer(bool reverse);
#endif

  // Control the minimum color values that result in an active pixel
  inline void setColorOffset(uint8_t r, uint8_t g,uint8_t b);

  // Set the multiplex implemention {BINARY, STRAIGHT, SHIFTREG} (default is BINARY)
  inline void setMuxPattern(mux_patterns mux_pattern);

    // Set the color order
  inline void setColorOrder(color_orders color_order);

  // Set the time in microseconds that we pause after selecting each mux channel
  // (May help if some rows are missing / the mux chip is too slow)
  inline void setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E);

  // Set the multiplex pattern {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG, WZAGZIG2} (default is LINE)
  inline void setScanPattern(scan_patterns scan_pattern);

 // Set the block pattern {ABCD, DBCA} (default is ABCD)
  inline void setBlockPattern(block_patterns block_pattern);

  // Set the number of panels that make up the display area width (default is 1)
  inline void setPanelsWidth(uint8_t panels);

  // Set the brightness of the panels (default is 255)
  inline void setBrightness(uint8_t brightness);

  // Set driver chip type
  inline void setDriverChip(driver_chips driver_chip);

 private:

 // the display buffer for the LED matrix
  uint8_t *PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
  uint8_t *PxMATRIX_buffer2;
#endif

  // GPIO pins
  uint8_t _LATCH_PIN;
  uint8_t _OE_PIN;
  uint8_t _A_PIN;
  uint8_t _B_PIN;
  uint8_t _C_PIN;
  uint8_t _D_PIN;
  uint8_t _E_PIN;

  // SPI pins
  uint8_t _SPI_CLK = SPI_BUS_CLK;
  uint8_t _SPI_MOSI = SPI_BUS_MOSI;
  uint8_t _SPI_MISO = SPI_BUS_MISO;
  uint8_t _SPI_SS = SPI_BUS_SS;

  uint16_t _width;
  uint16_t _height;
  uint8_t _panels_width;
  uint8_t _rows_per_buffer;
  uint8_t _row_sets_per_buffer;
  uint8_t _panel_width_bytes;

  // Color offset
  uint8_t _color_R_offset;
  uint8_t _color_G_offset;
  uint8_t _color_B_offset;

  // Panel Brightness
  uint8_t _brightness;

  // Color pattern that is pushed to the display
  uint8_t _display_color;

  // Holds some pre-computed values for faster pixel drawing
  uint32_t *_row_offset;

  // Holds the display row pattern type
  uint8_t _row_pattern;

  // Number of bytes in one color
  uint8_t _pattern_color_bytes;

  // Total number of bytes that is pushed to the display at a time
  // 3 * _pattern_color_bytes
  uint16_t _buffer_size;
  uint16_t _send_buffer_size;

  // This is for double buffering
  bool _active_buffer;

  // Display and color engine
  bool _rotate;
  bool _flip;
  bool _fast_update;

  // Holds multiplex pattern
  mux_patterns _mux_pattern;

  //Holdes the color order
  color_orders _color_order;

  uint8_t _mux_delay_A;
  uint8_t _mux_delay_B;
  uint8_t _mux_delay_C;
  uint8_t _mux_delay_D;
  uint8_t _mux_delay_E;

  // Holds the scan pattern
  scan_patterns _scan_pattern;

  // Holds the block pattern
  block_patterns _block_pattern;

  // Holds the used driver chip
  driver_chips _driver_chip;

  // Used for test pattern
  uint16_t _test_pixel_counter;
  uint16_t _test_line_counter;
  unsigned long _test_last_call;

  // Generic function that draw one pixel
inline void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b, bool selected_buffer);

  // Init code common to both constructors
inline void init(uint16_t width, uint16_t height ,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B);

  // Light up LEDs and hold for show_time microseconds
inline void latch(uint16_t show_time );

  // Set row multiplexer
inline void set_mux(uint8_t value, bool random_access);

inline void spi_init();

// Write configuration register in some driver chips
inline void writeRegister(uint16_t reg_value, uint8_t reg_position);
inline void fm612xWriteRegister(uint16_t reg_value, uint8_t reg_position);
};

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
inline uint16_t PxMATRIX::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Init code common to both constructors
inline void PxMATRIX::init(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A, uint8_t B){
  _LATCH_PIN = LATCH;
  _OE_PIN = OE;
  _display_color=0;

  _A_PIN = A;
  _B_PIN = B;

  _width = width;
  _height = height;

  _buffer_size = (_width*_height * 3 / 8);

  _brightness=255;
  _panels_width = 1;

  _rows_per_buffer = _height/2;
  _panel_width_bytes = (_width/_panels_width)/8;

  _active_buffer=false;

  _color_R_offset=0;
  _color_G_offset=0;
  _color_B_offset=0;

  _test_last_call=0;
  _test_pixel_counter=0;
  _test_line_counter=0;
  _rotate=0;
  _flip=0;
  _fast_update=0;

  _row_pattern=0;
  _scan_pattern=LINE;
  _block_pattern=ABCD;
  _driver_chip=SHIFT;

  _mux_delay_A=0;
  _mux_delay_B=0;
  _mux_delay_C=0;
  _mux_delay_D=0;
  _mux_delay_E=0;

    PxMATRIX_buffer= new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_buffer2=new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #endif

}


inline void PxMATRIX::setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E)
{
  _mux_delay_A=mux_delay_A;
  _mux_delay_B=mux_delay_B;
  _mux_delay_C=mux_delay_C;
  _mux_delay_D=mux_delay_D;
  _mux_delay_E=mux_delay_E;
}


inline void PxMATRIX::setPanelsWidth(uint8_t panels) {
  _panels_width=panels;
  _panel_width_bytes = (_width/_panels_width)/8;
}

inline void PxMATRIX::setRotate(bool rotate) {
  _rotate=rotate;
}

inline void PxMATRIX::setFlip(bool flip) {
  _flip=flip;
}

inline void PxMATRIX::setFastUpdate(bool fast_update) {
  _fast_update=fast_update;
}

inline void PxMATRIX::setBrightness(uint8_t brightness) {
  _brightness=brightness;
}


inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height, LATCH, OE, A, B);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  init(width, height, LATCH, OE, A, B);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  _D_PIN = D;
  init(width, height, LATCH, OE, A, B);
}

inline PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D, uint8_t E) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  _D_PIN = D;
  _E_PIN = E;
  init(width, height, LATCH, OE, A, B);
}

inline void PxMATRIX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  drawPixelRGB565(x, y, color);
}

inline void PxMATRIX::showBuffer() {
  _active_buffer=!_active_buffer;
}

#ifdef PxMATRIX_DOUBLE_BUFFER
inline void PxMATRIX::copyBuffer(bool reverse = false) {
  // This copies the display buffer to the drawing buffer (or reverse)
  // You may need this in case you rely on the framebuffer to always contain the last frame
  // _active_buffer = true means that PxMATRIX_buffer2 is displayed
  if(_active_buffer ^ reverse) {
    memcpy(PxMATRIX_buffer, PxMATRIX_buffer2, PxMATRIX_COLOR_DEPTH*_buffer_size);
  } else {
    memcpy(PxMATRIX_buffer2, PxMATRIX_buffer, PxMATRIX_COLOR_DEPTH*_buffer_size);
  }
}
#endif



inline void PxMATRIX::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b,bool selected_buffer)
{   
  if (_rotate){
    uint16_t temp_x=x;
    x=y;
    y=_height-1-temp_x;
  }

  // Panels are naturally flipped
  if (!_flip)
    x =_width - 1 -x;
  
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return; 
  
  
  uint32_t base_offset;
  uint32_t total_offset_r=0;
  uint32_t total_offset_g=0;
  uint32_t total_offset_b=0;


  {
    // can only be non-zero when _height/(2 inputs per panel)/_row_pattern > 1
    // i.e.: 32x32 panel with 1/8 scan (A/B/C lines) -> 32/2/8 = 2
    uint8_t vert_index_in_buffer = (y%_rows_per_buffer)/_row_pattern; // which set of rows per buffer

    // can only ever be 0/1 since there are only ever 2 separate input sets present for this variety of panels (R1G1B1/R2G2B2)
    uint8_t which_buffer = y/_rows_per_buffer;
    uint8_t x_byte = x/8;
    // assumes panels are only ever chained for more width
    uint8_t which_panel = x_byte/_panel_width_bytes;
    uint8_t in_row_byte_offset = x_byte%_panel_width_bytes;
    // this could be pretty easily extended to vertical stacking as well
    total_offset_r = _row_offset[y] - in_row_byte_offset - _panel_width_bytes*(_row_sets_per_buffer*(_panels_width*which_buffer + which_panel) + vert_index_in_buffer);
  }

  uint8_t bit_select = x%8;


  total_offset_g=total_offset_r-_pattern_color_bytes;
  total_offset_b=total_offset_g-_pattern_color_bytes;

uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;

#ifdef PxMATRIX_DOUBLE_BUFFER
  PxMATRIX_bufferp = selected_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

  r = r >> (8-PxMATRIX_COLOR_DEPTH);
  g = g >> (8-PxMATRIX_COLOR_DEPTH);
  b = b >> (8-PxMATRIX_COLOR_DEPTH);
  
  //Color interlacing
  for (int this_color_bit=0; this_color_bit<PxMATRIX_COLOR_DEPTH; this_color_bit++)
  {
    if ((r >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] &= ~_BV(bit_select);

    if ((g >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] &= ~_BV(bit_select);

    if ((b >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] &= ~_BV(bit_select);
  }
}

inline void PxMATRIX::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
  uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
  uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
  uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
  //Serial.printf("drawPixel %d, %d, %d\n", r, g, b);
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

inline void PxMATRIX::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

// the most basic function, get a single pixel
inline uint8_t PxMATRIX::getPixel(int8_t x, int8_t y) {
  return (0);//PxMATRIX_buffer[x+ (y/8)*LCDWIDTH] >> (y%8)) & 0x1;
}

inline void PxMATRIX::begin()
{
  begin(8);

}

void PxMATRIX::begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS)
{
  
  _SPI_CLK = CLK;
  _SPI_MOSI = MOSI;
  _SPI_MISO = MISO;
  _SPI_SS = SS;
  begin(row_pattern);

}

void PxMATRIX::spi_init(){

  #ifdef ESP32
    SPI.begin(_SPI_CLK, _SPI_MISO, _SPI_MOSI, _SPI_SS);
  #endif

  #if defined(ESP32) || defined(ESP8266)
    SPI.setFrequency(PxMATRIX_SPI_FREQUENCY);
  #endif

    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);

}

void PxMATRIX::begin(uint8_t row_pattern) {

  _row_pattern=row_pattern;

  _mux_pattern = BINARY;
  _color_order=RRGGBB;

  _pattern_color_bytes=(_height/_row_pattern)*(_width/8);
  _row_sets_per_buffer = _rows_per_buffer/_row_pattern;
  _send_buffer_size=_pattern_color_bytes*3;

  spi_init();

  pinMode(_OE_PIN, OUTPUT);
  pinMode(_LATCH_PIN, OUTPUT);
  pinMode(_A_PIN, OUTPUT);
  pinMode(_B_PIN, OUTPUT);
  digitalWrite(_A_PIN, LOW);
  digitalWrite(_B_PIN, LOW);
  digitalWrite(_OE_PIN, HIGH);

  if (_row_pattern >=8)
  {
    pinMode(_C_PIN, OUTPUT);
    digitalWrite(_C_PIN, LOW);
  }
  if (_row_pattern >=16)
  {
    pinMode(_D_PIN, OUTPUT);
    digitalWrite(_D_PIN, LOW);
  }
  if (_row_pattern >=32)
  {
    pinMode(_E_PIN, OUTPUT);
    digitalWrite(_E_PIN, LOW);
  }

  // Precompute row offset values
  _row_offset=new uint32_t[_height];
  for (uint8_t yy=0; yy<_height;yy++)
      _row_offset[yy]=((yy)%_row_pattern)*_send_buffer_size+_send_buffer_size-1;

}

void PxMATRIX::set_mux(uint8_t value, bool random_access = false)
{

  if (_mux_pattern==BINARY)
  {
    if (value & 0x01)
      digitalWrite(_A_PIN,HIGH);
    else
      digitalWrite(_A_PIN,LOW);
    if (_mux_delay_A) delayMicroseconds(_mux_delay_A);

    if (value & 0x02)
      digitalWrite(_B_PIN,HIGH);
    else
      digitalWrite(_B_PIN,LOW);
    if (_mux_delay_B) delayMicroseconds(_mux_delay_B);

    if (_row_pattern>=8)
    {

      if (value & 0x04)
      digitalWrite(_C_PIN,HIGH);
      else
      digitalWrite(_C_PIN,LOW);
      if (_mux_delay_C) delayMicroseconds(_mux_delay_C);
    }


    if (_row_pattern>=16)
    {
      if (value & 0x08)
          digitalWrite(_D_PIN,HIGH);
      else
          digitalWrite(_D_PIN,LOW);
      if (_mux_delay_D) delayMicroseconds(_mux_delay_D);
    }

    if (_row_pattern>=32)
    {
      if (value & 0x10)
          digitalWrite(_E_PIN,HIGH);
      else
          digitalWrite(_E_PIN,LOW);
      if (_mux_delay_E) delayMicroseconds(_mux_delay_E);
    }
  }


}

void PxMATRIX::latch(uint16_t show_time )
{

  if (_driver_chip==SHIFT)
  {
    //digitalWrite(_OE_PIN,0); // <<< remove this
    digitalWrite(_LATCH_PIN,HIGH);
    //delayMicroseconds(10);
    digitalWrite(_LATCH_PIN,LOW);

    //delayMicroseconds(10);
    if (show_time >0)
    {
      //delayMicroseconds(show_time);
      digitalWrite(_OE_PIN,0);
      unsigned long start_time=micros();
      while ((micros()-start_time)<show_time)
        asm volatile (" nop ");
      digitalWrite(_OE_PIN,1);
    }
  }

}

 void PxMATRIX::display()
{
  display(PxMATRIX_DEFAULT_SHOWTIME);
}

void PxMATRIX::display(uint16_t show_time) {
  if (show_time == 0)
    show_time =1;
  
  // How long do we keep the pixels on
  uint16_t latch_time = ((show_time*(1<<_display_color)*_brightness)/255/2);

  unsigned long start_time=0;
#ifdef ESP8266
  ESP.wdtFeed();
#endif

  uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;

#ifdef PxMATRIX_DOUBLE_BUFFER
  PxMATRIX_bufferp = _active_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

  for (uint8_t i=0;i<_row_pattern;i++)
  {
    if(_driver_chip == SHIFT) {
      if ((_fast_update)&&(_brightness==255)){

        // This will clock data into the display while the outputs are still
        // latched (LEDs on). We therefore utilize SPI transfer latency as LED
        // ON time and can reduce the waiting time (show_time). This is rather
        // timing sensitive and may lead to flicker however promises reduced
        // update times and increased brightness

        set_mux(i);
        digitalWrite(_LATCH_PIN,HIGH);
        digitalWrite(_LATCH_PIN,LOW);
        digitalWrite(_OE_PIN,LOW);
        start_time = micros();
        delayMicroseconds(1);
        if (i<_row_pattern-1)
        {
          // This pre-buffers the data for the next row pattern of this _display_color
          SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+(i+1)*_send_buffer_size],_send_buffer_size);
        }
        else 
        { 
          // This pre-buffers the data for the first row pattern of the next _display_color
          SPI_TRANSFER(&PxMATRIX_bufferp[((_display_color+1)%PxMATRIX_COLOR_DEPTH)*_buffer_size],_send_buffer_size); 
        }
       
        while ((micros()-start_time)<latch_time)
          delayMicroseconds(1);
        digitalWrite(_OE_PIN,HIGH);

      }
      else
      {
        set_mux(i);

        SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size],_send_buffer_size);

        latch(latch_time); 
      }
    }
    

  }
  _display_color++;
  if (_display_color>=PxMATRIX_COLOR_DEPTH)
  {
    _display_color=0;
  }
}

void PxMATRIX::flushDisplay(void) {
  for (int ii=0;ii<_send_buffer_size;ii++)
    SPI_BYTE(0x00);
}

void PxMATRIX::displayTestPattern(uint16_t show_time) {

  if ((millis()-_test_last_call)>500)
  {
    flushDisplay();
    for (int ii=0;ii<=_test_pixel_counter;ii++)
      SPI_BYTE(0xFF);
    _test_last_call=millis();
    _test_pixel_counter++;
  }

  if (_test_pixel_counter>_send_buffer_size)

  {
    _test_pixel_counter=0;
    _test_line_counter++;
    flushDisplay();
  }

  if (_test_line_counter> (_height/2))
        _test_line_counter=0;

  digitalWrite(_A_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_B_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_C_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_D_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_E_PIN,HIGH);
  delayMicroseconds(1);

  digitalWrite(_A_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_B_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_C_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_D_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_E_PIN,LOW);
  delayMicroseconds(1);
  set_mux(_test_line_counter, true);

  latch(show_time);
}

void PxMATRIX::displayTestPixel(uint16_t show_time) {
  if ((millis()-_test_last_call)>500)
  {
    flushDisplay();
    uint16_t blanks = _test_pixel_counter/8;
    SPI_BYTE(1<<_test_pixel_counter%8);
    while (blanks){
      SPI_BYTE(0x00);
      blanks--;
    }
    _test_last_call=millis();
    _test_pixel_counter++;
  }

  if (_test_pixel_counter>_send_buffer_size/3*8)

  {
    _test_pixel_counter=0;
    _test_line_counter++;
  }

  if (_test_line_counter> (_height/2))
        _test_line_counter=0;

  digitalWrite(_A_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_B_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_C_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_D_PIN,HIGH);
  delayMicroseconds(1);
  digitalWrite(_E_PIN,HIGH);
  delayMicroseconds(1);

  digitalWrite(_A_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_B_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_C_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_D_PIN,LOW);
  delayMicroseconds(1);
  digitalWrite(_E_PIN,LOW);
  delayMicroseconds(1);

  set_mux(_test_line_counter, true);

  latch(show_time);
}

void PxMATRIX::clearDisplay(void) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  clearDisplay(!_active_buffer);
#else
  clearDisplay(false);
#endif
}
//    void * memset ( void * ptr, int value, size_t num );

// clear everything
void PxMATRIX::clearDisplay(bool selected_buffer) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  if(selected_buffer)
    memset(PxMATRIX_buffer2, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
  else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#endif
}

#endif
