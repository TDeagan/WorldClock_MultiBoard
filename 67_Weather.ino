// ============================================================
// Saved-location weather and regional precipitation radar
// ============================================================
//
// Forecast data comes from Open-Meteo. The regional radar overlay comes
// from RainViewer and is centered on the saved home coordinates. Forecast
// and radar data are cached on the microSD card for offline display.
// ============================================================

WeatherSettings weatherSettings;
WeatherForecast weatherForecast;
WeatherRadarInfo weatherRadarInfo;

namespace {

static constexpr uint32_t WEATHER_CACHE_MAGIC = 0x57435831UL; // WCX1
static constexpr uint32_t WEATHER_RADAR_MAGIC = 0x57435231UL; // WCR1
static constexpr uint16_t WEATHER_CACHE_VERSION = 2;

struct WeatherForecastCacheRecord {
  uint32_t magic = WEATHER_CACHE_MAGIC;
  uint16_t version = WEATHER_CACHE_VERSION;
  uint16_t recordSize = sizeof(WeatherForecastCacheRecord);
  WeatherForecast data;
  uint32_t checksum = 0;
};

struct WeatherRadarCacheRecord {
  uint32_t magic = WEATHER_RADAR_MAGIC;
  uint16_t version = WEATHER_CACHE_VERSION;
  uint16_t recordSize = sizeof(WeatherRadarCacheRecord);
  WeatherRadarInfo data;
  uint32_t checksum = 0;
};

unsigned long lastWeatherAttemptAt = 0;
unsigned long lastRadarAttemptAt = 0;
unsigned long lastLocationNameAttemptAt = 0;
double lastLocationNameAttemptLatitude = NAN;
double lastLocationNameAttemptLongitude = NAN;
bool weatherRefreshRequested = false;
bool radarRefreshRequested = false;
String weatherLastError;
String radarLastError;
String weatherBaseMapLastError;

// Radar and basemap decoding reuse the shared PNG decoder and SD input file declared
// in 01_Hardware.ino. A second PNG object would reserve another large zlib
// workspace in internal DRAM and prevents non-PSRAM ESP32 builds from linking.

uint16_t weatherPngLine[WEATHER_RADAR_IMAGE_SIZE];
uint8_t weatherPngAlphaMask[(WEATHER_RADAR_IMAGE_SIZE + 7) / 8];

int weatherRadarDestinationX = WEATHER_RADAR_IMAGE_X;
int weatherRadarDestinationY = WEATHER_RADAR_IMAGE_Y;
int weatherRadarCropTop = WEATHER_RADAR_SOURCE_CROP_TOP;
bool weatherRadarDecodeFailed = false;
bool weatherPngValidationOnly = false;

int weatherBaseTileDestinationX = WEATHER_RADAR_IMAGE_X;
int weatherBaseTileDestinationY = WEATHER_RADAR_IMAGE_Y;
bool weatherBaseTileDecodeFailed = false;

bool ensureWeatherBaseMapTiles();
bool weatherBaseMapTilesAvailableForSavedLocation();

uint32_t weatherChecksum(
  const uint8_t *data,
  size_t length
) {
  uint32_t value = 2166136261UL;

  for (size_t index = 0; index < length; ++index) {
    value ^= data[index];
    value *= 16777619UL;
  }

  return value;
}

bool ensureWeatherDirectory() {
  if (!sdReady) {
    return false;
  }

  File directory = SD.open(WEATHER_DIRECTORY, FILE_READ);
  const bool exists = directory && directory.isDirectory();

  if (directory) {
    directory.close();
  }

  if (exists) {
    return true;
  }

  return SD.mkdir(WEATHER_DIRECTORY);
}

template <typename RecordType>
bool writeWeatherRecord(
  const char *path,
  const char *temporaryPath,
  RecordType &record
) {
  if (!ensureWeatherDirectory()) {
    return false;
  }

  record.checksum = 0;
  record.checksum = weatherChecksum(
    reinterpret_cast<const uint8_t *>(&record),
    sizeof(record) - sizeof(record.checksum)
  );

  SD.remove(temporaryPath);

  File output = SD.open(temporaryPath, FILE_WRITE);

  if (!output) {
    return false;
  }

  const size_t written = output.write(
    reinterpret_cast<const uint8_t *>(&record),
    sizeof(record)
  );

  output.flush();
  output.close();

  if (written != sizeof(record)) {
    SD.remove(temporaryPath);
    return false;
  }

  const String backupPath = String(path) + F(".bak");
  SD.remove(backupPath.c_str());

  const bool hadPreviousRecord = SD.exists(path);

  if (
    hadPreviousRecord &&
    !SD.rename(path, backupPath.c_str())
  ) {
    SD.remove(temporaryPath);
    return false;
  }

  if (!SD.rename(temporaryPath, path)) {
    SD.remove(temporaryPath);

    if (hadPreviousRecord) {
      SD.rename(backupPath.c_str(), path);
    }

    return false;
  }

  SD.remove(backupPath.c_str());
  return true;
}

template <typename RecordType>
bool readWeatherRecord(
  const char *path,
  RecordType &record,
  uint32_t expectedMagic
) {
  File input = SD.open(path, FILE_READ);

  if (!input || input.size() != sizeof(record)) {
    if (input) {
      input.close();
    }
    return false;
  }

  const size_t bytesRead = input.read(
    reinterpret_cast<uint8_t *>(&record),
    sizeof(record)
  );

  input.close();

  if (
    bytesRead != sizeof(record) ||
    record.magic != expectedMagic ||
    record.version != WEATHER_CACHE_VERSION ||
    record.recordSize != sizeof(record)
  ) {
    return false;
  }

  const uint32_t savedChecksum = record.checksum;
  record.checksum = 0;

  const uint32_t calculated = weatherChecksum(
    reinterpret_cast<const uint8_t *>(&record),
    sizeof(record) - sizeof(record.checksum)
  );

  record.checksum = savedChecksum;
  return calculated == savedChecksum;
}

bool saveWeatherForecastCache() {
  WeatherForecastCacheRecord record;
  record.data = weatherForecast;

  return writeWeatherRecord(
    WEATHER_FORECAST_CACHE_FILE,
    WEATHER_FORECAST_TEMP_FILE,
    record
  );
}

bool loadWeatherForecastCache() {
  WeatherForecastCacheRecord record;

  if (!readWeatherRecord(
        WEATHER_FORECAST_CACHE_FILE,
        record,
        WEATHER_CACHE_MAGIC
      )) {
    return false;
  }

  weatherForecast = record.data;
  return weatherForecast.valid;
}

bool saveWeatherRadarCache() {
  WeatherRadarCacheRecord record;
  record.data = weatherRadarInfo;

  return writeWeatherRecord(
    WEATHER_RADAR_META_FILE,
    WEATHER_RADAR_META_TEMP_FILE,
    record
  );
}

bool loadWeatherRadarCache() {
  WeatherRadarCacheRecord record;

  if (!readWeatherRecord(
        WEATHER_RADAR_META_FILE,
        record,
        WEATHER_RADAR_MAGIC
      )) {
    return false;
  }

  weatherRadarInfo = record.data;

  if (
    !weatherRadarInfo.valid ||
    !SD.exists(WEATHER_RADAR_IMAGE_FILE)
  ) {
    weatherRadarInfo.valid = false;
    return false;
  }

  return true;
}

bool findJsonValueStart(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  int &valueStartOut
) {
  const String token = String('"') + key + '"';
  const int keyPosition = json.indexOf(token, rangeStart);

  if (
    keyPosition < 0 ||
    keyPosition >= rangeEnd
  ) {
    return false;
  }

  const int colon = json.indexOf(':', keyPosition + token.length());

  if (colon < 0 || colon >= rangeEnd) {
    return false;
  }

  int position = colon + 1;

  while (
    position < rangeEnd &&
    (
      json[position] == ' ' ||
      json[position] == '\t' ||
      json[position] == '\r' ||
      json[position] == '\n'
    )
  ) {
    ++position;
  }

  if (position >= rangeEnd) {
    return false;
  }

  valueStartOut = position;
  return true;
}

bool findJsonObjectRange(
  const String &json,
  const char *key,
  int &objectStartOut,
  int &objectEndOut
) {
  int valueStart = 0;

  if (!findJsonValueStart(
        json,
        key,
        0,
        json.length(),
        valueStart
      ) || json[valueStart] != '{') {
    return false;
  }

  bool inString = false;
  bool escaped = false;
  int depth = 0;

  for (int position = valueStart; position < json.length(); ++position) {
    const char character = json[position];

    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (character == '\\') {
        escaped = true;
      } else if (character == '"') {
        inString = false;
      }
      continue;
    }

    if (character == '"') {
      inString = true;
      continue;
    }

    if (character == '{') {
      ++depth;
    } else if (character == '}') {
      --depth;

      if (depth == 0) {
        objectStartOut = valueStart;
        objectEndOut = position + 1;
        return true;
      }
    }
  }

