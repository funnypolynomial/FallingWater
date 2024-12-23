#include <Arduino.h>
#include "CFG.h"
#include "LDR.h"

namespace LDR {
// Treats the LDR as two-state -- Bright or Dim.  
// Logic is like a debounced button, rather than a rolling average.

int _pin = 0;
bool _PrevReading;
bool _PrevState;
bool _CurrentState;
unsigned long _TransitionTimeMS;
#define READING_COUNTER_START 200
byte readingCounter = 0;

bool Read()
{
  // Return true if the LDR/Ambient is "bright" right now
  if (!_pin)
    return false;
  int reading = analogRead(_pin);
  return reading > CFG::_LDRThreshold;  
}

void Init(int pin)
{
  // Init the readings
  _pin = pin;
  Reset();
}

void Reset()
{
  // Reset the "debounce"
  readingCounter = 0;
  _PrevReading = _PrevState = _CurrentState = Read();
  _TransitionTimeMS = millis();  
}

bool IsBright()
{
  // Return true if the LDR/Ambient is consistently "bright"
  if (!readingCounter) // only do an analog read occasionally
  {
    readingCounter = READING_COUNTER_START;
    bool ThisReading = Read();
    if (ThisReading != _PrevReading)
    {
      // state change, reset the timer
      _PrevReading = ThisReading;
      _TransitionTimeMS = millis();
    }
    else if (ThisReading != _PrevState &&
             (millis() - _TransitionTimeMS) >= CFG_LDR_HOLD_TIME_MS)
    {
      // a state other than the last one and held for long enough
      _CurrentState = _PrevState = ThisReading;
    }
  }
  readingCounter++;
  return _CurrentState;
}

int Raw()
{
  // return raw reading
  return analogRead(_pin);
}

}
