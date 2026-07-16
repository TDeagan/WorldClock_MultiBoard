namespace {

String fileNameOnly(const String &path) {
  const int slash = path.lastIndexOf('/');
  return slash >= 0 ? path.substring(slash + 1) : path;
}

bool hasPngExtension(const String &filename) {
  String lower = filename;
  lower.toLowerCase();
  return lower.endsWith(".png");
}

bool copySdFile(const char *sourcePath, const char *destinationPath) {
  File source = SD.open(sourcePath, FILE_READ);

  if (!source) {
    return false;
  }

  if (SD.exists(destinationPath)) {
    SD.remove(destinationPath);
  }

  File destination = SD.open(destinationPath, FILE_WRITE);

  if (!destination) {
    source.close();
    return false;
  }

  uint8_t buffer[1024];
  bool ok = true;

  while (source.available()) {
    const size_t bytesRead = source.read(buffer, sizeof(buffer));

    if (
      bytesRead == 0 ||
      destination.write(buffer, bytesRead) != bytesRead
    ) {
      ok = false;
      break;
    }
  }

  destination.flush();
  destination.close();
  source.close();

  if (!ok) {
    SD.remove(destinationPath);
  }

  return ok;
}

void ensureDayMapDirectory() {
  File directory = SD.open(DAY_MAP_DIRECTORY, FILE_READ);
  const bool exists = directory && directory.isDirectory();

  if (directory) {
    directory.close();
  }

  if (!exists) {
    SD.mkdir(DAY_MAP_DIRECTORY);
  }
}

void migrateLegacyDayMapIfNeeded() {
  const String destination =
    dayMapPngPath(DEFAULT_DAY_MAP_FILENAME);

  if (
    !SD.exists(destination.c_str()) &&
    SD.exists(LEGACY_DAY_PNG_FILE)
  ) {
    showStatus(
      "Migrating day map",
      "Copying it into /maps"
    );

    copySdFile(
      LEGACY_DAY_PNG_FILE,
      destination.c_str()
    );
  }
}

bool findFallbackDayMap(String &filenameOut) {
  filenameOut = "";

  File directory = SD.open(DAY_MAP_DIRECTORY, FILE_READ);

  if (!directory || !directory.isDirectory()) {
    if (directory) {
      directory.close();
    }

    return false;
  }

  String firstValid;
  bool defaultValid = false;

  File entry = directory.openNextFile();

  while (entry) {
    const bool isDirectory = entry.isDirectory();
    const String filename = fileNameOnly(entry.name());
    entry.close();

    if (
      !isDirectory &&
      isSafeDayMapFilename(filename)
    ) {
      const String path = dayMapPngPath(filename);

      if (pngMapIsValid(path.c_str(), true)) {
        if (filename == DEFAULT_DAY_MAP_FILENAME) {
          defaultValid = true;
        }

        if (
          firstValid.length() == 0 ||
          filename.compareTo(firstValid) < 0
        ) {
          firstValid = filename;
        }
      }
    }

    entry = directory.openNextFile();
  }

  directory.close();

  if (defaultValid) {
    filenameOut = DEFAULT_DAY_MAP_FILENAME;
  } else {
    filenameOut = firstValid;
  }

  return filenameOut.length() > 0;
}

} // namespace