  return false;
}

bool jsonNumberInRange(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  double &valueOut
) {
  int valueStart = 0;

  if (!findJsonValueStart(
        json,
        key,
        rangeStart,
        rangeEnd,
        valueStart
      )) {
    return false;
  }

  const char *start = json.c_str() + valueStart;
  char *endPointer = nullptr;
  const double value = strtod(start, &endPointer);

  if (endPointer == start || !isfinite(value)) {
    return false;
  }

  valueOut = value;
  return true;
}

bool jsonStringInRange(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  String &valueOut
) {
  int valueStart = 0;

  if (!findJsonValueStart(
        json,
        key,
        rangeStart,
        rangeEnd,
        valueStart
      ) || json[valueStart] != '"') {
    return false;
  }

  String value;
  bool escaped = false;

  for (int position = valueStart + 1; position < rangeEnd; ++position) {
    const char character = json[position];

    if (escaped) {
      switch (character) {
        case 'n': value += '\n'; break;
        case 'r': value += '\r'; break;
        case 't': value += '\t'; break;
        default: value += character; break;
      }
      escaped = false;
      continue;
    }

    if (character == '\\') {
      escaped = true;
      continue;
    }

    if (character == '"') {
      valueOut = value;
      return true;
    }

    value += character;
  }

  return false;
}

size_t jsonNumberArrayInRange(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  double *valuesOut,
  size_t maximumCount
) {
  int valueStart = 0;

  if (!findJsonValueStart(
        json,
        key,
        rangeStart,
        rangeEnd,
        valueStart
      ) || json[valueStart] != '[') {
    return 0;
  }

  size_t count = 0;
  int position = valueStart + 1;

  while (position < rangeEnd && count < maximumCount) {
    while (
      position < rangeEnd &&
      (
        json[position] == ' ' ||
        json[position] == '\t' ||
        json[position] == '\r' ||
        json[position] == '\n' ||
        json[position] == ','
      )
    ) {
      ++position;
    }

    if (position >= rangeEnd || json[position] == ']') {
      break;
    }

    const char *start = json.c_str() + position;
    char *endPointer = nullptr;
    const double value = strtod(start, &endPointer);

    if (endPointer == start || !isfinite(value)) {
      return 0;
    }

    valuesOut[count++] = value;
    position = static_cast<int>(endPointer - json.c_str());
  }

  return count;
}

void copyWeatherText(
  char *destination,
  size_t destinationSize,
  const String &source
) {
  if (destinationSize == 0) {
    return;
  }

  const size_t count = min(
    destinationSize - 1,
    source.length()
  );

  memcpy(destination, source.c_str(), count);
  destination[count] = '\0';
}

String weatherPrintableLocationText(const String &source) {
  String result;
  result.reserve(source.length());
  bool previousWasSpace = false;

  for (size_t index = 0; index < source.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(source[index]);

    if (character < 32) {
      continue;
    }

    if (character == ' ' || character == '\t') {
      if (!previousWasSpace && result.length()) {
        result += ' ';
      }
      previousWasSpace = true;
      continue;
    }

    result += static_cast<char>(character);
    previousWasSpace = false;
  }

  result.trim();
  return result;
}

bool parseNominatimLocationName(
  const String &response,
  String &nameOut
) {
  int addressStart = 0;
  int addressEnd = 0;

  if (!findJsonObjectRange(
        response,
        "address",
        addressStart,
        addressEnd
      )) {
    return false;
  }

  static const char *LOCALITY_KEYS[] = {
    "city",
    "town",
    "village",
    "municipality",
    "hamlet",
    "county"
  };

  String locality;

  for (const char *key : LOCALITY_KEYS) {
    if (
      jsonStringInRange(
        response,
        key,
        addressStart,
        addressEnd,
        locality
      ) && locality.length()
    ) {
      break;
    }
  }

  String state;
  String country;
  String countryCode;
  String regionCode;

  jsonStringInRange(
    response,
    "state",
    addressStart,
    addressEnd,
    state
  );

  jsonStringInRange(
    response,
    "country",
    addressStart,
    addressEnd,
    country
  );

  jsonStringInRange(
    response,
    "country_code",
    addressStart,
    addressEnd,
    countryCode
  );

  jsonStringInRange(
    response,
    "ISO3166-2-lvl4",
    addressStart,
    addressEnd,
    regionCode
  );

  locality = weatherPrintableLocationText(locality);
  state = weatherPrintableLocationText(state);
  country = weatherPrintableLocationText(country);
  countryCode.toLowerCase();

  if (locality.length()) {
    nameOut = locality;

    if (
      countryCode == "us" &&
      regionCode.startsWith("US-") &&
      regionCode.length() > 3
    ) {
      nameOut += ", ";
      nameOut += regionCode.substring(3);
    } else if (state.length() && state != locality) {
      nameOut += ", ";
      nameOut += state;
    } else if (country.length() && country != locality) {
      nameOut += ", ";
      nameOut += country;
    }
  } else {
    String displayName;

    if (!jsonStringInRange(
          response,
          "display_name",
          0,
          response.length(),
          displayName
        )) {
      return false;
    }

    displayName = weatherPrintableLocationText(displayName);
    const int firstComma = displayName.indexOf(',');
    const int secondComma = firstComma >= 0
      ? displayName.indexOf(',', firstComma + 1)
      : -1;

    nameOut = secondComma > 0
      ? displayName.substring(0, secondComma)
      : displayName;
  }

  nameOut.trim();

  if (nameOut.length() >= sizeof(weatherForecast.locationName)) {
    nameOut.remove(sizeof(weatherForecast.locationName) - 1);
  }

  return nameOut.length() > 0;
}

bool fetchSavedLocationName(String &nameOut) {
  if (
    WiFi.status() != WL_CONNECTED ||
    !homeLocationIsConfigured()
  ) {
    return false;
  }

  String url = NOMINATIM_REVERSE_ENDPOINT;
  url.reserve(240);
  url += F("?format=jsonv2&lat=");
  url += String(locationGridSettings.homeLatitude, 5);
  url += F("&lon=");
  url += String(locationGridSettings.homeLongitude, 5);
  url += F("&zoom=10&addressdetails=1&layer=address&accept-language=en");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  if (!http.begin(client, url)) {
    return false;
  }

  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  http.setUserAgent(
    String("ESP32-WorldClock/") + FIRMWARE_VERSION
  );

  const int status = http.GET();

  if (status != HTTP_CODE_OK) {
    Serial.printf("Location name HTTP %d\n", status);
    http.end();
    return false;
  }

  const String response = http.getString();
  http.end();

  if (!parseNominatimLocationName(response, nameOut)) {
    Serial.println("Location name response parse failed");
    return false;
  }

  Serial.printf("Weather location: %s\n", nameOut.c_str());
  return true;
}

