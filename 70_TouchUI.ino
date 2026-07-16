// ============================================================
// WorldClock Version 5 alpha touchscreen user interface
// ============================================================
//
// Alpha 1 intentionally provides navigation and diagnostics only. It proves
// that touch polling, SD access, Wi-Fi/web servicing, the clock scheduler, and
// full-screen menu drawing coexist before settings are made editable.
// ============================================================

namespace {

enum class TouchUiPage : uint8_t {
  Clock,
  MainMenu,
  TimeDisplay,
  Location,
  Overlays,
  Maps,
  Network,
  Diagnostics
};

enum class TouchUiButtonId : uint8_t {
  None,
  OpenMenu,
  TimeDisplay,
  Location,
  Overlays,
  Maps,
  Network,
  Diagnostics,
  PressureDown,
  PressureUp,
  Back,
  Home
};

struct TouchUiButton {
  TouchUiButtonId id;
  int x;
  int y;
  int w;
  int h;
  const char *label;
  uint16_t fillColor;
};

static constexpr TouchUiButton MENU_TIME_DISPLAY = {
  TouchUiButtonId::TimeDisplay,
  8, 40, 148, 46,
  "TIME / DISPLAY",
  TFT_NAVY
};

static constexpr TouchUiButton MENU_LOCATION = {
  TouchUiButtonId::Location,
  164, 40, 148, 46,
  "LOCATION",
  TFT_DARKGREEN
};

static constexpr TouchUiButton MENU_OVERLAYS = {
  TouchUiButtonId::Overlays,
  8, 92, 148, 46,
  "OVERLAYS",
  TFT_MAROON
};

static constexpr TouchUiButton MENU_MAPS = {
  TouchUiButtonId::Maps,
  164, 92, 148, 46,
  "MAPS",
  TFT_PURPLE
};

static constexpr TouchUiButton MENU_NETWORK = {
  TouchUiButtonId::Network,
  8, 144, 148, 46,
  "NETWORK",
  TFT_DARKGREY
};

static constexpr TouchUiButton MENU_DIAGNOSTICS = {
  TouchUiButtonId::Diagnostics,
  164, 144, 148, 46,
  "DIAGNOSTICS",
  TFT_BLUE
};

static constexpr TouchUiButton BUTTON_PRESSURE_DOWN = {
  TouchUiButtonId::PressureDown,
  8, 151, 70, 38,
  "PRESS -",
  TFT_DARKGREY
};

static constexpr TouchUiButton BUTTON_PRESSURE_UP = {
  TouchUiButtonId::PressureUp,
  86, 151, 70, 38,
  "PRESS +",
  TFT_DARKGREY
};

static constexpr TouchUiButton BUTTON_BACK = {
  TouchUiButtonId::Back,
  164, 151, 70, 38,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton BUTTON_HOME = {
  TouchUiButtonId::Home,
  242, 151, 70, 38,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton PLACEHOLDER_BACK = {
  TouchUiButtonId::Back,
  8, 188, 148, 40,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton PLACEHOLDER_HOME = {
  TouchUiButtonId::Home,
  164, 188, 148, 40,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton MAIN_HOME = {
  TouchUiButtonId::Home,
  86, 194, 148, 35,
  "RETURN TO CLOCK",
  TFT_DARKGREEN
};

static constexpr const TouchUiButton *MAIN_MENU_BUTTONS[] = {
  &MENU_TIME_DISPLAY,
  &MENU_LOCATION,
  &MENU_OVERLAYS,
  &MENU_MAPS,
  &MENU_NETWORK,
  &MENU_DIAGNOSTICS,
  &MAIN_HOME
};

static constexpr const TouchUiButton *PLACEHOLDER_BUTTONS[] = {
  &PLACEHOLDER_BACK,
  &PLACEHOLDER_HOME
};

static constexpr const TouchUiButton *DIAGNOSTIC_BUTTONS[] = {
  &BUTTON_PRESSURE_DOWN,
  &BUTTON_PRESSURE_UP,
  &BUTTON_BACK,
  &BUTTON_HOME
};

TouchUiPage activeTouchUiPage =
  TouchUiPage::Clock;

bool touchUiInitialized = false;
unsigned long touchUiLastActivityAt = 0;
unsigned long touchUiLastDiagnosticsDrawAt = 0;

TouchUiButtonId activeTouchUiButton =
  TouchUiButtonId::None;

TouchUiButtonId highlightedTouchUiButton =
  TouchUiButtonId::None;

bool activeTouchUiButtonValid = false;
uint8_t activeTouchUiOutsideFrames = 0;

bool pointInsideTouchUiButton(
  int x,
  int y,
  const TouchUiButton &button,
  int margin = 0
);
const TouchUiButton *touchUiButtonForId(TouchUiButtonId id);
TouchUiButtonId touchUiButtonAt(int x, int y);
void drawTouchUiButton(const TouchUiButton &button, bool pressed = false);
void setActiveTouchUiButton(TouchUiButtonId id);
void drawTouchUiHeader(const char *title);
void drawTouchUiFooter();
void drawTouchUiMainMenu();
const char *touchUiPageTitle(TouchUiPage page);
void drawTouchUiPlaceholder(TouchUiPage page);
void drawTouchUiDiagnosticsData();
void drawTouchUiDiagnostics();
void drawActiveTouchUiPage();
void openTouchUiMainMenu();
void showTouchUiPage(TouchUiPage page);
void handleTouchUiButton(TouchUiButtonId id);
void beginTouchUiPress(const TouchEvent &event);
void moveTouchUiPress(const TouchEvent &event);
void finishTouchUiPress();

bool pointInsideTouchUiButton(
  int x,
  int y,
  const TouchUiButton &button,
  int margin
) {
  return
    x >= button.x - margin &&
    y >= button.y - margin &&
    x < button.x + button.w + margin &&
    y < button.y + button.h + margin;
}

const TouchUiButton *touchUiButtonForId(
  TouchUiButtonId id
) {
  const TouchUiButton *const *buttons =
    nullptr;

  size_t buttonCount = 0;

  if (
    activeTouchUiPage ==
      TouchUiPage::MainMenu
  ) {
    buttons =
      MAIN_MENU_BUTTONS;

    buttonCount =
      sizeof(MAIN_MENU_BUTTONS) /
      sizeof(MAIN_MENU_BUTTONS[0]);
  } else if (
    activeTouchUiPage ==
      TouchUiPage::Diagnostics
  ) {
    buttons =
      DIAGNOSTIC_BUTTONS;

    buttonCount =
      sizeof(DIAGNOSTIC_BUTTONS) /
      sizeof(DIAGNOSTIC_BUTTONS[0]);
  } else if (
    activeTouchUiPage !=
      TouchUiPage::Clock
  ) {
    buttons =
      PLACEHOLDER_BUTTONS;

    buttonCount =
      sizeof(PLACEHOLDER_BUTTONS) /
      sizeof(PLACEHOLDER_BUTTONS[0]);
  }

  for (
    size_t i = 0;
    i < buttonCount;
    ++i
  ) {
    if (
      buttons[i]->id == id
    ) {
      return buttons[i];
    }
  }

  return nullptr;
}

TouchUiButtonId touchUiButtonAt(
  int x,
  int y
) {
  if (
    activeTouchUiPage ==
      TouchUiPage::Clock
  ) {
    if (
      y >= STATUS_BAR_Y &&
      y < SCREEN_H
    ) {
      return
        TouchUiButtonId::OpenMenu;
    }

    return
      TouchUiButtonId::None;
  }

  const TouchUiButton *const *buttons =
    nullptr;

  size_t buttonCount = 0;

  if (
    activeTouchUiPage ==
      TouchUiPage::MainMenu
  ) {
    buttons =
      MAIN_MENU_BUTTONS;

    buttonCount =
      sizeof(MAIN_MENU_BUTTONS) /
      sizeof(MAIN_MENU_BUTTONS[0]);
  } else if (
    activeTouchUiPage ==
      TouchUiPage::Diagnostics
  ) {
    buttons =
      DIAGNOSTIC_BUTTONS;

    buttonCount =
      sizeof(DIAGNOSTIC_BUTTONS) /
      sizeof(DIAGNOSTIC_BUTTONS[0]);
  } else {
    buttons =
      PLACEHOLDER_BUTTONS;

    buttonCount =
      sizeof(PLACEHOLDER_BUTTONS) /
      sizeof(PLACEHOLDER_BUTTONS[0]);
  }

  for (
    size_t i = 0;
    i < buttonCount;
    ++i
  ) {
    if (
      pointInsideTouchUiButton(
        x,
        y,
        *buttons[i]
      )
    ) {
      return
        buttons[i]->id;
    }
  }

  return
    TouchUiButtonId::None;
}

void drawTouchUiButton(
  const TouchUiButton &button,
  bool pressed
) {
  const uint16_t fillColor =
    pressed
      ? TFT_YELLOW
      : button.fillColor;

  const uint16_t textColor =
    pressed
      ? TFT_BLACK
      : TFT_WHITE;

  lcd.fillRoundRect(
    button.x,
    button.y,
    button.w,
    button.h,
    6,
    fillColor
  );

  lcd.drawRoundRect(
    button.x,
    button.y,
    button.w,
    button.h,
    6,
    TFT_WHITE
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    textColor,
    fillColor
  );

  lcd.drawString(
    button.label,
    button.x + button.w / 2,
    button.y + button.h / 2
  );
}

void setActiveTouchUiButton(
  TouchUiButtonId id
) {
  if (
    highlightedTouchUiButton == id
  ) {
    return;
  }

  const TouchUiButton *previous =
    touchUiButtonForId(
      highlightedTouchUiButton
    );

  if (
    previous != nullptr &&
    activeTouchUiPage !=
      TouchUiPage::Clock
  ) {
    drawTouchUiButton(
      *previous,
      false
    );
  }

  highlightedTouchUiButton = id;

  const TouchUiButton *current =
    touchUiButtonForId(
      highlightedTouchUiButton
    );

  if (
    current != nullptr &&
    activeTouchUiPage !=
      TouchUiPage::Clock
  ) {
    drawTouchUiButton(
      *current,
      true
    );
  }
}

void drawTouchUiHeader(
  const char *title
) {
  lcd.fillScreen(
    TFT_BLACK
  );

  lcd.fillRect(
    0,
    0,
    SCREEN_W,
    32,
    TFT_NAVY
  );

  lcd.setFont(
    &fonts::Font2
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_NAVY
  );

  lcd.drawString(
    title,
    SCREEN_W / 2,
    16
  );
}

void drawTouchUiFooter() {
  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::bottom_center
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  const String footer =
    String("World Clock ") +
    FIRMWARE_VERSION;

  lcd.drawString(
    footer,
    SCREEN_W / 2,
    SCREEN_H - 1
  );
}

void drawTouchUiMainMenu() {
  drawTouchUiHeader(
    "TOUCH MENU"
  );

  for (
    const TouchUiButton *button :
      MAIN_MENU_BUTTONS
  ) {
    drawTouchUiButton(
      *button
    );
  }

  drawTouchUiFooter();
}

const char *touchUiPageTitle(
  TouchUiPage page
) {
  switch (page) {
    case TouchUiPage::TimeDisplay:
      return "TIME / DISPLAY";

    case TouchUiPage::Location:
      return "LOCATION";

    case TouchUiPage::Overlays:
      return "OVERLAYS";

    case TouchUiPage::Maps:
      return "MAPS";

    case TouchUiPage::Network:
      return "NETWORK";

    case TouchUiPage::Diagnostics:
      return "DIAGNOSTICS";

    case TouchUiPage::MainMenu:
      return "TOUCH MENU";

    case TouchUiPage::Clock:
    default:
      return "WORLD CLOCK";
  }
}

void drawTouchUiPlaceholder(
  TouchUiPage page
) {
  drawTouchUiHeader(
    touchUiPageTitle(page)
  );

  lcd.setFont(
    &fonts::Font2
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  lcd.drawString(
    "Navigation test passed",
    SCREEN_W / 2,
    84
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    "Controls will be added in the next alpha.",
    SCREEN_W / 2,
    112
  );

  lcd.setTextColor(
    TFT_YELLOW,
    TFT_BLACK
  );

  lcd.drawString(
    "Browser configuration remains fully active.",
    SCREEN_W / 2,
    132
  );

  for (
    const TouchUiButton *button :
      PLACEHOLDER_BUTTONS
  ) {
    drawTouchUiButton(
      *button
    );
  }

  drawTouchUiFooter();
}

void drawTouchUiDiagnosticsData() {
  const TouchDiagnostics &diagnostics =
    getTouchDiagnostics();

  lcd.fillRect(
    0,
    33,
    SCREEN_W,
    114,
    TFT_BLACK
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::top_left
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  char line[128];

  snprintf(
    line,
    sizeof(line),
    "Board: %s",
    ACTIVE_BOARD.name
  );

  lcd.drawString(
    line,
    6,
    36
  );

  snprintf(
    line,
    sizeof(line),
    "Rotation: %u  calibration: %s r%u%s",
    diagnostics.activeRotation,
    diagnostics.calibrationReady
      ? "OK"
      : "MISSING",
    diagnostics.calibrationRotation,
    diagnostics.usingOppositeRotationCalibration
      ? " +180"
      : ""
  );

  lcd.setTextColor(
    diagnostics.calibrationReady
      ? TFT_GREEN
      : TFT_RED,
    TFT_BLACK
  );

  lcd.drawString(
    line,
    6,
    52
  );

  snprintf(
    line,
    sizeof(line),
    "State: %s  pressure: %u  threshold: %u",
    diagnostics.state,
    diagnostics.pressure,
    diagnostics.pressureMinimum
  );

  lcd.setTextColor(
    TFT_YELLOW,
    TFT_BLACK
  );

  lcd.drawString(
    line,
    6,
    68
  );

  snprintf(
    line,
    sizeof(line),
    "Raw: X=%u Y=%u",
    diagnostics.rawX,
    diagnostics.rawY
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    line,
    6,
    84
  );

  snprintf(
    line,
    sizeof(line),
    "Screen: X=%d Y=%d  contact=%s accepted=%s",
    diagnostics.screenX,
    diagnostics.screenY,
    diagnostics.contact
      ? "yes"
      : "no",
    diagnostics.accepted
      ? "yes"
      : "no"
  );

  lcd.drawString(
    line,
    6,
    100
  );

  snprintf(
    line,
    sizeof(line),
    "Wi-Fi: %s  SD: %s  NTP: %s",
    WiFi.status() == WL_CONNECTED
      ? "OK"
      : "--",
    sdReady
      ? "OK"
      : "--",
    timeValid
      ? "OK"
      : "--"
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  lcd.drawString(
    line,
    6,
    116
  );

  snprintf(
    line,
    sizeof(line),
    "Selected map: %s",
    selectedDayMapFilename.c_str()
  );

  lcd.drawString(
    line,
    6,
    132
  );

  touchUiLastDiagnosticsDrawAt =
    millis();
}

void drawTouchUiDiagnostics() {
  drawTouchUiHeader(
    "TOUCH DIAGNOSTICS"
  );

  drawTouchUiDiagnosticsData();

  for (
    const TouchUiButton *button :
      DIAGNOSTIC_BUTTONS
  ) {
    drawTouchUiButton(
      *button
    );
  }

  drawTouchUiFooter();
}

void drawActiveTouchUiPage() {
  activeTouchUiButton =
    TouchUiButtonId::None;

  highlightedTouchUiButton =
    TouchUiButtonId::None;

  activeTouchUiButtonValid =
    false;

  activeTouchUiOutsideFrames =
    0;

  switch (activeTouchUiPage) {
    case TouchUiPage::MainMenu:
      drawTouchUiMainMenu();
      break;

    case TouchUiPage::Diagnostics:
      drawTouchUiDiagnostics();
      break;

    case TouchUiPage::TimeDisplay:
    case TouchUiPage::Location:
    case TouchUiPage::Overlays:
    case TouchUiPage::Maps:
    case TouchUiPage::Network:
      drawTouchUiPlaceholder(
        activeTouchUiPage
      );
      break;

    case TouchUiPage::Clock:
    default:
      break;
  }
}

void openTouchUiMainMenu() {
  if (!touchCalibrationIsReady()) {
    return;
  }

  activeTouchUiPage =
    TouchUiPage::MainMenu;

  touchUiLastActivityAt =
    millis();

  drawActiveTouchUiPage();
}

void showTouchUiPage(
  TouchUiPage page
) {
  activeTouchUiPage =
    page;

  touchUiLastActivityAt =
    millis();

  drawActiveTouchUiPage();
}

void handleTouchUiButton(
  TouchUiButtonId id
) {
  switch (id) {
    case TouchUiButtonId::OpenMenu:
      openTouchUiMainMenu();
      break;

    case TouchUiButtonId::TimeDisplay:
      showTouchUiPage(
        TouchUiPage::TimeDisplay
      );
      break;

    case TouchUiButtonId::Location:
      showTouchUiPage(
        TouchUiPage::Location
      );
      break;

    case TouchUiButtonId::Overlays:
      showTouchUiPage(
        TouchUiPage::Overlays
      );
      break;

    case TouchUiButtonId::Maps:
      showTouchUiPage(
        TouchUiPage::Maps
      );
      break;

    case TouchUiButtonId::Network:
      showTouchUiPage(
        TouchUiPage::Network
      );
      break;

    case TouchUiButtonId::Diagnostics:
      showTouchUiPage(
        TouchUiPage::Diagnostics
      );
      break;

    case TouchUiButtonId::PressureDown:
      adjustTouchPressureMinimum(
        -static_cast<int>(
          TOUCH_PRESSURE_STEP
        )
      );

      drawTouchUiDiagnosticsData();
      break;

    case TouchUiButtonId::PressureUp:
      adjustTouchPressureMinimum(
        static_cast<int>(
          TOUCH_PRESSURE_STEP
        )
      );

      drawTouchUiDiagnosticsData();
      break;

    case TouchUiButtonId::Back:
      showTouchUiPage(
        TouchUiPage::MainMenu
      );
      break;

    case TouchUiButtonId::Home:
      closeTouchUi(true);
      break;

    case TouchUiButtonId::None:
    default:
      break;
  }
}

void beginTouchUiPress(
  const TouchEvent &event
) {
  touchUiLastActivityAt =
    millis();

  activeTouchUiButton =
    touchUiButtonAt(
      event.x,
      event.y
    );

  activeTouchUiButtonValid =
    activeTouchUiButton !=
      TouchUiButtonId::None;

  activeTouchUiOutsideFrames =
    0;

  if (
    activeTouchUiPage !=
      TouchUiPage::Clock
  ) {
    setActiveTouchUiButton(
      activeTouchUiButton
    );
  }
}

void moveTouchUiPress(
  const TouchEvent &event
) {
  touchUiLastActivityAt =
    millis();

  if (
    !activeTouchUiButtonValid ||
    activeTouchUiButton ==
      TouchUiButtonId::None
  ) {
    return;
  }

  if (
    activeTouchUiButton ==
      TouchUiButtonId::OpenMenu
  ) {
    if (
      event.y >=
        STATUS_BAR_Y -
        TOUCH_BUTTON_TRACK_MARGIN
    ) {
      activeTouchUiOutsideFrames = 0;
    } else {
      if (
        activeTouchUiOutsideFrames <
          255
      ) {
        ++activeTouchUiOutsideFrames;
      }
    }
  } else {
    const TouchUiButton *button =
      touchUiButtonForId(
        activeTouchUiButton
      );

    if (
      button != nullptr &&
      pointInsideTouchUiButton(
        event.x,
        event.y,
        *button,
        TOUCH_BUTTON_TRACK_MARGIN
      )
    ) {
      activeTouchUiOutsideFrames = 0;
    } else {
      if (
        activeTouchUiOutsideFrames <
          255
      ) {
        ++activeTouchUiOutsideFrames;
      }
    }
  }

  if (
    activeTouchUiOutsideFrames >=
      TOUCH_BUTTON_CANCEL_OUTSIDE_FRAMES
  ) {
    activeTouchUiButtonValid =
      false;

    if (
      activeTouchUiPage !=
        TouchUiPage::Clock
    ) {
      setActiveTouchUiButton(
        TouchUiButtonId::None
      );
    }
  }
}

void finishTouchUiPress() {
  const TouchUiButtonId action =
    activeTouchUiButtonValid
      ? activeTouchUiButton
      : TouchUiButtonId::None;

  if (
    activeTouchUiPage !=
      TouchUiPage::Clock
  ) {
    setActiveTouchUiButton(
      TouchUiButtonId::None
    );
  }

  activeTouchUiButton =
    TouchUiButtonId::None;

  highlightedTouchUiButton =
    TouchUiButtonId::None;

  activeTouchUiButtonValid =
    false;

  activeTouchUiOutsideFrames =
    0;

  if (
    action !=
      TouchUiButtonId::None
  ) {
    handleTouchUiButton(
      action
    );
  }
}

} // namespace


void initializeTouchUi() {
  touchUiInitialized = true;

  initializeTouchHardware();

  touchUiLastActivityAt =
    millis();

  Serial.printf(
    "Touch UI: %s; calibration: %s\n",
    ACTIVE_BOARD.touchAvailable
      ? "enabled"
      : "disabled",
    touchCalibrationIsReady()
      ? "ready"
      : "missing"
  );
}


void serviceTouchUi() {
  if (
    !touchUiInitialized ||
    !touchHardwareIsReady()
  ) {
    return;
  }

  TouchEvent event;

  if (
    pollTouchEvent(event)
  ) {
    switch (event.type) {
      case TouchEventType::Down:
        beginTouchUiPress(
          event
        );
        break;

      case TouchEventType::Move:
        moveTouchUiPress(
          event
        );
        break;

      case TouchEventType::Up:
        finishTouchUiPress();
        break;

      case TouchEventType::None:
      default:
        break;
    }
  }

  if (
    activeTouchUiPage ==
      TouchUiPage::Diagnostics &&
    activeTouchUiButton ==
      TouchUiButtonId::None &&
    millis() -
      touchUiLastDiagnosticsDrawAt >=
      TOUCH_UI_DIAGNOSTICS_REFRESH_MS
  ) {
    drawTouchUiDiagnosticsData();
  }

  if (
    activeTouchUiPage !=
      TouchUiPage::Clock &&
    millis() -
      touchUiLastActivityAt >=
      TOUCH_UI_INACTIVITY_MS
  ) {
    closeTouchUi(true);
  }
}


bool touchUiIsOpen() {
  return
    activeTouchUiPage !=
      TouchUiPage::Clock;
}


void closeTouchUi(
  bool redrawClock
) {
  activeTouchUiPage =
    TouchUiPage::Clock;

  activeTouchUiButton =
    TouchUiButtonId::None;

  highlightedTouchUiButton =
    TouchUiButtonId::None;

  activeTouchUiButtonValid =
    false;

  activeTouchUiOutsideFrames =
    0;

  touchUiLastActivityAt =
    millis();

  if (!redrawClock) {
    return;
  }

  if (
    timeValid &&
    sdReady
  ) {
    redrawWorldClock();
  } else {
    showStatus(
      "World Clock",
      timeValid
        ? "Waiting for map storage"
        : "Waiting for network time"
    );
  }
}


void touchUiHandleDisplayRotationChanged() {
  if (!touchUiInitialized) {
    return;
  }

  activeTouchUiPage =
    TouchUiPage::Clock;

  activeTouchUiButton =
    TouchUiButtonId::None;

  highlightedTouchUiButton =
    TouchUiButtonId::None;

  activeTouchUiButtonValid =
    false;

  activeTouchUiOutsideFrames =
    0;

  reloadTouchCalibrationForDisplayRotation();
}