void *pngOpenCallback(
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


void pngCloseCallback(void *handle) {
  (void)handle;

  if (pngInputFile) {
    pngInputFile.close();
  }
}


int32_t pngReadCallback(
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


int32_t pngSeekCallback(
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


int pngDrawCallback(PNGDRAW *draw) {
  if (
    draw == nullptr ||
    draw->iWidth != MAP_W ||
    draw->y < 0 ||
    draw->y >= MAP_H
  ) {
    pngWriteFailed = true;
    return 0;
  }

  // Decode every line during both validation and cache creation. This catches
  // corrupt image data and CRC failures rather than validating only the header.
  png.getLineAsRGB565(
    draw,
    pngLine,
    PNG_RGB565_LITTLE_ENDIAN,
    0x00000000
  );

  if (pngValidationOnly) {
    return 1;
  }

  if (pngWriteFailed || !pngOutputFile) {
    pngWriteFailed = true;
    return 0;
  }

  const size_t required =
    MAP_W * sizeof(uint16_t);

  const size_t written =
    pngOutputFile.write(
      reinterpret_cast<const uint8_t *>(pngLine),
      required
    );

  if (written != required) {
    pngWriteFailed = true;
    return 0;
  }

  return 1;
}



namespace {

int pngPreviewTopY = 0;
int pngPreviewHeight = 0;
bool pngPreviewFailed = false;

int pngPreviewDrawCallback(PNGDRAW *draw) {
  if (
    draw == nullptr ||
    draw->iWidth != MAP_W ||
    draw->y < 0 ||
    draw->y >= MAP_H ||
    pngPreviewHeight <= 0
  ) {
    pngPreviewFailed = true;
    return 0;
  }

  png.getLineAsRGB565(
    draw,
    pngLine,
    PNG_RGB565_LITTLE_ENDIAN,
    0x00000000
  );

  const int destinationY0 =
    pngPreviewTopY +
    (draw->y * pngPreviewHeight) / MAP_H;

  const int destinationY1 =
    pngPreviewTopY +
    ((draw->y + 1) * pngPreviewHeight) / MAP_H;

  for (
    int destinationY = destinationY0;
    destinationY < destinationY1;
    ++destinationY
  ) {
    if (
      destinationY >= 0 &&
      destinationY < SCREEN_H
    ) {
      lcd.pushImage(
        0,
        destinationY,
        MAP_W,
        1,
        pngLine
      );
    }
  }

  return 1;
}

} // namespace


bool renderPngPreview(
  const char *path,
  int topY,
  int height
) {
  if (
    path == nullptr ||
    height <= 0 ||
    !SD.exists(path)
  ) {
    return false;
  }

  pngPreviewTopY = topY;
  pngPreviewHeight = height;
  pngPreviewFailed = false;

  const int openResult = png.open(
    path,
    pngOpenCallback,
    pngCloseCallback,
    pngReadCallback,
    pngSeekCallback,
    pngPreviewDrawCallback
  );

  if (openResult != PNG_SUCCESS) {
    png.close();
    return false;
  }

  bool valid =
    png.getWidth() == MAP_W &&
    png.getHeight() == MAP_H &&
    !png.isInterlaced();

  if (valid) {
    lcd.startWrite();

    valid =
      png.decode(nullptr, PNG_CHECK_CRC) == PNG_SUCCESS &&
      !pngPreviewFailed;

    lcd.endWrite();
  }

  png.close();
  return valid;
}


bool rawMapIsValid(const char *path) {
  File file = SD.open(path, FILE_READ);

  if (!file) {
    return false;
  }

  const bool valid =
    file.size() == RAW_MAP_BYTES;

  file.close();
  return valid;
}


bool pngMapIsValid(
  const char *path,
  bool fullDecode
) {
  if (!path || !SD.exists(path)) {
    return false;
  }

  pngValidationOnly = true;
  pngWriteFailed = false;

  const int openResult = png.open(
    path,
    pngOpenCallback,
    pngCloseCallback,
    pngReadCallback,
    pngSeekCallback,
    pngDrawCallback
  );

  if (openResult != PNG_SUCCESS) {
    png.close();
    pngValidationOnly = false;
    return false;
  }

  bool valid =
    png.getWidth() == MAP_W &&
    png.getHeight() == MAP_H &&
    !png.isInterlaced();

  if (valid && fullDecode) {
    valid =
      png.decode(nullptr, PNG_CHECK_CRC) == PNG_SUCCESS &&
      !pngWriteFailed;
  }

  png.close();
  pngValidationOnly = false;
  return valid;
}


bool isSafeDayMapFilename(
  const String &filename
) {
  return
    filename.length() > 4 &&
    filename.length() <= MAX_DAY_MAP_FILENAME_LENGTH &&
    filename[0] != '.' &&
    filename.indexOf('/') < 0 &&
    filename.indexOf('\\') < 0 &&
    filename.indexOf("..") < 0 &&
    hasPngExtension(filename);
}


String dayMapPngPath(
  const String &filename
) {
  return String(DAY_MAP_DIRECTORY) + "/" + filename;
}


String dayMapCachePath(
  const String &filename
) {
  String base = filename;
  base.remove(base.length() - 4);
  return String(DAY_MAP_DIRECTORY) + "/" + base + ".rgb565";
}


size_t scanDaylightMaps(
  DaylightMapInfo *maps,
  size_t maximumCount,
  bool fullValidation,
  size_t *totalCountOut
) {
  if (totalCountOut) {
    *totalCountOut = 0;
  }

  if (!maps || maximumCount == 0) {
    return 0;
  }

  File directory = SD.open(DAY_MAP_DIRECTORY, FILE_READ);

  if (!directory || !directory.isDirectory()) {
    if (directory) {
      directory.close();
    }

    return 0;
  }

  size_t count = 0;
  size_t totalCount = 0;
  File entry = directory.openNextFile();

  while (entry) {
    const bool isDirectory = entry.isDirectory();
    const String filename = fileNameOnly(entry.name());
    const uint32_t pngBytes =
      isDirectory ? 0 : static_cast<uint32_t>(entry.size());
    entry.close();

    if (
      !isDirectory &&
      isSafeDayMapFilename(filename)
    ) {
      ++totalCount;

      size_t destination = count;

      if (count < maximumCount) {
        ++count;
      } else {
        // Keep the alphabetically first entries when the directory contains
        // more maps than the web page can safely render in one response.
        size_t largest = 0;

        for (size_t index = 1; index < count; ++index) {
          if (
            maps[index].filename.compareTo(
              maps[largest].filename
            ) > 0
          ) {
            largest = index;
          }
        }

        if (filename.compareTo(maps[largest].filename) >= 0) {
          entry = directory.openNextFile();
          continue;
        }

        destination = largest;
      }

      maps[destination] = DaylightMapInfo();
      maps[destination].filename = filename;
      maps[destination].pngBytes = pngBytes;
    }

    entry = directory.openNextFile();
  }

  directory.close();

  // Stable alphabetical ordering makes fallback and web-page ordering
  // predictable even when the FAT directory order changes.
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      if (maps[j].filename.compareTo(maps[i].filename) < 0) {
        DaylightMapInfo temporary = maps[i];
        maps[i] = maps[j];
        maps[j] = temporary;
      }
    }
  }

  for (size_t index = 0; index < count; ++index) {
    DaylightMapInfo &map = maps[index];
    map.pngPath = dayMapPngPath(map.filename);
    map.rawPath = dayMapCachePath(map.filename);
    map.rawBytes = fileSizeOrZero(map.rawPath.c_str());
    map.pngValid = pngMapIsValid(
      map.pngPath.c_str(),
      fullValidation
    );
    map.cacheValid = rawMapIsValid(map.rawPath.c_str());
    map.selected = map.filename == selectedDayMapFilename;
  }

  if (totalCountOut) {
    *totalCountOut = totalCount;
  }

  return count;
}


bool loadSelectedDayMapPreference() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, true)) {
    selectedDayMapFilename = DEFAULT_DAY_MAP_FILENAME;
    return false;
  }

  const String loaded = preferences.getString(
    PREF_KEY_SELECTED_DAY_MAP,
    DEFAULT_DAY_MAP_FILENAME
  );

  preferences.end();

  selectedDayMapFilename =
    isSafeDayMapFilename(loaded)
      ? loaded
      : String(DEFAULT_DAY_MAP_FILENAME);

  activeDayPngPath =
    dayMapPngPath(selectedDayMapFilename);

  activeDayRawPath =
    dayMapCachePath(selectedDayMapFilename);

  return true;
}


