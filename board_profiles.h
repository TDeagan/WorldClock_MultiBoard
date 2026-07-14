#pragma once

// ============================================================
// BOARD PROFILE DATA
// ============================================================
//
// The numeric board identifiers are declared in config.h before this
// file is included. Add future boards here without changing the
// application tabs.
//

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

  // ILI9341 panel configuration
  int memoryWidth;
  int memoryHeight;
  int panelWidth;
  int panelHeight;
  int displayRotation;
  bool rgbOrder;
  bool invertDisplay;

  // microSD SPI
  int sdSclk;
  int sdMiso;
  int sdMosi;
  int sdCs;

  // Runtime setup/reset button
  int configurationButtonPin;

  // Informational only. Set the actual value in Arduino IDE.
  int nominalFlashMegabytes;
};

// ------------------------------------------------------------
// Heltec-labeled ESP32-WROOM-32 2.8-inch display
//
// These values exactly reproduce the uploaded, working v3.4 build.
// ------------------------------------------------------------

static constexpr BoardProfile PROFILE_HELTEC_WROOM_28 = {
  "Heltec ESP32-WROOM-32 2.8",
  14, 13, 12, 15, 2, -1, 21, true,
  240, 320, 240, 320, 3, false, false,
  18, 19, 23, 5,
  0,
  8
};

// ------------------------------------------------------------
// Original E32R28T
//
// This preserves the unusual geometry and rotation that were proven
// on the original board.
// ------------------------------------------------------------

static constexpr BoardProfile PROFILE_E32R28T = {
  "E32R28T ESP32-32E",
  14, 13, 12, 15, 2, -1, 21, true,
  320, 240, 320, 240, 4, true, false,
  18, 19, 23, 5,
  0,
  4
};

// ------------------------------------------------------------
// Elegoo EL-EB-009 / ESP32-2432S028R CYD
//
// Standard CYD display/SD connections and normal ILI9341 geometry.
// If a particular production revision is upside down, change the
// rotation here from 1 to 3. If only red and blue are exchanged,
// change rgbOrder from false to true.
// ------------------------------------------------------------

static constexpr BoardProfile PROFILE_ELEGOO_EL_EB_009 = {
  "Elegoo EL-EB-009 CYD",
  14, 13, 12, 15, 2, -1, 21, true,
  240, 320, 240, 320, 1, false, false,
  18, 19, 23, 5,
  0,
  4
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
#elif WORLDCLOCK_BOARD == BOARD_ELEGOO_EL_EB_009
  static constexpr BoardProfile ACTIVE_BOARD =
    PROFILE_ELEGOO_EL_EB_009;
#else
  #error "Unsupported WORLDCLOCK_BOARD selection"
#endif
