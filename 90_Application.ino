// ============================================================
// Application startup, unified scheduler, redraw manager,
// and recovery service
// ============================================================

namespace {

// Epoch buckets align every clock to recognizable wall-clock boundaries.
// Epoch time is used rather than millis(), so updates remain aligned after
// restart and across multiple clocks.
time_t lastClockBucket = -1;
time_t lastAstronomyBucket = -1;

time_t schedulerBucket(
  time_t epoch,
  time_t intervalSeconds
) {
  if (
    epoch <= 0 ||
    intervalSeconds <= 0
  ) {
    return -1;
  }

  return epoch / intervalSeconds;
}

void markCurrentSchedulerBuckets() {
  if (!timeValid) {
    return;
  }

  const time_t now =
    time(nullptr);

  lastClockBucket =
    schedulerBucket(
      now,
      clockUpdateIntervalSeconds()
    );

  lastAstronomyBucket =
    schedulerBucket(
      now,
      ASTRONOMY_UPDATE_SECONDS
    );
}

} // namespace

void updateAstronomy() {
  // Sun, Moon, and the day/night terminator are calculated from the
  // current epoch during redrawWorldClock(). The only external data
  // needed here is the current ISS position.

  if (
    !overlaySettings.showISS &&
    !overlaySettings.showIssTrack
  ) {
    return;
  }

  if (
    WiFi.status() != WL_CONNECTED
  ) {
    return;
  }

  lastIssUpdate = millis();

  if (fetchIssPosition()) {
    // fetchIssPosition() normally rebuilds the track when it is enabled.
    // This fallback ensures a valid position always has a matching track.
    if (
      overlaySettings.showIssTrack &&
      issPosition.valid &&
      !issTrackValid
    ) {
      calculateIssOrbitTrack();
    }
  }
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

  // Load orientation before drawing the startup splash.
  loadDisplaySettings();

  lcd.init();
  applyBacklightBrightness();
  applyDisplayRotation();
  lcd.setColorDepth(16);
  lcd.setSwapBytes(true);
  lcd.fillScreen(TFT_BLACK);

  showSplashScreen();

  // Reuse compatible standalone touch-test calibration when present. On a
  // first boot without calibration, collect raw XPT2046 samples here before
  // continuing automatically into normal Wi-Fi setup.
  initializeTouchUi();

  if (
    ACTIVE_BOARD.touchAvailable &&
    touchHardwareIsReady() &&
    !touchCalibrationIsReady() &&
    !touchCalibrationWasBypassed()
  ) {
    runIntegratedTouchCalibration(true);
  }

  Serial.printf(
    "Board profile: %s\n",
    ACTIVE_BOARD.name
  );

  Serial.printf(
    "LCD dimensions: %d x %d\n",
    lcd.width(),
    lcd.height()
  );

  Serial.printf(
    "Display orientation: %s; rotation %u\n",
    displayOrientationName(),
    effectiveDisplayRotation()
  );

  if (!ensureNetworkConfigured()) {
    return;
  }

  Serial.printf(
    "Time zone: %s; clock: %s; seconds: %s\n",
    timeZoneDisplayName().c_str(),
    clockFormatName(),
    timeSettings.showSeconds ? "shown" : "hidden"
  );

  Serial.printf(
    "Home marker: %s at %s; coordinate grid: %s\n",
    locationGridSettings.showHomeMarker
      ? "shown"
      : "hidden",
    formatHomeLocation().c_str(),
    locationGridSettings.showCoordinateGrid
      ? "shown"
      : "hidden"
  );

  startRuntimeConfigServer();
  systemStatus.wifiConnected = true;

  timeValid =
    synchronizeConfiguredTime();

  if (!timeValid) {
    // Do not permanently stop. The service loop will retry NTP.
    showStatus(
      "NTP unavailable",
      "Will retry automatically"
    );
  }

  sdReady = initializeSD();
  systemStatus.lastStorageAttempt =
    millis();

  if (sdReady) {
    initializeWeatherService();
  }

  if (!sdReady) {
    showStatus(
      "Map unavailable",
      "Will retry automatically"
    );
  }

  if (timeValid) {
    updateAstronomy();
  }

  if (
    timeValid &&
    sdReady
  ) {
    redrawWorldClock();
  }

  markCurrentSchedulerBuckets();

  lastMapUpdate = millis();
  lastClockUpdate = millis();
  lastNtpRetry = millis();
}

void serviceWorldClock() {
  serviceConfigurationButton();
  serviceRuntimeConfigServer();
  serviceTouchUi();

  const unsigned long nowMs =
    millis();

  systemStatus.wifiConnected =
    WiFi.status() == WL_CONNECTED;

  if (
    !systemStatus.wifiConnected &&
    nowMs - lastWiFiRetry >=
      WIFI_RETRY_MS
  ) {
    lastWiFiRetry = nowMs;
    WiFi.reconnect();

    setSystemError(
      SystemError::WifiDisconnected,
      "Wi-Fi reconnecting"
    );
  }

  if (
    systemStatus.wifiConnected &&
    systemStatus.lastError ==
      SystemError::WifiDisconnected
  ) {
    setSystemError(
      SystemError::None,
      ""
    );
  }

  // Recovery retries are intentionally not tied to the five-minute
  // display schedule. A recovered clock should resume promptly.
  if (
    !timeValid ||
    !systemStatus.ntpSynced
  ) {
    if (
      systemStatus.wifiConnected &&
      nowMs - lastNtpRetry >=
        NTP_RETRY_MS
    ) {
      lastNtpRetry = nowMs;
      timeValid = retryNtpSync();

      if (timeValid) {
        updateAstronomy();

        if (sdReady) {
          redrawWorldClock();
        }

        markCurrentSchedulerBuckets();
        lastClockUpdate = nowMs;
      }
    }
  }

  if (
    !sdReady &&
    nowMs -
      systemStatus.lastStorageAttempt >=
      STORAGE_RETRY_MS
  ) {
    systemStatus.lastStorageAttempt =
      nowMs;

    sdReady = initializeSD();

    if (sdReady) {
      initializeWeatherService();
    }

    if (
      sdReady &&
      timeValid
    ) {
      redrawWorldClock();
      markCurrentSchedulerBuckets();
      lastClockUpdate = nowMs;
    }
  }

  serviceWeather();

  // While a touch page is open, keep Wi-Fi, the web server, configuration
  // button, weather service, and recovery services alive, but do not let
  // scheduled clock/map painting overwrite the touchscreen interface.
  if (touchUiIsOpen()) {
    delay(10);
    return;
  }

  if (
    !timeValid ||
    !sdReady
  ) {
    delay(20);
    return;
  }

  const time_t now =
    time(nullptr);

  const time_t clockBucket =
    schedulerBucket(
      now,
      clockUpdateIntervalSeconds()
    );

  const time_t astronomyBucket =
    schedulerBucket(
      now,
      ASTRONOMY_UPDATE_SECONDS
    );

  // Full astronomy/map update has priority because redrawWorldClock()
  // also paints the clock and IP address.
  if (
    astronomyBucket >= 0 &&
    astronomyBucket !=
      lastAstronomyBucket
  ) {
    lastAstronomyBucket =
      astronomyBucket;

    updateAstronomy();
    redrawWorldClock();

    lastClockBucket =
      clockBucket;

    lastClockUpdate = nowMs;
    delay(10);
    return;
  }

  if (
    clockBucket >= 0 &&
    clockBucket !=
      lastClockBucket
  ) {
    lastClockBucket =
      clockBucket;

    renderStatusBar();
    lastClockUpdate = nowMs;
  }

  delay(10);
}
