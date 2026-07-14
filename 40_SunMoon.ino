static double norm360(double x) {
  while (x >= 360.0) x -= 360.0;
  while (x < 0.0) x += 360.0;
  return x;
}


static double degToRad(double degrees) {
  return degrees * 3.14159265358979323846 / 180.0;
}


static double radToDeg(double radians) {
  return radians * 180.0 / 3.14159265358979323846;
}


void calculateMoonInfo(
  time_t epoch,
  double &moonLatitude,
  double &moonLongitude,
  double &moonElongation,
  bool &moonWaxing
) {
  const double jd =
    2440587.5 +
    static_cast<double>(epoch) / 86400.0;

  // Days from the epoch used by the low-precision orbital model.
  const double d = jd - 2451543.5;

  // Sun orbital elements, used for lunar perturbations and phase.
  const double sunPerihelion =
    282.9404 + 4.70935E-5 * d;

  const double sunEccentricity =
    0.016709 - 1.151E-9 * d;

  const double sunMeanAnomaly =
    norm360(356.0470 + 0.9856002585 * d);

  const double sunM =
    degToRad(sunMeanAnomaly);

  const double sunE =
    sunM +
    sunEccentricity *
    sin(sunM) *
    (1.0 + sunEccentricity * cos(sunM));

  const double sunX =
    cos(sunE) - sunEccentricity;

  const double sunY =
    sqrt(1.0 - sunEccentricity * sunEccentricity) *
    sin(sunE);

  const double sunTrueAnomaly =
    radToDeg(atan2(sunY, sunX));

  const double sunLongitude =
    norm360(sunTrueAnomaly + sunPerihelion);

  const double sunMeanLongitude =
    norm360(sunPerihelion + sunMeanAnomaly);

  // Moon orbital elements.
  const double nodeLongitude =
    norm360(125.1228 - 0.0529538083 * d);

  const double inclination = 5.1454;

  const double periapsis =
    norm360(318.0634 + 0.1643573223 * d);

  const double semiMajorAxis = 60.2666;
  const double eccentricity = 0.054900;

  const double meanAnomaly =
    norm360(115.3654 + 13.0649929509 * d);

  const double meanAnomalyRad =
    degToRad(meanAnomaly);

  const double eccentricAnomaly =
    meanAnomalyRad +
    eccentricity *
    sin(meanAnomalyRad) *
    (1.0 + eccentricity * cos(meanAnomalyRad));

  const double xv =
    semiMajorAxis *
    (cos(eccentricAnomaly) - eccentricity);

  const double yv =
    semiMajorAxis *
    sqrt(1.0 - eccentricity * eccentricity) *
    sin(eccentricAnomaly);

  const double trueAnomaly =
    radToDeg(atan2(yv, xv));

  const double radius =
    sqrt(xv * xv + yv * yv);

  const double argument =
    degToRad(trueAnomaly + periapsis);

  const double nodeRad =
    degToRad(nodeLongitude);

  const double inclinationRad =
    degToRad(inclination);

  const double xh =
    radius *
    (
      cos(nodeRad) * cos(argument) -
      sin(nodeRad) * sin(argument) *
      cos(inclinationRad)
    );

  const double yh =
    radius *
    (
      sin(nodeRad) * cos(argument) +
      cos(nodeRad) * sin(argument) *
      cos(inclinationRad)
    );

  const double zh =
    radius *
    sin(argument) *
    sin(inclinationRad);

  double moonEclipticLongitude =
    norm360(radToDeg(atan2(yh, xh)));

  double moonEclipticLatitude =
    radToDeg(
      atan2(
        zh,
        sqrt(xh * xh + yh * yh)
      )
    );

  // Principal lunar perturbations improve the marker position significantly.
  const double moonMeanLongitude =
    norm360(
      nodeLongitude +
      periapsis +
      meanAnomaly
    );

  const double elongation =
    norm360(
      moonMeanLongitude -
      sunMeanLongitude
    );

  const double argumentLatitude =
    norm360(
      moonMeanLongitude -
      nodeLongitude
    );

  moonEclipticLongitude +=
    -1.274 * sin(degToRad(meanAnomaly - 2.0 * elongation)) +
     0.658 * sin(degToRad(2.0 * elongation)) -
     0.186 * sin(degToRad(sunMeanAnomaly)) -
     0.059 * sin(degToRad(2.0 * meanAnomaly - 2.0 * elongation)) -
     0.057 * sin(degToRad(meanAnomaly - 2.0 * elongation + sunMeanAnomaly)) +
     0.053 * sin(degToRad(meanAnomaly + 2.0 * elongation)) +
     0.046 * sin(degToRad(2.0 * elongation - sunMeanAnomaly)) +
     0.041 * sin(degToRad(meanAnomaly - sunMeanAnomaly)) -
     0.035 * sin(degToRad(elongation)) -
     0.031 * sin(degToRad(meanAnomaly + sunMeanAnomaly)) -
     0.015 * sin(degToRad(2.0 * argumentLatitude - 2.0 * elongation)) +
     0.011 * sin(degToRad(meanAnomaly - 4.0 * elongation));

  moonEclipticLatitude +=
    -0.173 * sin(degToRad(argumentLatitude - 2.0 * elongation)) -
     0.055 * sin(degToRad(meanAnomaly - argumentLatitude - 2.0 * elongation)) -
     0.046 * sin(degToRad(meanAnomaly + argumentLatitude - 2.0 * elongation)) +
     0.033 * sin(degToRad(argumentLatitude + 2.0 * elongation)) +
     0.017 * sin(degToRad(2.0 * meanAnomaly + argumentLatitude));

  moonEclipticLongitude =
    norm360(moonEclipticLongitude);

  const double lonRad =
    degToRad(moonEclipticLongitude);

  const double latRad =
    degToRad(moonEclipticLatitude);

  const double obliquity =
    degToRad(
      23.4393 -
      3.563E-7 * d
    );

  const double xEcliptic =
    cos(lonRad) * cos(latRad);

  const double yEcliptic =
    sin(lonRad) * cos(latRad);

  const double zEcliptic =
    sin(latRad);

  const double xEquatorial =
    xEcliptic;

  const double yEquatorial =
    yEcliptic * cos(obliquity) -
    zEcliptic * sin(obliquity);

  const double zEquatorial =
    yEcliptic * sin(obliquity) +
    zEcliptic * cos(obliquity);

  const double rightAscension =
    norm360(
      radToDeg(
        atan2(
          yEquatorial,
          xEquatorial
        )
      )
    );

  const double declination =
    radToDeg(
      atan2(
        zEquatorial,
        sqrt(
          xEquatorial * xEquatorial +
          yEquatorial * yEquatorial
        )
      )
    );

  const double centuries =
    (jd - 2451545.0) / 36525.0;

  const double greenwichSiderealTime =
    norm360(
      280.46061837 +
      360.98564736629 *
      (jd - 2451545.0) +
      0.000387933 *
      centuries * centuries -
      centuries * centuries * centuries /
      38710000.0
    );

  const double sublunarLongitude =
    norm180(
      rightAscension -
      greenwichSiderealTime
    );

  const double phaseDifference =
    norm360(
      moonEclipticLongitude -
      sunLongitude
    );

  const double angularElongation =
    acos(
      constrain(
        cos(latRad) *
        cos(degToRad(phaseDifference)),
        -1.0,
        1.0
      )
    );

  moonLatitude = declination;
  moonLongitude = sublunarLongitude;
  moonElongation = angularElongation;
  moonWaxing = phaseDifference < 180.0;
}


