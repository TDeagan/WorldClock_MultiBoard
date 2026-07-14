#include "logo.h"

// ============================================================
// Embedded startup splash
// ============================================================

void showSplashScreen() {
  lcd.fillScreen(
    SPLASH_BACKGROUND_COLOR
  );

  lcd.pushImage(
    SPLASH_LOGO_X,
    SPLASH_LOGO_Y,
    DEAGAN_LOGO_WIDTH,
    DEAGAN_LOGO_HEIGHT,
    DEAGAN_LOGO_RGB565
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setFont(
    &fonts::Font2
  );

  lcd.setTextColor(
    SPLASH_TITLE_COLOR,
    SPLASH_BACKGROUND_COLOR
  );

  lcd.drawString(
    "World Clock",
    SCREEN_W / 2,
    SPLASH_TITLE_Y
  );

  lcd.setTextColor(
    SPLASH_VERSION_COLOR,
    SPLASH_BACKGROUND_COLOR
  );

  lcd.drawString(
    String("Firmware ") +
      FIRMWARE_VERSION,
    SCREEN_W / 2,
    SPLASH_VERSION_Y
  );

  // The board-profile name can be long, so use the smallest
  // built-in font for this line.
  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextColor(
    SPLASH_BOARD_COLOR,
    SPLASH_BACKGROUND_COLOR
  );

  lcd.drawString(
    ACTIVE_BOARD.name,
    SCREEN_W / 2,
    SPLASH_BOARD_Y
  );

  delay(
    SPLASH_DURATION_MS
  );
}
