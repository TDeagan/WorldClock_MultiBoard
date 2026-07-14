#pragma once

// ============================================================
// BOARD SELECTION
// ============================================================
//
// Change only WORLDCLOCK_BOARD to build for another carrier PCB.
//

#define BOARD_HELTEC_WROOM_28   1
#define BOARD_E32R28T           2
#define BOARD_ELEGOO_EL_EB_009  3

#define WORLDCLOCK_BOARD BOARD_HELTEC_WROOM_28

#include "board_profiles.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <LovyanGFX.hpp>
#include <SPI.h>
#include <SD.h>
#include <PNGdec.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ============================================================
// TARGET BOARD
// ============================================================
//
// Board-specific values are defined in board_profiles.h. The aliases
// below intentionally preserve the names used by the proven v3.4
// application, minimizing changes outside the display constructor.
//

// ============================================================
// USER-ADJUSTABLE APPEARANCE
// ============================================================

static constexpr int SUN_DISC_RADIUS  = 8;
static constexpr int MOON_DISC_RADIUS = 6;

// ISS is deliberately smaller than the Moon: a crisp 5 x 5 cyan plus.
static constexpr int ISS_PLUS_ARM = 2;

// ============================================================
// BOARD HARDWARE ALIASES
// ============================================================
//
// These aliases keep the v3.4 implementation tabs behavior-identical.
//

static constexpr int TFT_SCLK = ACTIVE_BOARD.tftSclk;
static constexpr int TFT_MOSI = ACTIVE_BOARD.tftMosi;
static constexpr int TFT_MISO = ACTIVE_BOARD.tftMiso;
static constexpr int TFT_CS   = ACTIVE_BOARD.tftCs;
static constexpr int TFT_DC   = ACTIVE_BOARD.tftDc;
static constexpr int TFT_BL   = ACTIVE_BOARD.tftBacklight;

static constexpr int SD_SCLK = ACTIVE_BOARD.sdSclk;
static constexpr int SD_MISO = ACTIVE_BOARD.sdMiso;
static constexpr int SD_MOSI = ACTIVE_BOARD.sdMosi;
static constexpr int SD_CS   = ACTIVE_BOARD.sdCs;

static constexpr int CONFIG_BUTTON_PIN =
  ACTIVE_BOARD.configurationButtonPin;

// This is application behavior, not a board characteristic.
// It is deliberately retained from the working v3.4 baseline.
static constexpr unsigned long CONFIG_BUTTON_HOLD_MS = 3000UL;

// ============================================================
// DISPLAY AND MAP GEOMETRY
// ============================================================

static constexpr int MAP_W = 320;
static constexpr int MAP_H = 240;

static constexpr int SCREEN_W = 320;
static constexpr int SCREEN_H = 240;
static constexpr int STATUS_H = 21;
static constexpr int SCREEN_MAP_H = SCREEN_H - STATUS_H;

// Centralized status-area layout.
static constexpr int STATUS_BAR_Y = SCREEN_MAP_H;
static constexpr int STATUS_BAR_HEIGHT = STATUS_H;
static constexpr int CLOCK_TEXT_Y = SCREEN_MAP_H + STATUS_H / 2 - 4;
static constexpr int IP_TEXT_Y = 232;
static constexpr int IP_BACKGROUND_X = 98;
static constexpr int IP_BACKGROUND_Y = 232;
static constexpr int IP_BACKGROUND_W = 124;
static constexpr int IP_BACKGROUND_H = 8;
static constexpr uint16_t STATUS_BACKGROUND_COLOR = TFT_BLACK;
static constexpr uint16_t STATUS_TEXT_COLOR = TFT_CYAN;
static constexpr uint16_t IP_TEXT_COLOR = TFT_CYAN;

static constexpr uint32_t RAW_MAP_BYTES =
  MAP_W * MAP_H * sizeof(uint16_t);

static constexpr int DISPLAY_ROTATION = ACTIVE_BOARD.displayRotation;

