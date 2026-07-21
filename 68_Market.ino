// ============================================================
// Market Mood: broad-market composite, sparklines, and SD cache
// ============================================================
//
// The first provider uses Yahoo Finance's no-key chart endpoint. It is an
// unofficial, best-effort source and is isolated behind the functions in this
// file so another compatible provider can replace it later. SPY, QQQ, and IWM
// are averaged into a descriptive market-mood score; no buy/sell signal is
// produced.
// ============================================================

MarketSettings marketSettings;
MarketSnapshot marketSnapshot;

namespace {

static constexpr uint32_t MARKET_CACHE_MAGIC = 0x57434D31UL; // WCM1
static constexpr uint16_t MARKET_CACHE_VERSION = 1;
static constexpr size_t MARKET_PROVIDER_MAX_POINTS = 96;
static constexpr size_t MARKET_SYMBOL_COUNT = 3;

static constexpr const char *MARKET_SYMBOLS[MARKET_SYMBOL_COUNT] = {
  "SPY", "QQQ", "IWM"
};

struct MarketCacheRecord {
  uint32_t magic = MARKET_CACHE_MAGIC;
  uint16_t version = MARKET_CACHE_VERSION;
  uint16_t recordSize = sizeof(MarketCacheRecord);
  MarketSnapshot data;
  uint32_t checksum = 0;
};

struct MarketProviderSeries {
  bool valid;
  char symbol[8];
  float currentPrice;
  float previousClose;
  bool marketOpen;
  size_t pointCount;
  time_t timestamps[MARKET_PROVIDER_MAX_POINTS];
  float closes[MARKET_PROVIDER_MAX_POINTS];
  bool closeValid[MARKET_PROVIDER_MAX_POINTS];
};

// Large provider/cache/snapshot work areas are allocated from the heap only
// while needed. Alpha2 placed equivalent objects on the Arduino loop-task
// stack; the nested HTTPS/parser path could exhaust that stack and reboot.

unsigned long lastMarketAttemptAt = 0;
bool marketRefreshRequested = false;
bool marketDailyRefreshRequested = false;
String marketError;

uint32_t marketChecksum(const uint8_t *data, size_t length) {
  uint32_t value = 2166136261UL;

  for (size_t index = 0; index < length; ++index) {
    value ^= data[index];
    value *= 16777619UL;
  }

  return value;
}

bool ensureMarketDirectory() {
  if (!sdReady) {
    return false;
  }

  File directory = SD.open(MARKET_DIRECTORY, FILE_READ);
  const bool exists = directory && directory.isDirectory();

  if (directory) {
    directory.close();
  }

  return exists || SD.mkdir(MARKET_DIRECTORY);
}

bool saveMarketCache() {
  if (!ensureMarketDirectory()) {
    return false;
  }

  MarketCacheRecord *record =
    new (std::nothrow) MarketCacheRecord();

  if (record == nullptr) {
    marketError = "Not enough memory to save market cache";
    return false;
  }

  record->magic = MARKET_CACHE_MAGIC;
  record->version = MARKET_CACHE_VERSION;
  record->recordSize = sizeof(MarketCacheRecord);
  record->data = marketSnapshot;
  record->checksum = 0;
  record->checksum = marketChecksum(
    reinterpret_cast<const uint8_t *>(record),
    sizeof(*record) - sizeof(record->checksum)
  );

  SD.remove(MARKET_CACHE_TEMP_FILE);
  File output = SD.open(MARKET_CACHE_TEMP_FILE, FILE_WRITE);

  if (!output) {
    delete record;
    return false;
  }

  const size_t written = output.write(
    reinterpret_cast<const uint8_t *>(record),
    sizeof(*record)
  );

  output.flush();
  output.close();
  delete record;

  if (written != sizeof(MarketCacheRecord)) {
    SD.remove(MARKET_CACHE_TEMP_FILE);
    return false;
  }

  const String backupPath = String(MARKET_CACHE_FILE) + F(".bak");
  SD.remove(backupPath.c_str());
  const bool hadPrevious = SD.exists(MARKET_CACHE_FILE);

  if (hadPrevious && !SD.rename(MARKET_CACHE_FILE, backupPath.c_str())) {
    SD.remove(MARKET_CACHE_TEMP_FILE);
    return false;
  }

  if (!SD.rename(MARKET_CACHE_TEMP_FILE, MARKET_CACHE_FILE)) {
    SD.remove(MARKET_CACHE_TEMP_FILE);

    if (hadPrevious) {
      SD.rename(backupPath.c_str(), MARKET_CACHE_FILE);
    }

    return false;
  }

  SD.remove(backupPath.c_str());
  return true;
}

bool loadMarketCache() {
  File input = SD.open(MARKET_CACHE_FILE, FILE_READ);

  if (!input || input.size() != sizeof(MarketCacheRecord)) {
    if (input) {
      input.close();
    }
    return false;
  }

  MarketCacheRecord *record =
    new (std::nothrow) MarketCacheRecord();

  if (record == nullptr) {
    input.close();
    marketError = "Not enough memory to load market cache";
    return false;
  }

  const size_t bytesRead = input.read(
    reinterpret_cast<uint8_t *>(record),
    sizeof(*record)
  );
  input.close();

  if (
    bytesRead != sizeof(*record) ||
    record->magic != MARKET_CACHE_MAGIC ||
    record->version != MARKET_CACHE_VERSION ||
    record->recordSize != sizeof(*record)
  ) {
    delete record;
    return false;
  }

  const uint32_t savedChecksum = record->checksum;
  record->checksum = 0;
  const uint32_t calculated = marketChecksum(
    reinterpret_cast<const uint8_t *>(record),
    sizeof(*record) - sizeof(record->checksum)
  );

  if (calculated != savedChecksum) {
    delete record;
    return false;
  }

  marketSnapshot = record->data;
  delete record;
  return marketSnapshot.valid;
}

int marketFindQuotedKey(
  const String &json,
  const char *key,
  int rangeStart = 0,
  int rangeEnd = -1
) {
  if (rangeEnd < 0 || rangeEnd > static_cast<int>(json.length())) {
    rangeEnd = json.length();
  }

  String token = "\"";
  token += key;
  token += '"';
  const int found = json.indexOf(token, rangeStart);
  return found >= 0 && found < rangeEnd ? found : -1;
}

bool marketFindValueStart(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  int &valueStartOut
) {
  const int keyStart = marketFindQuotedKey(json, key, rangeStart, rangeEnd);

  if (keyStart < 0) {
    return false;
  }

  int position = keyStart + strlen(key) + 2;

  while (position < rangeEnd && json[position] != ':') {
    ++position;
  }

  if (position >= rangeEnd) {
    return false;
  }

  ++position;

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

bool marketFindObjectRange(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  int &objectStartOut,
  int &objectEndOut
) {
  int valueStart = 0;

  if (
    !marketFindValueStart(
      json,
      key,
      rangeStart,
      rangeEnd,
      valueStart
    ) ||
    json[valueStart] != '{'
  ) {
    return false;
  }

  bool inString = false;
  bool escaped = false;
  int depth = 0;

  for (int position = valueStart; position < rangeEnd; ++position) {
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
    } else if (character == '{') {
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

bool marketNumberInRange(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  double &valueOut
) {
  int valueStart = 0;

  if (!marketFindValueStart(
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

void marketStoreParsedNumber(double value, time_t &output) {
  output = static_cast<time_t>(llround(value));
}

void marketStoreParsedNumber(double value, float &output) {
  output = static_cast<float>(value);
}

template <typename NumberType>
size_t marketNumberArray(
  const String &json,
  const char *key,
  int rangeStart,
  int rangeEnd,
  NumberType *valuesOut,
  bool *validOut,
  size_t maximumCount
) {
  int valueStart = 0;

  if (
    !marketFindValueStart(
      json,
      key,
      rangeStart,
      rangeEnd,
      valueStart
    ) ||
    json[valueStart] != '['
  ) {
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

    if (json.startsWith("null", position)) {
      valuesOut[count] = NumberType();
      validOut[count] = false;
      ++count;
      position += 4;
      continue;
    }

    const char *start = json.c_str() + position;
    char *endPointer = nullptr;
    const double value = strtod(start, &endPointer);

    if (endPointer == start || !isfinite(value)) {
      return 0;
    }

    marketStoreParsedNumber(value, valuesOut[count]);
    validOut[count] = true;
    ++count;
    position = static_cast<int>(endPointer - json.c_str());
  }

  return count;
}

bool parseYahooChartResponse(
  const String &response,
  const char *symbol,
  MarketProviderSeries &seriesOut
) {
  int metaStart = 0;
  int metaEnd = 0;

  if (!marketFindObjectRange(
        response,
        "meta",
        0,
        response.length(),
        metaStart,
        metaEnd
      )) {
    return false;
  }

  double currentPrice = 0.0;
  double previousClose = 0.0;

  const bool currentFound = marketNumberInRange(
    response,
    "regularMarketPrice",
    metaStart,
    metaEnd,
    currentPrice
  );

  bool previousFound = marketNumberInRange(
    response,
    "chartPreviousClose",
    metaStart,
    metaEnd,
    previousClose
  );

  if (!previousFound) {
    previousFound = marketNumberInRange(
      response,
      "previousClose",
      metaStart,
      metaEnd,
      previousClose
    );
  }

  if (!previousFound || previousClose <= 0.0) {
    return false;
  }

  memset(&seriesOut, 0, sizeof(seriesOut));
  bool timestampValid[MARKET_PROVIDER_MAX_POINTS] {};

  const size_t timestampCount = marketNumberArray(
    response,
    "timestamp",
    0,
    response.length(),
    seriesOut.timestamps,
    timestampValid,
    MARKET_PROVIDER_MAX_POINTS
  );

  const int quoteStart = marketFindQuotedKey(response, "quote");

  if (quoteStart < 0) {
    return false;
  }

  const size_t closeCount = marketNumberArray(
    response,
    "close",
    quoteStart,
    response.length(),
    seriesOut.closes,
    seriesOut.closeValid,
    MARKET_PROVIDER_MAX_POINTS
  );

  const size_t pointCount = min(timestampCount, closeCount);

  if (pointCount == 0) {
    return false;
  }

  seriesOut.valid = true;
  strncpy(seriesOut.symbol, symbol, sizeof(seriesOut.symbol) - 1);
  seriesOut.previousClose = static_cast<float>(previousClose);
  seriesOut.currentPrice = currentFound
    ? static_cast<float>(currentPrice)
    : 0.0f;
  seriesOut.pointCount = pointCount;

  int periodStart = 0;
  int periodEnd = 0;
  int regularStart = 0;
  int regularEnd = 0;
  double regularOpen = 0.0;
  double regularClose = 0.0;

  if (
    marketFindObjectRange(
      response,
      "currentTradingPeriod",
      metaStart,
      metaEnd,
      periodStart,
      periodEnd
    ) &&
    marketFindObjectRange(
      response,
      "regular",
      periodStart,
      periodEnd,
      regularStart,
      regularEnd
    ) &&
    marketNumberInRange(
      response,
      "start",
      regularStart,
      regularEnd,
      regularOpen
    ) &&
    marketNumberInRange(
      response,
      "end",
      regularStart,
      regularEnd,
      regularClose
    )
  ) {
    const time_t now = time(nullptr);
    seriesOut.marketOpen =
      now >= static_cast<time_t>(regularOpen) &&
      now < static_cast<time_t>(regularClose);
  }

  for (size_t index = 0; index < pointCount; ++index) {
    seriesOut.closeValid[index] =
      timestampValid[index] &&
      seriesOut.closeValid[index] &&
      seriesOut.timestamps[index] > 0 &&
      seriesOut.closes[index] > 0.0f;

    if (!seriesOut.closeValid[index]) {
      seriesOut.timestamps[index] = 0;
      seriesOut.closes[index] = 0.0f;
    }
  }

  if (seriesOut.currentPrice <= 0.0f) {
    for (size_t index = pointCount; index > 0; --index) {
      if (seriesOut.closeValid[index - 1]) {
        seriesOut.currentPrice = seriesOut.closes[index - 1];
        break;
      }
    }
  }

  return seriesOut.currentPrice > 0.0f;
}

String marketProviderUrl(
  const char *endpoint,
  const char *symbol,
  const char *interval,
  const char *range
) {
  String url = endpoint;
  url.reserve(220);
  url += '/';
  url += symbol;
  url += F("?interval=");
  url += interval;
  url += F("&range=");
  url += range;
  url += F("&includePrePost=false&events=div%2Csplits");
  return url;
}

bool fetchMarketSeriesFromEndpoint(
  const char *endpoint,
  const char *symbol,
  const char *interval,
  const char *range,
  MarketProviderSeries &seriesOut,
  String &errorOut
) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  const String url = marketProviderUrl(
    endpoint,
    symbol,
    interval,
    range
  );

  if (!http.begin(client, url)) {
    errorOut = "Could not start market request";
    return false;
  }

  http.setTimeout(MARKET_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  http.setUserAgent(
    String("Mozilla/5.0 (compatible; ESP32-WorldClock/") +
      FIRMWARE_VERSION + ")"
  );
  http.addHeader("Accept-Encoding", "identity");

  const int status = http.GET();

  if (status != HTTP_CODE_OK) {
    errorOut = String("HTTP ") + status;
    http.end();
    return false;
  }

  const String response = http.getString();
  http.end();

  if (!parseYahooChartResponse(response, symbol, seriesOut)) {
    errorOut = String(symbol) + " response parse failed";
    return false;
  }

  return true;
}

bool fetchMarketSeries(
  const char *symbol,
  const char *interval,
  const char *range,
  MarketProviderSeries &seriesOut,
  String &errorOut
) {
  if (fetchMarketSeriesFromEndpoint(
        MARKET_DATA_PRIMARY_ENDPOINT,
        symbol,
        interval,
        range,
        seriesOut,
        errorOut
      )) {
    return true;
  }

  String fallbackError;

  if (fetchMarketSeriesFromEndpoint(
        MARKET_DATA_FALLBACK_ENDPOINT,
        symbol,
        interval,
        range,
        seriesOut,
        fallbackError
      )) {
    return true;
  }

  errorOut += F("; fallback ");
  errorOut += fallbackError;
  return false;
}

int marketSeriesIndexAt(
  const MarketProviderSeries &series,
  time_t timestamp
) {
  for (size_t index = 0; index < series.pointCount; ++index) {
    if (
      series.closeValid[index] &&
      series.timestamps[index] == timestamp
    ) {
      return static_cast<int>(index);
    }
  }

  return -1;
}

int16_t marketPercentToBasisPoints(double percent) {
  return static_cast<int16_t>(constrain(
    lround(percent * 100.0),
    -32768L,
    32767L
  ));
}

bool buildIntradaySnapshot(
  const MarketProviderSeries *series,
  MarketSnapshot &snapshotOut
) {
  for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
    if (!series[symbol].valid || series[symbol].previousClose <= 0.0f) {
      return false;
    }
  }

  snapshotOut.todayCount = 0;
  time_t lastStored = 0;

  for (size_t index = 0; index < series[0].pointCount; ++index) {
    if (!series[0].closeValid[index]) {
      continue;
    }

    const time_t timestamp = series[0].timestamps[index];
    int matched[MARKET_SYMBOL_COUNT] = {
      static_cast<int>(index), -1, -1
    };

    for (size_t symbol = 1; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      matched[symbol] = marketSeriesIndexAt(series[symbol], timestamp);
    }

    if (matched[1] < 0 || matched[2] < 0) {
      continue;
    }

    // Downsample 5-minute provider bars to about one point every 10 minutes.
    const bool finalPoint = index + 1 == series[0].pointCount;

    if (
      lastStored > 0 &&
      timestamp - lastStored < 9 * 60 &&
      !finalPoint
    ) {
      continue;
    }

    double compositePercent = 0.0;

    for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      const float close = series[symbol].closes[matched[symbol]];
      compositePercent +=
        (close / series[symbol].previousClose - 1.0) * 100.0;
    }

    compositePercent /= MARKET_SYMBOL_COUNT;

    if (snapshotOut.todayCount >= MARKET_TODAY_MAX_POINTS) {
      memmove(
        snapshotOut.today,
        snapshotOut.today + 1,
        sizeof(snapshotOut.today[0]) * (MARKET_TODAY_MAX_POINTS - 1)
      );
      snapshotOut.todayCount = MARKET_TODAY_MAX_POINTS - 1;
    }

    MarketGraphPoint &point =
      snapshotOut.today[snapshotOut.todayCount++];
    point.timestamp = timestamp;
    point.moodBasisPoints = marketPercentToBasisPoints(compositePercent);
    lastStored = timestamp;
  }

  if (snapshotOut.todayCount == 0) {
    return false;
  }

  double currentComposite = 0.0;
  bool marketOpen = false;

  for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
    MarketQuote &quote = snapshotOut.quotes[symbol];
    quote = MarketQuote();
    strncpy(quote.symbol, MARKET_SYMBOLS[symbol], sizeof(quote.symbol) - 1);
    quote.valid = true;
    quote.price = series[symbol].currentPrice;
    quote.changePercent =
      (series[symbol].currentPrice / series[symbol].previousClose - 1.0f) * 100.0f;
    quote.timestamp = snapshotOut.today[snapshotOut.todayCount - 1].timestamp;
    currentComposite += quote.changePercent;
    marketOpen = marketOpen || series[symbol].marketOpen;
  }

  snapshotOut.currentMoodBasisPoints =
    marketPercentToBasisPoints(currentComposite / MARKET_SYMBOL_COUNT);
  snapshotOut.marketOpen = marketOpen;
  snapshotOut.fetchedAt = time(nullptr);
  snapshotOut.valid = true;
  return true;
}

bool buildDailySnapshot(
  const MarketProviderSeries *series,
  MarketSnapshot &snapshotOut
) {
  for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
    if (!series[symbol].valid) {
      return false;
    }
  }

  // First count timestamps shared by all three symbols. A second pass starts
  // at the final 31 shared closes, which is enough to produce 30 sessions.
  // This avoids the alpha2 commonTimes/commonCloses arrays on the task stack.
  size_t commonCount = 0;

  for (size_t index = 0; index < series[0].pointCount; ++index) {
    if (!series[0].closeValid[index]) {
      continue;
    }

    const time_t timestamp = series[0].timestamps[index];
    bool shared = true;

    for (size_t symbol = 1; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      if (marketSeriesIndexAt(series[symbol], timestamp) < 0) {
        shared = false;
        break;
      }
    }

    if (shared) {
      ++commonCount;
    }
  }

  if (commonCount < 2) {
    return false;
  }

  const size_t recordsNeeded = MARKET_30_DAY_MAX_POINTS + 1;
  const size_t sharedToSkip = commonCount > recordsNeeded
    ? commonCount - recordsNeeded
    : 0;

  size_t sharedSeen = 0;
  bool havePrevious = false;
  float previousClose[MARKET_SYMBOL_COUNT] {};
  double cumulative = 1.0;
  snapshotOut.thirtyDayCount = 0;

  for (size_t index = 0; index < series[0].pointCount; ++index) {
    if (!series[0].closeValid[index]) {
      continue;
    }

    const time_t timestamp = series[0].timestamps[index];
    int matched[MARKET_SYMBOL_COUNT] = {
      static_cast<int>(index), -1, -1
    };

    for (size_t symbol = 1; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      matched[symbol] = marketSeriesIndexAt(series[symbol], timestamp);
    }

    if (matched[1] < 0 || matched[2] < 0) {
      continue;
    }

    if (sharedSeen++ < sharedToSkip) {
      continue;
    }

    float currentClose[MARKET_SYMBOL_COUNT] {};

    for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      currentClose[symbol] = series[symbol].closes[matched[symbol]];
    }

    if (!havePrevious) {
      memcpy(previousClose, currentClose, sizeof(previousClose));
      havePrevious = true;
      continue;
    }

    double dailyComposite = 0.0;
    bool usable = true;

    for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
      if (previousClose[symbol] <= 0.0f || currentClose[symbol] <= 0.0f) {
        usable = false;
        break;
      }

      dailyComposite += currentClose[symbol] / previousClose[symbol] - 1.0;
    }

    memcpy(previousClose, currentClose, sizeof(previousClose));

    if (!usable) {
      continue;
    }

    dailyComposite /= MARKET_SYMBOL_COUNT;
    cumulative *= 1.0 + dailyComposite;

    if (snapshotOut.thirtyDayCount >= MARKET_30_DAY_MAX_POINTS) {
      break;
    }

    MarketGraphPoint &point =
      snapshotOut.thirtyDay[snapshotOut.thirtyDayCount++];
    point.timestamp = timestamp;
    point.moodBasisPoints = marketPercentToBasisPoints(
      (cumulative - 1.0) * 100.0
    );
  }

  if (snapshotOut.thirtyDayCount == 0) {
    return false;
  }

  snapshotOut.thirtyDayMoodBasisPoints =
    snapshotOut.thirtyDay[snapshotOut.thirtyDayCount - 1].moodBasisPoints;
  snapshotOut.dailyFetchedAt = time(nullptr);
  return true;
}

bool fetchMarketIntraday() {
  MarketProviderSeries *series =
    new (std::nothrow) MarketProviderSeries[MARKET_SYMBOL_COUNT]();

  if (series == nullptr) {
    marketError = "Not enough memory for intraday market data";
    return false;
  }

  for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
    String error;

    if (!fetchMarketSeries(
          MARKET_SYMBOLS[symbol],
          "5m",
          "1d",
          series[symbol],
          error
        )) {
      marketError = String(MARKET_SYMBOLS[symbol]) + F(": ") + error;
      delete[] series;
      return false;
    }

    delay(1);
  }

