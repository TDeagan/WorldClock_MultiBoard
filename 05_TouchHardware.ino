// ============================================================
// XPT2046 touch hardware, saved calibration, and filtered events
// ============================================================
//
// This layer deliberately contains no WorldClock screen drawing. It converts
// raw touch-controller samples into stable Down/Move/Up events for the UI.
//
// Calibration is read from the same "touchtest" Preferences namespace used by
// ESP32_MultiBoard_TouchTest v0.2.2-alpha2. If calibration for the active
// display rotation is absent, calibration for the opposite 180-degree
// rotation is reused and the mapped coordinates are inverted in software.
// ============================================================

namespace {

XPT2046Soft worldClockTouch;
TouchCalibration activeTouchCalibration;
TouchDiagnostics touchDiagnostics;

bool touchHardwareInitialized = false;
bool touchCalibrationReadyFlag = false;
bool transformCalibrationBy180 = false;

uint8_t stableAcceptedTouchFrames = 0;
bool touchGestureActive = false;
unsigned long touchCandidateStartedAt = 0;
unsigned long touchGestureStartedAt = 0;
unsigned long touchLastAcceptedAt = 0;
int touchLastAcceptedX = -1;
int touchLastAcceptedY = -1;
Xpt2046Sample touchLastAcceptedSample;

uint8_t touchOppositeRotation(uint8_t rotation);
String touchCalibrationKey(uint8_t rotation);
String touchPressureKey();
bool readTouchCalibration(uint8_t rotation, TouchCalibration &calibrationOut);
uint16_t loadTouchPressureMinimum();
void saveTouchPressureMinimum(uint16_t value);
int mapTouchRawAxis(int raw, int rawStart, int rawEnd, int screenMaximum);
bool mapTouchSampleToScreen(const Xpt2046Sample &sample, int &x, int &y);
void resetTouchGestureState();

uint8_t touchOppositeRotation(uint8_t rotation) {
  if (rotation < 4) {
    return static_cast<uint8_t>((rotation + 2) % 4);
  }

  return static_cast<uint8_t>(
    4 + ((rotation - 4 + 2) % 4)
  );
}

String touchCalibrationKey(uint8_t rotation) {
  char key[16];

  snprintf(
    key,
    sizeof(key),
    "b%ur%u",
    static_cast<unsigned>(WORLDCLOCK_BOARD),
    static_cast<unsigned>(rotation)
  );

  return String(key);
}

String touchPressureKey() {
  char key[16];

  snprintf(
    key,
    sizeof(key),
    "pmin_b%u",
    static_cast<unsigned>(WORLDCLOCK_BOARD)
  );

  return String(key);
}

bool readTouchCalibration(
  uint8_t rotation,
  TouchCalibration &calibrationOut
) {
  Preferences preferences;

  if (
    !preferences.begin(
      TOUCH_TEST_PREF_NAMESPACE,
      true
    )
  ) {
    return false;
  }

  const String key =
    touchCalibrationKey(rotation);

  const size_t storedBytes =
    preferences.getBytesLength(
      key.c_str()
    );

  if (
    storedBytes !=
      sizeof(TouchCalibration)
  ) {
    preferences.end();
    return false;
  }

  preferences.getBytes(
    key.c_str(),
    &calibrationOut,
    sizeof(calibrationOut)
  );

  preferences.end();

  return
    calibrationOut.magic ==
      TOUCH_CALIBRATION_MAGIC &&
    calibrationOut.boardId ==
      WORLDCLOCK_BOARD &&
    calibrationOut.rotation ==
      rotation &&
    calibrationOut.screenWidth ==
      lcd.width() &&
    calibrationOut.screenHeight ==
      lcd.height();
}

uint16_t loadTouchPressureMinimum() {
  uint16_t stored =
    ACTIVE_BOARD.touchPressureMinimum;

  Preferences preferences;

  if (
    preferences.begin(
      TOUCH_TEST_PREF_NAMESPACE,
      true
    )
  ) {
    const String key =
      touchPressureKey();

    if (
      preferences.getBytesLength(
        key.c_str()
      ) == sizeof(stored)
    ) {
      preferences.getBytes(
        key.c_str(),
        &stored,
        sizeof(stored)
      );
    }

    preferences.end();
  }

  return static_cast<uint16_t>(
    constrain(
      static_cast<int>(stored),
      static_cast<int>(
        TOUCH_PRESSURE_MINIMUM_ALLOWED
      ),
      static_cast<int>(
        TOUCH_PRESSURE_MAXIMUM_ALLOWED
      )
    )
  );
}

void saveTouchPressureMinimum(
  uint16_t value
) {
  Preferences preferences;

  if (
    !preferences.begin(
      TOUCH_TEST_PREF_NAMESPACE,
      false
    )
  ) {
    return;
  }

  const String key =
    touchPressureKey();

  preferences.putBytes(
    key.c_str(),
    &value,
    sizeof(value)
  );

  preferences.end();
}

int mapTouchRawAxis(
  int raw,
  int rawStart,
  int rawEnd,
  int screenMaximum
) {
  if (rawStart == rawEnd) {
    return 0;
  }

  const int32_t numerator =
    static_cast<int32_t>(
      raw - rawStart
    ) *
    screenMaximum;

  const int32_t denominator =
    rawEnd - rawStart;

  int32_t result =
    numerator / denominator;

  result =
    constrain(
      result,
      static_cast<int32_t>(0),
      static_cast<int32_t>(
        screenMaximum
      )
    );

  return static_cast<int>(result);
}

bool mapTouchSampleToScreen(
  const Xpt2046Sample &sample,
  int &x,
  int &y
) {
  if (
    !touchCalibrationReadyFlag ||
    !sample.pressed
  ) {
    return false;
  }

  const int rawForX =
    activeTouchCalibration.swapXY
      ? sample.rawY
      : sample.rawX;

  const int rawForY =
    activeTouchCalibration.swapXY
      ? sample.rawX
      : sample.rawY;

  x = mapTouchRawAxis(
    rawForX,
    activeTouchCalibration.rawLeft,
    activeTouchCalibration.rawRight,
    activeTouchCalibration.screenWidth - 1
  );

  y = mapTouchRawAxis(
    rawForY,
    activeTouchCalibration.rawTop,
    activeTouchCalibration.rawBottom,
    activeTouchCalibration.screenHeight - 1
  );

  if (transformCalibrationBy180) {
    x =
      activeTouchCalibration.screenWidth -
      1 - x;

    y =
      activeTouchCalibration.screenHeight -
      1 - y;
  }

  return true;
}

void resetTouchGestureState() {
  stableAcceptedTouchFrames = 0;
  touchGestureActive = false;
  touchCandidateStartedAt = 0;
  touchGestureStartedAt = 0;
  touchLastAcceptedAt = 0;
  touchLastAcceptedX = -1;
  touchLastAcceptedY = -1;
  touchLastAcceptedSample =
    Xpt2046Sample{};
}

} // namespace


