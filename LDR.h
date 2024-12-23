#pragma once

namespace LDR
{
  void Init(int pin);
  void Reset();
  bool IsBright();
  int Raw();
}
