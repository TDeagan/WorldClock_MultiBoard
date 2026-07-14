// ============================================================
// Approximate one-complete-orbit ISS ground track
//
// This stage uses the current live ISS position, the known ISS
// inclination, the approximate orbital period, and Earth rotation.
// It produces a useful one-orbit display without adding an SGP4
// library or TLE downloader. A later version can replace only the
// calculation routine with SGP4 while retaining the UI and renderer.
// ============================================================

namespace {
double normalizeRadians(double angle) {
  const double twoPi =
    2.0 * 3.14159265358979323846;

  while (angle >= twoPi) {
    angle -= twoPi;
  }

  while (angle < 0.0) {
    angle += twoPi;
  }

  return angle;
}

double orbitAlpha(
  double argumentOfLatitude,
  double inclinationRadians
) {
  return atan2(
    cos(inclinationRadians) *
      sin(argumentOfLatitude),
    cos(argumentOfLatitude)
  );
}

void drawTrackSegmentWrapped(
  int x1,
  int y1,
  int x2,
  int y2,
  uint16_t color
) {
  const int difference =
    x2 - x1;

  if (abs(difference) <= SCREEN_W / 2) {
    lcd.drawLine(
      x1,
      y1,
      x2,
      y2,
      color
    );

    return;
  }

  // Crossing from the right edge to the left edge.
  if (difference < 0) {
    const int adjustedX2 =
      x2 + SCREEN_W;

    const double fraction =
      static_cast<double>(SCREEN_W - x1) /
      static_cast<double>(adjustedX2 - x1);

    const int edgeY =
      static_cast<int>(
        y1 +
        fraction * (y2 - y1) +
        0.5
      );

    lcd.drawLine(
      x1,
      y1,
      SCREEN_W - 1,
      edgeY,
      color
    );

    lcd.drawLine(
      0,
      edgeY,
      x2,
      y2,
      color
    );

    return;
  }

  // Crossing from the left edge to the right edge.
  const int adjustedX2 =
    x2 - SCREEN_W;

  const double fraction =
    static_cast<double>(0 - x1) /
    static_cast<double>(adjustedX2 - x1);

  const int edgeY =
    static_cast<int>(
      y1 +
      fraction * (y2 - y1) +
      0.5
    );

  lcd.drawLine(
    x1,
    y1,
    0,
    edgeY,
    color
  );

  lcd.drawLine(
    SCREEN_W - 1,
    edgeY,
    x2,
    y2,
    color
  );
}
} // namespace

void calculateIssOrbitTrack() {
  issTrackValid = false;
  systemStatus.orbitTrackValid = false;

  if (
    !overlaySettings.showIssTrack ||
    !issPosition.valid
  ) {
    return;
  }

  const double inclination =
    degToRad(
      ISS_ORBIT_INCLINATION_DEGREES
    );

  const double latitude =
    degToRad(
      issPosition.latitude
    );

  double sinArgument =
    sin(latitude) /
    sin(inclination);

  if (sinArgument < -1.0) {
    sinArgument = -1.0;
  }

  if (sinArgument > 1.0) {
    sinArgument = 1.0;
  }

  // There are two orbital phases with the same latitude.
  // Choose the branch based on the measured latitude direction.
  const double ascendingArgument =
    normalizeRadians(
      asin(sinArgument)
    );

  const double descendingArgument =
    normalizeRadians(
      3.14159265358979323846 -
      asin(sinArgument)
    );

  double argument =
    issPosition.ascending
    ? ascendingArgument
    : descendingArgument;

  double alphaPrevious =
    orbitAlpha(
      argument,
      inclination
    );

  double longitude =
    issPosition.longitude;

  const double stepSeconds =
    ISS_ORBIT_PERIOD_SECONDS /
    static_cast<double>(
      ISS_TRACK_POINT_COUNT - 1
    );

  const double angularStep =
    2.0 *
    3.14159265358979323846 /
    static_cast<double>(
      ISS_TRACK_POINT_COUNT - 1
    );

  for (
    int index = 0;
    index < ISS_TRACK_POINT_COUNT;
    ++index
  ) {
    if (index > 0) {
      argument =
        normalizeRadians(
          argument + angularStep
        );

      const double alpha =
        orbitAlpha(
          argument,
          inclination
        );

      const double alphaChangeDegrees =
        norm180(
          radToDeg(
            alpha - alphaPrevious
          )
        );

      longitude =
        norm180(
          longitude +
          alphaChangeDegrees -
          EARTH_ROTATION_DEGREES_PER_SECOND *
          stepSeconds
        );

      alphaPrevious = alpha;
    }

    const double pointLatitude =
      radToDeg(
        asin(
          sin(inclination) *
          sin(argument)
        )
      );

    issTrack[index].latitude =
      static_cast<float>(
        pointLatitude
      );

    issTrack[index].longitude =
      static_cast<float>(
        longitude
      );

    issTrack[index].valid = true;
  }

  issTrackValid = true;
  systemStatus.orbitTrackValid = true;
}

void drawIssOrbitTrack() {
  if (
    !overlaySettings.showIssTrack ||
    !issTrackValid
  ) {
    return;
  }

  for (
    int index = 1;
    index < ISS_TRACK_POINT_COUNT;
    ++index
  ) {
    if (
      !issTrack[index - 1].valid ||
      !issTrack[index].valid
    ) {
      continue;
    }

    // Dotted mode draws alternating short segments.
    if (
      overlaySettings.issTrackDotted &&
      (index & 1) != 0
    ) {
      continue;
    }

    const int x1 =
      longitudeToX(
        issTrack[index - 1].longitude
      );

    const int y1 =
      latitudeToY(
        issTrack[index - 1].latitude
      );

    const int x2 =
      longitudeToX(
        issTrack[index].longitude
      );

    const int y2 =
      latitudeToY(
        issTrack[index].latitude
      );

    drawTrackSegmentWrapped(
      x1,
      y1,
      x2,
      y2,
      ISS_TRACK_COLOR
    );
  }
}