bool saveSelectedDayMapPreference(
  const String &filename
) {
  if (!isSafeDayMapFilename(filename)) {
    return false;
  }

  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, false)) {
    return false;
  }

  const bool ok =
    preferences.putString(
      PREF_KEY_SELECTED_DAY_MAP,
      filename
    ) > 0;

  preferences.end();
  return ok;
}


bool ensureSelectedDayMapAvailable(
  bool persistFallback
) {
  if (
    isSafeDayMapFilename(selectedDayMapFilename) &&
    pngMapIsValid(activeDayPngPath.c_str(), false)
  ) {
    return true;
  }

  String fallback;

  if (!findFallbackDayMap(fallback)) {
    return false;
  }

  selectedDayMapFilename = fallback;
  activeDayPngPath = dayMapPngPath(fallback);
  activeDayRawPath = dayMapCachePath(fallback);

  if (persistFallback) {
    saveSelectedDayMapPreference(fallback);
  }

  return true;
}


bool convertPngToRaw(
  const char *pngPath,
  const char *rawPath,
  const String &screenLabel
) {
  showStatus(
    screenLabel,
    "Creating RGB565 cache"
  );

  if (!pngMapIsValid(pngPath, false)) {
    return false;
  }

  // FILE_WRITE appends on ESP32, so remove any old or partial cache first.
  if (SD.exists(rawPath)) {
    SD.remove(rawPath);
  }

  pngOutputFile = SD.open(rawPath, FILE_WRITE);

  if (!pngOutputFile) {
    return false;
  }

  pngValidationOnly = false;
  pngWriteFailed = false;

  int result = png.open(
    pngPath,
    pngOpenCallback,
    pngCloseCallback,
    pngReadCallback,
    pngSeekCallback,
    pngDrawCallback
  );

  if (result != PNG_SUCCESS) {
    pngOutputFile.close();
    png.close();
    SD.remove(rawPath);
    return false;
  }

  if (
    png.getWidth() != MAP_W ||
    png.getHeight() != MAP_H ||
    png.isInterlaced()
  ) {
    pngOutputFile.close();
    png.close();
    SD.remove(rawPath);
    return false;
  }

  result = png.decode(nullptr, PNG_CHECK_CRC);

  png.close();
  pngOutputFile.flush();
  pngOutputFile.close();

  const bool valid =
    result == PNG_SUCCESS &&
    !pngWriteFailed &&
    rawMapIsValid(rawPath);

  if (!valid) {
    SD.remove(rawPath);
  }

  return valid;
}


