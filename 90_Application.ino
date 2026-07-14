// ============================================================
// Application startup, redraw manager, and recovery service
// ============================================================

void redrawWorldClock() {
  if (!timeValid || !sdReady) return;
  const time_t now = time(nullptr);
  tm utcTm {};
  gmtime_r(&now, &utcTm);
  if (!drawMap(utcTm)) {
    sdReady = false;
    setSystemError(SystemError::SdUnavailable, "Map scanline read failed");
    return;
  }
  drawIssOrbitTrack();
  drawCelestialMarkers(now);
  drawIssMarker();
  drawClock();
}

void initializeWorldClock() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);

  digitalWrite(
    TFT_BL,
    ACTIVE_BOARD.backlightActiveHigh
      ? HIGH
      : LOW
  );
  lcd.init();
  lcd.setRotation(DISPLAY_ROTATION);
  lcd.setColorDepth(16);
  lcd.setSwapBytes(true);
  lcd.fillScreen(TFT_BLACK);

  showSplashScreen();
  Serial.printf(
    "Board profile: %s\n",
    ACTIVE_BOARD.name
  );

  Serial.printf("LCD dimensions: %d x %d\n", lcd.width(), lcd.height());

  if (!ensureNetworkConfigured()) return;
  startRuntimeConfigServer();
  systemStatus.wifiConnected = true;

  timeValid = synchronizeConfiguredTime();
  if (!timeValid) {
    // Do not permanently stop. The service loop will retry NTP.
    showStatus("NTP unavailable", "Will retry automatically");
  }

  sdReady = initializeSD();
  systemStatus.lastStorageAttempt = millis();
  if (!sdReady) {
    showStatus("Map unavailable", "Will retry automatically");
  }

  if (overlaySettings.showISS || overlaySettings.showIssTrack) {
    fetchIssPosition();
    if (overlaySettings.showIssTrack && issPosition.valid && !issTrackValid) calculateIssOrbitTrack();
  }

  if (timeValid && sdReady) redrawWorldClock();
  lastMapUpdate = millis();
  lastClockUpdate = millis();
  lastIssUpdate = millis();
  lastNtpRetry = millis();
}

void serviceWorldClock() {
  serviceConfigurationButton();
  serviceRuntimeConfigServer();
  const unsigned long nowMs = millis();

  systemStatus.wifiConnected = WiFi.status() == WL_CONNECTED;
  if (!systemStatus.wifiConnected && nowMs - lastWiFiRetry >= WIFI_RETRY_MS) {
    lastWiFiRetry = nowMs;
    WiFi.reconnect();
    setSystemError(SystemError::WifiDisconnected, "Wi-Fi reconnecting");
  }

  if (systemStatus.wifiConnected && systemStatus.lastError == SystemError::WifiDisconnected) {
    setSystemError(SystemError::None, "");
  }

  if (!timeValid || !systemStatus.ntpSynced) {
    if (systemStatus.wifiConnected && nowMs - lastNtpRetry >= NTP_RETRY_MS) {
      lastNtpRetry = nowMs;
      timeValid = retryNtpSync();
      if (timeValid && sdReady) redrawWorldClock();
    }
  }

  if (!sdReady && nowMs - systemStatus.lastStorageAttempt >= STORAGE_RETRY_MS) {
    systemStatus.lastStorageAttempt = nowMs;
    sdReady = initializeSD();
    if (sdReady && timeValid) redrawWorldClock();
  }

  if (!timeValid || !sdReady) {
    delay(20);
    return;
  }

  if (nowMs - lastClockUpdate >= CLOCK_UPDATE_MS) {
    drawClock();
    lastClockUpdate = nowMs;
  }

  bool needRedraw = false;
  if ((overlaySettings.showISS || overlaySettings.showIssTrack) &&
      systemStatus.wifiConnected && nowMs - lastIssUpdate >= ISS_UPDATE_MS) {
    lastIssUpdate = nowMs;
    if (fetchIssPosition()) needRedraw = true;
  }

  if (nowMs - lastMapUpdate >= MAP_UPDATE_MS) {
    lastMapUpdate = nowMs;
    needRedraw = true;
  }

  if (needRedraw) {
    redrawWorldClock();
    lastClockUpdate = nowMs;
  }

  delay(10);
}