  MarketSnapshot *updated =
    new (std::nothrow) MarketSnapshot(marketSnapshot);

  if (updated == nullptr) {
    delete[] series;
    marketError = "Not enough memory to combine intraday market data";
    return false;
  }

  const bool combined = buildIntradaySnapshot(series, *updated);
  delete[] series;

  if (!combined) {
    delete updated;
    marketError = "Could not combine intraday market series";
    return false;
  }

  marketSnapshot = *updated;
  delete updated;
  marketError = "";
  saveMarketCache();

  Serial.printf(
    "Market mood: %+.2f%%; today points=%u; open=%d\n",
    marketSnapshot.currentMoodBasisPoints / 100.0,
    marketSnapshot.todayCount,
    marketSnapshot.marketOpen
  );

  return true;
}

bool fetchMarketDaily() {
  MarketProviderSeries *series =
    new (std::nothrow) MarketProviderSeries[MARKET_SYMBOL_COUNT]();

  if (series == nullptr) {
    marketError = "Not enough memory for daily market data";
    return false;
  }

  for (size_t symbol = 0; symbol < MARKET_SYMBOL_COUNT; ++symbol) {
    String error;

    if (!fetchMarketSeries(
          MARKET_SYMBOLS[symbol],
          "1d",
          "3mo",
          series[symbol],
          error
        )) {
      marketError = String(MARKET_SYMBOLS[symbol]) + F(" history: ") + error;
      delete[] series;
      return false;
    }

    delay(1);
  }

  MarketSnapshot *updated =
    new (std::nothrow) MarketSnapshot(marketSnapshot);

  if (updated == nullptr) {
    delete[] series;
    marketError = "Not enough memory to combine daily market data";
    return false;
  }

  const bool combined = buildDailySnapshot(series, *updated);
  delete[] series;

  if (!combined) {
    delete updated;
    marketError = "Could not combine daily market series";
    return false;
  }

  marketSnapshot = *updated;
  delete updated;
  marketError = "";
  saveMarketCache();

  Serial.printf(
    "Market 30-session graph: %+.2f%%; points=%u\n",
    marketSnapshot.thirtyDayMoodBasisPoints / 100.0,
    marketSnapshot.thirtyDayCount
  );

  return true;
}

