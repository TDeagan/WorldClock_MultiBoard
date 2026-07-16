// ============================================================
// Integrated XPT2046 touchscreen calibration
// ============================================================
//
// Calibration uses raw controller samples directly, so it works before any
// saved calibration exists. Four corner targets establish axis selection,
// direction, and range; a center target verifies the generated mapping before
// the existing TouchCalibration record is written to the "touchtest"
// Preferences namespace.
// ============================================================

namespace {

static constexpr uint8_t TOUCH_CALIBRATION_POINT_COUNT = 4;
static constexpr uint8_t TOUCH_CALIBRATION_RAW_SAMPLES = 9;
static constexpr int TOUCH_CALIBRATION_TARGET_MARGIN = 24;
static constexpr int TOUCH_CALIBRATION_TARGET_RADIUS = 10;
static constexpr int TOUCH_CALIBRATION_CENTER_TOLERANCE = 32;
static constexpr int TOUCH_CALIBRATION_MINIMUM_RAW_SPAN = 500;

struct TouchCalibrationPoint {
  int screenX = 0;
  int screenY = 0;
  uint16_t rawX = 0;
  uint16_t rawY = 0;
  uint16_t pressure = 0;
};

uint16_t calibrationMedian(uint16_t *values, uint8_t count);
void drawCalibrationText(
  const String &line1,
  const String &line2,
  uint16_t color
);
void drawCalibrationTarget(int x, int y, uint16_t color);
void waitForRawTouchRelease();
bool captureCalibrationPoint(
  TouchCalibrationPoint &point,
  uint8_t pointNumber,
  const char *label
);
int32_t averageRawValue(uint16_t first, uint16_t second);
int16_t extrapolateRawEndpoint(
  int32_t rawStart,
  int32_t rawEnd,
  int screenStart,
  int screenEnd,
  int requestedScreen
);
int32_t calibrationRawForScreenX(
  const TouchCalibrationPoint &point,
  bool swapXY
);
int32_t calibrationRawForScreenY(
  const TouchCalibrationPoint &point,
  bool swapXY
);
bool buildTouchCalibration(
  const TouchCalibrationPoint *points,
  TouchCalibration &calibrationOut
);
int mapCalibrationAxis(
  int raw,
  int rawStart,
  int rawEnd,
  int screenMaximum
);
void mapRawWithCalibration(
  const Xpt2046Sample &sample,
  const TouchCalibration &calibration,
  int &screenX,
  int &screenY
);
bool verifyTouchCalibration(
  const TouchCalibration &calibration
);

uint16_t calibrationMedian(
  uint16_t *values,
  uint8_t count
) {
  for (uint8_t i = 1; i < count; ++i) {
    const uint16_t value = values[i];
    int j = static_cast<int>(i) - 1;

    while (
      j >= 0 &&
      values[j] > value
    ) {
      values[j + 1] = values[j];
      --j;
    }

    values[j + 1] = value;
  }

  return values[count / 2];
}

void drawCalibrationText(
  const String &line1,
  const String &line2,
  uint16_t color
) {
  lcd.fillRect(
    38,
    84,
    lcd.width() - 76,
    72,
    TFT_BLACK
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setFont(
    &fonts::Font2
  );

  lcd.setTextColor(
    color,
    TFT_BLACK
  );

  lcd.drawString(
    line1,
    lcd.width() / 2,
    104
  );

  if (line2.length() > 0) {
    lcd.setFont(
      &fonts::Font0
    );

    lcd.setTextColor(
      TFT_CYAN,
      TFT_BLACK
    );

    lcd.drawString(
      line2,
      lcd.width() / 2,
      134
    );
  }
}

void drawCalibrationTarget(
  int x,
  int y,
  uint16_t color
) {
  lcd.drawCircle(
    x,
    y,
    TOUCH_CALIBRATION_TARGET_RADIUS,
    color
  );

  lcd.drawCircle(
    x,
    y,
    TOUCH_CALIBRATION_TARGET_RADIUS - 1,
    color
  );

  lcd.drawFastHLine(
    x - TOUCH_CALIBRATION_TARGET_RADIUS - 5,
    y,
    TOUCH_CALIBRATION_TARGET_RADIUS * 2 + 11,
    color
  );

  lcd.drawFastVLine(
    x,
    y - TOUCH_CALIBRATION_TARGET_RADIUS - 5,
    TOUCH_CALIBRATION_TARGET_RADIUS * 2 + 11,
    color
  );
}

void waitForRawTouchRelease() {
  uint8_t releasedFrames = 0;

  while (releasedFrames < 3) {
    Xpt2046Sample sample;

    if (readRawTouchSample(sample)) {
      releasedFrames = 0;
    } else {
      ++releasedFrames;
    }

    delay(10);
  }
}

bool captureCalibrationPoint(
  TouchCalibrationPoint &point,
  uint8_t pointNumber,
  const char *label
) {
  waitForRawTouchRelease();

  lcd.fillScreen(
    TFT_BLACK
  );

  drawCalibrationText(
    String("Touch ") + label,
    String("Corner ") +
      static_cast<unsigned>(pointNumber) +
      " of 4",
    TFT_WHITE
  );

  drawCalibrationTarget(
    point.screenX,
    point.screenY,
    TFT_YELLOW
  );

  uint16_t xValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint16_t yValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint16_t pressureValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint8_t collected = 0;

  while (
    collected <
      TOUCH_CALIBRATION_RAW_SAMPLES
  ) {
    Xpt2046Sample sample;

    if (
      readRawTouchSample(sample) &&
      sample.pressed
    ) {
      xValues[collected] = sample.rawX;
      yValues[collected] = sample.rawY;
      pressureValues[collected] = sample.pressure;
      ++collected;
    } else if (collected > 0) {
      collected = 0;
      drawCalibrationText(
        String("Touch ") + label,
        "Hold steadily until accepted",
        TFT_WHITE
      );
    }

    delay(8);
  }

  point.rawX = calibrationMedian(
    xValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  point.rawY = calibrationMedian(
    yValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  point.pressure = calibrationMedian(
    pressureValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  waitForRawTouchRelease();

  drawCalibrationTarget(
    point.screenX,
    point.screenY,
    TFT_GREEN
  );

  delay(250);
  return true;
}

int32_t averageRawValue(
  uint16_t first,
  uint16_t second
) {
  return
    (static_cast<int32_t>(first) +
     static_cast<int32_t>(second)) /
    2;
}

int16_t extrapolateRawEndpoint(
  int32_t rawStart,
  int32_t rawEnd,
  int screenStart,
  int screenEnd,
  int requestedScreen
) {
  if (screenStart == screenEnd) {
    return static_cast<int16_t>(rawStart);
  }

  const int32_t result =
    rawStart +
    ((rawEnd - rawStart) *
     (requestedScreen - screenStart)) /
    (screenEnd - screenStart);

  return static_cast<int16_t>(
    constrain(
      result,
      static_cast<int32_t>(-32768),
      static_cast<int32_t>(32767)
    )
  );
}

int32_t calibrationRawForScreenX(
  const TouchCalibrationPoint &point,
  bool swapXY
) {
  return swapXY
    ? point.rawY
    : point.rawX;
}

int32_t calibrationRawForScreenY(
  const TouchCalibrationPoint &point,
  bool swapXY
) {
  return swapXY
    ? point.rawX
    : point.rawY;
}

bool buildTouchCalibration(
  const TouchCalibrationPoint *points,
  TouchCalibration &calibrationOut
) {
  // Point order is top-left, top-right, bottom-right, bottom-left.
  const int32_t leftRawX = averageRawValue(
    points[0].rawX,
    points[3].rawX
  );

  const int32_t rightRawX = averageRawValue(
    points[1].rawX,
    points[2].rawX
  );

  const int32_t leftRawY = averageRawValue(
    points[0].rawY,
    points[3].rawY
  );

  const int32_t rightRawY = averageRawValue(
    points[1].rawY,
    points[2].rawY
  );

  const bool swapXY =
    abs(rightRawY - leftRawY) >
    abs(rightRawX - leftRawX);

  const int32_t rawAtLeft = averageRawValue(
    calibrationRawForScreenX(points[0], swapXY),
    calibrationRawForScreenX(points[3], swapXY)
  );

  const int32_t rawAtRight = averageRawValue(
    calibrationRawForScreenX(points[1], swapXY),
    calibrationRawForScreenX(points[2], swapXY)
  );

  const int32_t rawAtTop = averageRawValue(
    calibrationRawForScreenY(points[0], swapXY),
    calibrationRawForScreenY(points[1], swapXY)
  );

  const int32_t rawAtBottom = averageRawValue(
    calibrationRawForScreenY(points[3], swapXY),
    calibrationRawForScreenY(points[2], swapXY)
  );

  if (
    abs(rawAtRight - rawAtLeft) <
      TOUCH_CALIBRATION_MINIMUM_RAW_SPAN ||
    abs(rawAtBottom - rawAtTop) <
      TOUCH_CALIBRATION_MINIMUM_RAW_SPAN
  ) {
    return false;
  }

  calibrationOut = TouchCalibration{};
  calibrationOut.magic = TOUCH_CALIBRATION_MAGIC;
  calibrationOut.boardId = WORLDCLOCK_BOARD;
  calibrationOut.rotation = effectiveDisplayRotation();
  calibrationOut.screenWidth = lcd.width();
  calibrationOut.screenHeight = lcd.height();
  calibrationOut.swapXY = swapXY;

  calibrationOut.rawLeft = extrapolateRawEndpoint(
    rawAtLeft,
    rawAtRight,
    points[0].screenX,
    points[1].screenX,
    0
  );

  calibrationOut.rawRight = extrapolateRawEndpoint(
    rawAtLeft,
    rawAtRight,
    points[0].screenX,
    points[1].screenX,
    lcd.width() - 1
  );

  calibrationOut.rawTop = extrapolateRawEndpoint(
    rawAtTop,
    rawAtBottom,
    points[0].screenY,
    points[3].screenY,
    0
  );

  calibrationOut.rawBottom = extrapolateRawEndpoint(
    rawAtTop,
    rawAtBottom,
    points[0].screenY,
    points[3].screenY,
    lcd.height() - 1
  );

  return true;
}

int mapCalibrationAxis(
  int raw,
  int rawStart,
  int rawEnd,
  int screenMaximum
) {
  if (rawStart == rawEnd) {
    return 0;
  }

  int32_t mapped =
    (static_cast<int32_t>(raw - rawStart) *
     screenMaximum) /
    (rawEnd - rawStart);

  mapped = constrain(
    mapped,
    static_cast<int32_t>(0),
    static_cast<int32_t>(screenMaximum)
  );

  return static_cast<int>(mapped);
}

void mapRawWithCalibration(
  const Xpt2046Sample &sample,
  const TouchCalibration &calibration,
  int &screenX,
  int &screenY
) {
  const int rawForX =
    calibration.swapXY
      ? sample.rawY
      : sample.rawX;

  const int rawForY =
    calibration.swapXY
      ? sample.rawX
      : sample.rawY;

  screenX = mapCalibrationAxis(
    rawForX,
    calibration.rawLeft,
    calibration.rawRight,
    calibration.screenWidth - 1
  );

  screenY = mapCalibrationAxis(
    rawForY,
    calibration.rawTop,
    calibration.rawBottom,
    calibration.screenHeight - 1
  );
}

bool verifyTouchCalibration(
  const TouchCalibration &calibration
) {
  waitForRawTouchRelease();

  const int centerX =
    lcd.width() / 2;

  const int centerY =
    lcd.height() / 2;

  lcd.fillScreen(
    TFT_BLACK
  );

  drawCalibrationText(
    "Verify calibration",
    "Touch the center target",
    TFT_WHITE
  );

  drawCalibrationTarget(
    centerX,
    centerY,
    TFT_CYAN
  );

  Xpt2046Sample sample;
  uint16_t xValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint16_t yValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint16_t pressureValues[TOUCH_CALIBRATION_RAW_SAMPLES];
  uint8_t collected = 0;

  while (
    collected <
      TOUCH_CALIBRATION_RAW_SAMPLES
  ) {
    Xpt2046Sample current;

    if (
      readRawTouchSample(current) &&
      current.pressed
    ) {
      xValues[collected] = current.rawX;
      yValues[collected] = current.rawY;
      pressureValues[collected] = current.pressure;
      ++collected;
    } else if (collected > 0) {
      collected = 0;
    }

    delay(8);
  }

  sample.rawX = calibrationMedian(
    xValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  sample.rawY = calibrationMedian(
    yValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  sample.pressure = calibrationMedian(
    pressureValues,
    TOUCH_CALIBRATION_RAW_SAMPLES
  );

  sample.contact = true;
  sample.pressed = true;

  waitForRawTouchRelease();

  int mappedX = -1;
  int mappedY = -1;

  mapRawWithCalibration(
    sample,
    calibration,
    mappedX,
    mappedY
  );

  const bool verified =
    abs(mappedX - centerX) <=
      TOUCH_CALIBRATION_CENTER_TOLERANCE &&
    abs(mappedY - centerY) <=
      TOUCH_CALIBRATION_CENTER_TOLERANCE;

  drawCalibrationTarget(
    centerX,
    centerY,
    verified
      ? TFT_GREEN
      : TFT_RED
  );

  lcd.drawCircle(
    mappedX,
    mappedY,
    4,
    verified
      ? TFT_GREEN
      : TFT_RED
  );

  char resultLine[64];

  snprintf(
    resultLine,
    sizeof(resultLine),
    "Mapped center: %d, %d",
    mappedX,
    mappedY
  );

  drawCalibrationText(
    verified
      ? "Center verified"
      : "Verification failed",
    resultLine,
    verified
      ? TFT_GREEN
      : TFT_RED
  );

  delay(
    verified
      ? 700
      : 1400
  );

  return verified;
}

} // namespace


bool runIntegratedTouchCalibration(
  bool firstBoot
) {
  if (
    !ACTIVE_BOARD.touchAvailable ||
    !touchHardwareIsReady()
  ) {
    return false;
  }

  for (;;) {
    lcd.fillScreen(
      TFT_BLACK
    );

    drawCalibrationText(
      firstBoot
        ? "Touch setup required"
        : "Recalibrate touch",
      "Four corners, then center",
      TFT_WHITE
    );

    delay(900);

    const int left =
      TOUCH_CALIBRATION_TARGET_MARGIN;

    const int right =
      lcd.width() - 1 -
      TOUCH_CALIBRATION_TARGET_MARGIN;

    const int top =
      TOUCH_CALIBRATION_TARGET_MARGIN;

    const int bottom =
      lcd.height() - 1 -
      TOUCH_CALIBRATION_TARGET_MARGIN;

    TouchCalibrationPoint points[
      TOUCH_CALIBRATION_POINT_COUNT
    ];

    points[0].screenX = left;
    points[0].screenY = top;
    points[1].screenX = right;
    points[1].screenY = top;
    points[2].screenX = right;
    points[2].screenY = bottom;
    points[3].screenX = left;
    points[3].screenY = bottom;

    captureCalibrationPoint(
      points[0],
      1,
      "top-left"
    );

    captureCalibrationPoint(
      points[1],
      2,
      "top-right"
    );

    captureCalibrationPoint(
      points[2],
      3,
      "bottom-right"
    );

    captureCalibrationPoint(
      points[3],
      4,
      "bottom-left"
    );

    TouchCalibration calibration;

    if (
      !buildTouchCalibration(
        points,
        calibration
      )
    ) {
      lcd.fillScreen(TFT_BLACK);
      drawCalibrationText(
        "Calibration invalid",
        "Touch the targets again",
        TFT_RED
      );
      delay(1400);
      continue;
    }

    if (!verifyTouchCalibration(calibration)) {
      continue;
    }

    if (
      !saveTouchCalibrationForDisplayRotation(
        calibration
      )
    ) {
      lcd.fillScreen(TFT_BLACK);
      drawCalibrationText(
        "Could not save",
        "Calibration will retry",
        TFT_RED
      );
      delay(1400);
      continue;
    }

    lcd.fillScreen(
      TFT_BLACK
    );

    drawCalibrationText(
      "Touch calibration saved",
      firstBoot
        ? "Continuing to Wi-Fi setup"
        : "Returning to diagnostics",
      TFT_GREEN
    );

    Serial.printf(
      "Touch calibration saved: rotation=%u swapXY=%s "
      "left=%d right=%d top=%d bottom=%d\n",
      calibration.rotation,
      calibration.swapXY ? "yes" : "no",
      calibration.rawLeft,
      calibration.rawRight,
      calibration.rawTop,
      calibration.rawBottom
    );

    delay(900);
    return true;
  }
}
