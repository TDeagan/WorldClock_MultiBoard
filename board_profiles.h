#pragma once

#include <Arduino.h>

// ============================================================
// BOARD PROFILE DATA
// ============================================================
//
// The numeric board identifiers are declared in config.h before this file is
// included. Display and SD values are inherited from the working WorldClock
// 4.7.1 profiles. Touch wiring has been physically verified on the Heltec
// WROOM and E32R28T boards. The AITRIP dual-USB CYD profile uses the same
// verified XPT2046 wiring and has been physically validated.

struct BoardProfile {
  const char *name;

  // TFT SPI and control
  int tftSclk;
  int tftMosi;
  int tftMiso;
  int tftCs;
  int tftDc;
  int tftReset;
  int tftBacklight;
  bool backlightActiveHigh;

  // TFT panel configuration
  int memoryWidth;
  int memoryHeight;
  int panelWidth;
  int panelHeight;
  int displayRotation;
  bool rgbOrder;
  bool invertDisplay;
  int panelOffsetRotation;
  int dummyReadPixelBits;
  int dummyReadCommandBits;

  // microSD SPI
  int sdSclk;
  int sdMiso;
  int sdMosi;
  int sdCs;

  // XPT2046 touch controller. Touch uses software SPI so it does not
  // reconfigure either hardware SPI peripheral used by the TFT and SD card.
  bool touchAvailable;
  int touchSclk;
  int touchMosi;
  int touchMiso;
  int touchCs;
  int touchIrq;

  // Finger-oriented acquisition and gesture defaults.
  uint16_t touchPressureMinimum;
  uint8_t touchMedianSamples;
  uint8_t touchStableFrames;
  uint16_t touchTapMoveTolerance;
  uint16_t touchDropoutGraceMs;

  // Runtime setup/reset button
  int configurationButtonPin;

  // Informational only. Set the actual value in Arduino IDE.
  int nominalFlashMegabytes;

  // Display tuning defaults. Brightness uses LovyanGFX's 0-255 scale.
  // Gamma is stored in hundredths: 100 = unchanged, >100 darkens
  // midtones, and <100 lifts shadow detail.
  uint8_t defaultBacklightBrightness;
  uint16_t defaultDayMapGamma;
  uint16_t defaultNightMapGamma;
};

// Verified common XPT2046 wiring:
// CLK=25, DIN/MOSI=32, DO/MISO=39, CS=33, IRQ=36.

static constexpr BoardProfile PROFILE_HELTEC_WROOM_28 = {
  "Heltec ESP32-WROOM-32 2.8",
  14, 13, 12, 15, 2, -1, 21, true,
  240, 320, 240, 320, 3, false, false, 0, 8, 1,
  18, 19, 23, 5,
  true, 25, 32, 39, 33, 36,
  80, 9, 2, 22, 100,
  0,
  8,
  245, 100, 100
};

static constexpr BoardProfile PROFILE_E32R28T = {
  "E32R28T ESP32-32E",
  14, 13, 12, 15, 2, -1, 21, true,
  320, 240, 320, 240, 4, true, false, 0, 8, 1,
  18, 19, 23, 5,
  true, 25, 32, 39, 33, 36,
  60, 9, 2, 22, 120,
  0,
  4,
  230, 120, 100
};

// AITRIP ESP32-2432S028R dual-USB CYD. The USB-C + micro-USB revision
// uses an ST7789-class 240 x 320 controller. TFT, touch, and SD wiring are
// the standard CYD pin assignments. Rotation 1 produces the 320 x 240
// landscape layout used by World Clock.
static constexpr BoardProfile PROFILE_AITRIP_ESP32_2432S028R = {
  "AITRIP ESP32-2432S028R CYD 2USB",
  14, 13, 12, 15, 2, -1, 21, true,
  240, 320, 240, 320, 1, false, false, 0, 16, 1,
  18, 19, 23, 5,
  true, 25, 32, 39, 33, 36,
  80, 9, 2, 22, 100,
  0,
  4,
  255, 100, 82
};

// ============================================================
// ACTIVE PROFILE
// ============================================================

#if WORLDCLOCK_BOARD == BOARD_HELTEC_WROOM_28
  static constexpr BoardProfile ACTIVE_BOARD =
    PROFILE_HELTEC_WROOM_28;
#elif WORLDCLOCK_BOARD == BOARD_E32R28T
  static constexpr BoardProfile ACTIVE_BOARD =
    PROFILE_E32R28T;
#elif WORLDCLOCK_BOARD == BOARD_AITRIP_ESP32_2432S028R
  static constexpr BoardProfile ACTIVE_BOARD =
    PROFILE_AITRIP_ESP32_2432S028R;
#else
  #error "Unsupported WORLDCLOCK_BOARD selection"
#endif