bool marketDailyIsStaleInternal() {
  if (marketSnapshot.thirtyDayCount == 0 || marketSnapshot.dailyFetchedAt <= 0) {
    return true;
  }

  const time_t now = time(nullptr);
  return
    now <= 0 ||
    now - marketSnapshot.dailyFetchedAt >=
      static_cast<time_t>(MARKET_DAILY_REFRESH_MS / 1000UL);
}

} // namespace

bool loadMarketSettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, true)) {
    marketSettings = MarketSettings();
    return false;
  }

  const bool explicitlyConfigured = preferences.getBool(
    PREF_KEY_MARKET_CONFIGURED,
    false
  );

  marketSettings.enabled = explicitlyConfigured
    ? preferences.getBool(
        PREF_KEY_MARKET_ENABLED,
        MARKET_ENABLED_DEFAULT
      )
    : MARKET_ENABLED_DEFAULT;

  preferences.end();
  return true;
}

bool saveMarketSettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, false)) {
    return false;
  }

  const bool enabledSaved = preferences.putBool(
    PREF_KEY_MARKET_ENABLED,
    marketSettings.enabled
  ) > 0;

  const bool configuredSaved = preferences.putBool(
    PREF_KEY_MARKET_CONFIGURED,
    true
  ) > 0;

  preferences.end();
  return enabledSaved && configuredSaved;
}