bool parseOpenMeteoResponse(
  const String &response,
  WeatherForecast &parsedOut
) {
  int currentStart = 0;
  int currentEnd = 0;
  int dailyStart = 0;
  int dailyEnd = 0;

  if (
    !findJsonObjectRange(
      response,
      "current",
      currentStart,
      currentEnd
    ) ||
    !findJsonObjectRange(
      response,
      "daily",
      dailyStart,
      dailyEnd
    )
  ) {
    return false;
  }

  double currentTime = 0.0;
  double temperature = 0.0;
  double humidity = 0.0;
  double apparent = 0.0;
  double weatherCode = 0.0;
  double precipitation = 0.0;
  double windSpeed = 0.0;
  double windDirection = 0.0;
  double windGust = 0.0;
  double isDay = 0.0;
  double utcOffset = 0.0;

  if (
    !jsonNumberInRange(response, "time", currentStart, currentEnd, currentTime) ||
    !jsonNumberInRange(response, "temperature_2m", currentStart, currentEnd, temperature) ||
    !jsonNumberInRange(response, "relative_humidity_2m", currentStart, currentEnd, humidity) ||
    !jsonNumberInRange(response, "apparent_temperature", currentStart, currentEnd, apparent) ||
    !jsonNumberInRange(response, "weather_code", currentStart, currentEnd, weatherCode) ||
    !jsonNumberInRange(response, "precipitation", currentStart, currentEnd, precipitation) ||
    !jsonNumberInRange(response, "wind_speed_10m", currentStart, currentEnd, windSpeed) ||
    !jsonNumberInRange(response, "wind_direction_10m", currentStart, currentEnd, windDirection) ||
    !jsonNumberInRange(response, "wind_gusts_10m", currentStart, currentEnd, windGust) ||
    !jsonNumberInRange(response, "is_day", currentStart, currentEnd, isDay) ||
    !jsonNumberInRange(response, "utc_offset_seconds", 0, response.length(), utcOffset)
  ) {
    return false;
  }

  double dailyTimes[WEATHER_FORECAST_DAYS] {};
  double dailyCodes[WEATHER_FORECAST_DAYS] {};
  double dailyHighs[WEATHER_FORECAST_DAYS] {};
  double dailyLows[WEATHER_FORECAST_DAYS] {};
  double dailyRain[WEATHER_FORECAST_DAYS] {};
  double dailySunrise[WEATHER_FORECAST_DAYS] {};
  double dailySunset[WEATHER_FORECAST_DAYS] {};

  if (
    jsonNumberArrayInRange(response, "time", dailyStart, dailyEnd, dailyTimes, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "weather_code", dailyStart, dailyEnd, dailyCodes, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "temperature_2m_max", dailyStart, dailyEnd, dailyHighs, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "temperature_2m_min", dailyStart, dailyEnd, dailyLows, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "precipitation_probability_max", dailyStart, dailyEnd, dailyRain, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "sunrise", dailyStart, dailyEnd, dailySunrise, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS ||
    jsonNumberArrayInRange(response, "sunset", dailyStart, dailyEnd, dailySunset, WEATHER_FORECAST_DAYS) != WEATHER_FORECAST_DAYS
  ) {
    return false;
  }

  String zoneAbbreviation;
  jsonStringInRange(
    response,
    "timezone_abbreviation",
    0,
    response.length(),
    zoneAbbreviation
  );

  WeatherForecast parsed;
  parsed.valid = true;
  parsed.latitude = locationGridSettings.homeLatitude;
  parsed.longitude = locationGridSettings.homeLongitude;
  parsed.fetchedAt = time(nullptr);
  parsed.observationTime = static_cast<time_t>(llround(currentTime));
  parsed.utcOffsetSeconds = static_cast<int32_t>(lround(utcOffset));
  parsed.temperature = static_cast<float>(temperature);
  parsed.apparentTemperature = static_cast<float>(apparent);
  parsed.relativeHumidity = static_cast<uint8_t>(constrain(lround(humidity), 0L, 100L));
  parsed.weatherCode = static_cast<int16_t>(lround(weatherCode));
  parsed.precipitation = static_cast<float>(precipitation);
  parsed.windSpeed = static_cast<float>(windSpeed);
  parsed.windDirection = static_cast<uint16_t>(constrain(lround(windDirection), 0L, 360L));
  parsed.windGust = static_cast<float>(windGust);
  parsed.isDay = isDay >= 0.5;
  parsed.useFahrenheit = weatherSettings.useFahrenheit;

  copyWeatherText(
    parsed.timezoneAbbreviation,
    sizeof(parsed.timezoneAbbreviation),
    zoneAbbreviation
  );

  for (size_t index = 0; index < WEATHER_FORECAST_DAYS; ++index) {
    parsed.days[index].date = static_cast<time_t>(llround(dailyTimes[index]));
    parsed.days[index].weatherCode = static_cast<int16_t>(lround(dailyCodes[index]));
    parsed.days[index].highTemperature = static_cast<float>(dailyHighs[index]);
    parsed.days[index].lowTemperature = static_cast<float>(dailyLows[index]);
    parsed.days[index].precipitationProbability = static_cast<uint8_t>(constrain(lround(dailyRain[index]), 0L, 100L));
    parsed.days[index].sunrise = static_cast<time_t>(llround(dailySunrise[index]));
    parsed.days[index].sunset = static_cast<time_t>(llround(dailySunset[index]));
  }

  parsedOut = parsed;
  return true;
}

String buildOpenMeteoUrl() {
  String url = OPEN_METEO_FORECAST_ENDPOINT;
  url.reserve(620);
  url += F("?latitude=");
  url += String(locationGridSettings.homeLatitude, 5);
  url += F("&longitude=");
  url += String(locationGridSettings.homeLongitude, 5);
  url += F("&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,precipitation,wind_speed_10m,wind_direction_10m,wind_gusts_10m,is_day");
  url += F("&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset");
  url += weatherSettings.useFahrenheit
    ? F("&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch")
    : F("&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm");
  url += F("&timezone=auto&timeformat=unixtime&forecast_days=3");
  return url;
}

bool fetchOpenMeteoForecast() {
  if (
    WiFi.status() != WL_CONNECTED ||
    !weatherFeatureAvailable()
  ) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String url = buildOpenMeteoUrl();

  if (!http.begin(client, url)) {
    weatherLastError = "Could not start Open-Meteo request";
    return false;
  }

  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  http.setUserAgent(
    String("ESP32-WorldClock/") + FIRMWARE_VERSION
  );

  const int status = http.GET();

  if (status != HTTP_CODE_OK) {
    weatherLastError = String("Open-Meteo HTTP ") + status;
    http.end();
    return false;
  }

  const String response = http.getString();
  http.end();

  WeatherForecast parsed;

  if (!parseOpenMeteoResponse(response, parsed)) {
    weatherLastError = "Open-Meteo response parse failed";
    return false;
  }

  String locationName;

  if (
    weatherForecastAvailableForSavedLocation() &&
    weatherForecast.locationName[0] != '\0'
  ) {
    locationName = weatherForecast.locationName;
  }

  const unsigned long nowMs = millis();
  const bool locationChangedSinceLookup =
    !isfinite(lastLocationNameAttemptLatitude) ||
    !isfinite(lastLocationNameAttemptLongitude) ||
    fabs(
      lastLocationNameAttemptLatitude -
      locationGridSettings.homeLatitude
    ) > WEATHER_LOCATION_MATCH_DEGREES ||
    fabs(
      lastLocationNameAttemptLongitude -
      locationGridSettings.homeLongitude
    ) > WEATHER_LOCATION_MATCH_DEGREES;

  const bool locationLookupAllowed =
    locationName.length() == 0 &&
    (
      locationChangedSinceLookup ||
      lastLocationNameAttemptAt == 0 ||
      nowMs - lastLocationNameAttemptAt >= WEATHER_LOCATION_NAME_RETRY_MS
    );

  if (locationLookupAllowed) {
    lastLocationNameAttemptAt = nowMs;
    lastLocationNameAttemptLatitude =
      locationGridSettings.homeLatitude;
    lastLocationNameAttemptLongitude =
      locationGridSettings.homeLongitude;
    fetchSavedLocationName(locationName);
  }

  copyWeatherText(
    parsed.locationName,
    sizeof(parsed.locationName),
    locationName
  );

  weatherForecast = parsed;
  weatherLastError = "";

  if (sdReady) {
    saveWeatherForecastCache();
  }

  Serial.printf(
    "Weather: %.1f at %.4f, %.4f; code=%d\n",
    weatherForecast.temperature,
    weatherForecast.latitude,
    weatherForecast.longitude,
    weatherForecast.weatherCode
  );

  return true;
}

void *weatherPngOpenCallback(
  const char *filename,
  int32_t *fileSize
) {
  if (pngInputFile) {
    pngInputFile.close();
  }

  pngInputFile = SD.open(filename, FILE_READ);

  if (!pngInputFile) {
    *fileSize = 0;
    return nullptr;
  }

  *fileSize = static_cast<int32_t>(pngInputFile.size());
  return &pngInputFile;
}

void weatherPngCloseCallback(void *handle) {
  (void)handle;

  if (pngInputFile) {
    pngInputFile.close();
  }
}

int32_t weatherPngReadCallback(
  PNGFILE *handle,
  uint8_t *buffer,
  int32_t length
) {
  (void)handle;

  if (!pngInputFile || length <= 0) {
    return 0;
  }

  return static_cast<int32_t>(
    pngInputFile.read(buffer, length)
  );
}

int32_t weatherPngSeekCallback(
  PNGFILE *handle,
  int32_t position
) {
  (void)handle;

  if (!pngInputFile || position < 0) {
    return 0;
  }

  return pngInputFile.seek(
    static_cast<uint32_t>(position)
  ) ? 1 : 0;
}

int weatherRadarPngDrawCallback(PNGDRAW *draw) {
  if (
    draw == nullptr ||
    draw->iWidth != WEATHER_RADAR_IMAGE_SIZE ||
    draw->y < 0 ||
    draw->y >= WEATHER_RADAR_IMAGE_SIZE
  ) {
    weatherRadarDecodeFailed = true;
    return 0;
  }

  // Convert the native radar colors without blending them against a
  // synthetic background. RainViewer tiles are transparent overlays; the
  // alpha mask below decides which pixels are drawn over the daylight map.
  png.getLineAsRGB565(
    draw,
    weatherPngLine,
    PNG_RGB565_LITTLE_ENDIAN,
    0xFFFFFFFF
  );

  if (weatherPngValidationOnly) {
    return 1;
  }

  if (
    draw->y < weatherRadarCropTop ||
    draw->y >= weatherRadarCropTop + WEATHER_RADAR_IMAGE_H
  ) {
    return 1;
  }

  // PNGdec packs eight alpha decisions into each mask byte, with the first
  // pixel in bit 7. Draw only runs containing non-transparent radar pixels.
  // This preserves the regional map-tile background instead of turning
  // transparent tile pixels into the former magenta chroma-key color.
  if (draw->iHasAlpha) {
    if (!png.getAlphaMask(draw, weatherPngAlphaMask, 1)) {
      return 1;
    }
  } else {
    memset(
      weatherPngAlphaMask,
      0xFF,
      sizeof(weatherPngAlphaMask)
    );
  }

  const int destinationY =
    weatherRadarDestinationY + draw->y - weatherRadarCropTop;

  int x = 0;

  while (x < WEATHER_RADAR_IMAGE_SIZE) {
    while (
      x < WEATHER_RADAR_IMAGE_SIZE &&
      (weatherPngAlphaMask[x >> 3] & (0x80U >> (x & 7))) == 0
    ) {
      ++x;
    }

    const int runStart = x;

    while (
      x < WEATHER_RADAR_IMAGE_SIZE &&
      (weatherPngAlphaMask[x >> 3] & (0x80U >> (x & 7))) != 0
    ) {
      ++x;
    }

    if (x > runStart) {
      lcd.pushImage(
        weatherRadarDestinationX + runStart,
        destinationY,
        x - runStart,
        1,
        weatherPngLine + runStart
      );
    }
  }

  return 1;
}


int weatherBaseTilePngDrawCallback(PNGDRAW *draw) {
  if (
    draw == nullptr ||
    draw->iWidth != WEATHER_RADAR_IMAGE_SIZE ||
    draw->y < 0 ||
    draw->y >= WEATHER_RADAR_IMAGE_SIZE
  ) {
    weatherBaseTileDecodeFailed = true;
    return 0;
  }

  png.getLineAsRGB565(
    draw,
    weatherPngLine,
    PNG_RGB565_LITTLE_ENDIAN,
    0xFFFFFFFF
  );

  const int destinationY =
    weatherBaseTileDestinationY + draw->y;

  if (
    destinationY < WEATHER_RADAR_IMAGE_Y ||
    destinationY >= WEATHER_RADAR_IMAGE_Y + WEATHER_RADAR_IMAGE_H
  ) {
    return 1;
  }

  const int visibleLeft = max(
    weatherBaseTileDestinationX,
    WEATHER_RADAR_IMAGE_X
  );

  const int visibleRight = min(
    weatherBaseTileDestinationX + WEATHER_RADAR_IMAGE_SIZE,
    WEATHER_RADAR_IMAGE_X + WEATHER_RADAR_IMAGE_W
  );

  if (visibleRight <= visibleLeft) {
    return 1;
  }

  const int sourceX =
    visibleLeft - weatherBaseTileDestinationX;

  lcd.pushImage(
    visibleLeft,
    destinationY,
    visibleRight - visibleLeft,
    1,
    weatherPngLine + sourceX
  );

  return 1;
}

bool validateWeatherRadarPng(const char *path) {
  if (!path || !SD.exists(path)) {
    return false;
  }

  weatherPngValidationOnly = true;
  weatherRadarDecodeFailed = false;

  const int openResult = png.open(
    path,
    weatherPngOpenCallback,
    weatherPngCloseCallback,
    weatherPngReadCallback,
    weatherPngSeekCallback,
    weatherRadarPngDrawCallback
  );

  if (openResult != PNG_SUCCESS) {
    png.close();
    weatherPngValidationOnly = false;
    return false;
  }

  bool valid =
    png.getWidth() == WEATHER_RADAR_IMAGE_SIZE &&
    png.getHeight() == WEATHER_RADAR_IMAGE_SIZE &&
    !png.isInterlaced();

  if (valid) {
    valid =
      png.decode(nullptr, PNG_CHECK_CRC) == PNG_SUCCESS &&
      !weatherRadarDecodeFailed;
  }

  png.close();
  weatherPngValidationOnly = false;
  return valid;
}


bool renderWeatherBaseTile(
  const char *path,
  int destinationX,
  int destinationY
) {
  if (!path || !SD.exists(path)) {
    return false;
  }

  weatherBaseTileDestinationX = destinationX;
  weatherBaseTileDestinationY = destinationY;
  weatherBaseTileDecodeFailed = false;

  const int openResult = png.open(
    path,
    weatherPngOpenCallback,
    weatherPngCloseCallback,
    weatherPngReadCallback,
    weatherPngSeekCallback,
    weatherBaseTilePngDrawCallback
  );

  if (openResult != PNG_SUCCESS) {
    png.close();
    return false;
  }

  const bool valid =
    png.getWidth() == WEATHER_RADAR_IMAGE_SIZE &&
    png.getHeight() == WEATHER_RADAR_IMAGE_SIZE &&
    !png.isInterlaced();

  const bool decoded =
    valid &&
    png.decode(nullptr, PNG_CHECK_CRC) == PNG_SUCCESS &&
    !weatherBaseTileDecodeFailed;

  png.close();
  return decoded;
}

bool downloadUrlToSdFile(
  const String &url,
  const char *temporaryPath,
  String &errorOut
) {
  errorOut = "";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  if (!http.begin(client, url)) {
    errorOut = "request start failed";
    return false;
  }

  http.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  http.setUserAgent(
    String("ESP32-WorldClock/") + FIRMWARE_VERSION
  );
  http.addHeader("Accept", "image/png");
  http.addHeader("Accept-Encoding", "identity");

  const int status = http.GET();

  if (status != HTTP_CODE_OK) {
    errorOut = status > 0
      ? String("HTTP ") + status
      : String("transport ") + status;
    http.end();
    return false;
  }

  const String contentType = http.header("Content-Type");

  if (
    contentType.length() &&
    !contentType.startsWith("image/png")
  ) {
    errorOut = String("unexpected ") + contentType;
    http.end();
    return false;
  }

  SD.remove(temporaryPath);
  File output = SD.open(temporaryPath, FILE_WRITE);

  if (!output) {
    errorOut = "SD open failed";
    http.end();
    return false;
  }

  const int contentLength = http.getSize();
  const int written = http.writeToStream(&output);

  output.flush();
  output.close();
  http.end();

  if (written <= 0) {
    SD.remove(temporaryPath);
    errorOut = String("stream write ") + written;
    return false;
  }

  if (contentLength > 0 && written != contentLength) {
    SD.remove(temporaryPath);
    errorOut = String("short write ") + written + "/" + contentLength;
    return false;
  }

  return true;
}

bool parseRainViewerMetadata(
  const String &response,
  String &hostOut,
  String &pathOut,
  time_t &frameTimeOut
) {
  if (!jsonStringInRange(
        response,
        "host",
        0,
        response.length(),
        hostOut
      )) {
    return false;
  }

  const int nowcastPosition = response.indexOf("\"nowcast\"");
  const int searchEnd = nowcastPosition >= 0
    ? nowcastPosition
    : response.length();

  const int pathKey = response.lastIndexOf("\"path\"", searchEnd);
  const int timeKey = response.lastIndexOf("\"time\"", pathKey);

  if (pathKey < 0 || timeKey < 0) {
    return false;
  }

  if (!jsonStringInRange(
        response,
        "path",
        pathKey,
        searchEnd,
        pathOut
      )) {
    return false;
  }

  double frameTime = 0.0;

  if (!jsonNumberInRange(
        response,
        "time",
        timeKey,
        pathKey,
        frameTime
      )) {
    return false;
  }

  frameTimeOut = static_cast<time_t>(llround(frameTime));
  return true;
}

bool fetchRainViewerRadar() {
  if (
    WiFi.status() != WL_CONNECTED ||
    !weatherFeatureAvailable() ||
    !sdReady ||
    !ensureWeatherDirectory()
  ) {
    return false;
  }

  // The basemap changes only when the location crosses a tile boundary.
  // Download only missing current-view tiles; existing files remain cached.
  ensureWeatherBaseMapTiles();

  WiFiClientSecure metadataClient;
  metadataClient.setInsecure();

  HTTPClient metadataHttp;

  if (!metadataHttp.begin(
        metadataClient,
        RAINVIEWER_WEATHER_MAPS_ENDPOINT
      )) {
    radarLastError = "Could not start RainViewer metadata request";
    return false;
  }

  metadataHttp.setTimeout(WEATHER_HTTP_TIMEOUT_MS);
  metadataHttp.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  metadataHttp.useHTTP10(true);
  metadataHttp.setUserAgent(
    String("ESP32-WorldClock/") + FIRMWARE_VERSION
  );

  const int metadataStatus = metadataHttp.GET();

  if (metadataStatus != HTTP_CODE_OK) {
    radarLastError = String("RainViewer metadata HTTP ") + metadataStatus;
    metadataHttp.end();
    return false;
  }

  const String metadata = metadataHttp.getString();
  metadataHttp.end();

  String host;
  String framePath;
  time_t frameTime = 0;

  if (!parseRainViewerMetadata(
        metadata,
        host,
        framePath,
        frameTime
      )) {
    radarLastError = "RainViewer metadata parse failed";
    return false;
  }

  String imageUrl;
  imageUrl.reserve(host.length() + framePath.length() + 96);
  imageUrl += host;
  imageUrl += framePath;
  imageUrl += F("/256/");
  imageUrl += String(WEATHER_RADAR_ZOOM);
  imageUrl += '/';
  imageUrl += String(locationGridSettings.homeLatitude, 5);
  imageUrl += '/';
  imageUrl += String(locationGridSettings.homeLongitude, 5);
  imageUrl += F("/2/1_1.png");

  String downloadError;

  if (!downloadUrlToSdFile(
        imageUrl,
        WEATHER_RADAR_TEMP_FILE,
        downloadError
      )) {
    // Retry without separate snow colors. Both forms are documented by
    // RainViewer, and this gives older or partially updated tile edges a
    // second compatible request before reporting failure.
    imageUrl.replace("/2/1_1.png", "/2/1_0.png");

    if (!downloadUrlToSdFile(
          imageUrl,
          WEATHER_RADAR_TEMP_FILE,
          downloadError
        )) {
      radarLastError = String("RainViewer image: ") + downloadError;
      Serial.printf(
        "RainViewer radar download failed: %s\n",
        downloadError.c_str()
      );
      return false;
    }
  }

  if (!validateWeatherRadarPng(WEATHER_RADAR_TEMP_FILE)) {
    SD.remove(WEATHER_RADAR_TEMP_FILE);
    radarLastError = "RainViewer radar PNG was invalid";
    return false;
  }

  SD.remove(WEATHER_RADAR_BACKUP_FILE);

  const bool hadPreviousRadar =
    SD.exists(WEATHER_RADAR_IMAGE_FILE);

  if (
    hadPreviousRadar &&
    !SD.rename(
      WEATHER_RADAR_IMAGE_FILE,
      WEATHER_RADAR_BACKUP_FILE
    )
  ) {
    SD.remove(WEATHER_RADAR_TEMP_FILE);
    radarLastError = "Could not preserve previous radar cache";
    return false;
  }

  if (!SD.rename(
        WEATHER_RADAR_TEMP_FILE,
        WEATHER_RADAR_IMAGE_FILE
      )) {
    SD.remove(WEATHER_RADAR_TEMP_FILE);

    if (hadPreviousRadar) {
      SD.rename(
        WEATHER_RADAR_BACKUP_FILE,
        WEATHER_RADAR_IMAGE_FILE
      );
    }

    radarLastError = "Could not install radar cache image";
    return false;
  }

  SD.remove(WEATHER_RADAR_BACKUP_FILE);

  WeatherRadarInfo updated;
  updated.valid = true;
  updated.latitude = locationGridSettings.homeLatitude;
  updated.longitude = locationGridSettings.homeLongitude;
  updated.fetchedAt = time(nullptr);
  updated.frameTime = frameTime;
  updated.zoom = WEATHER_RADAR_ZOOM;

  weatherRadarInfo = updated;
  saveWeatherRadarCache();
  radarLastError = weatherBaseMapLastError;

  Serial.printf(
    "Weather radar: frame=%lld at %.4f, %.4f\n",
    static_cast<long long>(weatherRadarInfo.frameTime),
    weatherRadarInfo.latitude,
    weatherRadarInfo.longitude
  );

  return true;
}

double weatherMercatorWorldSize() {
  return 256.0 * static_cast<double>(1UL << WEATHER_RADAR_ZOOM);
}

double weatherLongitudeToWorldX(double longitude) {
  return (longitude + 180.0) / 360.0 * weatherMercatorWorldSize();
}

double weatherLatitudeToWorldY(double latitude) {
  latitude = constrain(latitude, -85.05112878, 85.05112878);
  const double radians = latitude * PI / 180.0;

  return (
    1.0 -
    log(tan(radians) + 1.0 / cos(radians)) / PI
  ) / 2.0 * weatherMercatorWorldSize();
}

int weatherWrapBaseTileX(int tileX) {
  const int tileCount = 1 << WEATHER_RADAR_ZOOM;

  while (tileX < 0) {
    tileX += tileCount;
  }

  while (tileX >= tileCount) {
    tileX -= tileCount;
  }

  return tileX;
}

String weatherBaseTilePath(
  int tileX,
  int tileY
) {
  String path = WEATHER_BASE_TILE_PREFIX;
  path.reserve(48);
  path += String(WEATHER_RADAR_ZOOM);
  path += '_';
  path += String(tileX);
  path += '_';
  path += String(tileY);
  path += F(".png");
  return path;
}

String weatherBaseTileUrl(
  int tileX,
  int tileY
) {
  String url = OPENSTREETMAP_TILE_ENDPOINT;
  url.reserve(96);
  url += '/';
  url += String(WEATHER_RADAR_ZOOM);
  url += '/';
  url += String(tileX);
  url += '/';
  url += String(tileY);
  url += F(".png");
  return url;
}

void weatherBaseTileRange(
  int32_t &viewportLeftOut,
  int32_t &viewportTopOut,
  int &minimumTileXOut,
  int &maximumTileXOut,
  int &minimumTileYOut,
  int &maximumTileYOut
) {
  const double centerX = weatherLongitudeToWorldX(
    locationGridSettings.homeLongitude
  );

  const double centerY = weatherLatitudeToWorldY(
    locationGridSettings.homeLatitude
  );

  viewportLeftOut = static_cast<int32_t>(
    floor(centerX - WEATHER_RADAR_IMAGE_W / 2.0)
  );

  viewportTopOut = static_cast<int32_t>(
    floor(centerY - WEATHER_RADAR_IMAGE_H / 2.0)
  );

  minimumTileXOut = static_cast<int>(
    floor(viewportLeftOut / 256.0)
  );

  maximumTileXOut = static_cast<int>(
    floor(
      (
        viewportLeftOut +
        WEATHER_RADAR_IMAGE_W -
        1
      ) / 256.0
    )
  );

  minimumTileYOut = static_cast<int>(
    floor(viewportTopOut / 256.0)
  );

  maximumTileYOut = static_cast<int>(
    floor(
      (
        viewportTopOut +
        WEATHER_RADAR_IMAGE_H -
        1
      ) / 256.0
    )
  );
}

bool weatherBaseMapTilesAvailableForSavedLocation() {
  if (
    !sdReady ||
    !homeLocationIsConfigured()
  ) {
    return false;
  }

  int32_t viewportLeft = 0;
  int32_t viewportTop = 0;
  int minimumTileX = 0;
  int maximumTileX = 0;
  int minimumTileY = 0;
  int maximumTileY = 0;

  weatherBaseTileRange(
    viewportLeft,
    viewportTop,
    minimumTileX,
    maximumTileX,
    minimumTileY,
    maximumTileY
  );

  (void)viewportLeft;
  (void)viewportTop;

  const int tileCount = 1 << WEATHER_RADAR_ZOOM;

  for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
    if (tileY < 0 || tileY >= tileCount) {
      continue;
    }

    for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
      const int wrappedX = weatherWrapBaseTileX(tileX);
      const String path = weatherBaseTilePath(wrappedX, tileY);

      if (!SD.exists(path.c_str())) {
        return false;
      }
    }
  }

  return true;
}