static int longitudeToX(double longitude) {
  int x =
    static_cast<int>(
      (longitude + 180.0) *
      SCREEN_W /
      360.0
    );

  while (x < 0) x += SCREEN_W;
  while (x >= SCREEN_W) x -= SCREEN_W;

  return x;
}


static int latitudeToY(double latitude) {
  int y =
    static_cast<int>(
      (90.0 - latitude) *
      SCREEN_MAP_H /
      180.0
    );

  if (y < 0) y = 0;
  if (y >= SCREEN_MAP_H) {
    y = SCREEN_MAP_H - 1;
  }

  return y;
}


void drawWrappedSunDisc(int x, int y) {
  constexpr int radius = SUN_DISC_RADIUS;

  for (int shift = -SCREEN_W;
       shift <= SCREEN_W;
       shift += SCREEN_W) {
    const int drawX = x + shift;

    if (
      drawX + radius < 0 ||
      drawX - radius >= SCREEN_W
    ) {
      continue;
    }

    lcd.fillCircle(
      drawX,
      y,
      radius + 1,
      TFT_BLACK
    );

    lcd.fillCircle(
      drawX,
      y,
      radius,
      TFT_YELLOW
    );
  }
}


void drawWrappedMoonPhase(
  int x,
  int y,
  double elongation,
  bool waxing
) {
  constexpr int radius = MOON_DISC_RADIUS;

  // Sunlight direction in the Moon-disc coordinate system.
  // At new moon the illuminated hemisphere faces away from Earth;
  // at full moon it faces toward Earth.
  const double lightX =
    (waxing ? 1.0 : -1.0) *
    sin(elongation);

  const double lightZ =
    -cos(elongation);

  for (int shift = -SCREEN_W;
       shift <= SCREEN_W;
       shift += SCREEN_W) {
    const int drawX = x + shift;

    if (
      drawX + radius + 1 < 0 ||
      drawX - radius - 1 >= SCREEN_W
    ) {
      continue;
    }

    lcd.fillCircle(
      drawX,
      y,
      radius + 1,
      TFT_BLACK
    );

    for (int dy = -radius;
         dy <= radius;
         ++dy) {
      for (int dx = -radius;
           dx <= radius;
           ++dx) {
        const double nx =
          static_cast<double>(dx) / radius;

        const double ny =
          static_cast<double>(dy) / radius;

        const double radialSquared =
          nx * nx + ny * ny;

        if (radialSquared > 1.0) {
          continue;
        }

        const double nz =
          sqrt(1.0 - radialSquared);

        const double illumination =
          nx * lightX +
          nz * lightZ;

        const uint16_t color =
          illumination > 0.0
          ? TFT_WHITE
          : TFT_DARKGREY;

        lcd.drawPixel(
          drawX + dx,
          y + dy,
          color
        );
      }
    }

    lcd.drawCircle(
      drawX,
      y,
      radius,
      TFT_WHITE
    );
  }
}


void drawCelestialMarkers(time_t epoch) {
  tm utc {};
  gmtime_r(&epoch, &utc);

  double solarDeclination = 0.0;
  double subsolarLongitude = 0.0;

  solarPositionUTC(
    utc,
    solarDeclination,
    subsolarLongitude
  );

  const int sunX =
    longitudeToX(
      subsolarLongitude
    );

  const int sunY =
    latitudeToY(
      radToDeg(solarDeclination)
    );

  double moonLatitude = 0.0;
  double moonLongitude = 0.0;
  double moonElongation = 0.0;
  bool moonWaxing = false;

  calculateMoonInfo(
    epoch,
    moonLatitude,
    moonLongitude,
    moonElongation,
    moonWaxing
  );

  const int moonX =
    longitudeToX(
      moonLongitude
    );

  const int moonY =
    latitudeToY(
      moonLatitude
    );

  if (overlaySettings.showSun) {
    drawWrappedSunDisc(
      sunX,
      sunY
    );
  }

  if (overlaySettings.showMoon) {
    drawWrappedMoonPhase(
      moonX,
      moonY,
      moonElongation,
      moonWaxing
    );
  }
}