bool marketFeatureAvailable() {
  return marketSettings.enabled;
}

bool marketDataAvailable() {
  return marketSnapshot.valid && marketSnapshot.todayCount > 0;
}

bool marketDataIsStale() {
  if (!marketDataAvailable()) {
    return true;
  }

  const time_t now = time(nullptr);
  return
    now <= 0 ||
    marketSnapshot.fetchedAt <= 0 ||
    now - marketSnapshot.fetchedAt >=
      static_cast<time_t>(MARKET_REFRESH_MS / 1000UL);
}

void initializeMarketService() {
  marketRefreshRequested = false;
  marketDailyRefreshRequested = false;
  marketError = "";

  if (!marketFeatureAvailable() || !sdReady || !ensureMarketDirectory()) {
    return;
  }

  loadMarketCache();

  Serial.printf(
    "Market cache: valid=%d today=%u thirty=%u enabled=%d\n",
    marketSnapshot.valid,
    marketSnapshot.todayCount,
    marketSnapshot.thirtyDayCount,
    marketSettings.enabled
  );
}

void requestMarketRefresh(bool includeDaily) {
  if (!marketFeatureAvailable()) {
    return;
  }

  marketRefreshRequested = true;
  marketDailyRefreshRequested = marketDailyRefreshRequested || includeDaily;
}