bool installWeatherBaseTile(
  int tileX,
  int tileY
) {
  const String path = weatherBaseTilePath(tileX, tileY);

  if (
    SD.exists(path.c_str()) &&
    validateWeatherRadarPng(path.c_str())
  ) {
    return true;
  }

  SD.remove(path.c_str());

  String downloadError;

  if (!downloadUrlToSdFile(
        weatherBaseTileUrl(tileX, tileY),
        WEATHER_BASE_TILE_TEMP_FILE,
        downloadError
      )) {
    weatherBaseMapLastError =
      String("OpenStreetMap tile: ") + downloadError;
    Serial.printf(
      "OpenStreetMap tile %d/%d/%d failed: %s\n",
      WEATHER_RADAR_ZOOM,
      tileX,
      tileY,
      downloadError.c_str()
    );
    return false;
  }

  if (!validateWeatherRadarPng(WEATHER_BASE_TILE_TEMP_FILE)) {
    SD.remove(WEATHER_BASE_TILE_TEMP_FILE);
    weatherBaseMapLastError =
      "OpenStreetMap tile PNG was invalid";
    return false;
  }

  if (
    !SD.rename(
      WEATHER_BASE_TILE_TEMP_FILE,
      path.c_str()
    )
  ) {
    SD.remove(WEATHER_BASE_TILE_TEMP_FILE);
    weatherBaseMapLastError =
      "Could not install OpenStreetMap tile";
    return false;
  }

  return true;
}

