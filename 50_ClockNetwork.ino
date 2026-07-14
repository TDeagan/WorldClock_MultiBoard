// ============================================================
// Clock display and configured network/time operation
// ============================================================

void drawClock() {
  if (!timeValid) {
    return;
  }

  const time_t now = time(nullptr);

  tm localTm {};
  tm utcTm {};

  localtime_r(&now, &localTm);
  gmtime_r(&now, &utcTm);

  char localTime[16];
  char utcTime[16];
  char dateLine[32];

  strftime(
    localTime,
    sizeof(localTime),
    "%H:%M",
    &localTm
  );

  strftime(
    utcTime,
    sizeof(utcTime),
    "UTC-%H:%M",
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

  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(STATUS_TEXT_COLOR, STATUS_BACKGROUND_COLOR);

  // Local time intentionally has no location/time-zone label.
  const String line =
    String(localTime) +
    "  " +
    utcTime +
    "  " +
    dateLine;

  lcd.drawString(
    line,
    SCREEN_W / 2,
    CLOCK_TEXT_Y
  );

  // Redraw the address after clearing the status bar.
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

  const unsigned long started = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - started < WIFI_CONNECT_TIMEOUT_MS
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
    "Using saved UTC offset"
  );

  const long offsetSeconds =
    static_cast<long>(
      networkSettings.utcOffsetMinutes
    ) * 60L;

  // A fixed UTC offset was requested, so daylight-saving time is zero.
  configTime(
    offsetSeconds,
    0,
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
  );

  const unsigned long started = millis();

  while (
    time(nullptr) < 1700000000 &&
    millis() - started < NTP_SYNC_TIMEOUT_MS
  ) {
    delay(200);
  }

  systemStatus.ntpSynced =
    time(nullptr) >= 1700000000;

  if (systemStatus.ntpSynced) {
    systemStatus.lastNtpSync = time(nullptr);
  } else {
    setSystemError(
      SystemError::NtpUnavailable,
      "Initial NTP synchronization failed"
    );
  }

  return systemStatus.ntpSynced;
}


bool retryNtpSync() {
  const long offsetSeconds =
    static_cast<long>(
      networkSettings.utcOffsetMinutes
    ) * 60L;

  configTime(
    offsetSeconds,
    0,
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
  );

  const unsigned long started = millis();

  while (
    time(nullptr) < 1700000000 &&
    millis() - started < 5000UL
  ) {
    delay(100);
  }

  systemStatus.ntpSynced =
    time(nullptr) >= 1700000000;

  if (systemStatus.ntpSynced) {
    systemStatus.lastNtpSync = time(nullptr);
    if (
      systemStatus.lastError ==
      SystemError::NtpUnavailable
    ) {
      setSystemError(SystemError::None, "");
    }
  } else {
    setSystemError(
      SystemError::NtpUnavailable,
      "Background NTP retry failed"
    );
  }

  return systemStatus.ntpSynced;
}
