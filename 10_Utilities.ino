static double norm180(double x) {
  while (x > 180.0) x -= 360.0;
  while (x < -180.0) x += 360.0;
  return x;
}


void centerText(
  const String &text,
  int y,
  uint16_t color
) {
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(color, TFT_BLACK);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.drawString(text, lcd.width() / 2, y);
}


void showStatus(
  const String &line1,
  const String &line2
) {
  lcd.fillScreen(TFT_BLACK);
  centerText(line1, 105, TFT_WHITE);

  if (line2.length()) {
    centerText(line2, 130, TFT_CYAN);
  }
}


void drawIpAddress() {
  if (
    WiFi.status() != WL_CONNECTED
  ) {
    return;
  }

  const String ipText =
    WiFi.localIP().toString();

  // Font0 is the smallest built-in LovyanGFX font.
  // A 15-character IPv4 address fits comfortably in this area.
  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(
    textdatum_t::top_center
  );

  // Place the address at the bottom center, inside the status area,
  // so normal map redraws no longer flash a black patch at the top.
  lcd.fillRect(
    IP_BACKGROUND_X,
    IP_BACKGROUND_Y,
    IP_BACKGROUND_W,
    IP_BACKGROUND_H,
    STATUS_BACKGROUND_COLOR
  );

  lcd.setTextColor(
    IP_TEXT_COLOR,
    STATUS_BACKGROUND_COLOR
  );

  lcd.drawString(
    ipText,
    SCREEN_W / 2,
    IP_TEXT_Y
  );
}


void setSystemError(
  SystemError error,
  const String &text
) {
  systemStatus.lastError = error;
  systemStatus.lastErrorText = text;
}

const char *systemErrorName(
  SystemError error
) {
  switch (error) {
    case SystemError::None: return "None";
    case SystemError::WifiDisconnected: return "Wi-Fi disconnected";
    case SystemError::NtpUnavailable: return "NTP unavailable";
    case SystemError::SdUnavailable: return "microSD unavailable";
    case SystemError::DayPngMissing: return "Day PNG missing";
    case SystemError::NightPngMissing: return "Night PNG missing";
    case SystemError::DayCacheInvalid: return "Day cache invalid";
    case SystemError::NightCacheInvalid: return "Night cache invalid";
    case SystemError::IssUnavailable: return "ISS service unavailable";
    default: return "Unknown";
  }
}