bool ensureRawMapCaches() {
  if (!rawMapIsValid(activeDayRawPath.c_str())) {
    if (!convertPngToRaw(
          activeDayPngPath.c_str(),
          activeDayRawPath.c_str(),
          "Converting day map"
        )) {
      return false;
    }
  }

  if (!rawMapIsValid(NIGHT_RAW_FILE)) {
    if (!convertPngToRaw(
          NIGHT_PNG_FILE,
          NIGHT_RAW_FILE,
          "Converting night map"
        )) {
      return false;
    }
  }

  return true;
}


void closeMapFiles() {
  if (dayFile) {
    dayFile.close();
  }

  if (nightFile) {
    nightFile.close();
  }
}


bool openMapFiles() {
  closeMapFiles();

  dayFile = SD.open(activeDayRawPath.c_str(), FILE_READ);
  nightFile = SD.open(NIGHT_RAW_FILE, FILE_READ);

  if (!dayFile || !nightFile) {
    closeMapFiles();
    return false;
  }

  if (
    dayFile.size() != RAW_MAP_BYTES ||
    nightFile.size() != RAW_MAP_BYTES
  ) {
    closeMapFiles();
    return false;
  }

  return true;
}


bool initializeSD() {
  closeMapFiles();

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  sdSPI.end();

  sdSPI.begin(
    SD_SCLK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );

  if (!SD.begin(SD_CS, sdSPI, 10000000)) {
    systemStatus.sdMounted = false;
    setSystemError(
      SystemError::SdUnavailable,
      "microSD mount failed"
    );
    return false;
  }

  systemStatus.sdMounted = true;

  ensureDayMapDirectory();
  migrateLegacyDayMapIfNeeded();
  loadSelectedDayMapPreference();

  if (!ensureSelectedDayMapAvailable(true)) {
    setSystemError(
      SystemError::DayPngMissing,
      "No valid 320 x 240 PNG files were found in /maps"
    );
    refreshStorageStatus();
    return false;
  }

  if (!ensureRawMapCaches()) {
    refreshStorageStatus();
    return false;
  }

  const bool opened = openMapFiles();
  refreshStorageStatus();
  return opened;
}


bool readMapLine(
  File &file,
  int sourceY,
  uint16_t *buffer
) {
  if (!file) {
    return false;
  }

  const uint32_t offset =
    static_cast<uint32_t>(sourceY) *
    MAP_W *
    sizeof(uint16_t);

  if (!file.seek(offset)) {
    return false;
  }

  const size_t bytesNeeded =
    MAP_W * sizeof(uint16_t);

  return file.read(
    reinterpret_cast<uint8_t *>(buffer),
    bytesNeeded
  ) == bytesNeeded;
}


bool recoverSD() {
  sdReady = initializeSD();

  if (sdReady) {
    showStatus(
      "microSD restored",
      "Redrawing map"
    );
    delay(400);
  }

  return sdReady;
}