bool ensureWeatherBaseMapTiles() {
  if (
    WiFi.status() != WL_CONNECTED ||
    !sdReady ||
    !ensureWeatherDirectory() ||
    !homeLocationIsConfigured()
  ) {
    return false;
  }

  int32_t viewportLeft = 0;
  int32_t viewportTop = 0;
  int minimumTileX = 0;
  int maximumTileX = 0;
  int minimumTileY = 0;
  int maximumTileY = 0;

  weatherBaseTileRange(
    viewportLeft,
    viewportTop,
    minimumTileX,
    maximumTileX,
    minimumTileY,
    maximumTileY
  );

  (void)viewportLeft;
  (void)viewportTop;

  const int tileCount = 1 << WEATHER_RADAR_ZOOM;
  bool allReady = true;

  for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
    if (tileY < 0 || tileY >= tileCount) {
      continue;
    }

    for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
      const int wrappedX = weatherWrapBaseTileX(tileX);

      if (!installWeatherBaseTile(wrappedX, tileY)) {
        allReady = false;
      }
    }
  }

  if (allReady) {
    weatherBaseMapLastError = "";
  }

  return allReady;
}

bool drawWeatherRadarBaseMap() {
  lcd.fillRect(
    WEATHER_RADAR_IMAGE_X,
    WEATHER_RADAR_IMAGE_Y,
    WEATHER_RADAR_IMAGE_W,
    WEATHER_RADAR_IMAGE_H,
    0x0861
  );

  if (!weatherBaseMapTilesAvailableForSavedLocation()) {
    if (!weatherBaseMapLastError.length()) {
      weatherBaseMapLastError =
        "No cached OpenStreetMap tiles";
    }
    return false;
  }

  int32_t viewportLeft = 0;
  int32_t viewportTop = 0;
  int minimumTileX = 0;
  int maximumTileX = 0;
  int minimumTileY = 0;
  int maximumTileY = 0;

  weatherBaseTileRange(
    viewportLeft,
    viewportTop,
    minimumTileX,
    maximumTileX,
    minimumTileY,
    maximumTileY
  );

  const int tileCount = 1 << WEATHER_RADAR_ZOOM;
  bool allDrawn = true;

  lcd.startWrite();

  for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
    if (tileY < 0 || tileY >= tileCount) {
      continue;
    }

    for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
      const int wrappedX = weatherWrapBaseTileX(tileX);
      const String path = weatherBaseTilePath(wrappedX, tileY);

      const int destinationX =
        WEATHER_RADAR_IMAGE_X +
        tileX * WEATHER_RADAR_IMAGE_SIZE -
        viewportLeft;

      const int destinationY =
        WEATHER_RADAR_IMAGE_Y +
        tileY * WEATHER_RADAR_IMAGE_SIZE -
        viewportTop;

      if (!renderWeatherBaseTile(
            path.c_str(),
            destinationX,
            destinationY
          )) {
        allDrawn = false;
        SD.remove(path.c_str());
      }
    }
  }

  lcd.endWrite();

  if (!allDrawn) {
    weatherBaseMapLastError =
      "OpenStreetMap tile decode failed";
  } else {
    weatherBaseMapLastError = "";
  }

  return allDrawn;
}

