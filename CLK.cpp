#include <Arduino.h>
#include "LCD.h"
#include "RTC.h"
#include "LDR.h"
#include "ABTN.h"
#include "CFG.h"
#include "CLK.h"

namespace CLK {
// NOTE: Segments are always represented in the code below in their normal upright form:
//   -A1- -A2-  
//  |F\H I J/B|
//   -G1-|-G2-
//  |E/K L M\C|
//   -D1- -D2-
//
// They are only rotated when displayed:
//     -B-  -C-
//  |A2\J G2 M/D2|
//     -I-| -L-
//  |A1/H G1 K\D1|
//     -F-  -E-

using namespace Segments;

#define BLANK_DIGIT 10  // indicated digit (i.e. leading 0) should be blank

// Larger digits using one and a half 16-segment character
const word PROGMEM doubleHeightTop[] = // top heavy
{
//   -A1- -A2-  
//  |F H I J B|
//   -G1-|-G2-
//  |E K L M C|
//   -D1- -D2-

//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
  0b1010001010100010, // 0
  0b0011000000101000,
#ifdef CFG_DOUBLE_SERIF_ONE
  0b1000010001000000, // 1 serif
  0b0001000001001000,
#else  
  0b0000000000100010, // 1
  0b0000000000100000,
#endif  
  0b1000000110100011, // 2
  0b0011000000001000,
  0b1000000110100011, // 3
  0b0001000000101000,
  0b0010001100100011, // 4
  0b0000000000100000,
  0b1010001110000001, // 5
  0b0001000000101000,
  0b1010001110000001, // 6
  0b0011000000101000,
  0b1000000010100010, // 7
  0b0000000000100000,
  0b1010001110100011, // 8
  0b0011000000101000,
  0b1010001110100011, // 9
  0b0001000000101000,

  0b0000000011101000  // .
};

const word PROGMEM doubleHeightBot[] = // bottom heavy
{
//   -A1- -A2-  
//  |F H I J B|
//   -G1-|-G2-
//  |E K L M C|
//   -D1- -D2-

//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
  0b0001001000001010, // 0
  0b0010001100100011,
#ifdef CFG_DOUBLE_SERIF_ONE
  0b0000110000000000, // 1 serif
  0b0000010101000001,
#else
  0b0000000000000010, // 1
  0b0000000000100010,
#endif  
  0b0001000000001010, // 2
  0b1010001110000001,
  0b0001000000001010, // 3
  0b1000000110100011,
  0b0000001000000010, // 4
  0b1000000010100010,
  0b0001001000001000, // 5
  0b1000000110100011,
  0b0001001000001000, // 6
  0b1010001110100011,
  0b0001000000001010, // 7
  0b0000000000100010,
  0b0001001000001010, // 8
  0b1010001110100011,
  0b0001001000001010, // 9
  0b1000000110100011,

  0b0000010000001011, // .
};

// Small versions of 'A' & 'P' used in double-height display
//  A  G   DA   G  D 
//  1HF1KLE12IBJ2MC2
#define SMALL_A \
  0b0000010011101010 // A
#define SMALL_P \
  0b0000010011101000 // P

// Data for the falling/tumbling animation, define the segments "below" another
//   -A1- -A2-  
//  |F H I J B|
//   -G1-|-G2-
//  |E K L M C|
//   -D1- -D2-
#define SEGS(_s, _a, _b, _c, _d) (word)(Segments::_a | (Segments::_b << 4) | (Segments::_c << 8) | (Segments::_d << 12))
const word PROGMEM SegmentsBelow[] =
{
  // first parm is the segment (also the index, not stored), the others are the segments "below" it, first seg is directly below
  SEGS(D2,  A2,  J,  B,  I),
  SEGS( C,   B, D2, A1,  J),
  SEGS( M,  D2, A1,  J,  I),
  SEGS(G2,   M,  C,  L,  L),
  SEGS( J,  G2,  M,  L,  C),
  SEGS( B,   C, G2,  M, G2),
  SEGS( I,   L,  K,  M, G2),  // so, for example, directly below I is L. Three other plausible segments for I to fall to are K, M and G2 
  SEGS(A2,   J,  B,  I,  I),
  SEGS(D1,  A1,  H,  F,  I),
  SEGS( E,   F, D1,  H, A1),
  SEGS( L,   I, D1, D2, A1),
  SEGS( K,  D1, A1,  H,  F),
  SEGS(G1,   K,  E,  L,  L),
  SEGS( F,   E, G1,  K, G1),
  SEGS( H,  G1,  K,  L,  E),
  SEGS(A1,   H,  F,  I,  I),
};

// Simplify choosing the segments in a row (like E,K,L,M,C) in random order.
// Pick a word at random, each octal digit is a column
const word segmentColumnPermutations[] =
{
  014230,
  043102,
  023041,
  030214,
  003142,
  041320,
  020431,
  013024,
};

byte displayedDigits[4];  // the four digits currently showing (Falling mode only)
byte nextDigits[4]; // what the four digits should be updated to (Falling mode only)

word chars[LCD_NUM_CHARS];  // All the segments for all the characters
#define NUM_DRIPS 50    // Could be less for just the time, but Splash needs more
byte drips[NUM_DRIPS];  // packed 0xIS I = char index, S is segment number
byte numDrips = 0;  // drip count for animation
byte displayedMin = 0xFF; // Current minute or 0xFF if none
// These used to blink the dot
byte dotCounter = 0;
bool dotOn = true;
unsigned long timerMS;
// Used to let a button-press break out of animations
tButton btnPressed;

// n-th entry is the rotated segment number of the n-th bit
// 16-segment character rotated 90deg anticlockwise, dot is top-right
const byte RotatedSegments[] = 
{
#ifdef CFG_DISPLAY_UPRIGHT  
  E, D1, K, L, M, D2, G2, C, F, Segments::A1, G1, H, I, Segments::A2, J, B, 
#else  
  B, Segments::A2, J, I, H, Segments::A1, G1, F, C, D2, G2, M, L, D1, K, E, // 90-deg clockwise
#endif  
};

#ifdef CFG_DEBUG
const char* Names = "D2" " C" " M" "G2" " J" " B" " I" "A2" "D1" " E" " L" " K" "G1" " F" " H" "A1";

void BuildRotatedSegmentTable()
{
  // Debug function to build the items for Rotated and write them to Serial, for pasting into the source code above
  byte nums[LCD_NUM_SEGS];
  for (int r = 0; r < 5; r++)
    for (int c = 0; c < 5; c++)
      if (Segments::Layout[r][c] != Segments::X)
#ifdef CFG_DISPLAY_UPRIGHT  
        //             (upright)       (rotated 90deg anti-clockwise)
        nums[Segments::Layout[r][c]] = Segments::Layout[c][4-r];
#else        
        //             (upright)       (rotated 90deg clockwise)
        nums[Segments::Layout[r][c]] = Segments::Layout[4-c][r];
#endif        
  
  for (int idx = 0; idx < LCD_NUM_SEGS; idx++)
  {
    if (Names[2*nums[idx]] != ' ')
      Serial.print(Names[2*nums[idx]]); 
    Serial.print(Names[2*nums[idx] + 1]); 
    Serial.print(", ");
  }
  Serial.println();
}
#endif

word RotateSegments(word segments)
{
  // segments is the bits for a char on the physical (horizontal) display
  // returns equivalent segments on the rotated display
  word rotated = 0;
  word mask = 1;
  const byte* pRotatedSegments = RotatedSegments;
  while (mask)
  {
    if (mask & segments)
      rotated |= LCD_SEG_BIT(*pRotatedSegments);
    pRotatedSegments++;
    mask <<= 1;
  }
  return rotated;
}

int CharIdx(int i)
{
  // Returns the char index on the display, adjusting for orientation
#ifdef CFG_DISPLAY_UPRIGHT
  return i;
#else    
  return LCD_NUM_CHARS - i - 1;
#endif    
}

void SetChar(byte idx, word segments)
{
  // Sets the segments, rotated, into the display at the idx-th char. 
  // idx 0 is the bottom
  LCD::SetChar(CharIdx(idx), RotateSegments(segments));  
}

void FillUpChar(byte idx, word segments)
{
  // Animates filling the char from bottom up
  // go a little faster -- show entire rows at a time
  word animatedSegs = 0;
  for (int r = 4; r >= 0; r--)
  {
    for (int c = 0; c < 5; c++)
    {
      word segBit = 1 << Segments::Layout[r][c];
      if (segments & segBit)
        animatedSegs |= segBit;
    }
    SetChar(idx, animatedSegs);
    delay(CFG_FRAME_DELAY_MS);
  }
}

bool Above(byte segA, byte segB)
{
  // true if segA is above segB on an upright digit, eg A1 is above G1
  for (int r = 0; r < 5; r++)
    for (int c = 0; c < 5; c++)
      if (Segments::Layout[r][c] == segA || Segments::Layout[r][c] == segB)
        return Segments::Layout[r][c] == segA;
  return false;
}

bool DropSegment(byte& segNumber, bool tumble)
{
  // true if new segment is in lower char
  byte prevSeg = segNumber;
  int offset = tumble?random(4):0;
  segNumber = (pgm_read_word(SegmentsBelow + segNumber) >> offset*4) & 0x0F;
  return Above(segNumber, prevSeg);
}

void UpdateChars()
{
  // Draw all chars
  for (int i = 0; i < LCD_NUM_CHARS; i++)
    SetChar(i, chars[i]);
}

void ConvertToDrips(word segments, byte idx)
{
  // Populate the drips with segments at idx
  // but order them so lower segments are first
  for (int r = 4; r >= 0; r--)
  {
    // pick a random permutation of the columns
    word colPermutations = segmentColumnPermutations[random(sizeof(segmentColumnPermutations)/sizeof(segmentColumnPermutations[0]))];
    for (int c = 0; c < 5; c++)
    {
      int col = (colPermutations & 7) % 5;
      colPermutations >>= 3;
      if (segments & LCD_SEG_BIT(Segments::Layout[r][col]))
        if (numDrips < NUM_DRIPS)
          drips[numDrips++] = (idx << 4) | Segments::Layout[r][col];
    }
  }
}

void DrawDrips(byte maxIdx)
{
  // Redraw characters from drips
  // Character positions 0 (bottom)..maxIdx are cleared because they will be filled from drips[]
  for (int i = 0; i <= maxIdx; i++)
      chars[i] = 0;
  for (int i = 0; i < numDrips; i++)
    if (drips[i] != 0xFF)
      chars[drips[i] >> 4] |= LCD_SEG_BIT(drips[i] & 0x0F);
  UpdateChars();
}

// The four vertical side segments which drop fast
const word FastSegments = LCD_SEG_BIT(Segments::F) | LCD_SEG_BIT(Segments::B) | LCD_SEG_BIT(Segments::E) | LCD_SEG_BIT(Segments::C);

bool DelayDrop(int Step, byte seg)
{
  // true if seg is a fast-falling one and the step is odd
  // lets F/B/E/C stay with the others
  return (Step % 2) && (LCD_SEG_BIT(seg) & FastSegments);
}

bool AdvanceDrips(int Step, bool tumble, byte maxIdx, int updateModulo)
{
  // Move the drips down, progressively
  bool anyActive = false;
  int numToUpdate = updateModulo?min(1 + (Step / updateModulo), numDrips):numDrips;
  for (int i = 0; i < numToUpdate; i++)
    if (drips[i] != 0xFF)
    {
      byte idx = drips[i] >> 4;
      byte seg = drips[i] & 0x0F;
      if (idx > maxIdx)
        break;
      if (tumble || !DelayDrop(Step, seg))
        if (DropSegment(seg, tumble))
          idx--;
      if (idx == 0xFF)
        drips[i] = 0xFF;
      else
        drips[i] = (idx << 4) | seg;
      anyActive = true;
    }
  return anyActive || numToUpdate != numDrips;
}

void SetDP(int idx, bool on)
{
  // Set the DP dot, adjusting for orientation
#ifdef CFG_DISPLAY_UPRIGHT
  LCD::SetDP(8 - idx, on);
#else
  LCD::SetDP(idx, on);
#endif
}

void UpdateDP()
{
  // Update the DP for the mode
  if (CFG::_mode == CFG::Falling || CFG::_mode == CFG::Still)
    SetDP(7, dotOn);
  else if (CFG::_mode == CFG::Word)
    SetDP(1, dotOn);
  else if (CFG::_mode == CFG::DoubleUpper || CFG::_mode == CFG::DoubleLower)
  {
#ifdef CFG_DOUBLE_DOT_CHAR    
    const word* pDoubleFont = (CFG::_mode == CFG::DoubleUpper)?doubleHeightTop:doubleHeightBot;
    chars[5] = dotOn?pgm_read_word(pDoubleFont + 20):0;
    SetChar(5, chars[5]);
#else    
    SetDP(4, dotOn);
#endif    
  }
}

int LoadString(const char* pMStr, int strN)
{
  // Load the strN-th string from pMStr into the display.
  // The first char is at the top, the rest of the display is blanked
  // pMStr is a progmem MStr
  // returns the length of the string
  while (strN)
  {
    while (pgm_read_byte(pMStr))
      pMStr++;
    pMStr++;
    strN--;
  }
  memset(chars, 0, sizeof(chars));
  int len = 0;
  for (int i = 1; i <= LCD_NUM_CHARS; i++)
    if (pgm_read_byte(pMStr))
      chars[LCD_NUM_CHARS - 1 - len++] = LCD::GetFontSegments(pgm_read_byte(pMStr++));
    else
      chars[LCD_NUM_CHARS - i] = 0;
  return len;
}

void LoadNumber(int atIdx, byte num) // two digits
{
  // loads a 2-digit number into chars at the given idx
  chars[atIdx--] = LCD::GetFontSegments('0' + num / 10);
  chars[atIdx] = LCD::GetFontSegments('0' + num % 10);
}

word DigitSegments(byte digit)
{
  // Return the segments for the given digit value (0, 1, etc)
  // Return blank if BLANK_DIGIT
  return (digit < BLANK_DIGIT)?LCD::GetFontSegments('0' + digit):0;
}

// Names of the hours
const char PROGMEM pHoursMStr[] = TEXT_MSTR("TWELVE") TEXT_MSTR("ONE") TEXT_MSTR("TWO") TEXT_MSTR("THREE") TEXT_MSTR("FOUR") TEXT_MSTR("FIVE") 
                                  TEXT_MSTR("SIX") TEXT_MSTR("SEVEN") TEXT_MSTR("EIGHT") TEXT_MSTR("NINE") TEXT_MSTR("TEN") TEXT_MSTR("ELEVEN");


void UpdateTime(byte hour, bool PM, bool force = false)
{
  // Update the time according to the mode
  // If force (and Falling mode) force the animation
  if (CFG::_mode != CFG::Falling && force)
    return;
  if (displayedMin != 0xFF)
  {
    LCD::SetBacklight((LDR::IsBright() || force)?CFG::_dayBrightness:CFG::_nightBrightness);
    // clear the previous time
    if (CFG::_mode == CFG::Falling)
    {
      SetDP(7, false);
      int top = 0;
      for (int i = 0; i < 4; i++)
        if (displayedDigits[i] != nextDigits[i] || force)
          top = i;
      numDrips = 0;
      memset(drips, 0xFF, sizeof(drips));
      for (int i = 0; i <= top; i++)
        ConvertToDrips(DigitSegments(displayedDigits[i]), 6 + i);
      int Step = 0;
      do
      {
        DrawDrips(LCD_NUM_CHARS - (4 - top));
        // let a btn interrupt animation
        if (!force)
        {
          tButton btn = abtn.Pressed();
          if (btn != eButtonNone)
          {
            btnPressed = btn;
            break;
          }
        }
        delay(CFG_FRAME_DELAY_MS);
      } while (AdvanceDrips(Step++, true, 6 + top, 2));
    }
  }
  
  if (CFG::_mode == CFG::Falling)
  {
    // show the new time, simple fill
    for (int i = 0; i < 4; i++)
    {
      if (displayedDigits[i] != nextDigits[i] || force)
      {
        chars[6 + i] = DigitSegments(nextDigits[i]);
        if (!force)
        {        
          tButton btn = abtn.Pressed();
          if (btn != eButtonNone)
          {
            btnPressed = btn;
            break;
          }
        }
        FillUpChar(6 + i, chars[6 + i]);
        displayedDigits[i] = nextDigits[i];
      }
    }
  }
  else if (CFG::_mode == CFG::Still)
  {
    // show the new time, no effects
    ::memset(chars, 0, sizeof(chars));
    for (int i = 0; i < 4; i++)
    {
        chars[6 + i] = DigitSegments(nextDigits[i]);
        displayedDigits[i] = nextDigits[i];
    }
#if defined(CFG_DISPLAY_12_HOUR) && defined(CFG_DISPLAY_AM_PM)
    chars[4] = LCD::GetFontSegments(PM?'P':'A');
    chars[3] = LCD::GetFontSegments('M');
#else
    (void)PM;    
#endif    
    UpdateChars();
  }
  else if (CFG::_mode == CFG::Word)
  {
    LoadString(pHoursMStr, hour % 12);
    chars[0] = DigitSegments(nextDigits[0]);
    chars[1] = DigitSegments(nextDigits[1]);
    UpdateChars();
  }
  else if (CFG::_mode == CFG::DoubleUpper || CFG::_mode == CFG::DoubleLower)
  {
    const word* pDoubleFont = (CFG::_mode == CFG::DoubleUpper)?doubleHeightTop:doubleHeightBot;
    int i = 9;
    for (int j = 3; j >= 0; j--)
    {
      int k = nextDigits[j]*2;
      if (nextDigits[j] < BLANK_DIGIT)
      {
        chars[i--] = pgm_read_word(pDoubleFont + k++);
        chars[i--] = pgm_read_word(pDoubleFont + k++);
      }
      else
      {
        chars[i--] = 0;
        chars[i--] = 0;
      }
      if (j == 2)
        i--;;
      if (j == 0)
        chars[i--] = 0;
      displayedDigits[j] = nextDigits[j];
    }
#if defined(CFG_DISPLAY_12_HOUR) && defined(CFG_DISPLAY_AM_PM)
    if (!force)
      chars[0] = PM?SMALL_P:SMALL_A;
#endif    
    UpdateChars();
  }
}

void Flash(const char* pMsg, int strN, int delayMS = 1000)
{
  // Flash the strN-th string from pMStr on the display.
  // pMsg is a progmem MStr
  LoadString(pMsg, strN);
  UpdateChars();
  delay(delayMS);
  memset(chars, 0, sizeof(chars));
  UpdateChars();
}

// Mode names. must be in config enum order
const char PROGMEM pModeMStr[] = TEXT_MSTR("FALL") TEXT_MSTR("STILL") TEXT_MSTR("WORD") TEXT_MSTR("HIGH") TEXT_MSTR("LOW");

void NewMode()
{
  // Switch to a new mode/face
  Init();
  Flash(pModeMStr, CFG::_mode);
}

const char PROGMEM pFlashMStr[] = TEXT_MSTR("DLS OFF") TEXT_MSTR("DLS ON") TEXT_MSTR("RESET"); 
void FlashDLS(byte DLS)
{
  // Show the DLS mode
  Flash(pFlashMStr, DLS?1:0);
}

void FlashReset()
{
  // Show the "RESET"
  Flash(pFlashMStr, 2);
}

void ShowTime(int Hour24, int Minute)
{
  // basic HHMM, if either is -1, blanked
  memset(chars, 0x00, sizeof(chars));
  chars[9] = DigitSegments((Hour24 >= 0)?Hour24 / 10:BLANK_DIGIT);
  chars[8] = DigitSegments((Hour24 >= 0)?Hour24 % 10:BLANK_DIGIT);
  chars[7] = DigitSegments((Minute >= 0)?Minute / 10:BLANK_DIGIT);
  chars[6] = DigitSegments((Minute >= 0)?Minute % 10:BLANK_DIGIT);
  UpdateChars();  
  SetDP(7, dotOn);  
}

const char PROGMEM pSplashStr[] = TEXT_MSTR("FALLING") TEXT_MSTR("WATER") TEXT_MSTR("MEW   2\xFF""24");
void Splash()
{
  // Show the splash screen: FALLING/WATER/MEW 2024
  for (int i = 0; i < 3; i++)
  {
    int len = LoadString(pSplashStr, i);
    if (i == 0)
    {
      // drip "FALLING"
      UpdateChars();
      delay(1000);
      numDrips = 0;
      memset(drips, 0xFF, sizeof(drips));
      for (int i = 0; i < LCD_NUM_CHARS; i++)
        ConvertToDrips(chars[i], i);
      int Step = 0;
      do
      {
        DrawDrips(LCD_NUM_CHARS - 1);
        delay(CFG_FRAME_DELAY_MS);
      } while (AdvanceDrips(Step++, true, 9, 2));    
    }
    else if (i == 1)
    {
      // fill-up "WATER"
      for (int i = 0; i < len; i++)
        FillUpChar(LCD_NUM_CHARS - len + i, chars[LCD_NUM_CHARS - len + i]);
      delay(1000);
    }
    else
    {
      // Paint "MEW..."
      UpdateChars();
      delay(1000);
    }
  }
  delay(1000);
  LCD::Clear();
}

void Init()
{
  // Init vars for new mode
  LCD::Clear();
  LDR::Reset();
  memset(displayedDigits, 0xFF, 4);
  displayedMin = 0xFF;
  timerMS = millis();
  dotCounter = 0;
  dotOn = true;
}

tButton Loop()
{
  // Main loop, update the time if required, check for buttons
  btnPressed = eButtonNone;
  unsigned long nowMS = millis();
  if ((nowMS - timerMS) > 250UL || displayedMin == 0xFF)
  {
    LCD::SetBacklight(LDR::IsBright()?CFG::_dayBrightness:CFG::_nightBrightness);
    timerMS = nowMS;
    dotCounter++;
    if (dotCounter > 3)
    {
      dotCounter = 0;
#ifdef CFG_DP_BLINKS      
      dotOn = !dotOn;
#endif      
      UpdateDP();
    }

    byte thisMin = rtc.ReadMinute();
    if (displayedMin != thisMin || displayedMin == 0xFF)
    {
      rtc.ReadTime(false);
      if (CFG::_DLSOn)
      {
        rtc.m_Hour24++;
        if (rtc.m_Hour24 == 24)
          rtc.m_Hour24 = 0;
      }      
      byte thisHour = rtc.m_Hour24;
      bool PM = false;
#ifdef CFG_DISPLAY_12_HOUR
      if (thisHour > 12)
      {
        thisHour -= 12;
        PM = true;
      }
      if (thisHour == 0)
        thisHour = 1;
      nextDigits[3] = thisHour / 10;
      if (nextDigits[3] == 0)
        nextDigits[3] = BLANK_DIGIT;
#else        
      nextDigits[3] = thisHour / 10;
#endif
      
      nextDigits[2] = thisHour % 10;
      nextDigits[1] = thisMin / 10;
      nextDigits[0] = thisMin % 10;

      UpdateTime(thisHour, PM);
      displayedMin = thisMin;
    }
  }
  return btnPressed;
}

void AllFallDown()
{
  // Animate the entire time falling down
  UpdateTime(0, false, true);
}

}
