// ============================================================
// WorldClock Version 5 alpha touchscreen user interface
// ============================================================
//
// Alpha 3 adds a functional Maps interface with a scrollable map catalog,
// source-PNG preview, selection/apply controls, validation and cache status,
// cache maintenance, and automatic fallback awareness. Network remains a
// placeholder for a later alpha.
// ============================================================

namespace {

enum class TouchUiPage : uint8_t {
  Clock,
  MainMenu,
  TimeDisplay,
  Location,
  Overlays,
  Maps,
  MapPreview,
  MapMaintenance,
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

  MapRow0,
  MapRow1,
  MapRow2,
  MapPagePrevious,
  MapPageNext,
  MapPreview,
  MapApply,
  MapTools,
  MapValidate,
  MapRescan,
  MapRebuildSelected,
  MapRebuildNight,
  MapRebuildAll,

  TimeZonePrevious,
  TimeZoneNext,
  OffsetDown,
  OffsetUp,
  ClockFormat,
  ShowSeconds,
  FlipDisplay,
  ApplyTimeDisplay,

  LatitudeDown,
  LatitudeUp,
  LatitudeHemisphere,
  LongitudeDown,
  LongitudeUp,
  LongitudeHemisphere,
  CoordinateStep,
  HomeMarker,
  CoordinateGrid,
  ApplyLocation,

  ShowSun,
  ShowMoon,
  ShowISS,
  ShowIssTrack,
  IssTrackStyle,
  ApplyOverlays,