bool renderWeatherRadarOverlay() {
  if (
    !weatherRadarAvailableForSavedLocation() ||
    !validateWeatherRadarPng(WEATHER_RADAR_IMAGE_FILE)
  ) {
    return false;
  }

  weatherPngValidationOnly = false;
  weatherRadarDecodeFailed = false;
  weatherRadarDestinationX = WEATHER_RADAR_IMAGE_X;
  weatherRadarDestinationY = WEATHER_RADAR_IMAGE_Y;
  weatherRadarCropTop = WEATHER_RADAR_SOURCE_CROP_TOP;

  const int openResult = png.open(
    WEATHER_RADAR_IMAGE_FILE,
    weatherPngOpenCallback,
    weatherPngCloseCallback,
    weatherPngReadCallback,
    weatherPngSeekCallback,
    weatherRadarPngDrawCallback
  );

  if (openResult != PNG_SUCCESS) {
    png.close();
    return false;
  }

  lcd.startWrite();

  const bool ok =
    png.decode(nullptr, PNG_CHECK_CRC) == PNG_SUCCESS &&
    !weatherRadarDecodeFailed;

  lcd.endWrite();
  png.close();
  return ok;
}

void drawWeatherLocationCrosshair() {
  const int centerX = WEATHER_RADAR_IMAGE_X + WEATHER_RADAR_IMAGE_W / 2;
  const int centerY = WEATHER_RADAR_IMAGE_Y + WEATHER_RADAR_IMAGE_H / 2;

  lcd.drawCircle(centerX, centerY, 5, TFT_BLACK);
  lcd.drawCircle(centerX, centerY, 4, TFT_WHITE);
  lcd.drawFastHLine(centerX - 8, centerY, 17, TFT_WHITE);
  lcd.drawFastVLine(centerX, centerY - 8, 17, TFT_WHITE);
  lcd.drawPixel(centerX, centerY, TFT_RED);
}

} // namespace