// ============================================================
// SD-CARD FILENAMES
// ============================================================

static constexpr const char *DAY_PNG_FILE =
  "/earth_day.png";

static constexpr const char *NIGHT_PNG_FILE =
  "/earth_night.png";

static constexpr const char *DAY_RAW_FILE =
  "/earth_day.rgb565";

static constexpr const char *NIGHT_RAW_FILE =
  "/earth_night.rgb565";

// ============================================================
// CAPTIVE PORTAL
// ============================================================

static constexpr const char *CONFIG_AP_PREFIX =
  "WorldClock";

static constexpr uint8_t CONFIG_AP_CHANNEL = 1;
static constexpr uint8_t CONFIG_DNS_PORT = 53;
static constexpr unsigned long PORTAL_RESTART_DELAY_MS = 1800UL;

// Normal-network configuration page.
static constexpr uint16_t RUNTIME_CONFIG_PORT = 80;
static constexpr const char *RUNTIME_CONFIG_PATH = "/";
static constexpr const char *DIAGNOSTICS_PATH = "/diagnostics";
static constexpr const char *MAP_MAINTENANCE_PATH = "/maps";
static constexpr const char *MAP_VALIDATE_PATH = "/maps/validate";
static constexpr const char *MAP_REBUILD_DAY_PATH = "/maps/rebuild-day";
static constexpr const char *MAP_REBUILD_NIGHT_PATH = "/maps/rebuild-night";
static constexpr const char *MAP_REBUILD_BOTH_PATH = "/maps/rebuild-both";

// ============================================================
// PREFERENCES / NONVOLATILE STORAGE
// ============================================================

static constexpr const char *PREF_NAMESPACE =
  "worldclock";

static constexpr const char *PREF_KEY_SSID =
  "ssid";

static constexpr const char *PREF_KEY_PASSWORD =
  "password";

static constexpr const char *PREF_KEY_OFFSET_MINUTES =
  "utcOffset";

static constexpr const char *PREF_KEY_CONFIGURED =
  "configured";

static constexpr const char *PREF_KEY_SHOW_SUN =
  "showSun";

static constexpr const char *PREF_KEY_SHOW_MOON =
  "showMoon";

static constexpr const char *PREF_KEY_SHOW_ISS =
  "showISS";

static constexpr const char *PREF_KEY_SHOW_ISS_TRACK =
  "showTrack";

static constexpr const char *PREF_KEY_ISS_TRACK_DOTTED =
  "trackDots";

static constexpr int MIN_UTC_OFFSET_MINUTES =
  -12 * 60;

static constexpr int MAX_UTC_OFFSET_MINUTES =
  14 * 60;

// ============================================================
// NETWORK AND UPDATE TIMING
// ============================================================

static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS =
  30000UL;

static constexpr unsigned long NTP_SYNC_TIMEOUT_MS =
  30000UL;

// Unified wall-clock scheduler.
//
// The status line updates on 30-second UTC epoch boundaries.
// All astronomy/map elements update together on five-minute boundaries.
static constexpr time_t CLOCK_UPDATE_SECONDS =
  30;

static constexpr time_t ASTRONOMY_UPDATE_SECONDS =
  5 * 60;

// Retained compatibility aliases. The application scheduler no longer
// runs separate map and ISS refresh timers.
static constexpr unsigned long CLOCK_UPDATE_MS =
  CLOCK_UPDATE_SECONDS * 1000UL;

static constexpr unsigned long MAP_UPDATE_MS =
  ASTRONOMY_UPDATE_SECONDS * 1000UL;

static constexpr unsigned long SD_RETRY_MS =
  5000UL;

static constexpr unsigned long WIFI_RETRY_MS =
  10000UL;

// ISS refresh is now part of the unified astronomy update.
static constexpr unsigned long ISS_UPDATE_MS =
  ASTRONOMY_UPDATE_SECONDS * 1000UL;

static constexpr unsigned long NTP_RETRY_MS =
  3600000UL;

