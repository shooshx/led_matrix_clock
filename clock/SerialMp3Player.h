#pragma once

#include "lib/SoftwareSerial.h"

/************Command byte**************************/
/*basic commands*/
#define CMD_PLAY  0X01
#define CMD_PAUSE 0X02
#define CMD_NEXT_SONG 0X03
#define CMD_PREV_SONG 0X04
#define CMD_VOLUME_UP   0X05
#define CMD_VOLUME_DOWN 0X06
#define CMD_FORWARD 0X0A // >>
#define CMD_REWIND  0X0B // <<
#define CMD_STOP 0X0E
#define CMD_STOP_INJECT 0X0F//stop interruptting with a song, just stop the interlude

/*5 bytes commands*/
#define CMD_SEL_DEV 0X35
  #define DEV_TF 0X01
#define CMD_IC_MODE 0X35
  #define CMD_SLEEP   0X03
  #define CMD_WAKE_UP 0X02
  #define CMD_RESET   0X05

/*6 bytes commands*/  
#define CMD_PLAY_W_INDEX   0X41
#define CMD_PLAY_FILE_NAME 0X42
#define CMD_INJECT_W_INDEX 0X43

/*Special commands*/
#define CMD_SET_VOLUME 0X31
#define CMD_PLAY_W_VOL 0X31

#define CMD_SET_PLAY_MODE 0X33
  #define ALL_CYCLE 0X00
  #define SINGLE_CYCLE 0X01

#define CMD_PLAY_COMBINE 0X45//can play combination up to 15 songs

#define SERIAL_MP3_RX 27
#define SERIAL_MP3_TX 33


class SerialMp3Player
{
public:
  SoftwareSerial m_conn;

  void begin()
  {
    m_conn.begin(9600, SERIAL_MP3_RX, SERIAL_MP3_TX);
    delay(500); //Wait chip initialization is complete
    send_5bytes(CMD_SEL_DEV, DEV_TF);//select the TF card  
    delay(200); //wait for 200ms
  }

  void set_volume(int8_t vol)
  {
    send_5bytes(CMD_SET_VOLUME, vol);
  }
  void play_file(uint8_t folder, uint8_t file)
  {
    uint16_t v = ((uint16_t)folder << 8) | ((uint16_t)file);
    send_6bytes(CMD_PLAY_FILE_NAME, v);
  }

private:
  void send_5bytes(int8_t command, uint8_t dat)
  {
    int8_t send_buf[5] = {0} ;
    send_buf[0] = 0x7e; //starting byte
    send_buf[1] = 0x03; //the number of bytes of the command without starting byte and ending byte
    send_buf[2] = command; 
    send_buf[3] = dat; //
    send_buf[4] = 0xef; //
    send_bytes(send_buf, 5);
  }
  
  void send_6bytes(int8_t command, int16_t dat)
  {
    int8_t send_buf[6] = {0} ;
    send_buf[0] = 0x7e; //starting byte
    send_buf[1] = 0x04; //the number of bytes of the command without starting byte and ending byte
    send_buf[2] = command; 
    send_buf[3] = (int8_t)(dat >> 8);//datah
    send_buf[4] = (int8_t)(dat); //datal
    send_buf[5] = 0xef; //
    send_bytes(send_buf, 6);
  }
  
  void send_bytes(int8_t* send_buf, uint8_t nbytes)
  {
    for(uint8_t i=0; i < nbytes; i++)//
    {
      m_conn.write(send_buf[i]) ;
    }
  }

};
