// ============================================================
// Shared settings controller
// ============================================================
//
// Touchscreen and browser controls both operate on the same application
// structures and Preferences keys. These helpers centralize validation and
// persistence for settings changed outside the captive portal.
// ============================================================

bool saveUtcOffsetPreference(
  int offsetMinutes
) {
  if (
    offsetMinutes < MIN_UTC_OFFSET_MINUTES ||
    offsetMinutes > MAX_UTC_OFFSET_MINUTES
  ) {
    return false;
  }

  Preferences preferences;

  if (
    !preferences.begin(
      PREF_NAMESPACE,
      false
    )
  ) {
    return false;
  }

  const bool saved =
    preferences.putInt(
      PREF_KEY_OFFSET_MINUTES,
      offsetMinutes
    ) > 0;

  preferences.end();
  return saved;
}


bool applyTimeDisplaySettings(
  const TimeSettings &requestedTime,
  int requestedUtcOffsetMinutes,
  const DisplaySettings &requestedDisplay,
  bool &timeZoneChangedOut,
  bool &clockDisplayChangedOut,
  bool &orientationChangedOut
) {
  if (
    !timeZonePresetIsValid(
      static_cast<uint8_t>(
        requestedTime.timeZone
      )
    ) ||
    requestedUtcOffsetMinutes < MIN_UTC_OFFSET_MINUTES ||
    requestedUtcOffsetMinutes > MAX_UTC_OFFSET_MINUTES
  ) {
    return false;
  }

  timeZoneChangedOut =
    requestedTime.timeZone !=
      timeSettings.timeZone ||
    (
      requestedTime.timeZone ==
        TimeZonePreset::FixedOffset &&
      requestedUtcOffsetMinutes !=
        networkSettings.utcOffsetMinutes
    );

  clockDisplayChangedOut =
    requestedTime.use24Hour !=
      timeSettings.use24Hour ||
    requestedTime.showSeconds !=
      timeSettings.showSeconds;

  orientationChangedOut =
    requestedDisplay.flip180 !=
      displaySettings.flip180;

  networkSettings.utcOffsetMinutes =
    requestedUtcOffsetMinutes;

  timeSettings = requestedTime;
  displaySettings = requestedDisplay;

  const bool offsetSaved =
    saveUtcOffsetPreference(
      requestedUtcOffsetMinutes
    );

  const bool timeSaved =
    saveTimeSettings();

  const bool displaySaved =
    saveDisplaySettings();

  Serial.printf(
    "Touch settings: time/display persistence "
    "offset=%d time=%d display=%d\n",
    offsetSaved,
    timeSaved,
    displaySaved
  );

  return
    offsetSaved &&
    timeSaved &&
    displaySaved;
}


bool applyLocationSettings(
  const LocationGridSettings &requestedLocation
) {
  if (
    !isfinite(
      requestedLocation.homeLatitude
    ) ||
    requestedLocation.homeLatitude < -90.0 ||
    requestedLocation.homeLatitude > 90.0 ||
    !isfinite(
      requestedLocation.homeLongitude
    ) ||
    requestedLocation.homeLongitude < -180.0 ||
    requestedLocation.homeLongitude > 180.0
  ) {
    return false;
  }

  locationGridSettings =
    requestedLocation;

  locationGridSettings.homeLocationConfigured = true;

  const bool saved =
    saveLocationGridSettings();

  Serial.printf(
    "Touch settings: location persistence=%d\n",
    saved
  );

  return saved;
}


bool applyOverlaySettings(
  const OverlaySettings &requestedOverlays
) {
  overlaySettings =
    requestedOverlays;

  const bool saved =
    saveOverlaySettings();

  Serial.printf(
    "Touch settings: overlay persistence=%d\n",
    saved
  );

  return saved;
}
