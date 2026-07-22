// ============================================================
// WorldClock Version 5.4-alpha5 touchscreen user interface
// ============================================================
//
// Version 5.4 adds cached RainViewer radar with explicit LATEST/LOOP modes
// while retaining Market Mood, weather, maps, and display diagnostics.
// ============================================================

namespace {

enum class TouchUiPage : uint8_t {
  Clock,
  Weather,
  WeatherRadar,
  Market,
  MainMenu,
  TimeDisplay,
  Location,
  Overlays,
  Maps,
  MapPreview,
  MapMaintenance,
  Network,
  Keyboard,
  Diagnostics,
  DisplayTest
};

enum class TouchUiButtonId : uint8_t {
  None,
  OpenMenu,
  OpenWeather,
  OpenMarket,
  WeatherRadar,
  WeatherForecast,
  WeatherRefresh,
  WeatherRadarRefresh,
  WeatherRadarLatest,
  WeatherRadarLoop,
  MarketToday,
  MarketThirtyDays,
  MarketRefresh,
  TimeDisplay,
  Location,
  Overlays,
  Maps,
  Network,
  Diagnostics,
  DisplayTest,
  BrightnessDown,
  BrightnessUp,

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

  NetworkRow0,
  NetworkRow1,
  NetworkRow2,
  NetworkPagePrevious,
  NetworkPageNext,
  NetworkOtherSsid,
  NetworkPassword,
  NetworkScan,
  NetworkForget,
  NetworkConnect,