  CancelSettings,
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

// ------------------------------------------------------------
// Main-menu buttons
// ------------------------------------------------------------

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

static constexpr TouchUiButton MAIN_HOME = {
  TouchUiButtonId::Home,
  86, 194, 148, 35,
  "RETURN TO CLOCK",
  TFT_DARKGREEN
};

// ------------------------------------------------------------
// Time/display page
// ------------------------------------------------------------

static constexpr TouchUiButton TIME_ZONE_PREVIOUS = {
  TouchUiButtonId::TimeZonePrevious,
  8, 43, 36, 28,
  "<",
  TFT_DARKGREY
};

static constexpr TouchUiButton TIME_ZONE_NEXT = {
  TouchUiButtonId::TimeZoneNext,
  276, 43, 36, 28,
  ">",
  TFT_DARKGREY
};

static constexpr TouchUiButton OFFSET_DOWN = {
  TouchUiButtonId::OffsetDown,
  8, 75, 36, 28,
  "-",
  TFT_DARKGREY
};

static constexpr TouchUiButton OFFSET_UP = {
  TouchUiButtonId::OffsetUp,
  276, 75, 36, 28,
  "+",
  TFT_DARKGREY
};

static constexpr TouchUiButton CLOCK_FORMAT = {
  TouchUiButtonId::ClockFormat,
  164, 107, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton SHOW_SECONDS = {
  TouchUiButtonId::ShowSeconds,
  164, 139, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton FLIP_DISPLAY = {
  TouchUiButtonId::FlipDisplay,
  164, 171, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton TIME_CANCEL = {
  TouchUiButtonId::CancelSettings,
  8, 205, 148, 28,
  "CANCEL",
  TFT_MAROON
};

static constexpr TouchUiButton TIME_APPLY = {
  TouchUiButtonId::ApplyTimeDisplay,
  164, 205, 148, 28,
  "APPLY",
  TFT_DARKGREEN
};

// ------------------------------------------------------------
// Location page
// ------------------------------------------------------------

static constexpr TouchUiButton LATITUDE_DOWN = {
  TouchUiButtonId::LatitudeDown,
  44, 43, 36, 28,
  "-",
  TFT_DARKGREY
};

static constexpr TouchUiButton LATITUDE_UP = {
  TouchUiButtonId::LatitudeUp,
  218, 43, 36, 28,
  "+",
  TFT_DARKGREY
};

static constexpr TouchUiButton LATITUDE_HEMISPHERE = {
  TouchUiButtonId::LatitudeHemisphere,
  260, 43, 52, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton LONGITUDE_DOWN = {
  TouchUiButtonId::LongitudeDown,
  44, 78, 36, 28,
  "-",
  TFT_DARKGREY
};

static constexpr TouchUiButton LONGITUDE_UP = {
  TouchUiButtonId::LongitudeUp,
  218, 78, 36, 28,
  "+",
  TFT_DARKGREY
};

static constexpr TouchUiButton LONGITUDE_HEMISPHERE = {
  TouchUiButtonId::LongitudeHemisphere,
  260, 78, 52, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton COORDINATE_STEP = {
  TouchUiButtonId::CoordinateStep,
  164, 113, 148, 28,
  "",
  TFT_PURPLE
};

static constexpr TouchUiButton HOME_MARKER = {
  TouchUiButtonId::HomeMarker,
  164, 145, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton COORDINATE_GRID = {
  TouchUiButtonId::CoordinateGrid,
  164, 177, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton LOCATION_CANCEL = {
  TouchUiButtonId::CancelSettings,
  8, 208, 148, 25,
  "CANCEL",
  TFT_MAROON
};

static constexpr TouchUiButton LOCATION_APPLY = {
  TouchUiButtonId::ApplyLocation,
  164, 208, 148, 25,
  "APPLY",
  TFT_DARKGREEN
};

// ------------------------------------------------------------
// Overlay page
// ------------------------------------------------------------

static constexpr TouchUiButton SHOW_SUN = {
  TouchUiButtonId::ShowSun,
  164, 43, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton SHOW_MOON = {
  TouchUiButtonId::ShowMoon,
  164, 75, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton SHOW_ISS = {
  TouchUiButtonId::ShowISS,
  164, 107, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton SHOW_ISS_TRACK = {
  TouchUiButtonId::ShowIssTrack,
  164, 139, 148, 28,
  "",
  TFT_NAVY
};

static constexpr TouchUiButton ISS_TRACK_STYLE = {
  TouchUiButtonId::IssTrackStyle,
  164, 171, 148, 28,
  "",
  TFT_PURPLE
};

static constexpr TouchUiButton OVERLAY_CANCEL = {
  TouchUiButtonId::CancelSettings,
  8, 205, 148, 28,
  "CANCEL",
  TFT_MAROON
};

static constexpr TouchUiButton OVERLAY_APPLY = {
  TouchUiButtonId::ApplyOverlays,
  164, 205, 148, 28,
  "APPLY",
  TFT_DARKGREEN
};

// ------------------------------------------------------------
// Maps page, preview, and maintenance buttons
// ------------------------------------------------------------

static constexpr TouchUiButton MAP_ROW_0 = {
  TouchUiButtonId::MapRow0,
  8, 43, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton MAP_ROW_1 = {
  TouchUiButtonId::MapRow1,
  8, 80, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton MAP_ROW_2 = {
  TouchUiButtonId::MapRow2,
  8, 117, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton MAP_PAGE_PREVIOUS = {
  TouchUiButtonId::MapPagePrevious,
  8, 155, 50, 28,
  "<",
  TFT_DARKGREY
};

static constexpr TouchUiButton MAP_PAGE_NEXT = {
  TouchUiButtonId::MapPageNext,
  62, 155, 50, 28,
  ">",
  TFT_DARKGREY
};

static constexpr TouchUiButton MAP_PREVIEW = {
  TouchUiButtonId::MapPreview,
  116, 155, 94, 28,
  "PREVIEW",
  TFT_PURPLE
};

static constexpr TouchUiButton MAP_APPLY = {
  TouchUiButtonId::MapApply,
  214, 155, 98, 28,
  "APPLY",
  TFT_DARKGREEN
};

static constexpr TouchUiButton MAP_TOOLS = {
  TouchUiButtonId::MapTools,
  8, 188, 96, 38,
  "TOOLS",
  TFT_MAROON
};

static constexpr TouchUiButton MAP_BACK = {
  TouchUiButtonId::Back,
  108, 188, 96, 38,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton MAP_HOME = {
  TouchUiButtonId::Home,
  208, 188, 104, 38,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton PREVIEW_BACK = {
  TouchUiButtonId::Back,
  8, 192, 96, 36,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton PREVIEW_APPLY = {
  TouchUiButtonId::MapApply,
  108, 192, 96, 36,
  "APPLY",
  TFT_DARKGREEN
};

static constexpr TouchUiButton PREVIEW_TOOLS = {
  TouchUiButtonId::MapTools,
  208, 192, 104, 36,
  "TOOLS",
  TFT_MAROON
};

static constexpr TouchUiButton MAP_VALIDATE = {
  TouchUiButtonId::MapValidate,
  8, 72, 148, 36,
  "VALIDATE PNGS",
  TFT_PURPLE
};

static constexpr TouchUiButton MAP_RESCAN = {
  TouchUiButtonId::MapRescan,
  164, 72, 148, 36,
  "RESCAN",
  TFT_NAVY
};

static constexpr TouchUiButton MAP_REBUILD_SELECTED = {
  TouchUiButtonId::MapRebuildSelected,
  8, 112, 304, 36,
  "REBUILD SELECTED CACHE",
  TFT_DARKGREEN
};

static constexpr TouchUiButton MAP_REBUILD_NIGHT = {
  TouchUiButtonId::MapRebuildNight,
  8, 152, 148, 36,
  "REBUILD NIGHT",
  TFT_DARKGREY
};

static constexpr TouchUiButton MAP_REBUILD_ALL = {
  TouchUiButtonId::MapRebuildAll,
  164, 152, 148, 36,
  "REBUILD ALL",
  TFT_MAROON
};

static constexpr TouchUiButton MAINTENANCE_BACK = {
  TouchUiButtonId::Back,
  8, 192, 148, 36,
  "BACK TO MAPS",
  TFT_NAVY
};

static constexpr TouchUiButton MAINTENANCE_HOME = {
  TouchUiButtonId::Home,
  164, 192, 148, 36,
  "CLOCK",
  TFT_DARKGREEN
};

// ------------------------------------------------------------
// Placeholder and diagnostics buttons
// ------------------------------------------------------------

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

static constexpr const TouchUiButton *MAIN_MENU_BUTTONS[] = {
  &MENU_TIME_DISPLAY,
  &MENU_LOCATION,
  &MENU_OVERLAYS,
  &MENU_MAPS,
  &MENU_NETWORK,
  &MENU_DIAGNOSTICS,
  &MAIN_HOME
};

static constexpr const TouchUiButton *TIME_DISPLAY_BUTTONS[] = {
  &TIME_ZONE_PREVIOUS,
  &TIME_ZONE_NEXT,
  &OFFSET_DOWN,
  &OFFSET_UP,
  &CLOCK_FORMAT,
  &SHOW_SECONDS,
  &FLIP_DISPLAY,
  &TIME_CANCEL,
  &TIME_APPLY
};

static constexpr const TouchUiButton *LOCATION_BUTTONS[] = {
  &LATITUDE_DOWN,
  &LATITUDE_UP,
  &LATITUDE_HEMISPHERE,
  &LONGITUDE_DOWN,
  &LONGITUDE_UP,
  &LONGITUDE_HEMISPHERE,
  &COORDINATE_STEP,
  &HOME_MARKER,
  &COORDINATE_GRID,
  &LOCATION_CANCEL,
  &LOCATION_APPLY
};

static constexpr const TouchUiButton *OVERLAY_BUTTONS[] = {
  &SHOW_SUN,
  &SHOW_MOON,
  &SHOW_ISS,
  &SHOW_ISS_TRACK,
  &ISS_TRACK_STYLE,
  &OVERLAY_CANCEL,
  &OVERLAY_APPLY
};

static constexpr const TouchUiButton *MAP_BUTTONS[] = {
  &MAP_ROW_0,
  &MAP_ROW_1,
  &MAP_ROW_2,
  &MAP_PAGE_PREVIOUS,
  &MAP_PAGE_NEXT,
  &MAP_PREVIEW,
  &MAP_APPLY,
  &MAP_TOOLS,
  &MAP_BACK,
  &MAP_HOME
};

static constexpr const TouchUiButton *MAP_PREVIEW_BUTTONS[] = {
  &PREVIEW_BACK,
  &PREVIEW_APPLY,
  &PREVIEW_TOOLS
};

static constexpr const TouchUiButton *MAP_MAINTENANCE_BUTTONS[] = {
  &MAP_VALIDATE,
  &MAP_RESCAN,
  &MAP_REBUILD_SELECTED,
  &MAP_REBUILD_NIGHT,
  &MAP_REBUILD_ALL,
  &MAINTENANCE_BACK,
  &MAINTENANCE_HOME
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

// ------------------------------------------------------------
// UI state and settings drafts
// ------------------------------------------------------------

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

TimeSettings draftTimeSettings;
DisplaySettings draftDisplaySettings;
int draftUtcOffsetMinutes = 0;

LocationGridSettings draftLocationSettings;
double draftLatitudeMagnitude = 0.0;
double draftLongitudeMagnitude = 0.0;
bool draftLatitudeSouth = false;
bool draftLongitudeWest = false;
uint8_t draftCoordinateStepIndex = 1;

OverlaySettings draftOverlaySettings;

DaylightMapInfo touchMapCatalog[MAX_DAYLIGHT_MAPS];
size_t touchMapCatalogCount = 0;
size_t touchMapCatalogTotalCount = 0;
size_t touchMapListOffset = 0;
int touchMapDraftIndex = -1;
String touchMapDraftFilename;
String touchMapMessage;
bool touchMapMessageError = false;
bool touchMapFullValidation = false;

static constexpr size_t TOUCH_MAP_ROWS_PER_PAGE = 3;

static constexpr double COORDINATE_STEPS[] = {
  0.1,
  1.0,
  10.0
};

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------

bool pointInsideTouchUiButton(
  int x,
  int y,
  const TouchUiButton &button,
  int margin = 0
);
void touchUiButtonsForPage(
  TouchUiPage page,
  const TouchUiButton *const *&buttonsOut,
  size_t &buttonCountOut
);
const TouchUiButton *touchUiButtonForId(TouchUiButtonId id);
TouchUiButtonId touchUiButtonAt(int x, int y);
String touchUiButtonDisplayLabel(const TouchUiButton &button);
void drawTouchUiButton(const TouchUiButton &button, bool pressed = false);
void setActiveTouchUiButton(TouchUiButtonId id);
void drawTouchUiHeader(const char *title);
void drawTouchUiFooter();
void drawTouchUiSettingsHint(const char *text = "Tap a value to change it");
void drawTouchUiRow(int y, const char *label);
void drawTouchUiMainMenu();
const char *touchUiPageTitle(TouchUiPage page);
void drawTouchUiPlaceholder(TouchUiPage page);
void drawTouchUiTimeDisplay();
void drawTouchUiLocation();
void drawTouchUiOverlays();
void drawTouchUiMaps();
void drawTouchUiMapPreview();
void drawTouchUiMapMaintenance();
void drawTouchUiMapRow(size_t visibleRow, bool pressed = false);
void drawTouchUiMapMessage();
void drawTouchUiBusy(const char *title, const String &message);
void refreshTouchMapCatalog(bool fullValidation, bool preserveDraft = true);
int touchMapCatalogIndexForRowButton(TouchUiButtonId id);
bool touchMapDraftIsUsable();
void applyTouchUiMap();
void rebuildTouchUiSelectedMapCache();
void rebuildTouchUiNightCache();
void rebuildTouchUiAllCaches();
void drawTouchUiDiagnosticsData();
void drawTouchUiDiagnostics();
void drawActiveTouchUiPage();
void prepareTouchUiDraft(TouchUiPage page);
void openTouchUiMainMenu();
void showTouchUiPage(TouchUiPage page);
void handleTouchUiButton(TouchUiButtonId id);
void beginTouchUiPress(const TouchEvent &event);
void moveTouchUiPress(const TouchEvent &event);
void finishTouchUiPress();
void applyTouchUiTimeDisplay();
void applyTouchUiLocation();
void applyTouchUiOverlays();

// ------------------------------------------------------------
// Button lookup and rendering
// ------------------------------------------------------------

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

void touchUiButtonsForPage(
  TouchUiPage page,
  const TouchUiButton *const *&buttonsOut,
  size_t &buttonCountOut
) {
  buttonsOut = nullptr;
  buttonCountOut = 0;

  switch (page) {
    case TouchUiPage::MainMenu:
      buttonsOut = MAIN_MENU_BUTTONS;
      buttonCountOut =
        sizeof(MAIN_MENU_BUTTONS) /
        sizeof(MAIN_MENU_BUTTONS[0]);
      break;

    case TouchUiPage::TimeDisplay:
      buttonsOut = TIME_DISPLAY_BUTTONS;
      buttonCountOut =
        sizeof(TIME_DISPLAY_BUTTONS) /
        sizeof(TIME_DISPLAY_BUTTONS[0]);
      break;

    case TouchUiPage::Location:
      buttonsOut = LOCATION_BUTTONS;
      buttonCountOut =
        sizeof(LOCATION_BUTTONS) /
        sizeof(LOCATION_BUTTONS[0]);
      break;

    case TouchUiPage::Overlays:
      buttonsOut = OVERLAY_BUTTONS;
      buttonCountOut =
        sizeof(OVERLAY_BUTTONS) /
        sizeof(OVERLAY_BUTTONS[0]);
      break;

    case TouchUiPage::Diagnostics:
      buttonsOut = DIAGNOSTIC_BUTTONS;
      buttonCountOut =
        sizeof(DIAGNOSTIC_BUTTONS) /
        sizeof(DIAGNOSTIC_BUTTONS[0]);
      break;

    case TouchUiPage::Maps:
      buttonsOut = MAP_BUTTONS;
      buttonCountOut =
        sizeof(MAP_BUTTONS) /
        sizeof(MAP_BUTTONS[0]);
      break;

    case TouchUiPage::MapPreview:
      buttonsOut = MAP_PREVIEW_BUTTONS;
      buttonCountOut =
        sizeof(MAP_PREVIEW_BUTTONS) /
        sizeof(MAP_PREVIEW_BUTTONS[0]);
      break;

    case TouchUiPage::MapMaintenance:
      buttonsOut = MAP_MAINTENANCE_BUTTONS;
      buttonCountOut =
        sizeof(MAP_MAINTENANCE_BUTTONS) /
        sizeof(MAP_MAINTENANCE_BUTTONS[0]);
      break;

    case TouchUiPage::Network:
      buttonsOut = PLACEHOLDER_BUTTONS;
      buttonCountOut =
        sizeof(PLACEHOLDER_BUTTONS) /
        sizeof(PLACEHOLDER_BUTTONS[0]);
      break;

    case TouchUiPage::Clock:
    default:
      break;
  }
}

const TouchUiButton *touchUiButtonForId(
  TouchUiButtonId id
) {
  const TouchUiButton *const *buttons =
    nullptr;

  size_t buttonCount = 0;

  touchUiButtonsForPage(
    activeTouchUiPage,
    buttons,
    buttonCount
  );

  for (
    size_t i = 0;
    i < buttonCount;
    ++i
  ) {
    if (buttons[i]->id == id) {
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

  touchUiButtonsForPage(
    activeTouchUiPage,
    buttons,
    buttonCount
  );

  for (
    size_t i = 0;
    i < buttonCount;
    ++i
  ) {
    if (
      (
        buttons[i]->id == TouchUiButtonId::MapRow0 ||
        buttons[i]->id == TouchUiButtonId::MapRow1 ||
        buttons[i]->id == TouchUiButtonId::MapRow2
      ) &&
      touchMapCatalogIndexForRowButton(buttons[i]->id) < 0
    ) {
      continue;
    }

    if (
      pointInsideTouchUiButton(
        x,
        y,
        *buttons[i]
      )
    ) {
      return buttons[i]->id;
    }
  }

  return TouchUiButtonId::None;
}

int touchMapCatalogIndexForRowButton(
  TouchUiButtonId id
) {
  size_t visibleRow = TOUCH_MAP_ROWS_PER_PAGE;

  switch (id) {
    case TouchUiButtonId::MapRow0:
      visibleRow = 0;
      break;

    case TouchUiButtonId::MapRow1:
      visibleRow = 1;
      break;

    case TouchUiButtonId::MapRow2:
      visibleRow = 2;
      break;

    default:
      return -1;
  }

  const size_t catalogIndex =
    touchMapListOffset + visibleRow;

  return catalogIndex < touchMapCatalogCount
    ? static_cast<int>(catalogIndex)
    : -1;
}


bool touchMapDraftIsUsable() {
  return
    touchMapDraftIndex >= 0 &&
    static_cast<size_t>(touchMapDraftIndex) < touchMapCatalogCount &&
    touchMapCatalog[touchMapDraftIndex].pngValid;
}


String touchUiButtonDisplayLabel(
  const TouchUiButton &button
) {
  switch (button.id) {
    case TouchUiButtonId::ClockFormat:
      return draftTimeSettings.use24Hour
        ? "24-HOUR"
        : "12-HOUR";

    case TouchUiButtonId::ShowSeconds:
      return draftTimeSettings.showSeconds
        ? "ON"
        : "OFF";

    case TouchUiButtonId::FlipDisplay:
      return draftDisplaySettings.flip180
        ? "FLIPPED"
        : "NORMAL";

    case TouchUiButtonId::LatitudeHemisphere:
      return draftLatitudeSouth
        ? "SOUTH"
        : "NORTH";

    case TouchUiButtonId::LongitudeHemisphere:
      return draftLongitudeWest
        ? "WEST"
        : "EAST";

    case TouchUiButtonId::CoordinateStep: {
      const double step =
        COORDINATE_STEPS[
          draftCoordinateStepIndex
        ];

      if (step < 0.5) {
        return "STEP 0.1 DEG";
      }

      if (step < 5.0) {
        return "STEP 1 DEG";
      }

      return "STEP 10 DEG";
    }

    case TouchUiButtonId::HomeMarker:
      return draftLocationSettings.showHomeMarker
        ? "ON"
        : "OFF";

    case TouchUiButtonId::CoordinateGrid:
      return draftLocationSettings.showCoordinateGrid
        ? "ON"
        : "OFF";

    case TouchUiButtonId::ShowSun:
      return draftOverlaySettings.showSun
        ? "ON"
        : "OFF";

    case TouchUiButtonId::ShowMoon:
      return draftOverlaySettings.showMoon
        ? "ON"
        : "OFF";

    case TouchUiButtonId::ShowISS:
      return draftOverlaySettings.showISS
        ? "ON"
        : "OFF";

    case TouchUiButtonId::ShowIssTrack:
      return draftOverlaySettings.showIssTrack
        ? "ON"
        : "OFF";

    case TouchUiButtonId::IssTrackStyle:
      return draftOverlaySettings.issTrackDotted
        ? "DOTTED"
        : "SOLID";

    default:
      return String(button.label);
  }
}

void drawTouchUiButton(
  const TouchUiButton &button,
  bool pressed
) {
  if (
    button.id == TouchUiButtonId::MapRow0 ||
    button.id == TouchUiButtonId::MapRow1 ||
    button.id == TouchUiButtonId::MapRow2
  ) {
    size_t visibleRow = 0;

    if (button.id == TouchUiButtonId::MapRow1) {
      visibleRow = 1;
    } else if (button.id == TouchUiButtonId::MapRow2) {
      visibleRow = 2;
    }

    drawTouchUiMapRow(
      visibleRow,
      pressed
    );
    return;
  }

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
    5,
    fillColor
  );

  lcd.drawRoundRect(
    button.x,
    button.y,
    button.w,
    button.h,
    5,
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
    touchUiButtonDisplayLabel(button),
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

// ------------------------------------------------------------
// Shared screen drawing
// ------------------------------------------------------------

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
    30,
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
    15
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

void drawTouchUiSettingsHint(
  const char *text
) {
  lcd.fillRect(
    0,
    30,
    SCREEN_W,
    12,
    TFT_BLACK
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    text,
    SCREEN_W / 2,
    35
  );
}

void drawTouchUiRow(
  int y,
  const char *label
) {
  lcd.fillRoundRect(
    8,
    y,
    304,
    28,
    4,
    0x2104
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_left
  );

  lcd.setTextColor(
    TFT_WHITE,
    0x2104
  );

  lcd.drawString(
    label,
    16,
    y + 14
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

    case TouchUiPage::MapPreview:
      return "MAP PREVIEW";

    case TouchUiPage::MapMaintenance:
      return "MAP TOOLS";

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
    "Page reserved for a later alpha",
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
    "Wi-Fi scan and keyboard controls will be added later.",
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

// ------------------------------------------------------------
// Time/display settings page
// ------------------------------------------------------------

void drawTouchUiTimeDisplay() {
  drawTouchUiHeader(
    "TIME / DISPLAY"
  );

  drawTouchUiSettingsHint();

  drawTouchUiRow(
    43,
    "TIME ZONE"
  );

  drawTouchUiButton(
    TIME_ZONE_PREVIOUS
  );

  drawTouchUiButton(
    TIME_ZONE_NEXT
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_right
  );

  lcd.setTextColor(
    TFT_YELLOW,
    0x2104
  );

  lcd.drawString(
    timeZonePresetName(
      draftTimeSettings.timeZone
    ),
    268,
    57
  );

  drawTouchUiRow(
    75,
    "FIXED OFFSET"
  );

  drawTouchUiButton(
    OFFSET_DOWN
  );

  drawTouchUiButton(
    OFFSET_UP
  );

  lcd.setTextDatum(
    textdatum_t::middle_right
  );

  lcd.setTextColor(
    draftTimeSettings.timeZone ==
      TimeZonePreset::FixedOffset
        ? TFT_YELLOW
        : TFT_DARKGREY,
    0x2104
  );

  lcd.drawString(
    formatUtcOffsetMinutes(
      draftUtcOffsetMinutes
    ),
    268,
    89
  );

  drawTouchUiRow(
    107,
    "CLOCK FORMAT"
  );

  drawTouchUiButton(
    CLOCK_FORMAT
  );

  drawTouchUiRow(
    139,
    "SHOW SECONDS"
  );

  drawTouchUiButton(
    SHOW_SECONDS
  );

  drawTouchUiRow(
    171,
    "ORIENTATION"
  );

  drawTouchUiButton(
    FLIP_DISPLAY
  );

  drawTouchUiButton(
    TIME_CANCEL
  );

  drawTouchUiButton(
    TIME_APPLY
  );
}

// ------------------------------------------------------------
// Location settings page
// ------------------------------------------------------------

void drawTouchUiLocation() {
  drawTouchUiHeader(
    "LOCATION"
  );

  drawTouchUiSettingsHint(
    "Use - / + and tap hemisphere; APPLY saves"
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_left
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  lcd.drawString(
    "LAT",
    8,
    57
  );

  drawTouchUiButton(
    LATITUDE_DOWN
  );

  drawTouchUiButton(
    LATITUDE_UP
  );

  drawTouchUiButton(
    LATITUDE_HEMISPHERE
  );

  lcd.fillRoundRect(
    86,
    43,
    126,
    28,
    4,
    0x2104
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    TFT_YELLOW,
    0x2104
  );

  char coordinateText[24];

  snprintf(
    coordinateText,
    sizeof(coordinateText),
    "%.1f deg",
    draftLatitudeMagnitude
  );

  lcd.drawString(
    coordinateText,
    149,
    57
  );

  lcd.setTextDatum(
    textdatum_t::middle_left
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  lcd.drawString(
    "LON",
    8,
    92
  );

  drawTouchUiButton(
    LONGITUDE_DOWN
  );

  drawTouchUiButton(
    LONGITUDE_UP
  );

  drawTouchUiButton(
    LONGITUDE_HEMISPHERE
  );

  lcd.fillRoundRect(
    86,
    78,
    126,
    28,
    4,
    0x2104
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    TFT_YELLOW,
    0x2104
  );

  snprintf(
    coordinateText,
    sizeof(coordinateText),
    "%.1f deg",
    draftLongitudeMagnitude
  );

  lcd.drawString(
    coordinateText,
    149,
    92
  );

  drawTouchUiRow(
    113,
    "ADJUSTMENT"
  );

  drawTouchUiButton(
    COORDINATE_STEP
  );

  drawTouchUiRow(
    145,
    "HOME MARKER"
  );

  drawTouchUiButton(
    HOME_MARKER
  );

  drawTouchUiRow(
    177,
    "COORDINATE GRID"
  );

  drawTouchUiButton(
    COORDINATE_GRID
  );

  drawTouchUiButton(
    LOCATION_CANCEL
  );

  drawTouchUiButton(
    LOCATION_APPLY
  );
}

// ------------------------------------------------------------
// Overlay settings page
// ------------------------------------------------------------

void drawTouchUiOverlays() {
  drawTouchUiHeader(
    "OVERLAYS"
  );

  drawTouchUiSettingsHint();

  drawTouchUiRow(
    43,
    "SUN"
  );

  drawTouchUiButton(
    SHOW_SUN
  );

  drawTouchUiRow(
    75,
    "MOON"
  );

  drawTouchUiButton(
    SHOW_MOON
  );

  drawTouchUiRow(
    107,
    "ISS MARKER"
  );

  drawTouchUiButton(
    SHOW_ISS
  );

  drawTouchUiRow(
    139,
    "ISS ORBIT TRACK"
  );

  drawTouchUiButton(
    SHOW_ISS_TRACK
  );

  drawTouchUiRow(
    171,
    "TRACK STYLE"
  );

  drawTouchUiButton(
    ISS_TRACK_STYLE
  );

  drawTouchUiButton(
    OVERLAY_CANCEL
  );

  drawTouchUiButton(
    OVERLAY_APPLY
  );
}

// ------------------------------------------------------------
// Maps page, preview, and maintenance
// ------------------------------------------------------------

String shortenedTouchMapFilename(
  const String &filename,
  size_t maximumLength
) {
  if (filename.length() <= maximumLength) {
    return filename;
  }

  if (maximumLength <= 3) {
    return filename.substring(0, maximumLength);
  }

  return
    filename.substring(0, maximumLength - 3) +
    "...";
}


void refreshTouchMapCatalog(
  bool fullValidation,
  bool preserveDraft
) {
  const String previousSelected =
    selectedDayMapFilename;

  const String preferredFilename =
    preserveDraft && touchMapDraftFilename.length() > 0
      ? touchMapDraftFilename
      : selectedDayMapFilename;

  touchMapCatalogCount = 0;
  touchMapCatalogTotalCount = 0;
  touchMapDraftIndex = -1;
  touchMapFullValidation = fullValidation;

  if (SD.cardType() == CARD_NONE) {
    systemStatus.sdMounted = false;
    touchMapDraftFilename = "";
    touchMapListOffset = 0;
    return;
  }

  systemStatus.sdMounted = true;
  refreshStorageStatus();

  touchMapCatalogCount = scanDaylightMaps(
    touchMapCatalog,
    MAX_DAYLIGHT_MAPS,
    fullValidation,
    &touchMapCatalogTotalCount
  );

  for (
    size_t index = 0;
    index < touchMapCatalogCount;
    ++index
  ) {
    if (
      touchMapCatalog[index].filename ==
        preferredFilename
    ) {
      touchMapDraftIndex =
        static_cast<int>(index);
      break;
    }
  }

  if (touchMapDraftIndex < 0) {
    for (
      size_t index = 0;
      index < touchMapCatalogCount;
      ++index
    ) {
      if (touchMapCatalog[index].selected) {
        touchMapDraftIndex =
          static_cast<int>(index);
        break;
      }
    }
  }

  if (
    touchMapDraftIndex < 0 &&
    touchMapCatalogCount > 0
  ) {
    touchMapDraftIndex = 0;
  }

  if (touchMapDraftIndex >= 0) {
    touchMapDraftFilename =
      touchMapCatalog[touchMapDraftIndex].filename;

    touchMapListOffset =
      (
        static_cast<size_t>(touchMapDraftIndex) /
        TOUCH_MAP_ROWS_PER_PAGE
      ) * TOUCH_MAP_ROWS_PER_PAGE;
  } else {
    touchMapDraftFilename = "";
    touchMapListOffset = 0;
  }

  if (
    previousSelected.length() > 0 &&
    selectedDayMapFilename != previousSelected
  ) {
    touchMapMessage =
      String("Fallback selected: ") +
      selectedDayMapFilename;
    touchMapMessageError = false;
  }
}


void drawTouchUiMapMessage() {
  lcd.fillRect(
    0,
    30,
    SCREEN_W,
    12,
    TFT_BLACK
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  if (touchMapMessage.length() > 0) {
    lcd.setTextColor(
      touchMapMessageError
        ? TFT_RED
        : TFT_CYAN,
      TFT_BLACK
    );

    lcd.drawString(
      shortenedTouchMapFilename(
        touchMapMessage,
        48
      ),
      SCREEN_W / 2,
      35
    );
    return;
  }

  const size_t pageNumber =
    touchMapCatalogCount == 0
      ? 0
      : touchMapListOffset /
          TOUCH_MAP_ROWS_PER_PAGE + 1;

  const size_t pageCount =
    touchMapCatalogCount == 0
      ? 0
      : (
          touchMapCatalogCount +
          TOUCH_MAP_ROWS_PER_PAGE - 1
        ) /
        TOUCH_MAP_ROWS_PER_PAGE;

  char line[96];

  snprintf(
    line,
    sizeof(line),
    "%u map%s  page %u/%u  %s validation",
    static_cast<unsigned>(touchMapCatalogCount),
    touchMapCatalogCount == 1 ? "" : "s",
    static_cast<unsigned>(pageNumber),
    static_cast<unsigned>(pageCount),
    touchMapFullValidation ? "full" : "quick"
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    line,
    SCREEN_W / 2,
    35
  );
}


void drawTouchUiMapRow(
  size_t visibleRow,
  bool pressed
) {
  const TouchUiButton *button = nullptr;

  switch (visibleRow) {
    case 0:
      button = &MAP_ROW_0;
      break;

    case 1:
      button = &MAP_ROW_1;
      break;

    case 2:
      button = &MAP_ROW_2;
      break;

    default:
      return;
  }

  const size_t catalogIndex =
    touchMapListOffset + visibleRow;

  if (catalogIndex >= touchMapCatalogCount) {
    lcd.fillRect(
      button->x,
      button->y,
      button->w,
      button->h,
      TFT_BLACK
    );
    return;
  }

  const DaylightMapInfo &map =
    touchMapCatalog[catalogIndex];

  const bool draftSelected =
    static_cast<int>(catalogIndex) ==
      touchMapDraftIndex;

  uint16_t fillColor = 0x2104;

  if (map.selected) {
    fillColor = TFT_DARKGREEN;
  }

  if (draftSelected) {
    fillColor = TFT_NAVY;
  }

  if (pressed) {
    fillColor = TFT_YELLOW;
  }

  const uint16_t textColor =
    pressed ? TFT_BLACK : TFT_WHITE;

  const uint16_t borderColor =
    pressed
      ? TFT_BLACK
      : draftSelected
          ? TFT_YELLOW
          : map.selected
              ? TFT_CYAN
              : TFT_DARKGREY;

  lcd.fillRoundRect(
    button->x,
    button->y,
    button->w,
    button->h,
    4,
    fillColor
  );

  lcd.drawRoundRect(
    button->x,
    button->y,
    button->w,
    button->h,
    4,
    borderColor
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::top_left
  );

  lcd.setTextColor(
    textColor,
    fillColor
  );

  String filename =
    shortenedTouchMapFilename(
      map.filename,
      39
    );

  if (map.selected) {
    filename = String("* ") + filename;
  } else {
    filename = String("  ") + filename;
  }

  lcd.drawString(
    filename,
    button->x + 5,
    button->y + 4
  );

  String status =
    map.pngValid
      ? "PNG OK"
      : "PNG BAD";

  status +=
    map.cacheValid
      ? "   CACHE OK"
      : "   CACHE MISSING";

  if (map.selected) {
    status += "   ACTIVE";
  }

  lcd.setTextColor(
    pressed
      ? TFT_BLACK
      : map.pngValid
          ? TFT_CYAN
          : TFT_RED,
    fillColor
  );

  lcd.drawString(
    status,
    button->x + 5,
    button->y + 19
  );
}


void drawTouchUiMaps() {
  drawTouchUiHeader(
    "MAPS"
  );

  drawTouchUiMapMessage();

  drawTouchUiMapRow(0);
  drawTouchUiMapRow(1);
  drawTouchUiMapRow(2);

  drawTouchUiButton(
    MAP_PAGE_PREVIOUS
  );

  drawTouchUiButton(
    MAP_PAGE_NEXT
  );

  drawTouchUiButton(
    MAP_PREVIEW
  );

  drawTouchUiButton(
    MAP_APPLY
  );

  drawTouchUiButton(
    MAP_TOOLS
  );

  drawTouchUiButton(
    MAP_BACK
  );

  drawTouchUiButton(
    MAP_HOME
  );

  drawTouchUiFooter();
}


void drawTouchUiMapPreview() {
  drawTouchUiHeader(
    "MAP PREVIEW"
  );

  lcd.fillRect(
    0,
    30,
    SCREEN_W,
    157,
    TFT_BLACK
  );

  bool previewed = false;

  if (touchMapDraftIsUsable()) {
    previewed = renderPngPreview(
      touchMapCatalog[
        touchMapDraftIndex
      ].pngPath.c_str(),
      30,
      157
    );
  }

  lcd.fillRect(
    0,
    30,
    SCREEN_W,
    14,
    TFT_BLACK
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_center
  );

  lcd.setTextColor(
    previewed ? TFT_WHITE : TFT_RED,
    TFT_BLACK
  );

  lcd.drawString(
    previewed
      ? shortenedTouchMapFilename(
          touchMapDraftFilename,
          45
        )
      : String("Preview unavailable"),
    SCREEN_W / 2,
    36
  );

  drawTouchUiButton(
    PREVIEW_BACK
  );

  drawTouchUiButton(
    PREVIEW_APPLY
  );

  drawTouchUiButton(
    PREVIEW_TOOLS
  );

  drawTouchUiFooter();
}


void drawTouchUiMapMaintenance() {
  drawTouchUiHeader(
    "MAP TOOLS"
  );

  lcd.fillRect(
    0,
    30,
    SCREEN_W,
    40,
    TFT_BLACK
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::top_center
  );

  lcd.setTextColor(
    TFT_WHITE,
    TFT_BLACK
  );

  lcd.drawString(
    String("Selected: ") +
      shortenedTouchMapFilename(
        touchMapDraftFilename,
        34
      ),
    SCREEN_W / 2,
    33
  );

  String status;

  if (touchMapDraftIsUsable()) {
    const DaylightMapInfo &map =
      touchMapCatalog[touchMapDraftIndex];

    status =
      map.cacheValid
        ? "Day cache OK"
        : "Day cache missing";
  } else {
    status = "No usable daylight map";
  }

  status +=
    rawMapIsValid(NIGHT_RAW_FILE)
      ? "   Night cache OK"
      : "   Night cache missing";

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    shortenedTouchMapFilename(
      status,
      47
    ),
    SCREEN_W / 2,
    47
  );

  if (touchMapMessage.length() > 0) {
    lcd.setTextColor(
      touchMapMessageError
        ? TFT_RED
        : TFT_YELLOW,
      TFT_BLACK
    );

    lcd.drawString(
      shortenedTouchMapFilename(
        touchMapMessage,
        47
      ),
      SCREEN_W / 2,
      59
    );
  }

  for (
    const TouchUiButton *button :
      MAP_MAINTENANCE_BUTTONS
  ) {
    drawTouchUiButton(
      *button
    );
  }

  drawTouchUiFooter();
}


void drawTouchUiBusy(
  const char *title,
  const String &message
) {
  drawTouchUiHeader(
    title
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
    message,
    SCREEN_W / 2,
    108
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextColor(
    TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    "Please wait; do not remove the SD card.",
    SCREEN_W / 2,
    136
  );
}


void applyTouchUiMap() {
  if (!touchMapDraftIsUsable()) {
    touchMapMessage = "Selected PNG is not usable";
    touchMapMessageError = true;
    drawTouchUiMaps();
    return;
  }

  const String filename =
    touchMapCatalog[touchMapDraftIndex].filename;

  drawTouchUiBusy(
    "APPLY MAP",
    String("Applying ") +
      shortenedTouchMapFilename(filename, 26)
  );

  const bool ok =
    activateDayMap(
      filename,
      false
    );

  Serial.printf(
    "Touch UI: map apply %s; ok=%d\n",
    filename.c_str(),
    ok
  );

  if (ok) {
    closeTouchUi(true);
    return;
  }

  refreshTouchMapCatalog(false, true);
  touchMapMessage = "Map could not be applied";
  touchMapMessageError = true;
  activeTouchUiPage = TouchUiPage::Maps;
  drawActiveTouchUiPage();
}


void rebuildTouchUiSelectedMapCache() {
  if (!touchMapDraftIsUsable()) {
    touchMapMessage = "No usable map selected";
    touchMapMessageError = true;
    drawTouchUiMapMaintenance();
    return;
  }

  const String filename =
    touchMapCatalog[touchMapDraftIndex].filename;

  drawTouchUiBusy(
    "REBUILD CACHE",
    shortenedTouchMapFilename(filename, 28)
  );

  const bool ok =
    rebuildDayMapCache(filename);

  refreshTouchMapCatalog(false, true);
  touchMapMessage = ok
    ? "Selected cache rebuilt"
    : "Selected cache rebuild failed";
  touchMapMessageError = !ok;
  activeTouchUiPage = TouchUiPage::MapMaintenance;
  drawActiveTouchUiPage();
}


void rebuildTouchUiNightCache() {
  drawTouchUiBusy(
    "REBUILD CACHE",
    "Rebuilding shared night map"
  );

  const bool ok =
    rebuildMapCache(
      false,
      true
    );

  refreshTouchMapCatalog(false, true);
  touchMapMessage = ok
    ? "Night cache rebuilt"
    : "Night cache rebuild failed";
  touchMapMessageError = !ok;
  activeTouchUiPage = TouchUiPage::MapMaintenance;
  drawActiveTouchUiPage();
}


void rebuildTouchUiAllCaches() {
  drawTouchUiBusy(
    "REBUILD CACHES",
    "Rebuilding daylight maps"
  );

  bool ok =
    rebuildAllDayMapCaches();

  if (ok) {
    drawTouchUiBusy(
      "REBUILD CACHES",
      "Rebuilding shared night map"
    );

    ok = rebuildMapCache(
      false,
      true
    );
  }

  refreshTouchMapCatalog(false, true);
  touchMapMessage = ok
    ? "All map caches rebuilt"
    : "One or more cache rebuilds failed";
  touchMapMessageError = !ok;
  activeTouchUiPage = TouchUiPage::MapMaintenance;
  drawActiveTouchUiPage();
}


// ------------------------------------------------------------
// Diagnostics page
// ------------------------------------------------------------

void drawTouchUiDiagnosticsData() {
  const TouchDiagnostics &diagnostics =
    getTouchDiagnostics();

  lcd.fillRect(
    0,
    31,
    SCREEN_W,
    116,
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

// ------------------------------------------------------------
// Page state and settings drafts
// ------------------------------------------------------------

void prepareTouchUiDraft(
  TouchUiPage page
) {
  switch (page) {
    case TouchUiPage::TimeDisplay:
      draftTimeSettings =
        timeSettings;

      draftDisplaySettings =
        displaySettings;

      draftUtcOffsetMinutes =
        networkSettings.utcOffsetMinutes;
      break;

    case TouchUiPage::Location:
      draftLocationSettings =
        locationGridSettings;

      draftLatitudeMagnitude =
        fabs(
          locationGridSettings.homeLatitude
        );

      draftLongitudeMagnitude =
        fabs(
          locationGridSettings.homeLongitude
        );

      draftLatitudeSouth =
        locationGridSettings.homeLatitude < 0.0;

      draftLongitudeWest =
        locationGridSettings.homeLongitude < 0.0;

      draftCoordinateStepIndex = 1;
      break;

    case TouchUiPage::Overlays:
      draftOverlaySettings =
        overlaySettings;
      break;

    case TouchUiPage::Maps:
      touchMapMessage = "";
      touchMapMessageError = false;
      refreshTouchMapCatalog(
        false,
        true
      );
      break;

    case TouchUiPage::MapPreview:
    case TouchUiPage::MapMaintenance:
      break;

    default:
      break;
  }
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

    case TouchUiPage::TimeDisplay:
      drawTouchUiTimeDisplay();
      break;

    case TouchUiPage::Location:
      drawTouchUiLocation();
      break;

    case TouchUiPage::Overlays:
      drawTouchUiOverlays();
      break;

    case TouchUiPage::Diagnostics:
      drawTouchUiDiagnostics();
      break;

    case TouchUiPage::Maps:
      drawTouchUiMaps();
      break;

    case TouchUiPage::MapPreview:
      drawTouchUiMapPreview();
      break;

    case TouchUiPage::MapMaintenance:
      drawTouchUiMapMaintenance();
      break;

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
  prepareTouchUiDraft(
    page
  );

  activeTouchUiPage =
    page;

  touchUiLastActivityAt =
    millis();

  drawActiveTouchUiPage();
}

// ------------------------------------------------------------
// Applying staged settings
// ------------------------------------------------------------

void applyTouchUiTimeDisplay() {
  bool timeZoneChanged = false;
  bool clockDisplayChanged = false;
  bool orientationChanged = false;

  const bool saved =
    applyTimeDisplaySettings(
      draftTimeSettings,
      draftUtcOffsetMinutes,
      draftDisplaySettings,
      timeZoneChanged,
      clockDisplayChanged,
      orientationChanged
    );

  Serial.printf(
    "Touch UI: time/display apply saved=%d "
    "tz=%d clock=%d orientation=%d\n",
    saved,
    timeZoneChanged,
    clockDisplayChanged,
    orientationChanged
  );

  if (timeZoneChanged) {
    applyConfiguredTimeZone();
  }

  if (orientationChanged) {
    applyDisplayRotation();
    lcd.fillScreen(TFT_BLACK);
  }

  closeTouchUi(true);
}

void applyTouchUiLocation() {
  draftLocationSettings.homeLatitude =
    draftLatitudeSouth
      ? -draftLatitudeMagnitude
      : draftLatitudeMagnitude;

  draftLocationSettings.homeLongitude =
    draftLongitudeWest
      ? -draftLongitudeMagnitude
      : draftLongitudeMagnitude;

  const bool saved =
    applyLocationSettings(
      draftLocationSettings
    );

  Serial.printf(
    "Touch UI: location apply saved=%d; %s\n",
    saved,
    formatHomeLocation().c_str()
  );

  closeTouchUi(true);
}

void applyTouchUiOverlays() {
  const bool saved =
    applyOverlaySettings(
      draftOverlaySettings
    );

  Serial.printf(
    "Touch UI: overlays apply saved=%d\n",
    saved
  );

  if (
    (
      overlaySettings.showISS ||
      overlaySettings.showIssTrack
    ) &&
    !issPosition.valid
  ) {
    fetchIssPosition();
  }

  if (
    overlaySettings.showIssTrack &&
    issPosition.valid
  ) {
    calculateIssOrbitTrack();
  }

  closeTouchUi(true);
}

// ------------------------------------------------------------
// Button actions
// ------------------------------------------------------------

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

    case TouchUiButtonId::MapRow0:
    case TouchUiButtonId::MapRow1:
    case TouchUiButtonId::MapRow2: {
      const int catalogIndex =
        touchMapCatalogIndexForRowButton(id);

      if (catalogIndex >= 0) {
        touchMapDraftIndex = catalogIndex;
        touchMapDraftFilename =
          touchMapCatalog[catalogIndex].filename;
        touchMapMessage = "";
        touchMapMessageError = false;
        drawTouchUiMaps();
      }
      break;
    }

    case TouchUiButtonId::MapPagePrevious:
      if (touchMapListOffset >= TOUCH_MAP_ROWS_PER_PAGE) {
        touchMapListOffset -= TOUCH_MAP_ROWS_PER_PAGE;
        drawTouchUiMaps();
      }
      break;

    case TouchUiButtonId::MapPageNext:
      if (
        touchMapListOffset + TOUCH_MAP_ROWS_PER_PAGE <
          touchMapCatalogCount
      ) {
        touchMapListOffset += TOUCH_MAP_ROWS_PER_PAGE;
        drawTouchUiMaps();
      }
      break;

    case TouchUiButtonId::MapPreview:
      if (touchMapDraftIsUsable()) {
        showTouchUiPage(
          TouchUiPage::MapPreview
        );
      } else {
        touchMapMessage = "Select a valid map first";
        touchMapMessageError = true;
        drawTouchUiMaps();
      }
      break;

    case TouchUiButtonId::MapApply:
      applyTouchUiMap();
      break;

    case TouchUiButtonId::MapTools:
      showTouchUiPage(
        TouchUiPage::MapMaintenance
      );
      break;

    case TouchUiButtonId::MapValidate:
      drawTouchUiBusy(
        "VALIDATE MAPS",
        "Decoding daylight PNG files"
      );
      refreshTouchMapCatalog(
        true,
        true
      );
      touchMapMessage = "Full PNG validation complete";
      touchMapMessageError = false;
      activeTouchUiPage = TouchUiPage::MapMaintenance;
      drawActiveTouchUiPage();
      break;

    case TouchUiButtonId::MapRescan:
      refreshTouchMapCatalog(
        false,
        true
      );
      touchMapMessage = "Map directory rescanned";
      touchMapMessageError = false;
      activeTouchUiPage = TouchUiPage::MapMaintenance;
      drawActiveTouchUiPage();
      break;

    case TouchUiButtonId::MapRebuildSelected:
      rebuildTouchUiSelectedMapCache();
      break;

    case TouchUiButtonId::MapRebuildNight:
      rebuildTouchUiNightCache();
      break;

    case TouchUiButtonId::MapRebuildAll:
      rebuildTouchUiAllCaches();
      break;

    case TouchUiButtonId::TimeZonePrevious: {
      int value =
        static_cast<int>(
          draftTimeSettings.timeZone
        ) - 1;

      if (value < 0) {
        value = static_cast<int>(
          TimeZonePreset::USPacific
        );
      }

      draftTimeSettings.timeZone =
        static_cast<TimeZonePreset>(
          value
        );

      drawTouchUiTimeDisplay();
      break;
    }

    case TouchUiButtonId::TimeZoneNext: {
      int value =
        static_cast<int>(
          draftTimeSettings.timeZone
        ) + 1;

      if (
        value > static_cast<int>(
          TimeZonePreset::USPacific
        )
      ) {
        value = 0;
      }

      draftTimeSettings.timeZone =
        static_cast<TimeZonePreset>(
          value
        );

      drawTouchUiTimeDisplay();
      break;
    }

    case TouchUiButtonId::OffsetDown:
      draftUtcOffsetMinutes =
        max(
          MIN_UTC_OFFSET_MINUTES,
          draftUtcOffsetMinutes - 15
        );

      drawTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::OffsetUp:
      draftUtcOffsetMinutes =
        min(
          MAX_UTC_OFFSET_MINUTES,
          draftUtcOffsetMinutes + 15
        );

      drawTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::ClockFormat:
      draftTimeSettings.use24Hour =
        !draftTimeSettings.use24Hour;

      drawTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::ShowSeconds:
      draftTimeSettings.showSeconds =
        !draftTimeSettings.showSeconds;

      drawTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::FlipDisplay:
      draftDisplaySettings.flip180 =
        !draftDisplaySettings.flip180;

      drawTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::ApplyTimeDisplay:
      applyTouchUiTimeDisplay();
      break;

    case TouchUiButtonId::LatitudeDown:
      draftLatitudeMagnitude =
        max(
          0.0,
          draftLatitudeMagnitude -
            COORDINATE_STEPS[
              draftCoordinateStepIndex
            ]
        );

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::LatitudeUp:
      draftLatitudeMagnitude =
        min(
          90.0,
          draftLatitudeMagnitude +
            COORDINATE_STEPS[
              draftCoordinateStepIndex
            ]
        );

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::LatitudeHemisphere:
      draftLatitudeSouth =
        !draftLatitudeSouth;

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::LongitudeDown:
      draftLongitudeMagnitude =
        max(
          0.0,
          draftLongitudeMagnitude -
            COORDINATE_STEPS[
              draftCoordinateStepIndex
            ]
        );

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::LongitudeUp:
      draftLongitudeMagnitude =
        min(
          180.0,
          draftLongitudeMagnitude +
            COORDINATE_STEPS[
              draftCoordinateStepIndex
            ]
        );

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::LongitudeHemisphere:
      draftLongitudeWest =
        !draftLongitudeWest;

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::CoordinateStep:
      draftCoordinateStepIndex =
        static_cast<uint8_t>(
          (
            draftCoordinateStepIndex + 1
          ) %
          (
            sizeof(COORDINATE_STEPS) /
            sizeof(COORDINATE_STEPS[0])
          )
        );

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::HomeMarker:
      draftLocationSettings.showHomeMarker =
        !draftLocationSettings.showHomeMarker;

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::CoordinateGrid:
      draftLocationSettings.showCoordinateGrid =
        !draftLocationSettings.showCoordinateGrid;

      drawTouchUiLocation();
      break;

    case TouchUiButtonId::ApplyLocation:
      applyTouchUiLocation();
      break;

    case TouchUiButtonId::ShowSun:
      draftOverlaySettings.showSun =
        !draftOverlaySettings.showSun;

      drawTouchUiOverlays();
      break;

    case TouchUiButtonId::ShowMoon:
      draftOverlaySettings.showMoon =
        !draftOverlaySettings.showMoon;

      drawTouchUiOverlays();
      break;

    case TouchUiButtonId::ShowISS:
      draftOverlaySettings.showISS =
        !draftOverlaySettings.showISS;

      drawTouchUiOverlays();
      break;

    case TouchUiButtonId::ShowIssTrack:
      draftOverlaySettings.showIssTrack =
        !draftOverlaySettings.showIssTrack;

      drawTouchUiOverlays();
      break;

    case TouchUiButtonId::IssTrackStyle:
      draftOverlaySettings.issTrackDotted =
        !draftOverlaySettings.issTrackDotted;

      drawTouchUiOverlays();
      break;

    case TouchUiButtonId::ApplyOverlays:
      applyTouchUiOverlays();
      break;

    case TouchUiButtonId::CancelSettings:
      showTouchUiPage(
        TouchUiPage::MainMenu
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
      if (
        activeTouchUiPage == TouchUiPage::MapPreview ||
        activeTouchUiPage == TouchUiPage::MapMaintenance
      ) {
        showTouchUiPage(
          TouchUiPage::Maps
        );
      } else {
        showTouchUiPage(
          TouchUiPage::MainMenu
        );
      }
      break;

    case TouchUiButtonId::Home:
      closeTouchUi(true);
      break;

    case TouchUiButtonId::None:
    default:
      break;
  }
}

// ------------------------------------------------------------
// Press tracking and release activation
// ------------------------------------------------------------

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
    } else if (
      activeTouchUiOutsideFrames < 255
    ) {
      ++activeTouchUiOutsideFrames;
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
    } else if (
      activeTouchUiOutsideFrames < 255
    ) {
      ++activeTouchUiOutsideFrames;
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
