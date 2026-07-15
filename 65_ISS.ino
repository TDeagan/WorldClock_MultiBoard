// ============================================================
// Optional ISS position overlay
// ============================================================

namespace {
bool extractJsonNumber(
  const String &json,
  const char *key,
  double &value
) {
  const String token =
    String("\"") + key + "\"";

  int position =
    json.indexOf(token);

  if (position < 0) {
    return false;
  }

  position =
    json.indexOf(':', position);

  if (position < 0) {
    return false;
  }

  ++position;

  while (
    position < json.length() &&
    (
      json[position] == ' ' ||
      json[position] == '\t'
    )
  ) {
    ++position;
  }

  const char *start =
    json.c_str() + position;

  char *endPointer = nullptr;

  value =
    strtod(start, &endPointer);

  return endPointer != start;
}
} // namespace

bool fetchIssPosition() {
  if (
    !(overlaySettings.showISS || overlaySettings.showIssTrack) ||
    WiFi.status() != WL_CONNECTED
  ) {
    return false;
  }

  WiFiClientSecure client;

  // The public ISS endpoint is used only for public position data.
  // Avoiding certificate storage keeps this ESP32 project small.
  client.setInsecure();

  HTTPClient http;

  if (!http.begin(client, ISS_API_URL)) {
    setSystemError(
      SystemError::IssUnavailable,
      "Could not start ISS request"
    );
    return false;
  }

  http.setTimeout(7000);

  const int status =
    http.GET();

  if (status != HTTP_CODE_OK) {
    http.end();
    setSystemError(
      SystemError::IssUnavailable,
      "ISS HTTP request failed"
    );
    return false;
  }

  const String response =
    http.getString();

  http.end();

  double latitude = 0.0;
  double longitude = 0.0;

  if (
    !extractJsonNumber(
      response,
      "latitude",
      latitude
    ) ||
    !extractJsonNumber(
      response,
      "longitude",
      longitude
    )
  ) {
    setSystemError(
      SystemError::IssUnavailable,
      "ISS response could not be parsed"
    );
    return false;
  }

  if (
    issPosition.valid &&
    fabs(latitude - issPosition.latitude) > 0.01
  ) {
    issPosition.ascending =
      latitude > issPosition.latitude;
  }

  issPosition.latitude = latitude;
  issPosition.longitude = longitude;
  issPosition.valid = true;
  issPosition.updatedAt = time(nullptr);
  systemStatus.issPositionValid = true;
  systemStatus.lastIssUpdate =
    issPosition.updatedAt;

  if (
    systemStatus.lastError ==
    SystemError::IssUnavailable
  ) {
    setSystemError(SystemError::None, "");
  }

  if (overlaySettings.showIssTrack) {
    calculateIssOrbitTrack();
  }

  return true;
}

void drawIssMarker() {
  if (
    !(overlaySettings.showISS || overlaySettings.showIssTrack) ||
    !issPosition.valid
  ) {
    return;
  }

  const int x =
    longitudeToX(
      issPosition.longitude
    );

  const int y =
    latitudeToY(
      issPosition.latitude
    );

  // Match the Sun/Moon date-line behavior by drawing a wrapped copy
  // when the marker is near either side of the map.
  for (
    int shift = -SCREEN_W;
    shift <= SCREEN_W;
    shift += SCREEN_W
  ) {
    const int drawX =
      x + shift;

    if (
      drawX + ISS_PLUS_ARM < 0 ||
      drawX - ISS_PLUS_ARM >= SCREEN_W
    ) {
      continue;
    }

    // One-pixel black surround keeps the 5 x 5 cyan plus visible
    // against both the daytime and nighttime maps.
    lcd.drawFastHLine(
      drawX - ISS_PLUS_ARM - 1,
      y,
      ISS_PLUS_ARM * 2 + 3,
      TFT_BLACK
    );

    lcd.drawFastVLine(
      drawX,
      y - ISS_PLUS_ARM - 1,
      ISS_PLUS_ARM * 2 + 3,
      TFT_BLACK
    );

    lcd.drawFastHLine(
      drawX - ISS_PLUS_ARM,
      y,
      ISS_PLUS_ARM * 2 + 1,
      TFT_CYAN
    );

    lcd.drawFastVLine(
      drawX,
      y - ISS_PLUS_ARM,
      ISS_PLUS_ARM * 2 + 1,
      TFT_CYAN
    );
  }
}

void renderIssTrackOverlay() {
  if (!overlaySettings.showIssTrack) {
    return;
  }

  drawIssOrbitTrack();
}


void renderIssOverlay() {
  drawIssMarker();
}
