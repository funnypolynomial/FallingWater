#include <Arduino.h>
#include <EEPROM.h>
#include "ABTN.h"
#include "RTC.h"
#include "LDR.h"
#include "LCD.h"
#include "CLK.h"
#include "CFG.h"

namespace CFG {
#define CHECK_CHAR 'F'
#define LDR_DIVISOR 100

byte _mode = Falling;  // display mode/face
byte _DLSOn = 0; // DLS on (+1hour)
int _LDRThreshold = CFG_LDR_DAY_THRESHOLD;
byte _dayBrightness = CFG_DAY_BRIGHTNESS;
byte _nightBrightness = CFG_NIGHT_BRIGHTNESS;// cfg bytes are
// 0: 'F'
// 1: mode/face
// 2: DLS
// 3: LDR hi/lo switch value /100, 0..10
// 4: HI bright PWM, stored as power of 2, 0..8
// 5: LO dim PWM, stored as power of 2, 0..8
enum {IDX_CHECK, IDX_MODE, IDX_DLS, IDX_LDR, IDX_HI, IDX_LO};

void Read()
{
  // read and validate EEPROM values
  _mode = EEPROM.read(IDX_MODE);
  if (_mode > DoubleLower)
    _mode = Falling;
  _DLSOn = EEPROM.read(IDX_DLS);
  if (_DLSOn)
    _DLSOn = 1;
  _LDRThreshold = EEPROM.read(IDX_LDR)*LDR_DIVISOR;
  if (_LDRThreshold > 1000)
    _LDRThreshold = 1000;
  _dayBrightness =  EEPROM.read(IDX_HI);
  _nightBrightness =  EEPROM.read(IDX_LO);   
}

void Write()
{
  // write EEPROM values
  EEPROM.write(IDX_CHECK, CHECK_CHAR);
  EEPROM.write(IDX_MODE, _mode);
  EEPROM.write(IDX_DLS, _DLSOn);
  EEPROM.write(IDX_LDR, _LDRThreshold/LDR_DIVISOR);
  EEPROM.write(IDX_HI, _dayBrightness);
  EEPROM.write(IDX_LO, _nightBrightness);
}

void Reset()
{
  _mode = Falling;
  _DLSOn = 0;
  _LDRThreshold = CFG_LDR_DAY_THRESHOLD;
  _dayBrightness = CFG_DAY_BRIGHTNESS;
  _nightBrightness = CFG_NIGHT_BRIGHTNESS;  
}

void Dimming();

void Init()
{
  // read from EEPROM and validate
  if (EEPROM.read(IDX_CHECK) != CHECK_CHAR)
  {
    Reset();
    Write();
  }
  else
    Read();
  tButton btn = abtn.Down();
  if (btn == eButtonSet)
    Dimming();
  else if (btn == eButtonSel)
  {
    Reset();
    Write();
    CLK::FlashReset();
  }
}

bool CheckForceFall()
{
  // check for SEL held -- in Falling mode, force fall animation
  if (_mode == Falling)
  {
    int delayCounter = 5;  // half seconds
    while (delayCounter)
    {
      if (abtn.Down() != eButtonSel)
      {
        break;
      }
      delayCounter--;
      delay(500);
    }
    if (!delayCounter && abtn.Down() == eButtonSel)
    {
      // held
      CLK::AllFallDown();
      return true;
    }    
  }
  return false;
}

void Mode()
{
  // increment the mode, wrapping around
  if (CheckForceFall()) // or demo instead
    return;  
  LCD::SetBacklight(255);
  _mode++;
  if (_mode > DoubleLower)
    _mode = Falling;
  EEPROM.write(IDX_MODE, _mode);
  CLK::NewMode();
}

bool CheckDLSToggle()
{
  // check for SET held -- toggle DLS, returns true if was toggled
  int delayCounter = 6;  // half seconds
  while (delayCounter)
  {
    if (abtn.Down() != eButtonSet)
    {
      break;
    }
    delayCounter--;
    delay(500);
  }
  if (!delayCounter && abtn.Down() == eButtonSet)
  {
    // held
    _DLSOn = _DLSOn?0:1;
    EEPROM.write(IDX_DLS, _DLSOn);
    CLK::FlashDLS(_DLSOn);
    CLK::Init();
    return true;
  }
  return false;
}

void Time()
{
  // set the time
  LCD::SetBacklight(255);
  if (CheckDLSToggle())
    return;
  bool blinkOn = true;
  bool editHour = true;
  bool redraw = true;
  unsigned long blinkTimeMS = millis();
  bool Write = false;
  unsigned long idleTimeMS = millis();
  byte hour24;
  byte minute;
  rtc.ReadTime(false);
  if (CFG::_DLSOn)
  {
    rtc.m_Hour24++;
    if (rtc.m_Hour24 == 24)
      rtc.m_Hour24 = 0;
  }        
  hour24 = rtc.m_Hour24;
  minute = rtc.m_Minute;  
  while (true)
  {
    if (redraw)
    {
      // redraw fields
      CLK::ShowTime((editHour && !blinkOn)?-1:hour24, (!editHour && !blinkOn)?-1:minute);
      redraw = false;
    }
    tButton btn = abtn.Pressed();
    if (btn == eButtonSet)
    {
      // next field
      if (editHour)
      {
        editHour = false;
        blinkOn = redraw = true;
        blinkTimeMS = idleTimeMS = millis();
      }
      else
      {
        // done
        Write = true;
        break;
      }
    }
    else if (btn == eButtonSel)
    {
      // field's next value
      if (editHour)
      {
        hour24++;
        if (hour24 >= 24)
          hour24 = 0;
      }
      else
      {
        minute++;
        if (minute >= 60)
          minute = 0;
      }
      redraw = true;
      idleTimeMS = millis();
    }
    else
    {
      unsigned long nowMS = millis();
      if ((nowMS - blinkTimeMS) > 500L) // half second blink
      {
        blinkTimeMS = nowMS;
        blinkOn = !blinkOn;
        redraw = true;
      }
      if ((nowMS - idleTimeMS) > 30000L)  // idle 30s, bail out
      {
        break;
      }
    }
  }
  
  if (Write)
  {
    rtc.ReadTime(true);
    if (CFG::_DLSOn)
    {
      hour24--;
      if (hour24 == 255)
        hour24 = 23;
    }          
    rtc.m_Hour24 = hour24;
    rtc.m_Minute = minute;
    rtc.m_Second = 0;
    rtc.WriteTime();    
  }
  CLK::Init();
}

const char PROGMEM pDimmingMStr[] = TEXT_MSTR("DIM") TEXT_MSTR("HI") TEXT_MSTR("LO") TEXT_MSTR("HI  OFF") TEXT_MSTR("LO  OFF");
void DisplayPWM(int idx, byte pwm)
{
  // display pwm value as OFF, 1..9
  if (pwm)
  {
    int val = 7;
    while (val && pwm != (1 << val))
      val--;
    val++;
    if (pwm == 255)
      val = 9;
    CLK::LoadNumber(5, val);
  }
  else
    CLK::LoadString(pDimmingMStr, idx);
}

void IncrementPWM(int& pwm)
{
  // increment wpm as a power of 2
  if (pwm == 128)
    pwm = 255;
  else if (pwm == 0)
    pwm = 1;
  else
    pwm = (pwm == 255)?0:pwm << 1;
}

void Dimming()
{
  enum Fields {eLDR, eHiPWM, eLoPWM};
  // edit the LDR/PWM fields
  LCD::Clear();
  bool save = true, blinkOn = true, update = true;
  int field = eLDR;
  int LDR = _LDRThreshold, HiPWM = _dayBrightness, LoPWM = _nightBrightness;
  unsigned long blinkTimeMS = millis();
  unsigned long idleTimeMS = blinkTimeMS;
  CLK::LoadString(pDimmingMStr, field);
  CLK::UpdateChars();
  while (abtn.Down() != eButtonNone)
   ;
  while (true)
  {
    if (update)
    {
      // draw fields
      CLK::LoadString(pDimmingMStr, field);
      byte brightness = 255;
      if (field == eHiPWM)
      {
        brightness = HiPWM;
        if (blinkOn)
          DisplayPWM(3, HiPWM);
      }
      else if (field == eLoPWM)
      {
        brightness = LoPWM;
        if (blinkOn)
          DisplayPWM(4, LoPWM);
      }
      else
      {
        // DIMXX
        if (blinkOn)
          CLK::LoadNumber(5, LDR/LDR_DIVISOR);
        CLK::LoadNumber(1, LDR::Raw()/LDR_DIVISOR);
      }
      if (brightness == 0)
        brightness = 255;
      LCD::SetBacklight(brightness);
      CLK::UpdateChars();

      update = false;
    }
    
    tButton btn = abtn.Pressed();
    if (btn == eButtonSet) // next field
    {
      idleTimeMS = blinkTimeMS = millis();
      update = blinkOn = true;
      if (field == eLoPWM)
      {
        save = true;
        break;
      }
      field++;
    }
    else if (btn == eButtonSel) // next value
    {
      switch (field)
      {
        case eLDR:
          LDR = (LDR >= 1000)?0:LDR + LDR_DIVISOR;
          break;
        case eHiPWM:
          IncrementPWM(HiPWM);
          break;
        case eLoPWM:
          IncrementPWM(LoPWM);
          break;
        default:
          break;
      }
      idleTimeMS = blinkTimeMS = millis();
      blinkOn = update = true;
    }
    else
    {
      unsigned long nowMS = millis();
      if ((nowMS - blinkTimeMS) > 500L) // half second blink
      {
        blinkTimeMS = millis();
        blinkOn = !blinkOn;
        update = true;
      }
      if ((nowMS - idleTimeMS) > 30000L)  // idle 30s, bail out
      {
        save = false;
        break;
      }
    }
  }

  if (save)
  {
    _LDRThreshold = LDR;
    _dayBrightness = HiPWM;
    _nightBrightness = LoPWM;
    Write();
  }
  LDR::Reset();
  LCD::SetBacklight(255);
}

}