bool initializeTouchHardware() {
  touchDiagnostics =
    TouchDiagnostics{};

  touchDiagnostics.hardwareAvailable =
    ACTIVE_BOARD.touchAvailable;

  touchDiagnostics.activeRotation =
    effectiveDisplayRotation();

  if (!ACTIVE_BOARD.touchAvailable) {
    touchDiagnostics.state =
      "DISABLED";

    Serial.printf(
      "Touch: disabled for board profile %s\n",
      ACTIVE_BOARD.name
    );

    return false;
  }

  const uint16_t pressureMinimum =
    loadTouchPressureMinimum();

  worldClockTouch.begin(
    TOUCH_SCLK,
    TOUCH_MOSI,
    TOUCH_MISO,
    TOUCH_CS,
    TOUCH_IRQ,
    pressureMinimum,
    ACTIVE_BOARD.touchMedianSamples
  );

  touchHardwareInitialized = true;
  touchDiagnostics.initialized = true;
  touchDiagnostics.pressureMinimum =
    pressureMinimum;
  touchDiagnostics.state =
    "NO CAL";

  resetTouchGestureState();
  reloadTouchCalibrationForDisplayRotation();

  Serial.printf(
    "Touch: CLK=%d MOSI=%d MISO=%d CS=%d IRQ=%d threshold=%u\n",
    TOUCH_SCLK,
    TOUCH_MOSI,
    TOUCH_MISO,
    TOUCH_CS,
    TOUCH_IRQ,
    pressureMinimum
  );

  if (!touchCalibrationReadyFlag) {
    Serial.println(
      "Touch calibration not found. Run the standalone "
      "ESP32_MultiBoard_TouchTest on this board."
    );
  }

  return touchCalibrationReadyFlag;
}


