// ============================================================
// Clock display and configured network/time operation
// ============================================================

namespace {

void removeLeadingZero(char *text) {
  if (
    text != nullptr &&
    text[0] == '0'
  ) {
    memmove(
      text,
      text + 1,
      strlen(text)
    );
  }
}


void startConfiguredNtp() {
  const String posixRule =
    configuredPosixTimeZone();

  configTzTime(
    posixRule.c_str(),
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
  );
}

} // namespace


String configuredPosixTimeZone() {
  switch (timeSettings.timeZone) {
    case TimeZonePreset::UTC:
      return "UTC0";

    case TimeZonePreset::USEastern:
      return "EST5EDT,M3.2.0/2,M11.1.0/2";

    case TimeZonePreset::USCentral:
      return "CST6CDT,M3.2.0/2,M11.1.0/2";

    case TimeZonePreset::USMountain:
      return "MST7MDT,M3.2.0/2,M11.1.0/2";

    case TimeZonePreset::USPacific:
      return "PST8PDT,M3.2.0/2,M11.1.0/2";

    case TimeZonePreset::FixedOffset:
    default:
      break;
  }

  const int offsetMinutes =
    networkSettings.utcOffsetMinutes;

  if (offsetMinutes == 0) {
    return "UTC0";
  }

  // POSIX TZ signs are opposite conventional UTC-offset notation.
  const char posixSign =
    offsetMinutes > 0 ? '-' : '+';

  const int absoluteMinutes =
    abs(offsetMinutes);

  char rule[24];

  snprintf(
    rule,
    sizeof(rule),
    "UTC%c%d:%02d",
    posixSign,
    absoluteMinutes / 60,
    absoluteMinutes % 60
  );

  return String(rule);
}


void applyConfiguredTimeZone() {
  const String posixRule =
    configuredPosixTimeZone();

  setenv(
    "TZ",
    posixRule.c_str(),
    1
  );

  tzset();
}


void drawClock() {
  if (!timeValid) {
    return;
  }

  const time_t now =
    time(nullptr);

  tm localTm {};
  tm utcTm {};

  localtime_r(
    &now,
    &localTm
  );

  gmtime_r(
    &now,
    &utcTm
  );

  char localTime[24];
  char utcTime[24];
  char dateLine[32];

  const char *localFormat =
    timeSettings.use24Hour
      ? (
          timeSettings.showSeconds
            ? "%H:%M:%S"
            : "%H:%M"
        )
      : (
          timeSettings.showSeconds
            ? "%I:%M:%S %p"
            : "%I:%M %p"
        );

  const char *utcFormat =
    timeSettings.showSeconds
      ? "UTC %H:%M:%S"
      : "UTC %H:%M";

  strftime(
    localTime,
    sizeof(localTime),
    localFormat,
    &localTm
  );

  if (!timeSettings.use24Hour) {
    removeLeadingZero(localTime);
  }

  strftime(
    utcTime,
    sizeof(utcTime),
    utcFormat,
    &utcTm
  );

  strftime(
    dateLine,
    sizeof(dateLine),
    "%a %b %d, %Y",
    &localTm
  );

  lcd.fillRect(
    0,
    STATUS_BAR_Y,
    SCREEN_W,
    STATUS_BAR_HEIGHT,
    STATUS_BACKGROUND_COLOR
  );

  const String line =
    String(localTime) +
    "  " +
    utcTime +
    "  " +
    dateLine;

  lcd.setFont(
    &fonts::Font2
  );

  // Long 12-hour strings or seconds may exceed 320 pixels. Keep all
  // information by falling back to the smallest built-in font.
  if (
    lcd.textWidth(line) >
      SCREEN_W - 4
  ) {
    lcd.setFont(
      &fonts::Font0
    );
  }

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    STATUS_TEXT_COLOR,
    STATUS_BACKGROUND_COLOR
  );

  lcd.drawString(
    line,
    SCREEN_W / 2,
    CLOCK_TEXT_Y
  );

  drawIpAddress();
}


bool connectConfiguredWiFi() {
  if (
    !networkSettings.configured ||
    networkSettings.ssid.length() == 0
  ) {
    return false;
  }

  showStatus(
    "Connecting to Wi-Fi",
    networkSettings.ssid
  );

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.disconnect(false, false);
  delay(100);

  WiFi.begin(
    networkSettings.ssid.c_str(),
    networkSettings.password.c_str()
  );

  const unsigned long started =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - started <
      WIFI_CONNECT_TIMEOUT_MS
  ) {
    delay(250);
  }

  systemStatus.wifiConnected =
    WiFi.status() == WL_CONNECTED;

  if (!systemStatus.wifiConnected) {
    setSystemError(
      SystemError::WifiDisconnected,
      "Could not connect to saved Wi-Fi"
    );
  }

  return systemStatus.wifiConnected;
}


bool synchronizeConfiguredTime() {
  showStatus(
    "Synchronizing time",
    timeZoneDisplayName()
  );

  startConfiguredNtp();

  const unsigned long started =
    millis();

  while (
    time(nullptr) < 1700000000 &&
    millis() - started <
      NTP_SYNC_TIMEOUT_MS
  ) {
    delay(200);
  }

  systemStatus.ntpSynced =
    time(nullptr) >= 1700000000;

  if (systemStatus.ntpSynced) {
    systemStatus.lastNtpSync =
      time(nullptr);
  } else {
    setSystemError(
      SystemError::NtpUnavailable,
      "Initial NTP synchronization failed"
    );
  }

  return systemStatus.ntpSynced;
}


bool retryNtpSync() {
  startConfiguredNtp();

  const unsigned long started =
    millis();

  while (
    time(nullptr) < 1700000000 &&
    millis() - started < 5000UL
  ) {
    delay(100);
  }

  systemStatus.ntpSynced =
    time(nullptr) >= 1700000000;

  if (systemStatus.ntpSynced) {
    systemStatus.lastNtpSync =
      time(nullptr);

    if (
      systemStatus.lastError ==
        SystemError::NtpUnavailable
    ) {
      setSystemError(
        SystemError::None,
        ""
      );
    }
  } else {
    setSystemError(
      SystemError::NtpUnavailable,
      "Background NTP retry failed"
    );
  }

  return systemStatus.ntpSynced;
}
