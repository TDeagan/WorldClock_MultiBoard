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
  if (touchUiIsOpen()) {
    return;
  }

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
  String("http://") +
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



bool parseCoordinateValue(
  const String &text,
  double minimum,
  double maximum,
  double &valueOut
) {
  String cleaned = text;
  cleaned.trim();

  if (cleaned.length() == 0) {
    return false;
  }

  char *endPointer = nullptr;

  const double value =
    strtod(
      cleaned.c_str(),
      &endPointer
    );

  if (
    endPointer == cleaned.c_str() ||
    *endPointer != '\0' ||
    !isfinite(value) ||
    value < minimum ||
    value > maximum
  ) {
    return false;
  }

  valueOut =
    fabs(value) < 0.0000005
      ? 0.0
      : value;

  return true;
}


String formatCoordinateInput(
  double value
) {
  if (fabs(value) < 0.00005) {
    value = 0.0;
  }

  String text(value, 4);

  while (
    text.indexOf('.') >= 0 &&
    text.endsWith("0")
  ) {
    text.remove(
      text.length() - 1
    );
  }

  if (text.endsWith(".")) {
    text.remove(
      text.length() - 1
    );
  }

  return text;
}


String formatLatitude(
  double latitude
) {
  const char hemisphere =
    latitude < 0.0
      ? 'S'
      : 'N';

  return
    formatCoordinateInput(
      fabs(latitude)
    ) +
    " deg " +
    hemisphere;
}


String formatLongitude(
  double longitude
) {
  const char hemisphere =
    longitude < 0.0
      ? 'W'
      : 'E';

  return
    formatCoordinateInput(
      fabs(longitude)
    ) +
    " deg " +
    hemisphere;
}


String formatHomeLocation() {
  return
    formatLatitude(
      locationGridSettings.homeLatitude
    ) +
    ", " +
    formatLongitude(
      locationGridSettings.homeLongitude
    );
}


uint8_t effectiveDisplayRotation() {
  const uint8_t nativeRotation =
    static_cast<uint8_t>(DISPLAY_ROTATION);

  if (!displaySettings.flip180) {
    return nativeRotation;
  }

  // LovyanGFX rotations 0-3 are normal and 4-7 are mirrored.
  // Keep the original group while rotating it by 180 degrees.
  if (nativeRotation < 4) {
    return static_cast<uint8_t>(
      (nativeRotation + 2) % 4
    );
  }

  return static_cast<uint8_t>(
    4 + ((nativeRotation - 4 + 2) % 4)
  );
}


void applyDisplayRotation() {
  lcd.setRotation(
    effectiveDisplayRotation()
  );

  // Safe during startup: the touch UI ignores this callback until its
  // hardware layer has been initialized.
  touchUiHandleDisplayRotationChanged();
}


const char *displayOrientationName() {
  return displaySettings.flip180
    ? "Flipped 180 degrees"
    : "Normal";
}


String formatUptime(unsigned long uptimeMs) {
  unsigned long seconds = uptimeMs / 1000UL;
  const unsigned long days = seconds / 86400UL;
  seconds %= 86400UL;
  const unsigned long hours = seconds / 3600UL;
  seconds %= 3600UL;
  const unsigned long minutes = seconds / 60UL;
  const unsigned long remainingSeconds = seconds % 60UL;

  char buffer[40];

  if (days > 0) {
    snprintf(
      buffer,
      sizeof(buffer),
      "%lu day%s %02lu:%02lu:%02lu",
      days,
      days == 1 ? "" : "s",
      hours,
      minutes,
      remainingSeconds
    );
  } else {
    snprintf(
      buffer,
      sizeof(buffer),
      "%02lu:%02lu:%02lu",
      hours,
      minutes,
      remainingSeconds
    );
  }

  return String(buffer);
}


const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "Power-on reset";
    case ESP_RST_EXT: return "External reset";
    case ESP_RST_SW: return "Software restart";
    case ESP_RST_PANIC: return "Exception or panic";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep-sleep wake";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO: return "SDIO reset";
    case ESP_RST_UNKNOWN:
    default: return "Unknown";
  }
}


String formatAge(time_t stamp) {
  if (stamp <= 0) {
    return "Never";
  }

  long age =
    static_cast<long>(
      time(nullptr) - stamp
    );

  if (age < 0) {
    age = 0;
  }

  if (age < 60) {
    return String(age) + " sec ago";
  }

  if (age < 3600) {
    return String(age / 60) + " min ago";
  }

  if (age < 86400) {
    return String(age / 3600) +
      " hr " +
      String((age % 3600) / 60) +
      " min ago";
  }

  return String(age / 86400) +
    " day" +
    ((age / 86400) == 1 ? "" : "s") +
    " ago";
}


String formatElapsedAge(
  unsigned long stampMs
) {
  if (stampMs == 0) {
    return "Never";
  }

  unsigned long ageSeconds =
    (millis() - stampMs) / 1000UL;

  if (ageSeconds < 60UL) {
    return String(ageSeconds) +
      " sec ago";
  }

  if (ageSeconds < 3600UL) {
    return String(ageSeconds / 60UL) +
      " min ago";
  }

  if (ageSeconds < 86400UL) {
    return String(ageSeconds / 3600UL) +
      " hr " +
      String((ageSeconds % 3600UL) / 60UL) +
      " min ago";
  }

  const unsigned long days =
    ageSeconds / 86400UL;

  return String(days) +
    " day" +
    (days == 1 ? "" : "s") +
    " ago";
}



bool timeZonePresetIsValid(uint8_t value) {
  return value <=
    static_cast<uint8_t>(
      TimeZonePreset::USPacific
    );
}


const char *timeZonePresetName(
  TimeZonePreset preset
) {
  switch (preset) {
    case TimeZonePreset::FixedOffset:
      return "Fixed UTC offset";

    case TimeZonePreset::UTC:
      return "UTC";

    case TimeZonePreset::USEastern:
      return "US Eastern";

    case TimeZonePreset::USCentral:
      return "US Central";

    case TimeZonePreset::USMountain:
      return "US Mountain";

    case TimeZonePreset::USPacific:
      return "US Pacific";

    default:
      return "Fixed UTC offset";
  }
}


String formatUtcOffsetMinutes(
  int offsetMinutes
) {
  const char sign =
    offsetMinutes < 0 ? '-' : '+';

  const int absoluteMinutes =
    abs(offsetMinutes);

  char buffer[16];

  snprintf(
    buffer,
    sizeof(buffer),
    "UTC%c%02d:%02d",
    sign,
    absoluteMinutes / 60,
    absoluteMinutes % 60
  );

  return String(buffer);
}


String timeZoneDisplayName() {
  if (
    timeSettings.timeZone ==
      TimeZonePreset::FixedOffset
  ) {
    return String("Fixed ") +
      formatUtcOffsetMinutes(
        networkSettings.utcOffsetMinutes
      );
  }

  return String(
    timeZonePresetName(
      timeSettings.timeZone
    )
  );
}


const char *clockFormatName() {
  return timeSettings.use24Hour
    ? "24-hour"
    : "12-hour";
}


time_t clockUpdateIntervalSeconds() {
  return timeSettings.showSeconds
    ? 1
    : CLOCK_UPDATE_SECONDS;
}