  KeyboardCell,
  KeyboardCancel,
  KeyboardPrevious,
  KeyboardNext,
  KeyboardAccept,

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
  RecalibrateTouch,
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

enum class TouchKeyboardMode : uint8_t {
  Upper,
  Lower,
  Symbols,
  Symbols2
};

enum class TouchKeyboardTarget : uint8_t {
  None,
  Ssid,
  Password
};

enum class TouchMarketPeriod : uint8_t {
  Today,
  ThirtyDays
};

enum class TouchKeyboardCellKind : uint8_t {
  Character,
  Space,
  Delete,
  Upper,
  Lower,
  Symbols,
  MoreSymbols,
  ToggleMaskOrClear,
  Enter
};

struct TouchKeyboardCell {
  const char *label;
  TouchKeyboardCellKind kind;
  char value;
};

struct TouchNetworkEntry {
  String ssid;
  int32_t rssi = -127;
  bool open = false;
};

// ------------------------------------------------------------
// Clock weather control and weather pages
// ------------------------------------------------------------

static constexpr TouchUiButton CLOCK_WEATHER_BUTTON = {
  TouchUiButtonId::OpenWeather,
  WEATHER_BUTTON_X, WEATHER_BUTTON_Y, WEATHER_BUTTON_W, WEATHER_BUTTON_H,
  "WEATHER",
  0x18E3
};

static constexpr TouchUiButton CLOCK_MARKET_BUTTON = {
  TouchUiButtonId::OpenMarket,
  MARKET_BUTTON_X, MARKET_BUTTON_Y, MARKET_BUTTON_W, MARKET_BUTTON_H,
  "MARKET",
  0x2104
};

static constexpr TouchUiButton WEATHER_RADAR_BUTTON = {
  TouchUiButtonId::WeatherRadar,
  8, 207, 96, 27,
  "RADAR",
  TFT_PURPLE
};

static constexpr TouchUiButton WEATHER_REFRESH_BUTTON = {
  TouchUiButtonId::WeatherRefresh,
  112, 207, 96, 27,
  "REFRESH",
  TFT_NAVY
};

static constexpr TouchUiButton WEATHER_HOME_BUTTON = {
  TouchUiButtonId::Home,
  216, 207, 96, 27,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton RADAR_FORECAST_BUTTON = {
  TouchUiButtonId::WeatherForecast,
  4, 207, 46, 27,
  "FCST",
  TFT_NAVY
};

static constexpr TouchUiButton RADAR_REFRESH_BUTTON = {
  TouchUiButtonId::WeatherRadarRefresh,
  54, 207, 46, 27,
  "REFR",
  TFT_PURPLE
};

static constexpr TouchUiButton RADAR_LATEST_BUTTON = {
  TouchUiButtonId::WeatherRadarLatest,
  104, 207, 68, 27,
  "LATEST",
  TFT_DARKGREEN
};

static constexpr TouchUiButton RADAR_LOOP_BUTTON = {
  TouchUiButtonId::WeatherRadarLoop,
  176, 207, 68, 27,
  "LOOP",
  TFT_MAROON
};

static constexpr TouchUiButton RADAR_HOME_BUTTON = {
  TouchUiButtonId::Home,
  248, 207, 68, 27,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton MARKET_TODAY_BUTTON = {
  TouchUiButtonId::MarketToday,
  8, 126, 148, 28,
  "TODAY",
  TFT_NAVY
};

static constexpr TouchUiButton MARKET_THIRTY_DAY_BUTTON = {
  TouchUiButtonId::MarketThirtyDays,
  164, 126, 148, 28,
  "30 DAYS",
  TFT_PURPLE
};

static constexpr TouchUiButton MARKET_REFRESH_BUTTON = {
  TouchUiButtonId::MarketRefresh,
  8, 207, 148, 27,
  "REFRESH",
  TFT_NAVY
};

static constexpr TouchUiButton MARKET_HOME_BUTTON = {
  TouchUiButtonId::Home,
  164, 207, 148, 27,
  "CLOCK",
  TFT_DARKGREEN
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
// Network and keyboard buttons
// ------------------------------------------------------------

static constexpr TouchUiButton NETWORK_ROW_0 = {
  TouchUiButtonId::NetworkRow0,
  8, 43, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton NETWORK_ROW_1 = {
  TouchUiButtonId::NetworkRow1,
  8, 80, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton NETWORK_ROW_2 = {
  TouchUiButtonId::NetworkRow2,
  8, 117, 304, 34,
  "",
  0x2104
};

static constexpr TouchUiButton NETWORK_PAGE_PREVIOUS = {
  TouchUiButtonId::NetworkPagePrevious,
  8, 155, 48, 28,
  "<",
  TFT_DARKGREY
};

static constexpr TouchUiButton NETWORK_PAGE_NEXT = {
  TouchUiButtonId::NetworkPageNext,
  60, 155, 48, 28,
  ">",
  TFT_DARKGREY
};

static constexpr TouchUiButton NETWORK_OTHER_SSID = {
  TouchUiButtonId::NetworkOtherSsid,
  112, 155, 96, 28,
  "OTHER SSID",
  TFT_PURPLE
};

static constexpr TouchUiButton NETWORK_PASSWORD = {
  TouchUiButtonId::NetworkPassword,
  212, 155, 100, 28,
  "PASSWORD",
  TFT_NAVY
};

static constexpr TouchUiButton NETWORK_SCAN = {
  TouchUiButtonId::NetworkScan,
  8, 188, 70, 38,
  "SCAN",
  TFT_DARKGREY
};

static constexpr TouchUiButton NETWORK_FORGET = {
  TouchUiButtonId::NetworkForget,
  82, 188, 70, 38,
  "FORGET",
  TFT_MAROON
};

static constexpr TouchUiButton NETWORK_CONNECT = {
  TouchUiButtonId::NetworkConnect,
  156, 188, 78, 38,
  "CONNECT",
  TFT_DARKGREEN
};

static constexpr TouchUiButton NETWORK_BACK = {
  TouchUiButtonId::Back,
  238, 188, 74, 38,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton KEYBOARD_CANCEL = {
  TouchUiButtonId::KeyboardCancel,
  0, 198, 92, 42,
  "CANCEL",
  TFT_MAROON
};

static constexpr TouchUiButton KEYBOARD_PREVIOUS = {
  TouchUiButtonId::KeyboardPrevious,
  94, 198, 64, 42,
  "<",
  TFT_DARKGREY
};

static constexpr TouchUiButton KEYBOARD_NEXT = {
  TouchUiButtonId::KeyboardNext,
  160, 198, 64, 42,
  ">",
  TFT_DARKGREY
};

static constexpr TouchUiButton KEYBOARD_ACCEPT = {
  TouchUiButtonId::KeyboardAccept,
  226, 198, 94, 42,
  "",
  TFT_DARKGREEN
};

static constexpr TouchKeyboardCell KEYBOARD_UPPER_CELLS[
  TOUCH_KEYBOARD_CELL_COUNT
] = {
  {"A", TouchKeyboardCellKind::Character, 'A'},
  {"B", TouchKeyboardCellKind::Character, 'B'},
  {"C", TouchKeyboardCellKind::Character, 'C'},
  {"D", TouchKeyboardCellKind::Character, 'D'},
  {"E", TouchKeyboardCellKind::Character, 'E'},
  {"F", TouchKeyboardCellKind::Character, 'F'},
  {"G", TouchKeyboardCellKind::Character, 'G'},
  {"H", TouchKeyboardCellKind::Character, 'H'},

  {"I", TouchKeyboardCellKind::Character, 'I'},
  {"J", TouchKeyboardCellKind::Character, 'J'},
  {"K", TouchKeyboardCellKind::Character, 'K'},
  {"L", TouchKeyboardCellKind::Character, 'L'},
  {"M", TouchKeyboardCellKind::Character, 'M'},
  {"N", TouchKeyboardCellKind::Character, 'N'},
  {"O", TouchKeyboardCellKind::Character, 'O'},
  {"P", TouchKeyboardCellKind::Character, 'P'},

  {"Q", TouchKeyboardCellKind::Character, 'Q'},
  {"R", TouchKeyboardCellKind::Character, 'R'},
  {"S", TouchKeyboardCellKind::Character, 'S'},
  {"T", TouchKeyboardCellKind::Character, 'T'},
  {"U", TouchKeyboardCellKind::Character, 'U'},
  {"V", TouchKeyboardCellKind::Character, 'V'},
  {"W", TouchKeyboardCellKind::Character, 'W'},
  {"X", TouchKeyboardCellKind::Character, 'X'},

  {"Y", TouchKeyboardCellKind::Character, 'Y'},
  {"Z", TouchKeyboardCellKind::Character, 'Z'},
  {"SPACE", TouchKeyboardCellKind::Space, 0},
  {"DEL", TouchKeyboardCellKind::Delete, 0},
  {"abc", TouchKeyboardCellKind::Lower, 0},
  {"123", TouchKeyboardCellKind::Symbols, 0},
  {"SHOW", TouchKeyboardCellKind::ToggleMaskOrClear, 0},
  {"ENTER", TouchKeyboardCellKind::Enter, 0}
};

static constexpr TouchKeyboardCell KEYBOARD_LOWER_CELLS[
  TOUCH_KEYBOARD_CELL_COUNT
] = {
  {"a", TouchKeyboardCellKind::Character, 'a'},
  {"b", TouchKeyboardCellKind::Character, 'b'},
  {"c", TouchKeyboardCellKind::Character, 'c'},
  {"d", TouchKeyboardCellKind::Character, 'd'},
  {"e", TouchKeyboardCellKind::Character, 'e'},
  {"f", TouchKeyboardCellKind::Character, 'f'},
  {"g", TouchKeyboardCellKind::Character, 'g'},
  {"h", TouchKeyboardCellKind::Character, 'h'},

  {"i", TouchKeyboardCellKind::Character, 'i'},
  {"j", TouchKeyboardCellKind::Character, 'j'},
  {"k", TouchKeyboardCellKind::Character, 'k'},
  {"l", TouchKeyboardCellKind::Character, 'l'},
  {"m", TouchKeyboardCellKind::Character, 'm'},
  {"n", TouchKeyboardCellKind::Character, 'n'},
  {"o", TouchKeyboardCellKind::Character, 'o'},
  {"p", TouchKeyboardCellKind::Character, 'p'},

  {"q", TouchKeyboardCellKind::Character, 'q'},
  {"r", TouchKeyboardCellKind::Character, 'r'},
  {"s", TouchKeyboardCellKind::Character, 's'},
  {"t", TouchKeyboardCellKind::Character, 't'},
  {"u", TouchKeyboardCellKind::Character, 'u'},
  {"v", TouchKeyboardCellKind::Character, 'v'},
  {"w", TouchKeyboardCellKind::Character, 'w'},
  {"x", TouchKeyboardCellKind::Character, 'x'},

  {"y", TouchKeyboardCellKind::Character, 'y'},
  {"z", TouchKeyboardCellKind::Character, 'z'},
  {"SPACE", TouchKeyboardCellKind::Space, 0},
  {"DEL", TouchKeyboardCellKind::Delete, 0},
  {"ABC", TouchKeyboardCellKind::Upper, 0},
  {"123", TouchKeyboardCellKind::Symbols, 0},
  {"SHOW", TouchKeyboardCellKind::ToggleMaskOrClear, 0},
  {"ENTER", TouchKeyboardCellKind::Enter, 0}
};

static constexpr TouchKeyboardCell KEYBOARD_SYMBOL_CELLS[
  TOUCH_KEYBOARD_CELL_COUNT
] = {
  {"1", TouchKeyboardCellKind::Character, '1'},
  {"2", TouchKeyboardCellKind::Character, '2'},
  {"3", TouchKeyboardCellKind::Character, '3'},
  {"4", TouchKeyboardCellKind::Character, '4'},
  {"5", TouchKeyboardCellKind::Character, '5'},
  {"6", TouchKeyboardCellKind::Character, '6'},
  {"7", TouchKeyboardCellKind::Character, '7'},
  {"8", TouchKeyboardCellKind::Character, '8'},

  {"9", TouchKeyboardCellKind::Character, '9'},
  {"0", TouchKeyboardCellKind::Character, '0'},
  {"!", TouchKeyboardCellKind::Character, '!'},
  {"@", TouchKeyboardCellKind::Character, '@'},
  {"#", TouchKeyboardCellKind::Character, '#'},
  {"$", TouchKeyboardCellKind::Character, '$'},
  {"%", TouchKeyboardCellKind::Character, '%'},
  {"^", TouchKeyboardCellKind::Character, '^'},

  {"&", TouchKeyboardCellKind::Character, '&'},
  {"*", TouchKeyboardCellKind::Character, '*'},
  {"(", TouchKeyboardCellKind::Character, '('},
  {")", TouchKeyboardCellKind::Character, ')'},
  {"-", TouchKeyboardCellKind::Character, '-'},
  {"_", TouchKeyboardCellKind::Character, '_'},
  {"+", TouchKeyboardCellKind::Character, '+'},
  {"=", TouchKeyboardCellKind::Character, '='},

  {".", TouchKeyboardCellKind::Character, '.'},
  {"SPACE", TouchKeyboardCellKind::Space, 0},
  {"DEL", TouchKeyboardCellKind::Delete, 0},
  {"ABC", TouchKeyboardCellKind::Upper, 0},
  {"abc", TouchKeyboardCellKind::Lower, 0},
  {"MORE", TouchKeyboardCellKind::MoreSymbols, 0},
  {"SHOW", TouchKeyboardCellKind::ToggleMaskOrClear, 0},
  {"ENTER", TouchKeyboardCellKind::Enter, 0}
};

static constexpr TouchKeyboardCell KEYBOARD_SYMBOL2_CELLS[
  TOUCH_KEYBOARD_CELL_COUNT
] = {
  {"\"", TouchKeyboardCellKind::Character, '"'},
  {"'", TouchKeyboardCellKind::Character, '\''},
  {",", TouchKeyboardCellKind::Character, ','},
  {"/", TouchKeyboardCellKind::Character, '/'},
  {":", TouchKeyboardCellKind::Character, ':'},
  {";", TouchKeyboardCellKind::Character, ';'},
  {"<", TouchKeyboardCellKind::Character, '<'},
  {">", TouchKeyboardCellKind::Character, '>'},

  {"?", TouchKeyboardCellKind::Character, '?'},
  {"[", TouchKeyboardCellKind::Character, '['},
  {"\\", TouchKeyboardCellKind::Character, '\\'},
  {"]", TouchKeyboardCellKind::Character, ']'},
  {"{", TouchKeyboardCellKind::Character, '{'},
  {"|", TouchKeyboardCellKind::Character, '|'},
  {"}", TouchKeyboardCellKind::Character, '}'},
  {"~", TouchKeyboardCellKind::Character, '~'},

  {"`", TouchKeyboardCellKind::Character, '`'},
  {".", TouchKeyboardCellKind::Character, '.'},
  {"-", TouchKeyboardCellKind::Character, '-'},
  {"_", TouchKeyboardCellKind::Character, '_'},
  {"@", TouchKeyboardCellKind::Character, '@'},
  {"#", TouchKeyboardCellKind::Character, '#'},
  {"$", TouchKeyboardCellKind::Character, '$'},
  {"&", TouchKeyboardCellKind::Character, '&'},

  {"123", TouchKeyboardCellKind::Symbols, 0},
  {"SPACE", TouchKeyboardCellKind::Space, 0},
  {"DEL", TouchKeyboardCellKind::Delete, 0},
  {"ABC", TouchKeyboardCellKind::Upper, 0},
  {"abc", TouchKeyboardCellKind::Lower, 0},
  {"!", TouchKeyboardCellKind::Character, '!'},
  {"SHOW", TouchKeyboardCellKind::ToggleMaskOrClear, 0},
  {"ENTER", TouchKeyboardCellKind::Enter, 0}
};

// ------------------------------------------------------------
// Placeholder and diagnostics buttons
// ------------------------------------------------------------

static constexpr TouchUiButton BUTTON_RECALIBRATE_TOUCH = {
  TouchUiButtonId::RecalibrateTouch,
  8, 151, 148, 38,
  "TOUCH CAL",
  TFT_MAROON
};

static constexpr TouchUiButton BUTTON_DISPLAY_TEST = {
  TouchUiButtonId::DisplayTest,
  164, 151, 148, 38,
  "DISPLAY TEST",
  TFT_PURPLE
};

static constexpr TouchUiButton BUTTON_PRESSURE_DOWN = {
  TouchUiButtonId::PressureDown,
  8, 195, 70, 32,
  "TOUCH\nPRESSURE -",
  TFT_DARKGREY
};

static constexpr TouchUiButton BUTTON_PRESSURE_UP = {
  TouchUiButtonId::PressureUp,
  86, 195, 70, 32,
  "TOUCH\nPRESSURE +",
  TFT_DARKGREY
};

static constexpr TouchUiButton BUTTON_BACK = {
  TouchUiButtonId::Back,
  164, 195, 70, 32,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton BUTTON_HOME = {
  TouchUiButtonId::Home,
  242, 195, 70, 32,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr TouchUiButton DISPLAY_TEST_BRIGHTNESS_DOWN = {
  TouchUiButtonId::BrightnessDown,
  8, 203, 70, 32,
  "LIGHT -",
  TFT_DARKGREY
};

static constexpr TouchUiButton DISPLAY_TEST_BRIGHTNESS_UP = {
  TouchUiButtonId::BrightnessUp,
  86, 203, 70, 32,
  "LIGHT +",
  TFT_DARKGREY
};

static constexpr TouchUiButton DISPLAY_TEST_BACK = {
  TouchUiButtonId::Back,
  164, 203, 70, 32,
  "BACK",
  TFT_NAVY
};

static constexpr TouchUiButton DISPLAY_TEST_HOME = {
  TouchUiButtonId::Home,
  242, 203, 70, 32,
  "CLOCK",
  TFT_DARKGREEN
};

static constexpr const TouchUiButton *CLOCK_BUTTONS[] = {
  &CLOCK_WEATHER_BUTTON,
  &CLOCK_MARKET_BUTTON
};

static constexpr const TouchUiButton *WEATHER_BUTTONS[] = {
  &WEATHER_RADAR_BUTTON,
  &WEATHER_REFRESH_BUTTON,
  &WEATHER_HOME_BUTTON
};

static constexpr const TouchUiButton *WEATHER_RADAR_BUTTONS[] = {
  &RADAR_FORECAST_BUTTON,
  &RADAR_REFRESH_BUTTON,
  &RADAR_LATEST_BUTTON,
  &RADAR_LOOP_BUTTON,
  &RADAR_HOME_BUTTON
};

static constexpr const TouchUiButton *MARKET_BUTTONS[] = {
  &MARKET_TODAY_BUTTON,
  &MARKET_THIRTY_DAY_BUTTON,
  &MARKET_REFRESH_BUTTON,
  &MARKET_HOME_BUTTON
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

static constexpr const TouchUiButton *NETWORK_BUTTONS[] = {
  &NETWORK_ROW_0,
  &NETWORK_ROW_1,
  &NETWORK_ROW_2,
  &NETWORK_PAGE_PREVIOUS,
  &NETWORK_PAGE_NEXT,
  &NETWORK_OTHER_SSID,
  &NETWORK_PASSWORD,
  &NETWORK_SCAN,
  &NETWORK_FORGET,
  &NETWORK_CONNECT,
  &NETWORK_BACK
};

static constexpr const TouchUiButton *KEYBOARD_BUTTONS[] = {
  &KEYBOARD_CANCEL,
  &KEYBOARD_PREVIOUS,
  &KEYBOARD_NEXT,
  &KEYBOARD_ACCEPT
};

static constexpr const TouchUiButton *DIAGNOSTIC_BUTTONS[] = {
  &BUTTON_RECALIBRATE_TOUCH,
  &BUTTON_DISPLAY_TEST,
  &BUTTON_PRESSURE_DOWN,
  &BUTTON_PRESSURE_UP,
  &BUTTON_BACK,
  &BUTTON_HOME
};

static constexpr const TouchUiButton *DISPLAY_TEST_BUTTONS[] = {
  &DISPLAY_TEST_BRIGHTNESS_DOWN,
  &DISPLAY_TEST_BRIGHTNESS_UP,
  &DISPLAY_TEST_BACK,
  &DISPLAY_TEST_HOME
};

// ------------------------------------------------------------
// UI state and settings drafts
// ------------------------------------------------------------

TouchUiPage activeTouchUiPage =
  TouchUiPage::Clock;

TouchMarketPeriod touchMarketPeriod =
  TouchMarketPeriod::Today;

bool touchUiInitialized = false;
unsigned long touchUiLastActivityAt = 0;
unsigned long touchUiLastDiagnosticsDrawAt = 0;
unsigned long touchUiLastRadarAnimationAt = 0;

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

TouchNetworkEntry touchNetworkCatalog[TOUCH_NETWORK_MAX_RESULTS];
size_t touchNetworkCatalogCount = 0;
size_t touchNetworkListOffset = 0;
int touchNetworkDraftIndex = -1;
String touchNetworkDraftSsid;
String touchNetworkDraftPassword;
String touchNetworkMessage;
bool touchNetworkMessageError = false;
bool touchNetworkForgetArmed = false;

TouchKeyboardMode touchKeyboardMode =
  TouchKeyboardMode::Upper;

TouchKeyboardTarget touchKeyboardTarget =
  TouchKeyboardTarget::None;

String touchKeyboardValue;
String touchKeyboardOriginalValue;
size_t touchKeyboardSelectedCell = 0;
int touchKeyboardPressedCell = -1;
bool touchKeyboardRevealPassword = false;

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
void drawTouchUiWeather();
void drawTouchUiWeatherRadar();
void drawTouchUiWeatherRadarFrame(bool drawControls);
void drawTouchUiMarket();
const char *touchUiPageTitle(TouchUiPage page);
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

int touchNetworkCatalogIndexForRowButton(TouchUiButtonId id);
void scanTouchNetworks();
void drawTouchUiNetwork();
void drawTouchUiNetworkRow(size_t visibleRow, bool pressed = false);
void drawTouchUiNetworkMessage();
bool touchNetworkDraftIsOpen();
void openTouchKeyboard(
  TouchKeyboardTarget target,
  const String &initialValue
);
void drawTouchUiKeyboard();
void drawTouchKeyboardCell(size_t index, bool pressed = false);
int touchKeyboardCellAt(int x, int y);
const TouchKeyboardCell *activeTouchKeyboardCells();
String touchKeyboardCellLabel(size_t index);
String touchKeyboardDisplayValue();
void moveTouchKeyboardSelection(int delta);
void activateTouchKeyboardSelection();
void cancelTouchKeyboard();
void completeTouchKeyboard();
void connectTouchNetwork();
void forgetTouchNetwork();

void drawTouchUiDiagnosticsData();
void drawTouchUiDiagnostics();
void drawTouchUiDisplayTest();
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
    case TouchUiPage::Clock:
      buttonsOut = CLOCK_BUTTONS;
      buttonCountOut =
        sizeof(CLOCK_BUTTONS) /
        sizeof(CLOCK_BUTTONS[0]);
      break;

    case TouchUiPage::Weather:
      buttonsOut = WEATHER_BUTTONS;
      buttonCountOut =
        sizeof(WEATHER_BUTTONS) /
        sizeof(WEATHER_BUTTONS[0]);
      break;

    case TouchUiPage::WeatherRadar:
      buttonsOut = WEATHER_RADAR_BUTTONS;
      buttonCountOut =
        sizeof(WEATHER_RADAR_BUTTONS) /
        sizeof(WEATHER_RADAR_BUTTONS[0]);
      break;

    case TouchUiPage::Market:
      buttonsOut = MARKET_BUTTONS;
      buttonCountOut =
        sizeof(MARKET_BUTTONS) /
        sizeof(MARKET_BUTTONS[0]);
      break;

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

    case TouchUiPage::DisplayTest:
      buttonsOut = DISPLAY_TEST_BUTTONS;
      buttonCountOut =
        sizeof(DISPLAY_TEST_BUTTONS) /
        sizeof(DISPLAY_TEST_BUTTONS[0]);
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
      buttonsOut = NETWORK_BUTTONS;
      buttonCountOut =
        sizeof(NETWORK_BUTTONS) /
        sizeof(NETWORK_BUTTONS[0]);
      break;

    case TouchUiPage::Keyboard:
      buttonsOut = KEYBOARD_BUTTONS;
      buttonCountOut =
        sizeof(KEYBOARD_BUTTONS) /
        sizeof(KEYBOARD_BUTTONS[0]);
      break;

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
      weatherFeatureAvailable() &&
      pointInsideTouchUiButton(
        x,
        y,
        CLOCK_WEATHER_BUTTON
      )
    ) {
      return TouchUiButtonId::OpenWeather;
    }

    if (
      marketFeatureAvailable() &&
      pointInsideTouchUiButton(
        x,
        y,
        CLOCK_MARKET_BUTTON
      )
    ) {
      return TouchUiButtonId::OpenMarket;
    }

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

  if (
    activeTouchUiPage ==
      TouchUiPage::Keyboard &&
    touchKeyboardCellAt(x, y) >= 0
  ) {
    return TouchUiButtonId::KeyboardCell;
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
      (
        buttons[i]->id == TouchUiButtonId::NetworkRow0 ||
        buttons[i]->id == TouchUiButtonId::NetworkRow1 ||
        buttons[i]->id == TouchUiButtonId::NetworkRow2
      ) &&
      touchNetworkCatalogIndexForRowButton(buttons[i]->id) < 0
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


int touchNetworkCatalogIndexForRowButton(
  TouchUiButtonId id
) {
  size_t visibleRow =
    TOUCH_NETWORK_ROWS_PER_PAGE;

  switch (id) {
    case TouchUiButtonId::NetworkRow0:
      visibleRow = 0;
      break;

    case TouchUiButtonId::NetworkRow1:
      visibleRow = 1;
      break;

    case TouchUiButtonId::NetworkRow2:
      visibleRow = 2;
      break;

    default:
      return -1;
  }

  const size_t catalogIndex =
    touchNetworkListOffset + visibleRow;

  return catalogIndex < touchNetworkCatalogCount
    ? static_cast<int>(catalogIndex)
    : -1;
}


bool touchNetworkDraftIsOpen() {
  return
    touchNetworkDraftIndex >= 0 &&
    static_cast<size_t>(touchNetworkDraftIndex) <
      touchNetworkCatalogCount &&
    touchNetworkCatalog[
      touchNetworkDraftIndex
    ].open;
}


const TouchKeyboardCell *activeTouchKeyboardCells() {
  switch (touchKeyboardMode) {
    case TouchKeyboardMode::Lower:
      return KEYBOARD_LOWER_CELLS;

    case TouchKeyboardMode::Symbols:
      return KEYBOARD_SYMBOL_CELLS;

    case TouchKeyboardMode::Symbols2:
      return KEYBOARD_SYMBOL2_CELLS;

    case TouchKeyboardMode::Upper:
    default:
      return KEYBOARD_UPPER_CELLS;
  }
}


int touchKeyboardCellAt(
  int x,
  int y
) {
  static constexpr int GRID_TOP = 68;
  static constexpr int CELL_W =
    SCREEN_W / TOUCH_KEYBOARD_COLUMNS;
  static constexpr int CELL_H = 30;

  if (
    x < 0 ||
    x >= SCREEN_W ||
    y < GRID_TOP ||
    y >= GRID_TOP +
      static_cast<int>(
        TOUCH_KEYBOARD_ROWS
      ) * CELL_H
  ) {
    return -1;
  }

  const int column =
    x / CELL_W;

  const int row =
    (y - GRID_TOP) / CELL_H;

  const int index =
    row *
      static_cast<int>(
        TOUCH_KEYBOARD_COLUMNS
      ) +
    column;

  return
    index >= 0 &&
    index <
      static_cast<int>(
        TOUCH_KEYBOARD_CELL_COUNT
      )
      ? index
      : -1;
}


String touchKeyboardCellLabel(
  size_t index
) {
  if (index >= TOUCH_KEYBOARD_CELL_COUNT) {
    return "";
  }

  const TouchKeyboardCell &cell =
    activeTouchKeyboardCells()[index];

  if (
    cell.kind ==
      TouchKeyboardCellKind::ToggleMaskOrClear
  ) {
    if (
      touchKeyboardTarget ==
        TouchKeyboardTarget::Password
    ) {
      return touchKeyboardRevealPassword
        ? "HIDE"
        : "SHOW";
    }

    return "CLEAR";
  }

  return String(cell.label);
}


String touchKeyboardDisplayValue() {
  String displayed;

  if (
    touchKeyboardTarget ==
      TouchKeyboardTarget::Password &&
    !touchKeyboardRevealPassword
  ) {
    displayed.reserve(
      touchKeyboardValue.length()
    );

    for (
      size_t i = 0;
      i < touchKeyboardValue.length();
      ++i
    ) {
      displayed += '*';
    }
  } else {
    displayed = touchKeyboardValue;
  }

  static constexpr size_t MAX_VISIBLE = 38;

  if (displayed.length() > MAX_VISIBLE) {
    displayed =
      String("...") +
      displayed.substring(
        displayed.length() -
          (MAX_VISIBLE - 3)
      );
  }

  return displayed;
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

    case TouchUiButtonId::NetworkPassword:
      return touchNetworkDraftPassword.length() > 0
        ? "PASSWORD SET"
        : "PASSWORD";

    case TouchUiButtonId::NetworkForget:
      return touchNetworkForgetArmed
        ? "CONFIRM"
        : "FORGET";

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

  if (
    button.id == TouchUiButtonId::NetworkRow0 ||
    button.id == TouchUiButtonId::NetworkRow1 ||
    button.id == TouchUiButtonId::NetworkRow2
  ) {
    size_t visibleRow = 0;

    if (button.id == TouchUiButtonId::NetworkRow1) {
      visibleRow = 1;
    } else if (button.id == TouchUiButtonId::NetworkRow2) {
      visibleRow = 2;
    }

    drawTouchUiNetworkRow(
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

  if (
    button.id ==
      TouchUiButtonId::KeyboardAccept
  ) {
    const int centerX =
      button.x + button.w / 2;

    const int centerY =
      button.y + button.h / 2;

    lcd.drawLine(
      centerX - 13,
      centerY,
      centerX - 4,
      centerY + 9,
      textColor
    );

    lcd.drawLine(
      centerX - 4,
      centerY + 9,
      centerX + 15,
      centerY - 10,
      textColor
    );

    lcd.drawLine(
      centerX - 12,
      centerY - 1,
      centerX - 3,
      centerY + 8,
      textColor
    );
  } else {
    const String label =
      touchUiButtonDisplayLabel(button);

    const int lineBreak =
      label.indexOf('\n');

    if (lineBreak >= 0) {
      const String firstLine =
        label.substring(0, lineBreak);

      const String secondLine =
        label.substring(lineBreak + 1);

      const int centerX =
        button.x + button.w / 2;

      const int centerY =
        button.y + button.h / 2;

      lcd.drawString(
        firstLine,
        centerX,
        centerY - 5
      );

      lcd.drawString(
        secondLine,
        centerX,
        centerY + 5
      );
    } else {
      lcd.drawString(
        label,
        button.x + button.w / 2,
        button.y + button.h / 2
      );
    }
  }
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

String touchUiWeatherHeader(const char *prefix) {
  String title = prefix;
  title += ": ";
  title += weatherLocationName();

  if (title.length() > 30) {
    title.remove(27);
    title += "...";
  }

  title.toUpperCase();
  return title;
}

void drawTouchUiWeather() {
  const String title = touchUiWeatherHeader("WEATHER");
  drawTouchUiHeader(title.c_str());

  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);

  if (!weatherFeatureAvailable()) {
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.drawString("Set a home location and enable weather", SCREEN_W / 2, 102);
    lcd.drawString("from the browser Settings page.", SCREEN_W / 2, 119);
  } else if (!weatherForecastAvailableForSavedLocation()) {
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString("No cached forecast for this location", SCREEN_W / 2, 82);

    lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    lcd.drawString(
      weatherForecastRefreshIsPending()
        ? "Forecast refresh requested"
        : "Tap REFRESH to retrieve weather",
      SCREEN_W / 2,
      104
    );

    if (weatherForecastLastError().length()) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
      lcd.drawString(weatherForecastLastError(), SCREEN_W / 2, 128);
    }
  } else {
    const uint16_t currentBackground = 0x18E3;

    lcd.fillRoundRect(8, 29, 304, 70, 7, currentBackground);
    drawWeatherIcon(
      44,
      60,
      weatherForecast.weatherCode,
      weatherForecast.isDay,
      currentBackground
    );

    lcd.setFont(&fonts::Font4);
    lcd.setTextDatum(textdatum_t::middle_left);
    lcd.setTextColor(TFT_WHITE, currentBackground);
    lcd.drawString(
      weatherTemperatureText(weatherForecast.temperature, false),
      72,
      52
    );

    lcd.setFont(&fonts::Font0);
    lcd.drawString(
      weatherSettings.useFahrenheit ? "F" : "C",
      125,
      44
    );

    lcd.setTextDatum(textdatum_t::middle_left);
    lcd.setTextColor(TFT_CYAN, currentBackground);
    lcd.drawString(
      weatherConditionText(weatherForecast.weatherCode),
      72,
      72
    );

    String details =
      String("Feels ") +
      weatherTemperatureText(weatherForecast.apparentTemperature, true) +
      "  Hum " + String(weatherForecast.relativeHumidity) + "%";

    lcd.setTextColor(TFT_WHITE, currentBackground);
    lcd.drawString(details, 158, 48);

    details =
      String("Wind ") +
      weatherWindDirectionText(weatherForecast.windDirection) +
      " " + String(weatherForecast.windSpeed, 0) +
      (weatherSettings.useFahrenheit ? " mph" : " km/h");

    if (weatherForecast.windGust > weatherForecast.windSpeed + 1.0f) {
      details += " G";
      details += String(weatherForecast.windGust, 0);
    }

    lcd.drawString(details, 158, 67);
    lcd.drawString(
      String("Updated ") + weatherForecastAgeText(),
      158,
      86
    );

    for (size_t day = 0; day < WEATHER_FORECAST_DAYS; ++day) {
      const int x = 8 + static_cast<int>(day) * 102;
      const uint16_t cardColor = 0x2104;

      lcd.fillRoundRect(x, 105, 96, 88, 6, cardColor);
      lcd.setTextDatum(textdatum_t::middle_center);
      lcd.setTextColor(TFT_YELLOW, cardColor);
      lcd.drawString(weatherDayName(day), x + 48, 117);

      drawWeatherIcon(
        x + 48,
        140,
        weatherForecast.days[day].weatherCode,
        true,
        cardColor
      );

      lcd.setTextColor(TFT_WHITE, cardColor);
      lcd.drawString(
        weatherTemperatureText(weatherForecast.days[day].highTemperature, false) +
        "/" +
        weatherTemperatureText(weatherForecast.days[day].lowTemperature, false),
        x + 48,
        166
      );

      lcd.setTextColor(TFT_CYAN, cardColor);
      lcd.drawString(
        String(weatherForecast.days[day].precipitationProbability) + "% rain",
        x + 48,
        184
      );
    }
  }

  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  lcd.drawString(
    weatherLocationNameIsResolved()
      ? "Open-Meteo / OpenStreetMap"
      : "Forecast: Open-Meteo",
    SCREEN_W / 2,
    200
  );

  for (const TouchUiButton *button : WEATHER_BUTTONS) {
    drawTouchUiButton(*button);
  }
}

String touchUiRadarFrameTimeLabel(time_t timestamp) {
  if (timestamp <= 0) {
    return "--";
  }

  tm localTm {};
  localtime_r(&timestamp, &localTm);
  char label[20];

  if (timeSettings.use24Hour) {
    strftime(label, sizeof(label), "%H:%M", &localTm);
  } else {
    strftime(label, sizeof(label), "%I:%M %p", &localTm);

    if (label[0] == '0') {
      memmove(label, label + 1, strlen(label));
    }
  }

  return String(label);
}

String touchUiRadarMarginTimeLabel(time_t timestamp) {
  if (timestamp <= 0) {
    return "--:--";
  }

  tm localTm {};
  localtime_r(&timestamp, &localTm);
  char label[8];

  if (timeSettings.use24Hour) {
    strftime(label, sizeof(label), "%H:%M", &localTm);
  } else {
    strftime(label, sizeof(label), "%I:%M", &localTm);

    if (label[0] == '0') {
      memmove(label, label + 1, strlen(label));
    }
  }

  return String(label);
}

void drawTouchUiRadarFrameTimeline() {
  // The radar image begins at x=32, leaving a narrow dedicated margin. Clear
  // it on every redraw so switching back to LATEST removes the LOOP timeline.
  const int marginWidth = WEATHER_RADAR_IMAGE_X - 1;
  lcd.fillRect(
    0,
    WEATHER_RADAR_IMAGE_Y,
    marginWidth,
    WEATHER_RADAR_IMAGE_H,
    TFT_BLACK
  );

  if (!weatherRadarLoopModeIsSelected()) {
    return;
  }

  const size_t frameCount = weatherRadarFrameCount();

  if (frameCount == 0) {
    return;
  }

  const size_t selectedIndex = weatherRadarSelectedFrameIndex();
  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_right);

  for (size_t index = 0; index < frameCount; ++index) {
    const int y = WEATHER_RADAR_IMAGE_Y +
      static_cast<int>(
        (
          index * static_cast<size_t>(WEATHER_RADAR_IMAGE_H) +
          static_cast<size_t>(WEATHER_RADAR_IMAGE_H / 2)
        ) / frameCount
      );

    lcd.setTextColor(
      index == selectedIndex ? TFT_YELLOW : TFT_DARKGREY,
      TFT_BLACK
    );
    lcd.drawString(
      touchUiRadarMarginTimeLabel(weatherRadarFrameTime(index)),
      marginWidth - 1,
      y
    );
  }
}

void drawTouchUiWeatherRadarFrame(bool drawControls) {
  const bool radarDrawn = drawWeatherRadarImage();
  drawTouchUiRadarFrameTimeline();

  lcd.fillRect(0, 191, SCREEN_W, 16, TFT_BLACK);
  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  String status;

  if (radarDrawn) {
    if (weatherRadarLatestModeIsSelected()) {
      status = String("LATEST  ") +
        touchUiRadarFrameTimeLabel(weatherRadarSelectedFrameTime());

      if (weatherRadarRefreshInProgress()) {
        status += String("  UPDATING ") +
          String(weatherRadarRefreshCompletedCount()) + "/" +
          String(weatherRadarRefreshTargetCount());
      }
    } else {
      status = String("LOOP  ") +
        String(weatherRadarSelectedFrameIndex() + 1) + "/" +
        String(weatherRadarFrameCount()) + "  " +
        touchUiRadarFrameTimeLabel(weatherRadarSelectedFrameTime());

      if (
        weatherRadarFrameCount() > 1 &&
        !weatherRadarSmoothPlaybackAvailable()
      ) {
        status += "  PAUSED STATIC";
      }
    }
  } else if (weatherRadarRefreshInProgress()) {
    status = String("Loading radar ") +
      String(weatherRadarRefreshCompletedCount()) + "/" +
      String(weatherRadarRefreshTargetCount());
  } else if (weatherRadarRefreshIsPending()) {
    status = "Radar refresh requested";
  } else {
    status = "No cached radar image";
  }

  if (
    !radarDrawn &&
    !weatherRadarRefreshInProgress() &&
    !weatherRadarRefreshIsPending() &&
    weatherRadarLastError().length()
  ) {
    status = weatherRadarLastError();
  }

  lcd.drawString(status, SCREEN_W / 2, 194);

  lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  lcd.drawString("(c) OpenStreetMap contributors | RainViewer", SCREEN_W / 2, 202);

  if (drawControls) {
    for (const TouchUiButton *button : WEATHER_RADAR_BUTTONS) {
      const bool selectedMode =
        (
          button->id == TouchUiButtonId::WeatherRadarLatest &&
          weatherRadarLatestModeIsSelected()
        ) ||
        (
          button->id == TouchUiButtonId::WeatherRadarLoop &&
          weatherRadarLoopModeIsSelected()
        );

      drawTouchUiButton(*button, selectedMode);
    }
  }
}

void drawTouchUiWeatherRadar() {
  const String title = touchUiWeatherHeader("RADAR");
  drawTouchUiHeader(title.c_str());
  drawTouchUiWeatherRadarFrame(true);
  touchUiLastRadarAnimationAt = millis();
}


String touchMarketHorizontalLabel(time_t timestamp, bool intraday) {
  if (timestamp <= 0) {
    return "--";
  }

  tm localTm {};
  localtime_r(&timestamp, &localTm);
  char label[32];

  if (intraday) {
    if (timeSettings.use24Hour) {
      snprintf(
        label,
        sizeof(label),
        "%02d:%02d",
        localTm.tm_hour,
        localTm.tm_min
      );
    } else {
      int hour = localTm.tm_hour % 12;
      if (hour == 0) {
        hour = 12;
      }

      snprintf(
        label,
        sizeof(label),
        "%d:%02d",
        hour,
        localTm.tm_min
      );
    }
  } else {
    snprintf(
      label,
      sizeof(label),
      "%d/%d",
      localTm.tm_mon + 1,
      localTm.tm_mday
    );
  }

  return String(label);
}

String touchMarketScaleLabel(int16_t basisPoints) {
  const float percent = basisPoints / 100.0f;
  String label;

  if (percent > 0.0001f) {
    label += '+';
  }

  label += String(percent, 1);
  label += '%';
  return label;
}

void drawTouchUiMarketGraph() {
  static constexpr int GRAPH_X = 8;
  static constexpr int GRAPH_Y = 32;
  static constexpr int GRAPH_W = 304;
  static constexpr int GRAPH_H = 89;

  lcd.fillRoundRect(
    GRAPH_X,
    GRAPH_Y,
    GRAPH_W,
    GRAPH_H,
    5,
    0x1082
  );

  lcd.drawRoundRect(
    GRAPH_X,
    GRAPH_Y,
    GRAPH_W,
    GRAPH_H,
    5,
    TFT_DARKGREY
  );

  const bool intraday =
    touchMarketPeriod == TouchMarketPeriod::Today;

  const MarketGraphPoint *points = intraday
    ? marketSnapshot.today
    : marketSnapshot.thirtyDay;

  const size_t pointCount = intraday
    ? marketSnapshot.todayCount
    : marketSnapshot.thirtyDayCount;

  const int16_t currentValue = intraday
    ? marketSnapshot.currentMoodBasisPoints
    : marketSnapshot.thirtyDayMoodBasisPoints;

  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setTextColor(TFT_LIGHTGREY, 0x1082);
  lcd.drawString(
    intraday
      ? "INTRADAY COMPOSITE"
      : "30-SESSION COMPOSITE",
    GRAPH_X + 7,
    GRAPH_Y + 4
  );

  lcd.setTextDatum(textdatum_t::top_right);
  lcd.setTextColor(
    marketMoodColor(marketMoodLevelForBasisPoints(currentValue)),
    0x1082
  );
  lcd.drawString(
    marketPercentText(currentValue),
    GRAPH_X + GRAPH_W - 7,
    GRAPH_Y + 4
  );

  if (pointCount < 2) {
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(TFT_CYAN, 0x1082);
    lcd.drawString(
      marketRefreshIsPending()
        ? "RETRIEVING MARKET DATA"
        : "NO CACHED GRAPH YET",
      GRAPH_X + GRAPH_W / 2,
      GRAPH_Y + GRAPH_H / 2
    );
    return;
  }

  int16_t minimum = 0;
  int16_t maximum = 0;

  for (size_t index = 0; index < pointCount; ++index) {
    minimum = min(minimum, points[index].moodBasisPoints);
    maximum = max(maximum, points[index].moodBasisPoints);
  }

  if (maximum - minimum < 40) {
    const int16_t middle = static_cast<int16_t>((maximum + minimum) / 2);
    minimum = middle - 20;
    maximum = middle + 20;
  }

  // Reserve the left edge for compact percentage labels and the bottom edge
  // for three timeline labels. Ticks are used instead of a full grid so the
  // small 320x240 display remains uncluttered.
  const int plotLeft = GRAPH_X + 39;
  const int plotRight = GRAPH_X + GRAPH_W - 7;
  const int plotTop = GRAPH_Y + 18;
  const int plotBottom = GRAPH_Y + GRAPH_H - 19;
  const int plotHeight = plotBottom - plotTop;
  const int valueRange = maximum - minimum;
  const uint16_t axisColor = 0x5AEB;

  lcd.drawFastVLine(
    plotLeft,
    plotTop,
    plotBottom - plotTop + 1,
    axisColor
  );
  lcd.drawFastHLine(
    plotLeft,
    plotBottom,
    plotRight - plotLeft + 1,
    axisColor
  );

  auto valueToY = [&](int16_t value) {
    return valueRange <= 0
      ? (plotTop + plotBottom) / 2
      : plotBottom - static_cast<int>(
          (value - minimum) * plotHeight / valueRange
        );
  };

  auto drawValueTick = [&](int y, int16_t value) {
    lcd.drawFastHLine(plotLeft - 3, y, 7, axisColor);
    lcd.setTextDatum(textdatum_t::middle_right);
    lcd.setTextColor(TFT_LIGHTGREY, 0x1082);
    lcd.drawString(
      touchMarketScaleLabel(value),
      plotLeft - 5,
      y
    );
  };

  drawValueTick(plotTop, maximum);
  drawValueTick(plotBottom, minimum);

  if (minimum < 0 && maximum > 0 && valueRange > 0) {
    const int zeroY = valueToY(0);

    if (zeroY - plotTop >= 8 && plotBottom - zeroY >= 8) {
      drawValueTick(zeroY, 0);
    }
  }

  const size_t middleIndex = pointCount / 2;
  const int tickXs[3] = {
    plotLeft,
    (plotLeft + plotRight) / 2,
    plotRight
  };
  const size_t labelIndexes[3] = {
    0,
    middleIndex,
    pointCount - 1
  };

  for (size_t tick = 0; tick < 3; ++tick) {
    lcd.drawFastVLine(tickXs[tick], plotBottom - 2, 6, axisColor);
    lcd.setTextColor(TFT_LIGHTGREY, 0x1082);

    if (tick == 0) {
      lcd.setTextDatum(textdatum_t::top_left);
    } else if (tick == 2) {
      lcd.setTextDatum(textdatum_t::top_right);
    } else {
      lcd.setTextDatum(textdatum_t::top_center);
    }

    lcd.drawString(
      touchMarketHorizontalLabel(
        points[labelIndexes[tick]].timestamp,
        intraday
      ),
      tickXs[tick],
      plotBottom + 5
    );
  }

  int previousX = plotLeft;
  int previousY = plotBottom;
  const uint16_t lineColor =
    marketMoodColor(marketMoodLevelForBasisPoints(currentValue));

  for (size_t index = 0; index < pointCount; ++index) {
    const int x = pointCount <= 1
      ? plotLeft
      : plotLeft + static_cast<int>(
          index * (plotRight - plotLeft) / (pointCount - 1)
        );

    const int y = valueToY(points[index].moodBasisPoints);

    if (index > 0) {
      lcd.drawLine(previousX, previousY, x, y, lineColor);
    }

    previousX = x;
    previousY = y;
  }

  lcd.fillCircle(previousX, previousY, 2, TFT_WHITE);
}

void drawTouchUiMarket() {
  drawTouchUiHeader("MARKET MOOD");
  drawTouchUiMarketGraph();

  drawTouchUiButton(
    MARKET_TODAY_BUTTON,
    touchMarketPeriod == TouchMarketPeriod::Today
  );

  drawTouchUiButton(
    MARKET_THIRTY_DAY_BUTTON,
    touchMarketPeriod == TouchMarketPeriod::ThirtyDays
  );

  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);

  if (marketDataAvailable()) {
    for (size_t index = 0; index < 3; ++index) {
      const int x = 54 + static_cast<int>(index) * 106;
      const MarketQuote &quote = marketSnapshot.quotes[index];
      String text = quote.symbol;
      text += ' ';

      if (quote.changePercent > 0.0001f) {
        text += '+';
      }

      text += String(quote.changePercent, 2);
      text += '%';

      lcd.setTextColor(
        quote.changePercent >= 0.0f ? TFT_GREEN : TFT_RED,
        TFT_BLACK
      );
      lcd.drawString(text, x, 166);
    }

    const MarketMoodLevel mood = currentMarketMoodLevel();
    drawMarketMoodFace(28, 186, 10, mood, TFT_BLACK);

    lcd.setTextDatum(textdatum_t::middle_left);
    lcd.setTextColor(marketMoodColor(mood), TFT_BLACK);
    lcd.drawString(
      String(marketMoodLabel(mood)) +
        "  " +
        (marketSnapshot.marketOpen ? "MARKET OPEN" : "MARKET CLOSED"),
      44,
      183
    );

    lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    lcd.drawString(
      String("Updated ") + marketAgeText() + "  |  delayed / best effort",
      44,
      197
    );
  } else {
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    lcd.drawString(
      marketRefreshIsPending()
        ? "Market refresh requested"
        : "Tap REFRESH to retrieve Market Mood",
      SCREEN_W / 2,
      176
    );

    if (marketLastError().length()) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
      String error = marketLastError();

      if (error.length() > 46) {
        error.remove(43);
        error += "...";
      }

      lcd.drawString(error, SCREEN_W / 2, 194);
    }
  }

  drawTouchUiButton(MARKET_REFRESH_BUTTON);
  drawTouchUiButton(MARKET_HOME_BUTTON);
}

const char *touchUiPageTitle(
  TouchUiPage page
) {
  switch (page) {
    case TouchUiPage::Weather:
      return "WEATHER";

    case TouchUiPage::WeatherRadar:
      return "REGIONAL RADAR";

    case TouchUiPage::Market:
      return "MARKET MOOD";

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

    case TouchUiPage::Keyboard:
      return touchKeyboardTarget ==
        TouchKeyboardTarget::Password
          ? "WI-FI PASSWORD"
          : "WI-FI SSID";

    case TouchUiPage::Diagnostics:
      return "DIAGNOSTICS";

    case TouchUiPage::DisplayTest:
      return "DISPLAY TEST";

    case TouchUiPage::MainMenu:
      return "TOUCH MENU";

    case TouchUiPage::Clock:
    default:
      return "WORLD CLOCK";
  }
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
    rawMapIsValid(activeNightRawPath.c_str())
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
// Network page and Zoom R4-style keyboard
// ------------------------------------------------------------

void scanTouchNetworks() {
  const String preferredSsid =
    touchNetworkDraftSsid;

  drawTouchUiBusy(
    "WI-FI SCAN",
    "Searching for nearby networks"
  );

const wifi_mode_t currentMode =
  WiFi.getMode();

WiFi.mode(
  currentMode == WIFI_AP ||
  currentMode == WIFI_AP_STA
    ? WIFI_AP_STA
    : WIFI_STA
);

  const int found =
    WiFi.scanNetworks(
      false,
      true
    );

  touchNetworkCatalogCount = 0;
  touchNetworkDraftIndex = -1;
  touchNetworkListOffset = 0;
  touchNetworkForgetArmed = false;

  if (found < 0) {
    touchNetworkMessage =
      "Wi-Fi scan failed";
    touchNetworkMessageError = true;
    WiFi.scanDelete();
    drawTouchUiNetwork();
    return;
  }

  for (
    int i = 0;
    i < found &&
      touchNetworkCatalogCount <
        TOUCH_NETWORK_MAX_RESULTS;
    ++i
  ) {
    const String ssid =
      WiFi.SSID(i);

    if (ssid.length() == 0) {
      continue;
    }

    bool duplicate = false;

    for (
      size_t j = 0;
      j < touchNetworkCatalogCount;
      ++j
    ) {
      if (
        touchNetworkCatalog[j].ssid ==
          ssid
      ) {
        duplicate = true;
        break;
      }
    }

    if (duplicate) {
      continue;
    }

    TouchNetworkEntry &entry =
      touchNetworkCatalog[
        touchNetworkCatalogCount++
      ];

    entry.ssid = ssid;
    entry.rssi = WiFi.RSSI(i);
    entry.open =
      WiFi.encryptionType(i) ==
        WIFI_AUTH_OPEN;
  }

  WiFi.scanDelete();

  String selectedSsid =
    preferredSsid;

  if (
    selectedSsid.length() == 0 &&
    networkSettings.configured
  ) {
    selectedSsid =
      networkSettings.ssid;
  }

  for (
    size_t i = 0;
    i < touchNetworkCatalogCount;
    ++i
  ) {
    if (
      touchNetworkCatalog[i].ssid ==
        selectedSsid
    ) {
      touchNetworkDraftIndex =
        static_cast<int>(i);

      touchNetworkDraftSsid =
        touchNetworkCatalog[i].ssid;

      touchNetworkListOffset =
        (i / TOUCH_NETWORK_ROWS_PER_PAGE) *
        TOUCH_NETWORK_ROWS_PER_PAGE;
      break;
    }
  }

  if (touchNetworkCatalogCount == 0) {
    touchNetworkMessage =
      "No visible Wi-Fi networks found";
    touchNetworkMessageError = true;
  } else {
    touchNetworkMessage =
      String(touchNetworkCatalogCount) +
      " network" +
      (touchNetworkCatalogCount == 1
        ? " found"
        : "s found");
    touchNetworkMessageError = false;
  }

  drawTouchUiNetwork();
}


void drawTouchUiNetworkRow(
  size_t visibleRow,
  bool pressed
) {
  const size_t catalogIndex =
    touchNetworkListOffset + visibleRow;

  static constexpr int ROW_Y[] = {
    43,
    80,
    117
  };

  const int y =
    ROW_Y[visibleRow];

  if (catalogIndex >= touchNetworkCatalogCount) {
    lcd.fillRoundRect(
      8,
      y,
      304,
      34,
      4,
      TFT_BLACK
    );

    lcd.drawRoundRect(
      8,
      y,
      304,
      34,
      4,
      TFT_DARKGREY
    );
    return;
  }

  const TouchNetworkEntry &entry =
    touchNetworkCatalog[catalogIndex];

  const bool selected =
    static_cast<int>(catalogIndex) ==
      touchNetworkDraftIndex ||
    (
      touchNetworkDraftIndex < 0 &&
      entry.ssid == touchNetworkDraftSsid
    );

  const bool connected =
    WiFi.status() == WL_CONNECTED &&
    WiFi.SSID() == entry.ssid;

  const bool saved =
    networkSettings.configured &&
    networkSettings.ssid == entry.ssid;

  const uint16_t fillColor =
    pressed
      ? TFT_YELLOW
      : selected
        ? 0x4208
        : 0x2104;

  const uint16_t textColor =
    pressed
      ? TFT_BLACK
      : TFT_WHITE;

  lcd.fillRoundRect(
    8,
    y,
    304,
    34,
    4,
    fillColor
  );

  lcd.drawRoundRect(
    8,
    y,
    304,
    34,
    4,
    selected
      ? TFT_YELLOW
      : connected
        ? TFT_CYAN
        : TFT_DARKGREY
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

  lcd.drawString(
    shortenedTouchMapFilename(
      entry.ssid,
      30
    ),
    15,
    y + 4
  );

  lcd.setTextDatum(
    textdatum_t::top_right
  );

  lcd.drawString(
    String(entry.rssi) + " dBm",
    305,
    y + 4
  );

  String status =
    entry.open
      ? "OPEN"
      : "SECURED";

  if (connected) {
    status += "  CONNECTED";
  } else if (saved) {
    status += "  SAVED";
  }

  lcd.setTextDatum(
    textdatum_t::bottom_left
  );

  lcd.setTextColor(
    pressed
      ? TFT_BLACK
      : entry.open
        ? TFT_GREEN
        : TFT_CYAN,
    fillColor
  );

  lcd.drawString(
    status,
    15,
    y + 30
  );
}


void drawTouchUiNetworkMessage() {
  String message =
    touchNetworkMessage;

  if (message.length() == 0) {
    if (touchNetworkDraftSsid.length() > 0) {
      message =
        String("Selected: ") +
        touchNetworkDraftSsid;
    } else {
      message =
        "Select a network or choose OTHER SSID";
    }
  }

  message =
    shortenedTouchMapFilename(
      message,
      48
    );

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
    touchNetworkMessageError
      ? TFT_RED
      : TFT_CYAN,
    TFT_BLACK
  );

  lcd.drawString(
    message,
    SCREEN_W / 2,
    35
  );
}


void drawTouchUiNetwork() {
  drawTouchUiHeader(
    "NETWORK"
  );

  drawTouchUiNetworkMessage();

  drawTouchUiNetworkRow(0);
  drawTouchUiNetworkRow(1);
  drawTouchUiNetworkRow(2);

  drawTouchUiButton(
    NETWORK_PAGE_PREVIOUS
  );

  drawTouchUiButton(
    NETWORK_PAGE_NEXT
  );

  drawTouchUiButton(
    NETWORK_OTHER_SSID
  );

  drawTouchUiButton(
    NETWORK_PASSWORD
  );

  drawTouchUiButton(
    NETWORK_SCAN
  );

  drawTouchUiButton(
    NETWORK_FORGET
  );

  drawTouchUiButton(
    NETWORK_CONNECT
  );

  drawTouchUiButton(
    NETWORK_BACK
  );
}


void openTouchKeyboard(
  TouchKeyboardTarget target,
  const String &initialValue
) {
  touchKeyboardTarget = target;
  touchKeyboardOriginalValue = initialValue;
  touchKeyboardValue = initialValue;
  touchKeyboardMode =
    target == TouchKeyboardTarget::Password
      ? TouchKeyboardMode::Lower
      : TouchKeyboardMode::Upper;
  touchKeyboardSelectedCell = 0;
  touchKeyboardPressedCell = -1;
  touchKeyboardRevealPassword =
    target != TouchKeyboardTarget::Password;

  activeTouchUiPage =
    TouchUiPage::Keyboard;

  touchUiLastActivityAt =
    millis();

  drawActiveTouchUiPage();
}


void drawTouchKeyboardCell(
  size_t index,
  bool pressed
) {
  if (index >= TOUCH_KEYBOARD_CELL_COUNT) {
    return;
  }

  static constexpr int GRID_TOP = 68;
  static constexpr int CELL_W =
    SCREEN_W / TOUCH_KEYBOARD_COLUMNS;
  static constexpr int CELL_H = 30;

  const int column =
    index % TOUCH_KEYBOARD_COLUMNS;

  const int row =
    index / TOUCH_KEYBOARD_COLUMNS;

  const int x =
    column * CELL_W;

  const int y =
    GRID_TOP + row * CELL_H;

  const bool selected =
    index == touchKeyboardSelectedCell;

  const uint16_t fillColor =
    pressed
      ? TFT_WHITE
      : selected
        ? TFT_YELLOW
        : 0xBDF7;

  const uint16_t textColor =
    TFT_BLACK;

  lcd.fillRoundRect(
    x + 1,
    y + 1,
    CELL_W - 2,
    CELL_H - 2,
    4,
    fillColor
  );

  lcd.drawRoundRect(
    x + 1,
    y + 1,
    CELL_W - 2,
    CELL_H - 2,
    4,
    selected
      ? TFT_WHITE
      : TFT_DARKGREY
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
    touchKeyboardCellLabel(index),
    x + CELL_W / 2,
    y + CELL_H / 2
  );
}


void drawTouchUiKeyboard() {
  drawTouchUiHeader(
    touchUiPageTitle(
      TouchUiPage::Keyboard
    )
  );

  lcd.fillRoundRect(
    8,
    34,
    304,
    28,
    5,
    TFT_WHITE
  );

  lcd.drawRoundRect(
    8,
    34,
    304,
    28,
    5,
    TFT_CYAN
  );

  lcd.setFont(
    &fonts::Font0
  );

  lcd.setTextDatum(
    textdatum_t::middle_left
  );

  lcd.setTextColor(
    TFT_BLACK,
    TFT_WHITE
  );

  lcd.drawString(
    touchKeyboardDisplayValue(),
    14,
    48
  );

  lcd.setTextDatum(
    textdatum_t::middle_right
  );

  const size_t maximumLength =
    touchKeyboardTarget ==
      TouchKeyboardTarget::Ssid
      ? TOUCH_NETWORK_SSID_MAX_LENGTH
      : TOUCH_NETWORK_PASSWORD_MAX_LENGTH;

  lcd.drawString(
    String(touchKeyboardValue.length()) +
      "/" +
      String(maximumLength),
    306,
    48
  );

  for (
    size_t i = 0;
    i < TOUCH_KEYBOARD_CELL_COUNT;
    ++i
  ) {
    drawTouchKeyboardCell(i);
  }

  drawTouchUiButton(
    KEYBOARD_CANCEL
  );

  drawTouchUiButton(
    KEYBOARD_PREVIOUS
  );

  drawTouchUiButton(
    KEYBOARD_NEXT
  );

  drawTouchUiButton(
    KEYBOARD_ACCEPT
  );
}


void moveTouchKeyboardSelection(
  int delta
) {
  int next =
    static_cast<int>(
      touchKeyboardSelectedCell
    ) + delta;

  while (next < 0) {
    next += TOUCH_KEYBOARD_CELL_COUNT;
  }

  while (
    next >=
      static_cast<int>(
        TOUCH_KEYBOARD_CELL_COUNT
      )
  ) {
    next -= TOUCH_KEYBOARD_CELL_COUNT;
  }

  const size_t previous =
    touchKeyboardSelectedCell;

  touchKeyboardSelectedCell =
    static_cast<size_t>(next);

  drawTouchKeyboardCell(previous);
  drawTouchKeyboardCell(
    touchKeyboardSelectedCell
  );
}


void completeTouchKeyboard() {
  if (
    touchKeyboardTarget ==
      TouchKeyboardTarget::Ssid
  ) {
    String ssid =
      touchKeyboardValue;

    ssid.trim();

    if (ssid.length() == 0) {
      return;
    }

    const bool changed =
      ssid != touchNetworkDraftSsid;

    touchNetworkDraftSsid = ssid;
    touchNetworkDraftIndex = -1;

    for (
      size_t i = 0;
      i < touchNetworkCatalogCount;
      ++i
    ) {
      if (touchNetworkCatalog[i].ssid == ssid) {
        touchNetworkDraftIndex =
          static_cast<int>(i);
        touchNetworkListOffset =
          (i / TOUCH_NETWORK_ROWS_PER_PAGE) *
          TOUCH_NETWORK_ROWS_PER_PAGE;
        break;
      }
    }

    if (
      changed &&
      ssid != networkSettings.ssid
    ) {
      touchNetworkDraftPassword = "";
    } else if (
      ssid == networkSettings.ssid &&
      touchNetworkDraftPassword.length() == 0
    ) {
      touchNetworkDraftPassword =
        networkSettings.password;
    }

    touchNetworkMessage =
      String("SSID: ") + ssid;
  } else if (
    touchKeyboardTarget ==
      TouchKeyboardTarget::Password
  ) {
    touchNetworkDraftPassword =
      touchKeyboardValue;

    touchNetworkMessage =
      touchNetworkDraftPassword.length() > 0
        ? "Password updated"
        : "Password cleared";
  }

  touchNetworkMessageError = false;
  touchKeyboardTarget =
    TouchKeyboardTarget::None;
  activeTouchUiPage =
    TouchUiPage::Network;
  drawActiveTouchUiPage();
}


void cancelTouchKeyboard() {
  touchKeyboardValue =
    touchKeyboardOriginalValue;
  touchKeyboardTarget =
    TouchKeyboardTarget::None;
  activeTouchUiPage =
    TouchUiPage::Network;
  drawActiveTouchUiPage();
}


void activateTouchKeyboardSelection() {
  if (
    touchKeyboardSelectedCell >=
      TOUCH_KEYBOARD_CELL_COUNT
  ) {
    return;
  }

  const TouchKeyboardCell &cell =
    activeTouchKeyboardCells()[
      touchKeyboardSelectedCell
    ];

  const size_t maximumLength =
    touchKeyboardTarget ==
      TouchKeyboardTarget::Ssid
      ? TOUCH_NETWORK_SSID_MAX_LENGTH
      : TOUCH_NETWORK_PASSWORD_MAX_LENGTH;

  switch (cell.kind) {
    case TouchKeyboardCellKind::Character:
      if (
        touchKeyboardValue.length() <
          maximumLength
      ) {
        touchKeyboardValue +=
          cell.value;
      }
      break;

    case TouchKeyboardCellKind::Space:
      if (
        touchKeyboardValue.length() <
          maximumLength
      ) {
        touchKeyboardValue += ' ';
      }
      break;

    case TouchKeyboardCellKind::Delete:
      if (touchKeyboardValue.length() > 0) {
        touchKeyboardValue.remove(
          touchKeyboardValue.length() - 1
        );
      }
      break;

    case TouchKeyboardCellKind::Upper:
      touchKeyboardMode =
        TouchKeyboardMode::Upper;
      break;

    case TouchKeyboardCellKind::Lower:
      touchKeyboardMode =
        TouchKeyboardMode::Lower;
      break;

    case TouchKeyboardCellKind::Symbols:
      touchKeyboardMode =
        TouchKeyboardMode::Symbols;
      break;

    case TouchKeyboardCellKind::MoreSymbols:
      touchKeyboardMode =
        TouchKeyboardMode::Symbols2;
      break;

    case TouchKeyboardCellKind::ToggleMaskOrClear:
      if (
        touchKeyboardTarget ==
          TouchKeyboardTarget::Password
      ) {
        touchKeyboardRevealPassword =
          !touchKeyboardRevealPassword;
      } else {
        touchKeyboardValue = "";
      }
      break;

    case TouchKeyboardCellKind::Enter:
      completeTouchKeyboard();
      return;
  }

  drawTouchUiKeyboard();
}


void connectTouchNetwork() {
  if (touchNetworkDraftSsid.length() == 0) {
    touchNetworkMessage =
      "Select or enter an SSID first";
    touchNetworkMessageError = true;
    drawTouchUiNetwork();
    return;
  }

  if (
    touchNetworkDraftIndex >= 0 &&
    !touchNetworkDraftIsOpen() &&
    touchNetworkDraftPassword.length() == 0
  ) {
    touchNetworkMessage =
      "Enter a password, or connect if the network is open";
    touchNetworkMessageError = true;
    drawTouchUiNetwork();
    return;
  }

  drawTouchUiBusy(
    "CONNECTING",
    touchNetworkDraftSsid
  );

  String result;

  const bool connected =
    applyTouchNetworkSettings(
      touchNetworkDraftSsid,
      touchNetworkDraftPassword,
      result
    );

  touchNetworkMessage = result;
  touchNetworkMessageError =
    !connected;
  touchNetworkForgetArmed = false;

  if (connected) {
    touchNetworkDraftSsid =
      networkSettings.ssid;
    touchNetworkDraftPassword =
      networkSettings.password;
  }

  activeTouchUiPage =
    TouchUiPage::Network;

  drawActiveTouchUiPage();
}


void forgetTouchNetwork() {
  if (!networkSettings.configured) {
    touchNetworkMessage =
      "No saved Wi-Fi network";
    touchNetworkMessageError = true;
    touchNetworkForgetArmed = false;
    drawTouchUiNetwork();
    return;
  }

  if (!touchNetworkForgetArmed) {
    touchNetworkForgetArmed = true;
    touchNetworkMessage =
      "Press CONFIRM to erase Wi-Fi and restart setup";
    touchNetworkMessageError = true;
    drawTouchUiNetwork();
    return;
  }

  drawTouchUiBusy(
    "FORGET WI-FI",
    "Restarting in setup mode"
  );

  if (!clearSavedWifiCredentials()) {
    touchNetworkMessage =
      "Saved Wi-Fi could not be cleared";
    touchNetworkMessageError = true;
    touchNetworkForgetArmed = false;
    activeTouchUiPage =
      TouchUiPage::Network;
    drawActiveTouchUiPage();
    return;
  }

  WiFi.disconnect(
    true,
    true
  );

  delay(800);
  ESP.restart();
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
    "Light:%u  Day gamma:%s  Night gamma:%s",
    displaySettings.brightness,
    mapGammaText(displaySettings.dayMapGamma).c_str(),
    mapGammaText(displaySettings.nightMapGamma).c_str()
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
    "DIAGNOSTICS"
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


uint16_t displayTestGray565(uint8_t value) {
  return static_cast<uint16_t>(
    ((value >> 3) << 11) |
    ((value >> 2) << 5) |
    (value >> 3)
  );
}


void drawTouchUiDisplayTest() {
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("GRAYSCALE", 2, 2);

  for (int step = 0; step < 16; ++step) {
    const uint8_t value = static_cast<uint8_t>(step * 17);
    lcd.fillRect(
      step * 20,
      15,
      20,
      34,
      displayTestGray565(value)
    );
  }

  lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  lcd.drawString("NEAR BLACK", 2, 51);

  for (int step = 0; step < 16; ++step) {
    const uint8_t value = static_cast<uint8_t>(step * 2);
    lcd.fillRect(
      step * 20,
      63,
      20,
      20,
      displayTestGray565(value)
    );
  }

  lcd.drawString("NEAR WHITE", 2, 85);

  for (int step = 0; step < 16; ++step) {
    const uint8_t value = static_cast<uint8_t>(225 + step * 2);
    lcd.fillRect(
      step * 20,
      97,
      20,
      20,
      displayTestGray565(value)
    );
  }

  for (int x = 0; x < SCREEN_W; ++x) {
    const uint8_t value = static_cast<uint8_t>(
      (static_cast<uint32_t>(x) * 255U) / (SCREEN_W - 1)
    );

    lcd.drawFastVLine(
      x,
      121,
      20,
      static_cast<uint16_t>((value >> 3) << 11)
    );

    lcd.drawFastVLine(
      x,
      143,
      20,
      static_cast<uint16_t>((value >> 2) << 5)
    );

    lcd.drawFastVLine(
      x,
      165,
      20,
      static_cast<uint16_t>(value >> 3)
    );
  }

  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("R", 2, 126);
  lcd.drawString("G", 2, 148);
  lcd.drawString("B", 2, 170);

  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.drawString(
    String("LIGHT ") + displayBrightnessText() +
      "  D " + mapGammaText(displaySettings.dayMapGamma) +
      "  N " + mapGammaText(displaySettings.nightMapGamma),
    SCREEN_W / 2,
    194
  );

  for (
    const TouchUiButton *button :
      DISPLAY_TEST_BUTTONS
  ) {
    drawTouchUiButton(*button);
  }
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

    case TouchUiPage::Network:
      touchNetworkDraftSsid =
        networkSettings.ssid;

      touchNetworkDraftPassword =
        networkSettings.password;

      touchNetworkDraftIndex = -1;
      touchNetworkListOffset = 0;
      touchNetworkMessage = "";
      touchNetworkMessageError = false;
      touchNetworkForgetArmed = false;
      break;

    case TouchUiPage::MapPreview:
    case TouchUiPage::MapMaintenance:
    case TouchUiPage::Keyboard:
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
    case TouchUiPage::Weather:
      drawTouchUiWeather();
      break;

    case TouchUiPage::WeatherRadar:
      drawTouchUiWeatherRadar();
      break;

    case TouchUiPage::Market:
      drawTouchUiMarket();
      break;

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

    case TouchUiPage::DisplayTest:
      drawTouchUiDisplayTest();
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
      drawTouchUiNetwork();
      break;

    case TouchUiPage::Keyboard:
      drawTouchUiKeyboard();
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

  if (activeTouchUiPage == TouchUiPage::WeatherRadar) {
    releaseWeatherRadarDisplayBuffer();
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
  if (
    activeTouchUiPage == TouchUiPage::WeatherRadar &&
    page != TouchUiPage::WeatherRadar
  ) {
    releaseWeatherRadarDisplayBuffer();
  }

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

  if (saved && sdReady) {
    initializeWeatherService();

    if (weatherFeatureAvailable()) {
      requestWeatherForecastRefresh();
    }
  }

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

    case TouchUiButtonId::OpenWeather:
      showTouchUiPage(TouchUiPage::Weather);

      if (weatherForecastIsStale()) {
        requestWeatherForecastRefresh();
      }
      break;

    case TouchUiButtonId::OpenMarket:
      showTouchUiPage(TouchUiPage::Market);

      if (marketDataIsStale()) {
        requestMarketRefresh(
          marketSnapshot.thirtyDayCount == 0
        );
      }
      break;

    case TouchUiButtonId::WeatherRadar:
      resetWeatherRadarDisplayMode();
      showTouchUiPage(TouchUiPage::WeatherRadar);

      if (weatherRadarIsStale()) {
        requestWeatherRadarRefresh();
      }
      break;

    case TouchUiButtonId::WeatherForecast:
      showTouchUiPage(TouchUiPage::Weather);
      break;

    case TouchUiButtonId::WeatherRefresh:
      requestWeatherForecastRefresh();
      drawTouchUiWeather();
      break;

    case TouchUiButtonId::WeatherRadarRefresh:
      requestWeatherRadarRefresh();
      drawTouchUiWeatherRadar();
      break;

    case TouchUiButtonId::WeatherRadarLatest:
      selectWeatherRadarLatestMode();
      drawTouchUiWeatherRadarFrame(true);
      touchUiLastRadarAnimationAt = millis();
      break;

    case TouchUiButtonId::WeatherRadarLoop:
      selectWeatherRadarLoopMode();
      drawTouchUiWeatherRadarFrame(true);
      touchUiLastRadarAnimationAt = millis();
      break;

    case TouchUiButtonId::MarketToday:
      touchMarketPeriod = TouchMarketPeriod::Today;
      drawTouchUiMarket();
      break;

    case TouchUiButtonId::MarketThirtyDays:
      touchMarketPeriod = TouchMarketPeriod::ThirtyDays;
      drawTouchUiMarket();
      break;

    case TouchUiButtonId::MarketRefresh:
      requestMarketRefresh(true);
      drawTouchUiMarket();
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

      scanTouchNetworks();
      break;

    case TouchUiButtonId::Diagnostics:
      showTouchUiPage(
        TouchUiPage::Diagnostics
      );
      break;

    case TouchUiButtonId::DisplayTest:
      showTouchUiPage(
        TouchUiPage::DisplayTest
      );
      break;

    case TouchUiButtonId::BrightnessDown:
      setDisplayBrightness(
        static_cast<uint8_t>(max(
          static_cast<int>(DISPLAY_BRIGHTNESS_MIN),
          static_cast<int>(displaySettings.brightness) -
            static_cast<int>(DISPLAY_BRIGHTNESS_STEP)
        )),
        true
      );
      drawTouchUiDisplayTest();
      break;

    case TouchUiButtonId::BrightnessUp:
      setDisplayBrightness(
        static_cast<uint8_t>(min(
          static_cast<int>(DISPLAY_BRIGHTNESS_MAX),
          static_cast<int>(displaySettings.brightness) +
            static_cast<int>(DISPLAY_BRIGHTNESS_STEP)
        )),
        true
      );
      drawTouchUiDisplayTest();
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

    case TouchUiButtonId::NetworkRow0:
    case TouchUiButtonId::NetworkRow1:
    case TouchUiButtonId::NetworkRow2: {
      const int catalogIndex =
        touchNetworkCatalogIndexForRowButton(id);

      if (catalogIndex >= 0) {
        const String previousSsid =
          touchNetworkDraftSsid;

        touchNetworkDraftIndex =
          catalogIndex;

        touchNetworkDraftSsid =
          touchNetworkCatalog[
            catalogIndex
          ].ssid;

        if (
          touchNetworkDraftSsid ==
            networkSettings.ssid
        ) {
          touchNetworkDraftPassword =
            networkSettings.password;
        } else if (
          previousSsid !=
            touchNetworkDraftSsid
        ) {
          touchNetworkDraftPassword = "";
        }

        touchNetworkMessage =
          touchNetworkCatalog[
            catalogIndex
          ].open
            ? "Open network selected"
            : "Secured network selected";

        touchNetworkMessageError = false;
        touchNetworkForgetArmed = false;
        drawTouchUiNetwork();
      }
      break;
    }

    case TouchUiButtonId::NetworkPagePrevious:
      if (
        touchNetworkListOffset >=
          TOUCH_NETWORK_ROWS_PER_PAGE
      ) {
        touchNetworkListOffset -=
          TOUCH_NETWORK_ROWS_PER_PAGE;
        drawTouchUiNetwork();
      }
      break;

    case TouchUiButtonId::NetworkPageNext:
      if (
        touchNetworkListOffset +
          TOUCH_NETWORK_ROWS_PER_PAGE <
          touchNetworkCatalogCount
      ) {
        touchNetworkListOffset +=
          TOUCH_NETWORK_ROWS_PER_PAGE;
        drawTouchUiNetwork();
      }
      break;

    case TouchUiButtonId::NetworkOtherSsid:
      openTouchKeyboard(
        TouchKeyboardTarget::Ssid,
        touchNetworkDraftSsid
      );
      break;

    case TouchUiButtonId::NetworkPassword:
      openTouchKeyboard(
        TouchKeyboardTarget::Password,
        touchNetworkDraftPassword
      );
      break;

    case TouchUiButtonId::NetworkScan:
      scanTouchNetworks();
      break;

    case TouchUiButtonId::NetworkForget:
      forgetTouchNetwork();
      break;

    case TouchUiButtonId::NetworkConnect:
      connectTouchNetwork();
      break;

    case TouchUiButtonId::KeyboardCancel:
      cancelTouchKeyboard();
      break;

    case TouchUiButtonId::KeyboardPrevious:
      moveTouchKeyboardSelection(-1);
      break;

    case TouchUiButtonId::KeyboardNext:
      moveTouchKeyboardSelection(1);
      break;

    case TouchUiButtonId::KeyboardAccept:
      activateTouchKeyboardSelection();
      break;

    case TouchUiButtonId::KeyboardCell:
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

    case TouchUiButtonId::RecalibrateTouch:
      runIntegratedTouchCalibration(false);

      activeTouchUiPage =
        TouchUiPage::Diagnostics;

      touchUiLastActivityAt =
        millis();

      drawTouchUiDiagnostics();
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
      if (activeTouchUiPage == TouchUiPage::WeatherRadar) {
        showTouchUiPage(TouchUiPage::Weather);
      } else if (activeTouchUiPage == TouchUiPage::DisplayTest) {
        showTouchUiPage(TouchUiPage::Diagnostics);
      } else if (
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

  if (
    activeTouchUiPage ==
      TouchUiPage::Keyboard &&
    activeTouchUiButton ==
      TouchUiButtonId::KeyboardCell
  ) {
    const int cellIndex =
      touchKeyboardCellAt(
        event.x,
        event.y
      );

    if (cellIndex >= 0) {
      const size_t previous =
        touchKeyboardSelectedCell;

      touchKeyboardSelectedCell =
        static_cast<size_t>(
          cellIndex
        );

      touchKeyboardPressedCell =
        cellIndex;

      drawTouchKeyboardCell(
        previous
      );

      drawTouchKeyboardCell(
        touchKeyboardSelectedCell,
        true
      );
    }

    // Directly touching a cell only moves the selection. The bottom check
    // control deliberately activates the selected cell.
    activeTouchUiButton =
      TouchUiButtonId::None;

    activeTouchUiButtonValid = false;
    activeTouchUiOutsideFrames = 0;
    return;
  }

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
  if (
    activeTouchUiPage ==
      TouchUiPage::Keyboard &&
    touchKeyboardPressedCell >= 0
  ) {
    drawTouchKeyboardCell(
      static_cast<size_t>(
        touchKeyboardPressedCell
      )
    );

    touchKeyboardPressedCell = -1;
  }

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


void showTouchUiDisplayTest() {
  showTouchUiPage(TouchUiPage::DisplayTest);
}


void initializeTouchUi() {

  if (touchUiInitialized) {
    return;
  }

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

  const unsigned long radarAnimationDelay =
    weatherRadarFrameCount() > 0 &&
    weatherRadarSelectedFrameIndex() + 1 >= weatherRadarFrameCount()
      ? WEATHER_RADAR_ANIMATION_LATEST_HOLD_MS
      : WEATHER_RADAR_ANIMATION_FRAME_MS;

  if (
    activeTouchUiPage == TouchUiPage::WeatherRadar &&
    activeTouchUiButton == TouchUiButtonId::None &&
    weatherRadarAnimationIsPlaying() &&
    !weatherRadarRefreshInProgress() &&
    millis() - touchUiLastRadarAnimationAt >=
      radarAnimationDelay
  ) {
    selectNextWeatherRadarFrame();
    drawTouchUiWeatherRadarFrame(false);
    touchUiLastRadarAnimationAt = millis();
  }

  // Keep the live radar display open until the user explicitly leaves it.
  // Other touchscreen pages still return to the clock after inactivity.
  if (
    activeTouchUiPage !=
      TouchUiPage::Clock &&
    activeTouchUiPage !=
      TouchUiPage::WeatherRadar &&
    activeTouchUiPage !=
      TouchUiPage::Market &&
    activeTouchUiPage !=
      TouchUiPage::DisplayTest &&
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
  if (activeTouchUiPage == TouchUiPage::WeatherRadar) {
    releaseWeatherRadarDisplayBuffer();
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

  if (activeTouchUiPage == TouchUiPage::WeatherRadar) {
    releaseWeatherRadarDisplayBuffer();
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

bool touchUiWeatherPageIsOpen() {
  return
    activeTouchUiPage == TouchUiPage::Weather ||
    activeTouchUiPage == TouchUiPage::WeatherRadar;
}


bool touchUiWeatherRadarIsOpen() {
  return activeTouchUiPage == TouchUiPage::WeatherRadar;
}


void touchUiHandleWeatherUpdated() {
  if (activeTouchUiPage == TouchUiPage::Weather) {
    drawTouchUiWeather();
  } else if (activeTouchUiPage == TouchUiPage::WeatherRadar) {
    drawTouchUiWeatherRadar();
  }
}


bool touchUiMarketPageIsOpen() {
  return activeTouchUiPage == TouchUiPage::Market;
}


void touchUiHandleMarketUpdated() {
  if (activeTouchUiPage == TouchUiPage::Market) {
    drawTouchUiMarket();
  }
}