void reloadTouchCalibrationForDisplayRotation() {
  if (!touchHardwareInitialized) {
    return;
  }

  const uint8_t currentRotation =
    effectiveDisplayRotation();

  touchDiagnostics.activeRotation =
    currentRotation;

  TouchCalibration loaded;

  if (
    readTouchCalibration(
      currentRotation,
      loaded
    )
  ) {
    activeTouchCalibration = loaded;
    touchCalibrationReadyFlag = true;
    transformCalibrationBy180 = false;
  } else {
    const uint8_t oppositeRotation =
      touchOppositeRotation(
        currentRotation
      );

    if (
      readTouchCalibration(
        oppositeRotation,
        loaded
      )
    ) {
      activeTouchCalibration = loaded;
      touchCalibrationReadyFlag = true;
      transformCalibrationBy180 = true;
    } else {
      touchCalibrationReadyFlag = false;
      transformCalibrationBy180 = false;
    }
  }

  touchDiagnostics.calibrationReady =
    touchCalibrationReadyFlag;

  touchDiagnostics.usingOppositeRotationCalibration =
    transformCalibrationBy180;

  touchDiagnostics.calibrationRotation =
    touchCalibrationReadyFlag
      ? activeTouchCalibration.rotation
      : 0;

  touchDiagnostics.state =
    touchCalibrationReadyFlag
      ? "UP"
      : "NO CAL";

  resetTouchGestureState();

  if (touchCalibrationReadyFlag) {
    Serial.printf(
      "Touch calibration: display rotation %u, source rotation %u%s\n",
      currentRotation,
      activeTouchCalibration.rotation,
      transformCalibrationBy180
        ? " (180-degree transform)"
        : ""
    );
  } else {
    Serial.printf(
      "Touch calibration unavailable for rotations %u and %u\n",
      currentRotation,
      touchOppositeRotation(
        currentRotation
      )
    );
  }
}