static constexpr unsigned long STORAGE_RETRY_MS =
  30000UL;

static constexpr const char *FIRMWARE_VERSION =
  "4.2";

// ============================================================
// STARTUP SPLASH
// ============================================================

// Embedded 128 x 128 logo; no SD card or PNG decoder is required.
static constexpr unsigned long SPLASH_DURATION_MS = 3000UL;

static constexpr int SPLASH_LOGO_W = 128;
static constexpr int SPLASH_LOGO_H = 128;
static constexpr int SPLASH_LOGO_X =
  (SCREEN_W - SPLASH_LOGO_W) / 2;
static constexpr int SPLASH_LOGO_Y = 12;

static constexpr int SPLASH_TITLE_Y = 151;
static constexpr int SPLASH_VERSION_Y = 174;
static constexpr int SPLASH_BOARD_Y = 203;

static constexpr uint16_t SPLASH_BACKGROUND_COLOR = TFT_BLACK;
static constexpr uint16_t SPLASH_TITLE_COLOR = TFT_WHITE;
static constexpr uint16_t SPLASH_VERSION_COLOR = TFT_CYAN;
static constexpr uint16_t SPLASH_BOARD_COLOR = TFT_YELLOW;


// Approximate one-orbit ground track.
// 187 points is roughly one point every 30 seconds.
static constexpr int ISS_TRACK_POINT_COUNT = 187;
static constexpr double ISS_ORBIT_PERIOD_SECONDS = 92.68 * 60.0;
static constexpr double ISS_ORBIT_INCLINATION_DEGREES = 51.64;
static constexpr double EARTH_ROTATION_DEGREES_PER_SECOND =
  360.0 / 86164.0905;

// Half-bright cyan keeps the 1-pixel track from dominating the map.
static constexpr uint16_t ISS_TRACK_COLOR = 0x03EF;

static constexpr const char *ISS_API_URL =
  "https://api.wheretheiss.at/v1/satellites/25544";

// ============================================================
// DISPLAY CLASS
// ============================================================

class E32R28T_Display : public lgfx::LGFX_Device {
private:
  lgfx::Panel_ILI9341 panel;
  lgfx::Bus_SPI bus;

public:
  E32R28T_Display();
};

// ============================================================
// SAVED NETWORK SETTINGS
// ============================================================

struct NetworkSettings {
  String ssid;
  String password;
  int utcOffsetMinutes = 0;
  bool configured = false;
};

struct OverlaySettings {
  bool showSun = true;
  bool showMoon = true;
  bool showISS = false;
  bool showIssTrack = false;
  bool issTrackDotted = false;
};

struct IssPosition {
  double latitude = 0.0;
  double longitude = 0.0;
  bool valid = false;
  bool ascending = true;
  time_t updatedAt = 0;
};

struct OrbitTrackPoint {
  float latitude = 0.0f;
  float longitude = 0.0f;
  bool valid = false;
};



enum class SystemError {
  None,
  WifiDisconnected,
  NtpUnavailable,
  SdUnavailable,
  DayPngMissing,
  NightPngMissing,
  DayCacheInvalid,
  NightCacheInvalid,
  IssUnavailable
};

struct SystemStatus {
  bool wifiConnected = false;
  bool ntpSynced = false;
  bool sdMounted = false;
  bool dayPngFound = false;
  bool nightPngFound = false;
  bool dayCacheValid = false;
  bool nightCacheValid = false;
  bool issPositionValid = false;
  bool orbitTrackValid = false;
  time_t lastNtpSync = 0;
  time_t lastIssUpdate = 0;
  unsigned long lastStorageAttempt = 0;
  SystemError lastError = SystemError::None;
  String lastErrorText;
};

// ============================================================
// SHARED OBJECTS AND STATE
// ============================================================

extern E32R28T_Display lcd;
extern SPIClass sdSPI;
extern PNG png;

extern File dayFile;
extern File nightFile;
extern File pngInputFile;
extern File pngOutputFile;