uint32_t fileSizeOrZero(
  const char *path
) {
  File file = SD.open(path, FILE_READ);

  if (!file) {
    return 0;
  }

  const uint32_t size = file.size();
  file.close();
  return size;
}


bool refreshStorageStatus() {
  systemStatus.sdMounted =
    SD.cardType() != CARD_NONE;

  if (!systemStatus.sdMounted) {
    systemStatus.dayPngFound = false;
    systemStatus.nightPngFound = false;
    systemStatus.dayCacheValid = false;
    systemStatus.nightCacheValid = false;
    setSystemError(
      SystemError::SdUnavailable,
      "microSD is not mounted"
    );
    return false;
  }

  // If the chosen file was removed, choose the default map when available,
  // otherwise the alphabetically first valid PNG in /maps.
  if (!pngMapIsValid(activeDayPngPath.c_str(), false)) {
    const String previousSelection =
      selectedDayMapFilename;

    closeMapFiles();

    const bool recovered =
      ensureSelectedDayMapAvailable(true) &&
      ensureRawMapCaches() &&
      openMapFiles();

    sdReady = recovered;

    if (
      recovered &&
      timeValid &&
      selectedDayMapFilename != previousSelection
    ) {
      redrawWorldClock();
    }
  }

  systemStatus.dayPngFound =
    SD.exists(activeDayPngPath.c_str());

  systemStatus.nightPngFound =
    SD.exists(NIGHT_PNG_FILE);

  systemStatus.dayCacheValid =
    rawMapIsValid(activeDayRawPath.c_str());

  systemStatus.nightCacheValid =
    rawMapIsValid(NIGHT_RAW_FILE);

  if (!systemStatus.dayPngFound) {
    setSystemError(
      SystemError::DayPngMissing,
      "No usable daylight PNG is available in /maps"
    );
  } else if (!pngMapIsValid(activeDayPngPath.c_str(), false)) {
    setSystemError(
      SystemError::DayPngMissing,
      selectedDayMapFilename + " is not a valid 320 x 240 PNG"
    );
  } else if (!systemStatus.nightPngFound) {
    setSystemError(
      SystemError::NightPngMissing,
      "earth_night.png is missing"
    );
  } else if (!pngMapIsValid(NIGHT_PNG_FILE, false)) {
    setSystemError(
      SystemError::NightPngMissing,
      "earth_night.png is not a valid 320 x 240 PNG"
    );
  } else if (!systemStatus.dayCacheValid) {
    setSystemError(
      SystemError::DayCacheInvalid,
      selectedDayMapFilename + " RGB565 cache is invalid"
    );
  } else if (!systemStatus.nightCacheValid) {
    setSystemError(
      SystemError::NightCacheInvalid,
      "Night RGB565 cache is invalid"
    );
  } else if (
    systemStatus.lastError == SystemError::SdUnavailable ||
    systemStatus.lastError == SystemError::DayPngMissing ||
    systemStatus.lastError == SystemError::NightPngMissing ||
    systemStatus.lastError == SystemError::DayCacheInvalid ||
    systemStatus.lastError == SystemError::NightCacheInvalid
  ) {
    setSystemError(SystemError::None, "");
  }

  return
    systemStatus.dayCacheValid &&
    systemStatus.nightCacheValid;
}


bool activateDayMap(
  const String &filename,
  bool rebuildCache
) {
  if (!isSafeDayMapFilename(filename)) {
    return false;
  }

  const String pngPath = dayMapPngPath(filename);
  const String rawPath = dayMapCachePath(filename);

  if (!pngMapIsValid(pngPath.c_str(), true)) {
    return false;
  }

  const bool replacingActive =
    filename == selectedDayMapFilename;

  if (rebuildCache && replacingActive) {
    closeMapFiles();
  }

  if (rebuildCache && SD.exists(rawPath.c_str())) {
    SD.remove(rawPath.c_str());
  }

  if (
    !rawMapIsValid(rawPath.c_str()) &&
    !convertPngToRaw(
      pngPath.c_str(),
      rawPath.c_str(),
      String("Caching ") + filename
    )
  ) {
    if (replacingActive) {
      openMapFiles();
    }

    refreshStorageStatus();
    return false;
  }

  const String previousFilename = selectedDayMapFilename;
  const String previousPngPath = activeDayPngPath;
  const String previousRawPath = activeDayRawPath;

  if (!saveSelectedDayMapPreference(filename)) {
    if (replacingActive) {
      openMapFiles();
    }

    return false;
  }

  closeMapFiles();
  selectedDayMapFilename = filename;
  activeDayPngPath = pngPath;
  activeDayRawPath = rawPath;

  if (!openMapFiles()) {
    selectedDayMapFilename = previousFilename;
    activeDayPngPath = previousPngPath;
    activeDayRawPath = previousRawPath;
    saveSelectedDayMapPreference(previousFilename);
    openMapFiles();
    refreshStorageStatus();
    return false;
  }

  sdReady = true;
  refreshStorageStatus();
  redrawWorldClock();
  return true;
}


