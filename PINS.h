#pragma once

// pins -- Leonardo Tiny
#define PIN_LCD_WR         9
#define PIN_LCD_DATA      10
#define PIN_LCD_BACKLIGHT 11

#define PIN_LCD_CS        A0
#define PIN_BTN           A1
#define PIN_LDR           A2

// *** Schematic ***
//                                                                             SET <2>
//                                                                             ---
//        +-----------------+                                              +---   -------------------+
//  [SDA]-+(SDA)  <1>  (SCL)+-[SCL]               +-DS3221-+               |                         |
//        |                 |                     |  RTC   |               |            SEL/ADJ <2>  |
//   [CS]-+A0    LEO     ~D9+-[WR]         [SDA]--+        |               |  +------+    ---        |          +------+
//        |      TINY       |              [SCL]--+        |       [5VDC]--+--| 220R |----   --------+----------| 220R |---[GND]
//  [BTN]-+A1           ~D10+-[DATA]       [GND]--+        |                  +------+               |          +------+
//        |                 |              [5VDC]-+        |                                         +---[BTN]
//  [LDR]-+A2           ~D11+-[B/L]               |        |       
//        |                 |                     +--------+       
// [5VDC]-+"+"           "0"+-[GND]                                         +---------+       +------+
//        |                 |                     +-DM8BA10--+     [5VDC]---+ LDR <3> +---+---| 22kR |---[GND]
//        |                 |                     | Display  |              +---------+   |   +------+
//        +------|USB|------+              [CS]---+  10char  |                            |
//                                         [WR]---+  16seg   |                            +---[LDR]
//                                         [DATA]-+          |
//                                         [GND]--+          |
//                                         [5VDC]-+VDD       |
//                                         [B/L]--+LED+      |
//                                                +----------+
//
//  Like [labels] are connected.
//  <1> Leonardo ETH Tiny, https://www.jaycar.co.nz/duinotech-leonardo-tiny-atmega32u4-development-board/p/XC4431
//  <2> Two push-buttons (N/O), https://www.jaycar.co.nz/spst-pcb-tactile-switch/p/SP0611
//      BTN pin reads ~0V with no button pressed, ~5V with SET pressed, ~2.5V with SEL/ADJ pressed
//  <3> LDR is ~200R bright, ~20MR dark 
