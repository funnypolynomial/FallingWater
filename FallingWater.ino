// FallingWater
// Clock using a vertically-oriented blue back-lit 16-segment 10-digit LCD display (DM8BA10)
// The main clock face ("FALL") features an animation of the time updating. 
//   The segments of the changing digits tumble down the display like drops of water.
//   The segments of the new digits "fill up".
//
// -- Faces --
// There are some other "experimental" faces:
//  * the time displayed as in the main face (i.e. 4 digits at the top of the display) but without the animation ("STILL")
//  * the time displayed with the hour as a word, e.g. "ELEVEN  23" ("WORD")
//  * the time displayed with 1.5x digits where the full character is the top ("HIGH")
//  * the time displayed with 1.5x digits where the full character is the bottom ("LOW")
//
// -- Buttons --
// There are two buttons, Set and Sel/Adj
//  * Pressing Set configures the time, Sel increments the blinking field, Set accepts it.
//  * Pressing-and-holding Set toggles daylight saving off and on. DLS ON/OFF is shown briefly.
//  * Pressing Sel cycles through the faces.  The name of the face ("FALL" etc) is shown briefly.
//  * Pressing-and-holding Sel in FALL mode forces a full animation of the digits.
//  * Holding Set when turning ON configures the dimming in 3 screens:
//      [DIM nn  mm]      nn is the LDR threshold (blinks), mm is the current reading
//      [HI  nnn   ]      nn is the bright PWM
//      [LO  nnn   ]      nn is the dark PWM
//  * Holding Sel when turning ON resets the configuration to defaults.
//
// -- Brightness --
// A light-dependent-resistor is used to dim the display in low ambient light.
//
// -- Configuration --
// The selected clock face, the DLS status and dimming values are stored in EEPROM
// Other settings (12/24-hour mode, blinking dot etc) are compile-time, see the defines in CFG.h
//
// Mark Wilson December 2024

#include "PINS.h"
#include "ABTN.h"
#include "LDR.h"
#include "LCD.h"
#include "RTC.h"
#include "CFG.h"
#include "CLK.h"

void setup()
{
#ifdef CFG_DEBUG  
  Serial.begin(38400);
  Serial.println("FallingWater");
#endif  
  abtn.Init(PIN_BTN, 900, 400);
  LDR::Init(PIN_LDR);
  rtc.setup();
#ifndef CFG_DEBUG  
  // seed the PRNG from time: hr, min, sec & day
  randomSeed(*(reinterpret_cast<unsigned long*>(&rtc.m_Hour24)));
#endif  
 
  LCD::Init();
  CFG::Init();
  CLK::Init();
#ifndef CFG_DEBUG  
  CLK::Splash();
#endif
}


void loop()
{
  tButton btn1 = abtn.Pressed();
  tButton btn2 = CLK::Loop();
  if (btn1 == eButtonSet || btn2 == eButtonSet)
    CFG::Time();
  else if (btn1 == eButtonSel || btn2 == eButtonSel)
    CFG::Mode();
}
