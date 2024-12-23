#pragma once

// Basic LCD (DM8B10) functions, driving the display and providing Character definitions

// Segment names (https://en.wikipedia.org/wiki/Sixteen-segment_display)
//   -A1- -A2-  
//  |F\H I J/B|
//   -G1-|-G2-
//  |E/K L M\C|
//   -D1- -D2-

// Segment bit numbers (hex)
//   - F- - 7-
//  |D E 6 4 5|
//   - C-|- 3-
//  |9 B A 2 1|
//   - 8- - 0-
// 

namespace Segments
{
  // Ordered so that 1 << Num is the corresponding bit
  // In a namespace because A1, A2 are taken
  enum  {D2, C, M, G2, J, B, I, A2,   D1, E, L, K, G1, F, H, A1,    X}; // X is not a segment 

  // The physical arrangement of segments, upright, padded with X's
  extern const byte Layout[5][5];
}

#define LCD_NUM_CHARS 10
#define LCD_NUM_SEGS  16
#define LCD_SEG_BIT(_seg_num) ((word)(1 << (_seg_num)))

namespace LCD
{
  void Init();
  void Clear(bool on = false);
  // idx is from the leftmost (horizontal) character
  void SetChar(byte idx, word segments);
  void SetDP(byte idx, bool on); // other DPs not preserved
  void SetBacklight(byte pwm);
  word GetFontSegments(char ch);
}
