// v4.2: drawMap() is invoked only by the unified full-display scheduler.
void solarPositionUTC(
  const tm &utc,
  double &declination,
  double &subsolarLongitude
) {
  constexpr double pi =
    3.14159265358979323846;

  const int dayOfYear =
    utc.tm_yday + 1;

  const double hour =
    utc.tm_hour +
    utc.tm_min / 60.0 +
    utc.tm_sec / 3600.0;

  const double gamma =
    2.0 * pi / 365.0 *
    (
      dayOfYear - 1 +
      (hour - 12.0) / 24.0
    );

  const double equationOfTime =
    229.18 *
    (
      0.000075 +
      0.001868 * cos(gamma) -
      0.032077 * sin(gamma) -
      0.014615 * cos(2.0 * gamma) -
      0.040849 * sin(2.0 * gamma)
    );

  declination =
    0.006918 -
    0.399912 * cos(gamma) +
    0.070257 * sin(gamma) -
    0.006758 * cos(2.0 * gamma) +
    0.000907 * sin(2.0 * gamma) -
    0.002697 * cos(3.0 * gamma) +
    0.001480 * sin(3.0 * gamma);

  subsolarLongitude =
    norm180(
      180.0 -
      15.0 * hour -
      equationOfTime / 4.0
    );
}


uint16_t blend565(
  uint16_t night,
  uint16_t day,
  uint8_t amount
) {
  const uint32_t nr = (night >> 11) & 31;
  const uint32_t ng = (night >> 5) & 63;
  const uint32_t nb = night & 31;

  const uint32_t dr = (day >> 11) & 31;
  const uint32_t dg = (day >> 5) & 63;
  const uint32_t db = day & 31;

  const uint32_t inverse =
    255U - amount;

  const uint32_t r =
    (nr * inverse + dr * amount + 127U) / 255U;

  const uint32_t g =
    (ng * inverse + dg * amount + 127U) / 255U;

  const uint32_t b =
    (nb * inverse + db * amount + 127U) / 255U;

  return static_cast<uint16_t>(
    (r << 11) |
    (g << 5) |
    b
  );
}


bool drawMap(const tm &utc) {
  if (!sdReady || !dayFile || !nightFile) {
    return false;
  }

  double solarDeclination = 0.0;
  double subsolarLongitude = 0.0;

  solarPositionUTC(
    utc,
    solarDeclination,
    subsolarLongitude
  );

  const double sinDec =
    sin(solarDeclination);

  const double cosDec =
    cos(solarDeclination);

  constexpr double deg =
    3.14159265358979323846 / 180.0;

  int previousSourceY = -1;

  for (int sy = 0; sy < SCREEN_MAP_H; ++sy) {
    const int sourceY =
      (sy * MAP_H) /
      SCREEN_MAP_H;

    if (sourceY != previousSourceY) {
      if (
        !readMapLine(dayFile, sourceY, dayLine) ||
        !readMapLine(nightFile, sourceY, nightLine)
      ) {
        return false;
      }

      previousSourceY = sourceY;
    }

    const double latitude =
      (
        90.0 -
        (sourceY + 0.5) *
        180.0 /
        MAP_H
      ) * deg;

    const double sinLat =
      sin(latitude);

    const double cosLat =
      cos(latitude);

    for (int sx = 0; sx < SCREEN_W; ++sx) {
      const int sourceX =
        (sx * MAP_W) /
        SCREEN_W;

      const double longitude =
        -180.0 +
        (sourceX + 0.5) *
        360.0 /
        MAP_W;

      const double dot =
        sinLat * sinDec +
        cosLat * cosDec *
        cos(
          (
            longitude -
            subsolarLongitude
          ) * deg
        );

      double blend =
        (dot + 0.08) / 0.16;

      if (blend < 0.0) {
        blend = 0.0;
      }

      if (blend > 1.0) {
        blend = 1.0;
      }

      blend =
        blend *
        blend *
        (3.0 - 2.0 * blend);

      const uint8_t amount =
        static_cast<uint8_t>(
          blend * 255.0 + 0.5
        );

      outputLine[sx] =
        blend565(
          nightLine[sourceX],
          dayLine[sourceX],
          amount
        );
    }

    lcd.pushImage(
      0,
      sy,
      SCREEN_W,
      1,
      outputLine
    );
  }

  return true;
}