bool marketRefreshIsPending() {
  return marketRefreshRequested || marketDailyRefreshRequested;
}

const String &marketLastError() {
  return marketError;
}

MarketMoodLevel marketMoodLevelForBasisPoints(int16_t basisPoints) {
  if (basisPoints >= 75) {
    return MarketMoodLevel::VeryUpbeat;
  }

  if (basisPoints >= 20) {
    return MarketMoodLevel::Upbeat;
  }

  if (basisPoints > -20) {
    return MarketMoodLevel::Neutral;
  }

  if (basisPoints > -75) {
    return MarketMoodLevel::Uneasy;
  }

  return MarketMoodLevel::Distressed;
}

MarketMoodLevel currentMarketMoodLevel() {
  return marketDataAvailable()
    ? marketMoodLevelForBasisPoints(marketSnapshot.currentMoodBasisPoints)
    : MarketMoodLevel::Unknown;
}

const char *marketMoodLabel(MarketMoodLevel level) {
  switch (level) {
    case MarketMoodLevel::VeryUpbeat: return "VERY UPBEAT";
    case MarketMoodLevel::Upbeat: return "UPBEAT";
    case MarketMoodLevel::Neutral: return "NEUTRAL";
    case MarketMoodLevel::Uneasy: return "UNEASY";
    case MarketMoodLevel::Distressed: return "DISTRESSED";
    case MarketMoodLevel::Unknown:
    default: return "WAITING";
  }
}