bool pollTouchEvent(
  TouchEvent &eventOut
) {
  eventOut = TouchEvent{};

  if (
    !touchHardwareInitialized ||
    !touchCalibrationReadyFlag
  ) {
    return false;
  }

  Xpt2046Sample sample;

  const bool contact =
    worldClockTouch.read(sample);

  const unsigned long now =
    millis();

  touchDiagnostics.contact =
    contact;

  touchDiagnostics.rawX =
    sample.rawX;

  touchDiagnostics.rawY =
    sample.rawY;

  touchDiagnostics.pressure =
    sample.pressure;

  int screenX = -1;
  int screenY = -1;

  const bool accepted =
    contact &&
    sample.pressed &&
    mapTouchSampleToScreen(
      sample,
      screenX,
      screenY
    );

  touchDiagnostics.accepted =
    accepted;

  if (accepted) {
    touchDiagnostics.screenX =
      screenX;

    touchDiagnostics.screenY =
      screenY;

    touchDiagnostics.state =
      touchGestureActive
        ? "DOWN"
        : "WAIT";

    touchLastAcceptedSample =
      sample;

    touchLastAcceptedX =
      screenX;

    touchLastAcceptedY =
      screenY;

    touchLastAcceptedAt =
      now;

    if (!touchGestureActive) {
      if (
        stableAcceptedTouchFrames == 0
      ) {
        touchCandidateStartedAt =
          now;
      }

      if (
        stableAcceptedTouchFrames <
          255
      ) {
        ++stableAcceptedTouchFrames;
      }

      if (
        stableAcceptedTouchFrames >=
          ACTIVE_BOARD.touchStableFrames
      ) {
        touchGestureActive = true;

        touchGestureStartedAt =
          touchCandidateStartedAt == 0
            ? now
            : touchCandidateStartedAt;

        touchDiagnostics.state =
          "DOWN";

        eventOut.type =
          TouchEventType::Down;

        eventOut.x =
          screenX;

        eventOut.y =
          screenY;

        eventOut.rawX =
          sample.rawX;

        eventOut.rawY =
          sample.rawY;

        eventOut.pressure =
          sample.pressure;

        eventOut.durationMs =
          now - touchGestureStartedAt;

        return true;
      }

      return false;
    }

    eventOut.type =
      TouchEventType::Move;

    eventOut.x =
      screenX;

    eventOut.y =
      screenY;

    eventOut.rawX =
      sample.rawX;

    eventOut.rawY =
      sample.rawY;

    eventOut.pressure =
      sample.pressure;

    eventOut.durationMs =
      now - touchGestureStartedAt;

    return true;
  }

  stableAcceptedTouchFrames = 0;

  if (touchGestureActive) {
    if (
      now - touchLastAcceptedAt <
        ACTIVE_BOARD.touchDropoutGraceMs
    ) {
      touchDiagnostics.state =
        contact
          ? "LIGHT"
          : "HOLD";

      return false;
    }

    touchGestureActive = false;
    touchCandidateStartedAt = 0;

    touchDiagnostics.contact = false;
    touchDiagnostics.accepted = false;
    touchDiagnostics.state = "UP";

    // Retain the last accepted coordinates and pressure after release.
    touchDiagnostics.rawX =
      touchLastAcceptedSample.rawX;

    touchDiagnostics.rawY =
      touchLastAcceptedSample.rawY;

    touchDiagnostics.pressure =
      touchLastAcceptedSample.pressure;

    touchDiagnostics.screenX =
      touchLastAcceptedX;

    touchDiagnostics.screenY =
      touchLastAcceptedY;

    eventOut.type =
      TouchEventType::Up;

    eventOut.x =
      touchLastAcceptedX;

    eventOut.y =
      touchLastAcceptedY;

    eventOut.rawX =
      touchLastAcceptedSample.rawX;

    eventOut.rawY =
      touchLastAcceptedSample.rawY;

    eventOut.pressure =
      touchLastAcceptedSample.pressure;

    eventOut.durationMs =
      now - touchGestureStartedAt;

    return true;
  }

  if (contact) {
    touchDiagnostics.state =
      sample.pressed
        ? "WAIT"
        : "LIGHT";
  } else {
    touchDiagnostics.state =
      "UP";
  }

  return false;
}


bool touchHardwareIsReady() {
  return
    touchHardwareInitialized;
}


bool touchCalibrationIsReady() {
  return
    touchCalibrationReadyFlag;
}


const TouchDiagnostics &
getTouchDiagnostics() {
  return touchDiagnostics;
}


uint16_t getTouchPressureMinimum() {
  return
    worldClockTouch.getPressureMinimum();
}


void adjustTouchPressureMinimum(
  int delta
) {
  if (!touchHardwareInitialized) {
    return;
  }

  int changed =
    static_cast<int>(
      worldClockTouch.getPressureMinimum()
    ) +
    delta;

  changed =
    constrain(
      changed,
      static_cast<int>(
        TOUCH_PRESSURE_MINIMUM_ALLOWED
      ),
      static_cast<int>(
        TOUCH_PRESSURE_MAXIMUM_ALLOWED
      )
    );

  const uint16_t changedValue =
    static_cast<uint16_t>(
      changed
    );

  worldClockTouch.setPressureMinimum(
    changedValue
  );

  touchDiagnostics.pressureMinimum =
    changedValue;

  saveTouchPressureMinimum(
    changedValue
  );

  Serial.printf(
    "Touch pressure threshold changed to %u\n",
    changedValue
  );
}