bool loadWeatherSettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, true)) {
    weatherSettings = WeatherSettings();
    return false;
  }

  weatherSettings.enabled = preferences.getBool(
    PREF_KEY_WEATHER_ENABLED,
    true
  );

  weatherSettings.useFahrenheit = preferences.getBool(
    PREF_KEY_WEATHER_FAHRENHEIT,
    true
  );

  preferences.end();
  return true;
}

bool saveWeatherSettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, false)) {
    return false;
  }

  const bool enabledSaved = preferences.putBool(
    PREF_KEY_WEATHER_ENABLED,
    weatherSettings.enabled
  ) > 0;

  const bool unitSaved = preferences.putBool(
    PREF_KEY_WEATHER_FAHRENHEIT,
    weatherSettings.useFahrenheit
  ) > 0;

  preferences.end();
  return enabledSaved && unitSaved;
}

bool homeLocationIsConfigured() {
  return
    locationGridSettings.homeLocationConfigured &&
    isfinite(locationGridSettings.homeLatitude) &&
    locationGridSettings.homeLatitude >= -90.0 &&
    locationGridSettings.homeLatitude <= 90.0 &&
    isfinite(locationGridSettings.homeLongitude) &&
    locationGridSettings.homeLongitude >= -180.0 &&
    locationGridSettings.homeLongitude <= 180.0;
}

bool weatherFeatureAvailable() {
  return weatherSettings.enabled && homeLocationIsConfigured();
}

bool weatherForecastAvailableForSavedLocation() {
  return
    weatherForecast.valid &&
    weatherForecast.useFahrenheit == weatherSettings.useFahrenheit &&
    fabs(weatherForecast.latitude - locationGridSettings.homeLatitude) <= WEATHER_LOCATION_MATCH_DEGREES &&
    fabs(weatherForecast.longitude - locationGridSettings.homeLongitude) <= WEATHER_LOCATION_MATCH_DEGREES;
}

bool weatherRadarAvailableForSavedLocation() {
  return
    weatherRadarInfo.valid &&
    sdReady &&
    SD.exists(WEATHER_RADAR_IMAGE_FILE) &&
    fabs(weatherRadarInfo.latitude - locationGridSettings.homeLatitude) <= WEATHER_LOCATION_MATCH_DEGREES &&
    fabs(weatherRadarInfo.longitude - locationGridSettings.homeLongitude) <= WEATHER_LOCATION_MATCH_DEGREES;
}

bool weatherForecastIsStale() {
  if (!weatherForecastAvailableForSavedLocation()) {
    return true;
  }

  const time_t now = time(nullptr);

  return
    now <= 0 ||
    weatherForecast.fetchedAt <= 0 ||
    now - weatherForecast.fetchedAt >= static_cast<time_t>(WEATHER_REFRESH_MS / 1000UL);
}

bool weatherRadarIsStale() {
  if (!weatherRadarAvailableForSavedLocation()) {
    return true;
  }

  const time_t now = time(nullptr);

  return
    now <= 0 ||
    weatherRadarInfo.fetchedAt <= 0 ||
    now - weatherRadarInfo.fetchedAt >= static_cast<time_t>(WEATHER_RADAR_REFRESH_MS / 1000UL);
}

void initializeWeatherService() {
  weatherRefreshRequested = false;
  radarRefreshRequested = false;
  weatherLastError = "";
  radarLastError = "";
  weatherBaseMapLastError = "";

  if (!sdReady || !ensureWeatherDirectory()) {
    weatherForecast.valid = false;
    weatherRadarInfo.valid = false;
    return;
  }

  loadWeatherForecastCache();
  loadWeatherRadarCache();

  if (!weatherForecastAvailableForSavedLocation()) {
    weatherForecast.valid = false;
  }

  if (!weatherRadarAvailableForSavedLocation()) {
    weatherRadarInfo.valid = false;
  }

  Serial.printf(
    "Weather cache: forecast=%d radar=%d enabled=%d location=%d\n",
    weatherForecast.valid,
    weatherRadarInfo.valid,
    weatherSettings.enabled,
    homeLocationIsConfigured()
  );
}

void requestWeatherForecastRefresh() {
  if (weatherFeatureAvailable()) {
    weatherRefreshRequested = true;
  }
}

void requestWeatherRadarRefresh() {
  if (weatherFeatureAvailable()) {
    radarRefreshRequested = true;
  }
}

bool weatherForecastRefreshIsPending() {
  return weatherRefreshRequested;
}

bool weatherRadarRefreshIsPending() {
  return radarRefreshRequested;
}

const String &weatherForecastLastError() {
  return weatherLastError;
}

const String &weatherRadarLastError() {
  return radarLastError;
}

void serviceWeather() {
  if (!weatherFeatureAvailable()) {
    weatherRefreshRequested = false;
    radarRefreshRequested = false;
    return;
  }

  if (!weatherForecastAvailableForSavedLocation()) {
    weatherForecast.valid = false;
  }

  if (!weatherRadarAvailableForSavedLocation()) {
    weatherRadarInfo.valid = false;
  }

  const unsigned long now = millis();

  const bool forecastDue =
    weatherRefreshRequested ||
    weatherForecastIsStale();

  if (
    forecastDue &&
    WiFi.status() == WL_CONNECTED &&
    !renderState.fullRedrawInProgress &&
    now >= WEATHER_INITIAL_FETCH_DELAY_MS &&
    (
      weatherRefreshRequested ||
      lastWeatherAttemptAt == 0 ||
      now - lastWeatherAttemptAt >= WEATHER_RETRY_MS
    )
  ) {
    weatherRefreshRequested = false;
    lastWeatherAttemptAt = now;
    const bool updated = fetchOpenMeteoForecast();

    if (touchUiWeatherPageIsOpen()) {
      touchUiHandleWeatherUpdated();
    } else if (updated && !touchUiIsOpen()) {
      renderWeatherButtonOverlay();
    }

    return;
  }

  const bool radarFrameDue =
    radarRefreshRequested ||
    (
      (
        touchUiWeatherRadarIsOpen() ||
        weatherRadarInfo.valid
      ) &&
      weatherRadarIsStale()
    );

  const bool baseMapDue =
    touchUiWeatherRadarIsOpen() &&
    !weatherBaseMapTilesAvailableForSavedLocation();

  if (
    (radarFrameDue || baseMapDue) &&
    WiFi.status() == WL_CONNECTED &&
    sdReady &&
    !renderState.fullRedrawInProgress &&
    (
      radarRefreshRequested ||
      lastRadarAttemptAt == 0 ||
      now - lastRadarAttemptAt >= WEATHER_RETRY_MS
    )
  ) {
    const bool explicitRadarRefresh = radarRefreshRequested;
    radarRefreshRequested = false;
    lastRadarAttemptAt = now;

    if (
      baseMapDue &&
      !explicitRadarRefresh &&
      !weatherRadarIsStale()
    ) {
      ensureWeatherBaseMapTiles();
    } else {
      fetchRainViewerRadar();
    }

    if (touchUiWeatherPageIsOpen()) {
      touchUiHandleWeatherUpdated();
    }
  }
}