uint16_t marketMoodColor(MarketMoodLevel level) {
  switch (level) {
    case MarketMoodLevel::VeryUpbeat: return TFT_GREEN;
    case MarketMoodLevel::Upbeat: return 0xAFE5;
    case MarketMoodLevel::Neutral: return TFT_YELLOW;
    case MarketMoodLevel::Uneasy: return TFT_ORANGE;
    case MarketMoodLevel::Distressed: return TFT_RED;
    case MarketMoodLevel::Unknown:
    default: return TFT_LIGHTGREY;
  }
}

String marketPercentText(int16_t basisPoints) {
  const float value = basisPoints / 100.0f;
  String text;

  if (value > 0.0001f) {
    text += '+';
  }

  text += String(value, 2);
  text += '%';
  return text;
}

String marketAgeText() {
  return marketDataAvailable()
    ? formatAge(marketSnapshot.fetchedAt)
    : "No cached market data";
}

void drawMarketMoodFace(
  int centerX,
  int centerY,
  int radius,
  MarketMoodLevel level,
  uint16_t backgroundColor
) {
  const uint16_t faceColor = marketMoodColor(level);
  lcd.fillCircle(centerX, centerY, radius, faceColor);
  lcd.drawCircle(centerX, centerY, radius, TFT_WHITE);

  if (level == MarketMoodLevel::Unknown) {
    lcd.setFont(&fonts::Font0);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(TFT_BLACK, faceColor);
    lcd.drawString("?", centerX, centerY);
    return;
  }

  const int eyeY = centerY - max(2, radius / 3);
  const int eyeOffset = max(3, radius / 3);
  lcd.fillCircle(centerX - eyeOffset, eyeY, 1, TFT_BLACK);
  lcd.fillCircle(centerX + eyeOffset, eyeY, 1, TFT_BLACK);

  const int mouthHalf = max(3, radius / 2);
  const int mouthY = centerY + max(2, radius / 3);

  switch (level) {
    case MarketMoodLevel::VeryUpbeat:
      lcd.drawLine(
        centerX - mouthHalf,
        mouthY - 2,
        centerX,
        mouthY + 3,
        TFT_BLACK
      );
      lcd.drawLine(
        centerX,
        mouthY + 3,
        centerX + mouthHalf,
        mouthY - 2,
        TFT_BLACK
      );
      lcd.drawFastHLine(
        centerX - max(2, radius / 3),
        mouthY + 3,
        max(5, radius * 2 / 3 + 1),
        TFT_BLACK
      );
      break;

    case MarketMoodLevel::Upbeat:
      lcd.drawLine(
        centerX - mouthHalf,
        mouthY - 1,
        centerX,
        mouthY + 2,
        TFT_BLACK
      );
      lcd.drawLine(
        centerX,
        mouthY + 2,
        centerX + mouthHalf,
        mouthY - 1,
        TFT_BLACK
      );
      break;

    case MarketMoodLevel::Neutral:
      lcd.drawFastHLine(
        centerX - mouthHalf,
        mouthY,
        mouthHalf * 2 + 1,
        TFT_BLACK
      );
      break;

    case MarketMoodLevel::Uneasy:
      lcd.drawLine(
        centerX - mouthHalf,
        mouthY + 2,
        centerX,
        mouthY - 1,
        TFT_BLACK
      );
      lcd.drawLine(
        centerX,
        mouthY - 1,
        centerX + mouthHalf,
        mouthY + 2,
        TFT_BLACK
      );
      break;

    case MarketMoodLevel::Distressed:
      lcd.drawLine(
        centerX - eyeOffset - 2,
        eyeY - 2,
        centerX - eyeOffset + 2,
        eyeY,
        TFT_BLACK
      );
      lcd.drawLine(
        centerX + eyeOffset - 2,
        eyeY,
        centerX + eyeOffset + 2,
        eyeY - 2,
        TFT_BLACK
      );
      lcd.drawCircle(
        centerX,
        mouthY,
        max(2, radius / 4),
        TFT_BLACK
      );
      break;

    case MarketMoodLevel::Unknown:
    default:
      break;
  }

  (void)backgroundColor;
}

