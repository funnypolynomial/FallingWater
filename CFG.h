#pragma once

// *** Compile-time configuration ***

// If defined, display is oriented with pins at the bottom (DPs on right), otherwise at the top  (DPs on left)
#define CFG_DISPLAY_UPRIGHT

// Delay between steps in the falling water animation, milliseconds
#define CFG_FRAME_DELAY_MS 100

// If defined, the single char '1' is the standard, on the right. Otherwise centred (serif)
//#define CFG_STANDARD_ONE

// If defined, double-height fonts have a centred '1' with serifs, otherwise simple vertical, on the right
#define CFG_DOUBLE_SERIF_ONE

// If defined, double-height modes use the 20th table entry as the blinking dot character, otherwise a DP 
#define CFG_DOUBLE_DOT_CHAR

// If defined, lowercase chars are supported
#define CFG_LOWERCASE

// If defined, time is displayed in 12-hour mode, otherwise 24-hour
#define CFG_DISPLAY_12_HOUR

// If defined, and CFG_DISPLAY_12_HOUR, A or P is added to the time display (except in Fall & Word modes)
#define CFG_DISPLAY_AM_PM

// These three LDR/PWM's are just DEFAULTS
// An LDR reading above this is considered "bright" (day) ambient (MULTIPLE OF 100)
#define CFG_LDR_DAY_THRESHOLD 100
// Backlight PWM value in high ambient light, 0 (off) ...(Powers of 2)... 255 (max).
#define CFG_DAY_BRIGHTNESS    255
// Backlight PWM value in low ambient light, 0 (off) ...(Powers of 2)... 255 (max).
#define CFG_NIGHT_BRIGHTNESS  16

// Duration of a consistent LDR reading for it to become current
#define CFG_LDR_HOLD_TIME_MS 10000UL

// If defined, Hour-minute DP separator blinks, otherwise solid
#define CFG_DP_BLINKS

// If defined, no splash, no randomSeed, serial output on, etc
//#define CFG_DEBUG

// *** Run-time configuration ***
namespace CFG
{
  enum Mode {Falling, Still, Word, DoubleUpper, DoubleLower};
  extern byte _mode;  // display mode/face
  extern byte _DLSOn; // DLS on (+1hour)
  extern int _LDRThreshold;
  extern byte _dayBrightness;
  extern byte _nightBrightness;
  void Init();
  void Reset();
  void Mode();
  void Time();
};
