#pragma once

// Clock logic

namespace CLK
{
  void Init();
  void Splash();
  tButton Loop();
  void NewMode();
  void FlashDLS(byte DLS);
  void FlashReset();
  void ShowTime(int Hour24, int Minute);
  void AllFallDown();

  // (These Used by CFG too)
  // Multi-strings are one or more strings concatenated into a single string, in PROGMEM.
  // Strings are separated by a NUL and ends with two NUL's
  #define TEXT_MSTR(s) s "\0"  
  void UpdateChars();
  int LoadString(const char* pMStr, int strN);
  void LoadNumber(int atIdx, byte num); // two digits
}

