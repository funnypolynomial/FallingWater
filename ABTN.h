#pragma once

// 2 debounced buttons from a single analog pin
enum tButton {eButtonNone, eButtonSet, eButtonSel};
class ABTN
{
  public:
    void Init(int Pin, int ThresholdSet, int ThresholdSel);
    tButton Pressed();
    tButton Down();
    
    void WaitFor(tButton btn);
    
  private:
    int m_iPin;
    int m_iThresholdSet;
    int m_iThresholdSel;
    int m_iPrevReading;
    int m_iPrevState;
    unsigned long m_iTransitionTimeMS;
};

extern ABTN abtn;