extern uint16_t dayLine[MAP_W];
extern uint16_t nightLine[MAP_W];
extern uint16_t outputLine[SCREEN_W];
extern uint16_t pngLine[MAP_W];

extern bool timeValid;
extern bool sdReady;
extern bool pngWriteFailed;

extern unsigned long lastMapUpdate;
extern unsigned long lastClockUpdate;
extern unsigned long lastSDRetry;
extern unsigned long lastWiFiRetry;
extern unsigned long lastIssUpdate;

extern NetworkSettings networkSettings;
extern OverlaySettings overlaySettings;
extern IssPosition issPosition;
extern OrbitTrackPoint issTrack[ISS_TRACK_POINT_COUNT];
extern bool issTrackValid;
extern SystemStatus systemStatus;
extern unsigned long lastNtpRetry;

// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

// Main application
void initializeWorldClock();
void serviceWorldClock();
void updateAstronomy();
void showSplashScreen();
void drawIpAddress();
void redrawWorldClock();
void setSystemError(SystemError error, const String &text);
const char *systemErrorName(SystemError error);

// Internal math/map helpers
static double norm180(double x);
static double norm360(double x);
static double degToRad(double degrees);
static double radToDeg(double radians);
static int longitudeToX(double longitude);
static int latitudeToY(double latitude);

// Utilities/display messages
void centerText(
  const String &text,
  int y,
  uint16_t color
);

void showStatus(
  const String &line1,
  const String &line2 = ""
);

// PNG decoding and SD cache
void *pngOpenCallback(
  const char *filename,
  int32_t *fileSize
);

void pngCloseCallback(void *handle);

int32_t pngReadCallback(
  PNGFILE *handle,
  uint8_t *buffer,
  int32_t length
);

int32_t pngSeekCallback(
  PNGFILE *handle,
  int32_t position
);

int pngDrawCallback(PNGDRAW *draw);

bool rawMapIsValid(const char *path);

bool convertPngToRaw(
  const char *pngPath,
  const char *rawPath,
  const String &screenLabel
);

bool ensureRawMapCaches();
void closeMapFiles();
bool openMapFiles();
bool initializeSD();

bool readMapLine(
  File &file,
  int sourceY,
  uint16_t *buffer
);

bool recoverSD();
bool refreshStorageStatus();
bool rebuildMapCache(bool rebuildDay, bool rebuildNight);
uint32_t fileSizeOrZero(const char *path);

// Solar/map
void solarPositionUTC(
  const tm &utc,
  double &declination,
  double &subsolarLongitude
);

uint16_t blend565(
  uint16_t night,
  uint16_t day,
  uint8_t amount
);

bool drawMap(const tm &utc);

// Sun and Moon
void calculateMoonInfo(
  time_t epoch,
  double &moonLatitude,
  double &moonLongitude,
  double &moonElongation,
  bool &moonWaxing
);

void drawWrappedSunDisc(int x, int y);

void drawWrappedMoonPhase(
  int x,
  int y,
  double elongation,
  bool waxing
);

void drawCelestialMarkers(time_t epoch);

// ISS
bool fetchIssPosition();
void drawIssMarker();

// ISS one-orbit track
void calculateIssOrbitTrack();
void drawIssOrbitTrack();

// Clock/network
void drawClock();
bool connectConfiguredWiFi();
bool synchronizeConfiguredTime();
bool retryNtpSync();

// Captive portal and saved settings
bool loadNetworkSettings();

bool saveNetworkSettings(
  const String &ssid,
  const String &password,
  int utcOffsetMinutes
);

bool loadOverlaySettings();
bool saveOverlaySettings();

void clearNetworkSettings();
void serviceConfigurationButton();
String makeConfigurationApSsid();
void runConfigurationPortal();
bool ensureNetworkConfigured();

// Runtime configuration page on the normal Wi-Fi network.
void startRuntimeConfigServer();
void serviceRuntimeConfigServer();
String buildDiagnosticsPage();
String buildMapMaintenancePage(const String &message = "", bool error = false);