void renderMarketButtonOverlay() {
  if (
    touchUiIsOpen() ||
    !marketFeatureAvailable() ||
    !touchCalibrationIsReady()
  ) {
    return;
  }

  const uint16_t fill = 0x2104;

  lcd.fillRoundRect(
    MARKET_BUTTON_X,
    MARKET_BUTTON_Y,
    MARKET_BUTTON_W,
    MARKET_BUTTON_H,
    5,
    fill
  );

  lcd.drawRoundRect(
    MARKET_BUTTON_X,
    MARKET_BUTTON_Y,
    MARKET_BUTTON_W,
    MARKET_BUTTON_H,
    5,
    marketMoodColor(currentMarketMoodLevel())
  );

  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_WHITE, fill);
  lcd.drawString(
    "$",
    MARKET_BUTTON_X + 20,
    MARKET_BUTTON_Y + MARKET_BUTTON_H / 2
  );

  drawMarketMoodFace(
    MARKET_BUTTON_X + 47,
    MARKET_BUTTON_Y + MARKET_BUTTON_H / 2,
    10,
    currentMarketMoodLevel(),
    fill
  );
}

void serviceMarket() {
  if (!marketFeatureAvailable()) {
    marketRefreshRequested = false;
    marketDailyRefreshRequested = false;

    if (touchUiMarketPageIsOpen()) {
      closeTouchUi(true);
    }

    return;
  }

  const unsigned long now = millis();
  const bool intradayDue = marketRefreshRequested || marketDataIsStale();
  const bool dailyDue = marketDailyRefreshRequested || marketDailyIsStaleInternal();

  if (
    (intradayDue || dailyDue) &&
    WiFi.status() == WL_CONNECTED &&
    sdReady &&
    !renderState.fullRedrawInProgress &&
    now >= MARKET_INITIAL_FETCH_DELAY_MS &&
    (
      marketRefreshRequested ||
      marketDailyRefreshRequested ||
      lastMarketAttemptAt == 0 ||
      now - lastMarketAttemptAt >= MARKET_RETRY_MS
    )
  ) {
    lastMarketAttemptAt = now;
    // Do at most one three-symbol dataset per service pass. Intraday has
    // priority; after it succeeds, the requested/stale daily dataset is left
    // queued for the next trip through loop(). This keeps the web server,
    // touch service, and watchdog responsive between the two HTTPS batches.
    const bool fetchIntraday = intradayDue;
    const bool fetchDaily = !fetchIntraday && dailyDue;
    marketRefreshRequested = false;
    marketDailyRefreshRequested = false;

    bool updated = false;
    String refreshErrors;

    if (fetchIntraday) {
      const bool intradayUpdated = fetchMarketIntraday();
      updated = intradayUpdated;

      if (!intradayUpdated && marketError.length()) {
        refreshErrors = marketError;
      } else if (dailyDue) {
        marketDailyRefreshRequested = true;
      }
    } else if (fetchDaily) {
      const bool dailyUpdated = fetchMarketDaily();
      updated = dailyUpdated;

      if (!dailyUpdated && marketError.length()) {
        refreshErrors = marketError;
      }
    }

    if (refreshErrors.length()) {
      marketError = refreshErrors;
    } else if (updated) {
      marketError = "";
    }

    if (touchUiMarketPageIsOpen()) {
      touchUiHandleMarketUpdated();
    } else if (updated && !touchUiIsOpen()) {
      renderMarketButtonOverlay();
    }
  }
}