bool rebuildDayMapCache(
  const String &filename
) {
  if (!isSafeDayMapFilename(filename)) {
    return false;
  }

  const String pngPath = dayMapPngPath(filename);
  const String rawPath = dayMapCachePath(filename);

  if (!pngMapIsValid(pngPath.c_str(), true)) {
    return false;
  }

  const bool active = filename == selectedDayMapFilename;

  if (active) {
    closeMapFiles();
  }

  if (SD.exists(rawPath.c_str())) {
    SD.remove(rawPath.c_str());
  }

  bool ok = convertPngToRaw(
    pngPath.c_str(),
    rawPath.c_str(),
    String("Rebuilding ") + filename
  );

  if (active && ok) {
    ok = openMapFiles();
  } else if (active) {
    openMapFiles();
  }

  sdReady =
    rawMapIsValid(activeDayRawPath.c_str()) &&
    rawMapIsValid(NIGHT_RAW_FILE) &&
    dayFile &&
    nightFile;

  refreshStorageStatus();

  if (active && ok) {
    redrawWorldClock();
  }

  return ok;
}


bool rebuildAllDayMapCaches() {
  if (!systemStatus.sdMounted) {
    return false;
  }

  closeMapFiles();

  File directory = SD.open(DAY_MAP_DIRECTORY, FILE_READ);

  if (!directory || !directory.isDirectory()) {
    if (directory) {
      directory.close();
    }

    return false;
  }

  bool ok = true;
  size_t validCount = 0;
  File entry = directory.openNextFile();

  while (entry) {
    const bool isDirectory = entry.isDirectory();
    const String filename = fileNameOnly(entry.name());
    entry.close();

    if (
      !isDirectory &&
      isSafeDayMapFilename(filename)
    ) {
      const String pngPath = dayMapPngPath(filename);

      if (pngMapIsValid(pngPath.c_str(), true)) {
        ++validCount;
        const String rawPath = dayMapCachePath(filename);

        if (SD.exists(rawPath.c_str())) {
          SD.remove(rawPath.c_str());
        }

        if (!convertPngToRaw(
              pngPath.c_str(),
              rawPath.c_str(),
              String("Caching ") + filename
            )) {
          ok = false;
          break;
        }
      }
    }

    entry = directory.openNextFile();
  }

  directory.close();

  if (validCount == 0) {
    ok = false;
  }

  // Always try to restore the active display files, even if rebuilding an
  // unrelated map failed partway through the directory.
  const bool displayRestored =
    ensureSelectedDayMapAvailable(true) &&
    ensureRawMapCaches() &&
    openMapFiles();

  sdReady = displayRestored;
  refreshStorageStatus();

  if (displayRestored) {
    redrawWorldClock();
  }

  return ok && displayRestored;
}


bool rebuildMapCache(
  bool rebuildDay,
  bool rebuildNight
) {
  if (!systemStatus.sdMounted) {
    return false;
  }

  closeMapFiles();
  bool ok = true;

  if (rebuildDay) {
    if (SD.exists(activeDayRawPath.c_str())) {
      SD.remove(activeDayRawPath.c_str());
    }

    ok = convertPngToRaw(
      activeDayPngPath.c_str(),
      activeDayRawPath.c_str(),
      "Rebuilding selected map"
    );
  }

  if (ok && rebuildNight) {
    if (SD.exists(NIGHT_RAW_FILE)) {
      SD.remove(NIGHT_RAW_FILE);
    }

    ok = convertPngToRaw(
      NIGHT_PNG_FILE,
      NIGHT_RAW_FILE,
      "Rebuilding night map"
    );
  }

  if (ok) {
    ok = openMapFiles();
  }

  sdReady = ok;
  refreshStorageStatus();

  if (ok) {
    redrawWorldClock();
  }

  return ok;
}
