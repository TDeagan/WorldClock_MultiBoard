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
    pngWriteFailed ||
    !pngOutputFile ||
    draw == nullptr ||
    draw->iWidth != MAP_W ||
    draw->y < 0 ||
    draw->y >= MAP_H
  ) {
    pngWriteFailed = true;
    return 0;
  }

  // Creates normal ESP32 host-order RGB565 values.
  // The raw cache is therefore written little-endian.
  png.getLineAsRGB565(
    draw,
    pngLine,
    PNG_RGB565_LITTLE_ENDIAN,
    0x00000000
  );

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


bool convertPngToRaw(
  const char *pngPath,
  const char *rawPath,
  const String &screenLabel
) {
  showStatus(
    screenLabel,
    "Creating RGB565 cache"
  );

  if (!SD.exists(pngPath)) {
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
  if (!rawMapIsValid(DAY_RAW_FILE)) {
    if (!convertPngToRaw(
          DAY_PNG_FILE,
          DAY_RAW_FILE,
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

  dayFile = SD.open(DAY_RAW_FILE, FILE_READ);
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

  if (!ensureRawMapCaches()) {
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

  systemStatus.dayPngFound =
    SD.exists(DAY_PNG_FILE);

  systemStatus.nightPngFound =
    SD.exists(NIGHT_PNG_FILE);

  systemStatus.dayCacheValid =
    rawMapIsValid(DAY_RAW_FILE);

  systemStatus.nightCacheValid =
    rawMapIsValid(NIGHT_RAW_FILE);

  if (!systemStatus.dayPngFound) {
    setSystemError(
      SystemError::DayPngMissing,
      "earth_day.png is missing"
    );
  } else if (!systemStatus.nightPngFound) {
    setSystemError(
      SystemError::NightPngMissing,
      "earth_night.png is missing"
    );
  } else if (!systemStatus.dayCacheValid) {
    setSystemError(
      SystemError::DayCacheInvalid,
      "Day RGB565 cache is invalid"
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
    if (SD.exists(DAY_RAW_FILE)) {
      SD.remove(DAY_RAW_FILE);
    }

    ok = convertPngToRaw(
      DAY_PNG_FILE,
      DAY_RAW_FILE,
      "Rebuilding day map"
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