String weatherLocationName() {
  if (
    weatherForecastAvailableForSavedLocation() &&
    weatherForecast.locationName[0] != '\0'
  ) {
    return String(weatherForecast.locationName);
  }

  if (homeLocationIsConfigured()) {
    const char latitudeHemisphere =
      locationGridSettings.homeLatitude < 0.0 ? 'S' : 'N';
    const char longitudeHemisphere =
      locationGridSettings.homeLongitude < 0.0 ? 'W' : 'E';

    return
      String(fabs(locationGridSettings.homeLatitude), 1) +
      " " + latitudeHemisphere +
      ", " +
      String(fabs(locationGridSettings.homeLongitude), 1) +
      " " + longitudeHemisphere;
  }

  return "Saved location";
}

bool weatherLocationNameIsResolved() {
  return
    weatherForecastAvailableForSavedLocation() &&
    weatherForecast.locationName[0] != '\0';
}

const char *weatherConditionText(int weatherCode) {
  switch (weatherCode) {
    case 0: return "Clear";
    case 1: return "Mostly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55:
    case 56:
    case 57: return "Drizzle";
    case 61:
    case 63:
    case 65:
    case 66:
    case 67: return "Rain";
    case 71:
    case 73:
    case 75:
    case 77: return "Snow";
    case 80:
    case 81:
    case 82: return "Rain showers";
    case 85:
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Storm / hail";
    default: return "Weather unavailable";
  }
}

const char *weatherWindDirectionText(uint16_t degrees) {
  static const char *directions[] = {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  };

  return directions[((degrees + 11) / 22) % 16];
}

String weatherTemperatureText(float temperature, bool includeUnit) {
  String text = String(static_cast<int>(lround(temperature)));

  if (includeUnit) {
    text += weatherSettings.useFahrenheit ? " F" : " C";
  }

  return text;
}

String weatherDayName(size_t dayIndex) {
  if (
    dayIndex >= WEATHER_FORECAST_DAYS ||
    !weatherForecastAvailableForSavedLocation()
  ) {
    return "---";
  }

  time_t adjusted =
    weatherForecast.days[dayIndex].date +
    weatherForecast.utcOffsetSeconds;

  tm dayTm {};
  gmtime_r(&adjusted, &dayTm);

  char text[8];
  strftime(text, sizeof(text), "%a", &dayTm);
  return String(text);
}

String weatherForecastAgeText() {
  if (!weatherForecastAvailableForSavedLocation()) {
    return "No cached forecast";
  }

  return formatAge(weatherForecast.fetchedAt);
}

String weatherRadarAgeText() {
  if (!weatherRadarAvailableForSavedLocation()) {
    return "No cached radar";
  }

  return formatAge(weatherRadarInfo.frameTime);
}

void drawWeatherIcon(
  int centerX,
  int centerY,
  int weatherCode,
  bool isDay,
  uint16_t backgroundColor
) {
  const bool thunder = weatherCode >= 95;
  const bool snow =
    (weatherCode >= 71 && weatherCode <= 77) ||
    weatherCode == 85 ||
    weatherCode == 86;
  const bool rain =
    (weatherCode >= 51 && weatherCode <= 67) ||
    (weatherCode >= 80 && weatherCode <= 82);
  const bool fog = weatherCode == 45 || weatherCode == 48;
  const bool cloudy = weatherCode >= 1 || rain || snow || thunder || fog;

  if (!cloudy || weatherCode <= 2) {
    const uint16_t sunColor = isDay ? TFT_YELLOW : TFT_LIGHTGREY;
    lcd.fillCircle(centerX - 5, centerY - 4, 5, sunColor);

    for (int angle = 0; angle < 360; angle += 45) {
      const double radians = angle * PI / 180.0;
      const int x0 = centerX - 5 + static_cast<int>(lround(cos(radians) * 7));
      const int y0 = centerY - 4 + static_cast<int>(lround(sin(radians) * 7));
      const int x1 = centerX - 5 + static_cast<int>(lround(cos(radians) * 10));
      const int y1 = centerY - 4 + static_cast<int>(lround(sin(radians) * 10));
      lcd.drawLine(x0, y0, x1, y1, sunColor);
    }
  }

  if (cloudy) {
    lcd.fillCircle(centerX - 2, centerY, 6, TFT_LIGHTGREY);
    lcd.fillCircle(centerX + 5, centerY - 2, 7, TFT_LIGHTGREY);
    lcd.fillRoundRect(centerX - 9, centerY, 20, 7, 3, TFT_LIGHTGREY);
  }

  if (rain) {
    lcd.drawLine(centerX - 5, centerY + 9, centerX - 7, centerY + 13, TFT_CYAN);
    lcd.drawLine(centerX + 1, centerY + 9, centerX - 1, centerY + 13, TFT_CYAN);
    lcd.drawLine(centerX + 7, centerY + 9, centerX + 5, centerY + 13, TFT_CYAN);
  }

  if (snow) {
    for (int index = 0; index < 2; ++index) {
      const int x = centerX + (index == 0 ? -5 : 5);
      const int y = centerY + 11;
      lcd.drawFastHLine(x - 2, y, 5, TFT_WHITE);
      lcd.drawFastVLine(x, y - 2, 5, TFT_WHITE);
      lcd.drawLine(x - 2, y - 2, x + 2, y + 2, TFT_WHITE);
      lcd.drawLine(x + 2, y - 2, x - 2, y + 2, TFT_WHITE);
    }
  }

  if (thunder) {
    lcd.fillTriangle(
      centerX + 1, centerY + 6,
      centerX + 7, centerY + 6,
      centerX + 2, centerY + 16,
      TFT_YELLOW
    );
  }

  if (fog) {
    lcd.drawFastHLine(centerX - 10, centerY + 8, 21, TFT_LIGHTGREY);
    lcd.drawFastHLine(centerX - 7, centerY + 12, 17, TFT_LIGHTGREY);
  }

  (void)backgroundColor;
}

void renderWeatherButtonOverlay() {
  if (
    touchUiIsOpen() ||
    !weatherFeatureAvailable() ||
    !touchCalibrationIsReady()
  ) {
    return;
  }

  const uint16_t fill = 0x18E3;

  lcd.fillRoundRect(
    WEATHER_BUTTON_X,
    WEATHER_BUTTON_Y,
    WEATHER_BUTTON_W,
    WEATHER_BUTTON_H,
    5,
    fill
  );

  lcd.drawRoundRect(
    WEATHER_BUTTON_X,
    WEATHER_BUTTON_Y,
    WEATHER_BUTTON_W,
    WEATHER_BUTTON_H,
    5,
    TFT_CYAN
  );

  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_WHITE, fill);

  if (weatherForecastAvailableForSavedLocation()) {
    drawWeatherIcon(
      WEATHER_BUTTON_X + 15,
      WEATHER_BUTTON_Y + WEATHER_BUTTON_H / 2 - 1,
      weatherForecast.weatherCode,
      weatherForecast.isDay,
      fill
    );

    lcd.drawString(
      weatherTemperatureText(weatherForecast.temperature, false),
      WEATHER_BUTTON_X + 46,
      WEATHER_BUTTON_Y + WEATHER_BUTTON_H / 2
    );
  } else {
    lcd.drawString(
      "WX --",
      WEATHER_BUTTON_X + WEATHER_BUTTON_W / 2,
      WEATHER_BUTTON_Y + WEATHER_BUTTON_H / 2
    );
  }
}

bool drawWeatherRadarImage() {
  const bool baseMapDrawn = drawWeatherRadarBaseMap();
  const bool radarDrawn = renderWeatherRadarOverlay();
  drawWeatherLocationCrosshair();

  lcd.drawRect(
    WEATHER_RADAR_IMAGE_X - 1,
    WEATHER_RADAR_IMAGE_Y - 1,
    WEATHER_RADAR_IMAGE_W + 2,
    WEATHER_RADAR_IMAGE_H + 2,
    TFT_LIGHTGREY
  );

  if (!baseMapDrawn && weatherBaseMapLastError.length()) {
    radarLastError = weatherBaseMapLastError;
  } else if (
    baseMapDrawn &&
    radarDrawn &&
    radarLastError.startsWith("OpenStreetMap")
  ) {
    radarLastError = "";
  }

  return baseMapDrawn && radarDrawn;
}
