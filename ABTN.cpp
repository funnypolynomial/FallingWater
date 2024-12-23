#include <Arduino.h>
#include "PINS.h"
#include "ABTN.h"

/*
             _ PBA
+5VDC--+----- --------+
       |        _ PBB |
       +--[R1]-- -----+--[Input Pin]
                      |
  GND-----[R2]--------+

none = ~LOW
PBA  = ~HIGH
PBB  = ~1024*R2/(R1+R2)
R1=120R, R2=220R
*/

#define HOLD_TIME_MS 50

void ABTN::Init(int Pin, int ThresholdSet, int ThresholdSel)
{
  // pin and ADC thresholds, Set if > ThresholdSet, else Sel if >= ThresholdSel.  ThresholdSet > ThresholdSel
  m_iPin = Pin;
  pinMode(m_iPin, INPUT);
  m_iThresholdSet = ThresholdSet;
  m_iThresholdSel = ThresholdSel;
  m_iPrevReading = -1;
  m_iPrevState = -1;
  m_iTransitionTimeMS = millis();
}

bool CheckTimer(unsigned long& Timer, unsigned long PeriodMS)
{
  // has the timer interval gone off?
  unsigned long Now = millis();
  if (Now - Timer > PeriodMS)
  {
    Timer = Now;
    return true;
  }
  return false;
}

tButton ABTN::Pressed()
{
  // debounced buttons, transitions only
  int ThisReading = Down();

  if (ThisReading != m_iPrevReading)
  {
    // state change, reset the timer
    m_iPrevReading = ThisReading;
    m_iTransitionTimeMS = millis();
  }
  else if (ThisReading != m_iPrevState &&
           CheckTimer(m_iTransitionTimeMS, HOLD_TIME_MS))
  {
    // a state other than the last one and held for long enough
    m_iPrevState = ThisReading;
    return (tButton)ThisReading;
  }
  return eButtonNone;
}

tButton ABTN::Down()
{
  // just the current status
  int adc = analogRead(m_iPin);
  if (adc >= m_iThresholdSet)
  {
    return eButtonSet;
  }
  else if (adc >= m_iThresholdSel)
  {
    return eButtonSel;
  }
  return eButtonNone;
}

void ABTN::WaitFor(tButton btn)
{
  while (Pressed() != btn)
    ;
}

ABTN abtn;
