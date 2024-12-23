#include <Arduino.h>
#include "avr/pgmspace.h"
#include "PINS.h"
#include "CFG.h"
#include "LCD.h"

namespace Segments
{
#define _ X // make gaps in table a little easier to read
  const byte Layout[5][5] = 
  {
    // the arrangement of segments on an upright digit, PADDED with X's to make a 5x5 table
     {_, A1,_,A2, _},
     {  F,H,I,J,B  },
     {_, G1,_,G2, _},
     {  E,K,L,M,C  },
     {_, D1,_,D2, _}
  };
#undef _  
}

namespace LCD {
const word PROGMEM font[] =
{
//   -A1- -A2-  
//  |F H I J B|
//   -G1-|-G2-
//  |E K L M C|
//   -D1- -D2-

//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
  0b1010001110100011, // '0'
#ifdef CFG_STANDARD_ONE  
  0b0000000000100010, // std '1', on the right
#else  
//0b0000010001000000, // '1' centred
  0b1000010101000001, // '1' serifs, centred
#endif  
  0b1001001110101001,
  0b1001000110101011,
  0b0011000000101010,
  0b1011000110001011,
  0b1011001110001011,
  0b1000000010100010,
  0b1011001110101011,
  0b1011000110101011, // '9'

//   --- GAP! ---

//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
  0b1011001010101010, // 'A'
  0b1000010111101011,
  0b1010001110000001,
  0b1000010111100011,
  0b1011001110000001,
  0b1011001010000000,
  0b1010001110001011,
  0b0011001000101010,
  0b1000010111000001,
  0b1000011111000000,
  0b0011001000010100,
  0b0010001100000001,
  0b0110001000110010,
  0b0110001000100110,
  0b1010001110100011,
  0b1011001010101000,
  0b1010001110100111,
  0b1011001010101100,
  0b1011000110001011,
  0b1000010011000000,
  0b0010001100100011,
  0b0010101000010000,
  0b0010101000100110,
  0b0100100000010100,
  0b0011010000101000,
  0b1000100110010001, // 'Z'
#ifdef CFG_LOWERCASE
//   --- GAP! ---

//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
  0b0001011100000100, // 'a'
  0b0011011100000000,
  0b0001001100000000,
  0b0001011101000000,
  0b0001101100000000,
  0b0001010011001000,
  0b1011010101000000,
  0b0011011000000000,
  0b0000010010000000,
  0b0000010111000000,
  0b0000010001010100,
  0b0000010001000000,
  0b0001011000001010,
  0b0001011000000000,
  0b0001011100000000,
  0b1011001001000000,
  0b1011010001000000,
  0b0001001000000000,
  0b0000000000001101,
  0b0001010001001001,
  0b0000011100000000,
  0b0000101000000000,
  0b0000101000000110,
  0b0100100000010100,
  0b0100100000010000,
  0b0001100100000000  // 'z'
#endif  
};

// Based on https://github.com/road-t/DM8BA10 but much simplified and without compiler warnings
#define WRITE_DELAY_US 2
// Command codes, including prefix, X=don't care    
//                  0b100CCCCCCCCX
#define CMD_OSC_ON  0b100000000010
#define CMD_OSC_OFF 0b100000000000
#define CMD_LCD_ON  0b100000000110
#define CMD_LCD_OFF 0b100000000100

byte backlightPWM = 0;

void Bits(word data, byte bits)
{
  // Send <bits> bits of <data>, msb first
  word mask = 1 << (bits - 1);
  for (byte i = bits; i > 0; i--)
  {
      digitalWrite(PIN_LCD_WR, LOW);
      delayMicroseconds(WRITE_DELAY_US);
      digitalWrite(PIN_LCD_DATA, (data & mask)?HIGH:LOW);
      delayMicroseconds(WRITE_DELAY_US);
      digitalWrite(PIN_LCD_WR, HIGH);
      delayMicroseconds(WRITE_DELAY_US);
      data <<= 1;
  }
}

void Command(word cmd)
{
  // Send the command word
  digitalWrite(PIN_LCD_CS, LOW);
  Bits(cmd, 12);
  digitalWrite(PIN_LCD_CS, HIGH);  
}

void Data(byte addr, word data, byte bits)
{
  // Send the data bits starting at addr
  digitalWrite(PIN_LCD_CS, LOW);
  Bits(0b101000000 | addr, 9);  // data prefix is 0b101
  Bits(data, bits);
  digitalWrite(PIN_LCD_CS, HIGH);  
}


void Clear(bool on)
{
  // Clear all segments including DPs
  for (byte addr = 0; addr < 0x3F; addr += 4)
    Data(addr, on?0xFFFF:0x0000, LCD_NUM_SEGS);
}  

void Init()
{
  // Turn on the display
  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_LCD_WR, OUTPUT);
  pinMode(PIN_LCD_DATA, OUTPUT);
  pinMode(PIN_LCD_BACKLIGHT, OUTPUT);
  
  Command(CMD_OSC_ON);
  Command(CMD_LCD_ON);
  SetBacklight(255);
  Clear(false);
}

void SetChar(byte idx, word segments)
{
  // Set the idx-th char to segments, idx 0 is left-most/lowest
  Data(0x24 - 4*idx, segments, LCD_NUM_SEGS);
}

void SetBacklight(byte pwm)
{
  // Set backlight intensity
  if (pwm != backlightPWM)
  {
    analogWrite(PIN_LCD_BACKLIGHT, pwm);
    backlightPWM = pwm;
  }
}

void SetDP(byte idx, bool on)
{
  // Set the idx-th DP dot on/off, idx = 0..8
  // NOT a general implementation, only one DP at a time is implemented (they are spread across multiple addresses)
  byte nibble = 0;
  if (on)
    nibble = 1 << (idx % 3);
  Data(0x2D - 2*(idx  / 3), nibble, 4);
}

word GetFontSegments(char ch)
{
  // Returns the physical (un-rotated) segments for ch
  // '\xFF'= all segments
#ifndef CFG_LOWERCASE
  if ('a' <= ch && ch <= 'z')
    ch = toupper(ch);
#endif
  if ('0' <= ch && ch <= '9')
    return pgm_read_word(font + (ch - '0'));
  else if ('A' <= ch && ch <= 'Z')
    return pgm_read_word(font + (ch - 'A' + 10));
#ifdef CFG_LOWERCASE
  else if ('a' <= ch && ch <= 'z')
    return pgm_read_word(font + (ch - 'a' + 10 + 26));
#endif
  else if (ch == '\xFF')
    return 0xFFFF;
  else
    return 0;
}

}
