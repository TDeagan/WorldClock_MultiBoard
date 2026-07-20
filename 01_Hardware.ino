// ============================================================
// Board-independent display constructor
//
// Pin aliases come from config.h; panel geometry and color behavior
// come directly from ACTIVE_BOARD in board_profiles.h.
// ============================================================

E32R28T_Display::E32R28T_Display() {
  auto b = bus.config();

  // LCD is connected to the ESP32 HSPI/SPI2 peripheral.
  b.spi_host = SPI2_HOST;
  b.spi_mode = 0;
  b.freq_write = 40000000;
  b.freq_read = 16000000;
  b.spi_3wire = false;
  b.use_lock = true;
  b.dma_channel = SPI_DMA_CH_AUTO;

  b.pin_sclk = TFT_SCLK;
  b.pin_mosi = TFT_MOSI;
  b.pin_miso = TFT_MISO;
  b.pin_dc = TFT_DC;

  bus.config(b);
  panel.setBus(&bus);

  auto p = panel.config();

  p.pin_cs = TFT_CS;

  // LCD reset is connected to the board reset circuit.
  p.pin_rst = ACTIVE_BOARD.tftReset;
  p.pin_busy = -1;

  p.memory_width = ACTIVE_BOARD.memoryWidth;
  p.memory_height = ACTIVE_BOARD.memoryHeight;
  p.panel_width = ACTIVE_BOARD.panelWidth;
  p.panel_height = ACTIVE_BOARD.panelHeight;

  p.offset_x = 0;
  p.offset_y = 0;
  p.offset_rotation = ACTIVE_BOARD.panelOffsetRotation;
  p.dummy_read_pixel = ACTIVE_BOARD.dummyReadPixelBits;
  p.dummy_read_bits = ACTIVE_BOARD.dummyReadCommandBits;

  p.readable = true;
  p.invert = ACTIVE_BOARD.invertDisplay;

  p.rgb_order = ACTIVE_BOARD.rgbOrder;

  p.dlen_16bit = false;
  p.bus_shared = false;

  panel.config(p);
  setPanel(&panel);
}

// ============================================================
// Shared global objects
// ============================================================

E32R28T_Display lcd;

// microSD uses the separate VSPI/SPI3 peripheral.
SPIClass sdSPI(VSPI);

PNG png;

File dayFile;
File nightFile;
File pngInputFile;
File pngOutputFile;

uint16_t dayLine[MAP_W];
uint16_t nightLine[MAP_W];
uint16_t outputLine[SCREEN_W];
uint16_t pngLine[MAP_W];

bool timeValid = false;
bool sdReady = false;
bool pngWriteFailed = false;
bool pngValidationOnly = false;

String selectedDayMapFilename = DEFAULT_DAY_MAP_FILENAME;
String activeDayPngPath = String(DAY_MAP_DIRECTORY) + "/" + DEFAULT_DAY_MAP_FILENAME;
String activeDayRawPath = String(DAY_MAP_DIRECTORY) + "/earth_day.rgb565";

unsigned long lastMapUpdate = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastSDRetry = 0;
unsigned long lastWiFiRetry = 0;
unsigned long lastIssUpdate = 0;

NetworkSettings networkSettings;
OverlaySettings overlaySettings;
DisplaySettings displaySettings;
LocationGridSettings locationGridSettings;
TimeSettings timeSettings;
IssPosition issPosition;
OrbitTrackPoint issTrack[ISS_TRACK_POINT_COUNT];
bool issTrackValid = false;

SystemStatus systemStatus;
unsigned long lastNtpRetry = 0;
