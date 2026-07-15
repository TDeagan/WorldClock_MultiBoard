// ============================================================
// Optional equirectangular coordinate grid and home marker
// ============================================================
//
// The base map spans:
//   longitude -180 to +180 across SCREEN_W
//   latitude   +90 to  -90 across SCREEN_MAP_H
//
// Grid lines are drawn after the map/terminator and before the ISS,
// Sun, and Moon overlays. The home marker is drawn after those
// overlays so it remains easy to locate.
// ============================================================

namespace {

int locationLongitudeToX(
  double longitude
) {
  while (longitude < -180.0) {
    longitude += 360.0;
  }

  while (longitude > 180.0) {
    longitude -= 360.0;
  }

  int x =
    static_cast<int>(
      (
        longitude + 180.0
      ) *
      SCREEN_W /
      360.0
    );

  if (x < 0) {
    x = 0;
  }

  if (x >= SCREEN_W) {
    x = SCREEN_W - 1;
  }

  return x;
}


int locationLatitudeToY(
  double latitude
) {
  if (latitude > 90.0) {
    latitude = 90.0;
  }

  if (latitude < -90.0) {
    latitude = -90.0;
  }

  int y =
    static_cast<int>(
      (
        90.0 - latitude
      ) *
      SCREEN_MAP_H /
      180.0
    );

  if (y < 0) {
    y = 0;
  }

  if (y >= SCREEN_MAP_H) {
    y = SCREEN_MAP_H - 1;
  }

  return y;
}


void drawDottedVerticalLine(
  int x,
  uint16_t color
) {
  for (
    int y = 0;
    y < SCREEN_MAP_H;
    y += 4
  ) {
    lcd.drawFastVLine(
      x,
      y,
      min(
        2,
        SCREEN_MAP_H - y
      ),
      color
    );
  }
}


void drawDottedHorizontalLine(
  int y,
  uint16_t color
) {
  for (
    int x = 0;
    x < SCREEN_W;
    x += 4
  ) {
    lcd.drawFastHLine(
      x,
      y,
      min(
        2,
        SCREEN_W - x
      ),
      color
    );
  }
}


void drawHomeMarkerAt(
  int x,
  int y
) {
  lcd.drawCircle(
    x,
    y,
    HOME_MARKER_RADIUS,
    HOME_MARKER_COLOR
  );

  lcd.drawFastHLine(
    x - HOME_MARKER_ARM,
    y,
    HOME_MARKER_ARM * 2 + 1,
    HOME_MARKER_COLOR
  );

  lcd.drawFastVLine(
    x,
    y - HOME_MARKER_ARM,
    HOME_MARKER_ARM * 2 + 1,
    HOME_MARKER_COLOR
  );

  lcd.fillCircle(
    x,
    y,
    1,
    HOME_MARKER_CENTER_COLOR
  );
}

} // namespace


void renderCoordinateGridOverlay() {
  if (
    !locationGridSettings.showCoordinateGrid
  ) {
    return;
  }

  for (
    int longitude =
      -180 + LOCATION_GRID_STEP_DEGREES;
    longitude < 180;
    longitude += LOCATION_GRID_STEP_DEGREES
  ) {
    const int x =
      locationLongitudeToX(
        longitude
      );

    if (longitude == 0) {
      lcd.drawFastVLine(
        x,
        0,
        SCREEN_MAP_H,
        LOCATION_AXIS_COLOR
      );
    } else {
      drawDottedVerticalLine(
        x,
        LOCATION_GRID_COLOR
      );
    }
  }

  for (
    int latitude =
      -90 + LOCATION_GRID_STEP_DEGREES;
    latitude < 90;
    latitude += LOCATION_GRID_STEP_DEGREES
  ) {
    const int y =
      locationLatitudeToY(
        latitude
      );

    if (latitude == 0) {
      lcd.drawFastHLine(
        0,
        y,
        SCREEN_W,
        LOCATION_AXIS_COLOR
      );
    } else {
      drawDottedHorizontalLine(
        y,
        LOCATION_GRID_COLOR
      );
    }
  }
}


void renderHomeLocationOverlay() {
  if (
    !locationGridSettings.showHomeMarker
  ) {
    return;
  }

  const int x =
    locationLongitudeToX(
      locationGridSettings.homeLongitude
    );

  const int y =
    locationLatitudeToY(
      locationGridSettings.homeLatitude
    );

  // Drawing wrapped copies keeps the marker intact when the selected
  // longitude lies near the map's -180/+180 seam.
  drawHomeMarkerAt(
    x,
    y
  );

  if (
    x < HOME_MARKER_ARM
  ) {
    drawHomeMarkerAt(
      x + SCREEN_W,
      y
    );
  }

  if (
    x >= SCREEN_W - HOME_MARKER_ARM
  ) {
    drawHomeMarkerAt(
      x - SCREEN_W,
      y
    );
  }
}
