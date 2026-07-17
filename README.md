# ESP32 World Clock v5.1

ESP32 World Clock displays a live day/night world map on a 320 × 240 ILI9341 screen. It can show local and UTC time, the Sun, the Moon and its phase, the current International Space Station position, an approximate one-orbit ISS ground track, a home-location marker, a latitude/longitude grid, and saved-location weather.

The clock can be configured from its resistive touchscreen or from a phone, tablet, or computer through its built-in web interface. Daylight maps are stored on a microSD card and can be previewed, validated, selected, and cached without recompiling the firmware. Weather forecasts and a fixed-scale regional precipitation-radar image are cached on the card for immediate and offline display.

Version 5.1 adds weather based on the saved latitude and longitude. Forecasts come from Open-Meteo and precipitation radar comes from RainViewer. Alpha2 corrected the ESP32 DRAM linker overflow found in alpha1 by reusing the project's existing PNG decoder. Alpha3 made RainViewer downloads tolerant of CDN redirects and streamed responses, reported the actual HTTP or transport failure, and added a cached human-readable location name using OpenStreetMap Nominatim with coordinate fallback. Alpha4 preserves the transparent portions of RainViewer radar tiles and shortens the coordinate fallback heading by omitting the word `deg`. Alpha5 replaces the enlarged low-resolution world-map background with cached, native-resolution OpenStreetMap raster tiles at the same Web-Mercator zoom as the radar layer. Alpha6 keeps the touchscreen radar page open indefinitely until the user presses **CLOCK** or otherwise explicitly leaves the page. The underlying v5.0 firmware and the WROOM and E32R28T board profiles have been tested on physical hardware; the v5.1 weather feature should be treated as an alpha feature until it has completed the same hardware test cycle. The Elegoo/CYD profile is included, but its touchscreen remains intentionally disabled until verified on that hardware.

## Section 1: Using an Installed World Clock

### What appears on the display

The main display contains:

- A world map blended between daylight and nighttime imagery according to the current position of the Sun.
- Optional Sun, Moon, ISS, ISS orbit-track, home-location, and coordinate-grid overlays.
- A lower-left weather control when weather is enabled and a home location has been saved.
- A bottom status area showing local time, UTC time, and the local date.
- The clock's web address, such as `http://192.168.1.42`, at the bottom center.

The address is available only while the clock is connected to Wi-Fi. Open it from a device connected to the same local network.

### First-time setup

When a touch-enabled board has no saved calibration, touch calibration runs before Wi-Fi setup. Touch the four corner targets in order, then touch the center target for verification. A successful calibration is stored in the existing `touchtest` Preferences namespace and is reused on later boots.

If the touch controller is disconnected or unusable, hold the physical **BOOT** button for three seconds during calibration. The clock stores a touch-disabled state, waits for the button to be released, and then continues automatically into Wi-Fi setup. Wi-Fi and all other World Clock settings are preserved. Touch remains disabled on later boots until calibration is started from the browser **Diagnostics** page.

When the clock has no saved Wi-Fi configuration, it starts its own temporary setup network after touch setup completes or is bypassed.

1. Power on the clock with the microSD card installed.
2. Complete the four-corner and center touch calibration, or hold **BOOT** for three seconds to disable touch and continue.
3. On a phone, tablet, or computer, open the Wi-Fi settings.
4. Connect to the open network named `WorldClock-XXXX`, where `XXXX` is unique to the clock.
5. The World Clock Setup page should open automatically. If it does not, open a browser and enter the address shown on the clock display. This is normally `http://192.168.4.1`.
6. Select a detected Wi-Fi network, or type the name of a hidden network manually.
7. Enter the Wi-Fi password. Leave it blank only for an open network.
8. Select a time zone:
   - Fixed UTC offset
   - UTC
   - US Eastern
   - US Central
   - US Mountain
   - US Pacific
9. When using **Fixed UTC offset**, select `+` or `−` and enter the offset magnitude in hours. Quarter-hour offsets are supported.
10. Choose 12-hour or 24-hour time and whether seconds are shown.
11. Enter the home location in decimal degrees:
    - Choose North or South and enter a latitude magnitude from 0 through 90.
    - Choose East or West and enter a longitude magnitude from 0 through 180.
    - This saved point is also used for weather and regional radar.
12. Enable any desired map overlays.
13. Select **Save and connect**.

The clock saves the settings, restarts, connects to the selected Wi-Fi network, synchronizes its time, prepares the map cache if needed, and then displays the world clock.

US time-zone presets apply daylight-saving-time changes automatically. A fixed UTC offset does not.

### Using the touchscreen menu

Tap the bottom status bar on the clock display to open the touchscreen menu. The verified touchscreen interface provides:

- Time-zone, clock-format, seconds, and display-orientation settings
- Home latitude, longitude, hemisphere, marker, and coordinate-grid settings
- Sun, Moon, ISS, orbit-track, and track-style settings
- A scrollable daylight-map catalog with PNG and cache status
- Source-PNG map previews
- Map selection and application
- Daylight, night, and complete cache rebuild controls
- Nearby Wi-Fi scanning, SSID selection, password entry, and connection testing
- A Zoom R4-style on-screen keyboard with a selected character cell and four bottom controls: Cancel, left, right, and check
- Saved-network removal with confirmation and automatic restart into setup mode
- Touch, Wi-Fi, SD-card, NTP, and selected-map diagnostics
- Touch recalibration from Diagnostics without erasing Wi-Fi or other settings
- A saved-location weather page with current conditions and a three-day forecast
- A fixed-scale regional precipitation-radar page centered on the saved location

Settings changed on the touchscreen use the same saved Preferences values as the browser controls. Most touchscreen menus return to the clock automatically after one minute without activity. The radar page is the exception: it remains displayed until the user presses **CLOCK** or explicitly navigates away.

### Changing Wi-Fi from the touchscreen

1. Tap the bottom status bar and choose **NETWORK**.
2. Select one of the scanned networks. Use the paging arrows when more than three are listed.
3. For a hidden network, choose **OTHER SSID** and enter its name.
4. Choose **PASSWORD** to enter or edit the network password.
5. On the keyboard, touch a character cell to select it. Use the bottom `<` and `>` controls to move the selection and the check control to enter the selected character.
6. Select the keyboard's **ENTER** cell and press the check control to return to the Network page.
7. Choose **CONNECT**. The clock tests the new credentials before saving them. If the test fails, it attempts to restore the previous network.
8. To erase the saved network, choose **FORGET**, then **CONFIRM**. The clock restarts in first-time setup mode.

The password keyboard includes uppercase, lowercase, number/symbol, space, delete, show/hide, and Enter cells. The Network page keeps passwords masked outside the keyboard.

### Opening the control panel later

While the clock is operating normally:

1. Read the `http://` address displayed at the bottom center of the screen.
2. Connect the phone, tablet, or computer to the same Wi-Fi network as the clock.
3. Enter the complete address in a browser, including `http://`.

The control panel has four pages: **Settings**, **Weather**, **Diagnostics**, and **Maps**.

#### Settings

The Settings page shows the active configuration and permits changes to:

- Time zone or fixed UTC offset
- 12-hour or 24-hour clock format
- Seconds display
- Normal or 180-degree display orientation
- Home latitude and longitude
- Home-location marker
- 30-degree coordinate grid
- Weather enabled or disabled
- Fahrenheit/mph/inches or Celsius/km/h/millimeters
- Sun marker
- Moon marker and phase
- ISS marker
- One-orbit ISS ground track
- Solid or dotted ISS track

Select **Apply settings now** after making changes. Most changes are applied immediately without rebooting.

The **Forget Wi-Fi and start setup** control erases the saved network credentials and restarts the clock in setup mode. It asks for confirmation before doing so.

### Using saved-location weather

When weather is enabled and a home latitude and longitude have been saved, a weather control appears in the lower-left corner of the world map. Before the first forecast arrives it displays `WX --`; afterward it displays a compact condition icon and the current temperature. Tap this control to open the Weather page.

The touchscreen Weather page names the saved location in its header when a reverse-geocoded name is available. Until then, it shows the saved coordinates. The page shows:

- Current condition and temperature
- Feels-like temperature
- Relative humidity
- Wind direction, speed, and gusts
- The age of the cached report
- Three daily cards with condition, high, low, and maximum precipitation probability

Use **REFRESH** to request a new forecast. Use **RADAR** to open the regional precipitation-radar page. The radar page shows a fixed-scale regional map centered on the saved latitude and longitude, places a crosshair at the saved point, and overlays the latest available RainViewer precipitation frame. The background is assembled from native 256 × 256 OpenStreetMap raster tiles at the same Web-Mercator zoom, so roads, boundaries, coastlines, and labels remain sharp instead of enlarging the 320 × 240 world map. It is not an interactive map and does not pan, zoom, or animate in this alpha release. The radar page stays open until **CLOCK** is pressed; the normal one-minute touchscreen inactivity timeout does not close it.

The browser **Weather** page shows the resolved location name, saved coordinates, the same current conditions, and the three-day summary. It can schedule forecast and radar refreshes and can display the cached radar overlay. The browser image remains the transparent radar layer; the clock display combines it with cached OpenStreetMap raster tiles. Areas without precipitation remain transparent so the basemap remains visible.

Normal refresh intervals are:

- Forecast: every 30 minutes
- Radar: every 15 minutes after radar has first been requested

Opening the radar page requests a frame when none is cached or when the cached frame is stale. It also downloads only the small set of OpenStreetMap tiles needed by the current viewport when those files are missing. Basemap tiles are then reused from the microSD card and are not refreshed with every 15-minute radar update. The last successful forecast, radar image, and basemap remain available when the internet connection is unavailable. Failed downloads do not replace the last valid radar image. The Weather and Diagnostics pages display the cache age and retrieval errors.

Weather uses the saved coordinates exactly; it does not infer coordinates from the Wi-Fi network and it does not use a finger press on the world map. After a successful forecast, the firmware makes a low-frequency reverse-geocoding request to label those coordinates. The label is cached with the forecast, and coordinates remain the fallback if naming is unavailable. Change the point on the Settings or touchscreen Location page. Changing coordinates invalidates both weather caches. Changing units invalidates only the forecast values.

Forecast data is provided by [Open-Meteo](https://open-meteo.com/). Radar data is provided by [RainViewer](https://www.rainviewer.com/). The radar basemap and resolved location names use data from OpenStreetMap contributors. Source attribution appears on the device and browser weather pages. The standard OpenStreetMap raster service requires no API key, but it is a community-funded best-effort service: the firmware identifies itself with a World Clock User-Agent, requests only tiles currently needed by the radar viewport, and retains those tiles on the SD card rather than prefetching regions or repeatedly downloading the same background.

### Re-entering setup with the hardware button

To change the Wi-Fi network when the normal control panel is unavailable:

1. Allow the clock to finish starting.
2. Press and hold the board's **BOOT** button for about three seconds.
3. Release it when the display reports that settings are being cleared.
4. The clock restarts and creates the `WorldClock-XXXX` setup network.

This clears the saved Wi-Fi credentials and fixed UTC offset. Display, overlay, location, and selected-map preferences remain stored.

### Using the Maps page

The Maps page scans the microSD card's `/maps` folder for daylight PNG files. Each map card shows:

- A thumbnail streamed from the original PNG
- The filename
- Whether it is the selected map
- PNG validation status
- RGB565 cache status
- PNG and cache file sizes

Available controls include:

- **Select and apply** — selects a daylight map and builds its cache if needed.
- **Rebuild this cache** — deletes and recreates that map's RGB565 cache.
- **Validate every PNG with CRC** — performs a full validation of all listed PNG files.
- **Rebuild all daylight caches** — rebuilds caches for every valid daylight map.
- **Rebuild shared night cache** — rebuilds `/earth_night.rgb565`.
- **Rebuild every cache** — rebuilds all daylight caches and the shared night cache.

The selected daylight filename is stored in nonvolatile memory. If that file is later missing or invalid, the clock automatically chooses `earth_day.png` when available; otherwise it chooses the alphabetically first valid daylight PNG.

The page displays up to the first 20 daylight PNG files in alphabetical order.

### Adding a daylight map

1. Turn off the clock or safely remove the microSD card.
2. Copy the new daylight PNG into `/maps/`.
3. Reinsert the card and power on the clock.
4. Open the Maps page.
5. Confirm that the PNG is valid.
6. Select **Select and apply**, or rebuild its cache first.

A daylight map must be:

- Exactly 320 × 240 pixels
- Plate Carrée/equirectangular
- A complete 360° × 180° world map
- North-up
- Greenwich-centered, with the International Date Line at the left and right edges
- Full globe from the North Pole to the South Pole
- 8-bit-per-channel RGB PNG
- Non-interlaced
- Without transparency, borders, labels, or a pre-rendered day/night boundary

Every daylight map must align exactly with the shared `/earth_night.png`. See `Map requirements.txt` for the complete specification.

Do not copy `.rgb565` files from unrelated maps. The clock generates the correct cache files automatically.

### Diagnostics

The Diagnostics page reports information useful for troubleshooting, including:

- Active board profile and display rotation
- Firmware version and build time
- Uptime and reset reason
- Current time-zone settings
- Memory and flash usage
- Wi-Fi network, IP address, and signal strength
- NTP synchronization status
- microSD status
- Selected map and source/cache paths
- Day and night PNG/cache status
- Weather enable state, units, forecast/radar cache ages, pending refreshes, and retrieval errors
- ISS data and orbit-track status
- Last system error and error detail
- Touch hardware, calibration, rotation, state, and pressure-threshold status

The browser Diagnostics page includes **Recalibrate Touch**. Selecting it starts four-corner calibration on the World Clock display. Calibration changes only the touch record in the `touchtest` namespace; it does not erase Wi-Fi, maps, location, overlays, clock settings, or other World Clock preferences. Hold **BOOT** for three seconds to cancel. When an earlier calibration exists, cancelling retains it. When no calibration exists, cancelling leaves touch disabled.

Use the browser's **Refresh** link to update the values.

### Normal update behavior

- The clock display updates on wall-clock boundaries.
- With seconds hidden, the status line normally updates every 30 seconds.
- With seconds shown, the clock updates every second.
- The map, terminator, astronomy overlays, and ISS data normally update together every five minutes.
- Wi-Fi, NTP, and microSD failures are retried automatically.
- Weather forecasts refresh in the background every 30 minutes when weather and a saved location are available.
- After a radar image has been requested, stale radar is refreshed in the background every 15 minutes.
- Sun, Moon, and terminator positions are calculated by the clock.
- ISS marker and track features require internet access to retrieve a current ISS position.

### End-user troubleshooting

**The browser cannot open the control panel**

- Enter the complete address shown on the display, including `http://`.
- Make sure the browser device and clock are on the same Wi-Fi network.
- Do not substitute `https://`.
- The address may change after the router or clock restarts; use the currently displayed address.

**The setup page does not appear automatically**

- Stay connected to `WorldClock-XXXX`, even if the phone reports that the network has no internet access.
- Disable mobile-data switching temporarily if the phone immediately leaves the setup network.
- Manually open the IP address shown on the clock, normally `http://192.168.4.1`.

**The display says that the map is unavailable**

- Confirm that the microSD card is fully inserted.
- Confirm that `/earth_night.png` exists in the card root.
- Confirm that at least one valid daylight PNG exists in `/maps/`.
- Open the Maps page and rebuild invalid or missing caches.

**A newly added map is rejected**

- Confirm that it is exactly 320 × 240 pixels.
- Use a non-interlaced RGB PNG without transparency.
- Confirm that the file is inside `/maps/`, not the card root.
- Review `Map requirements.txt`.

**The time is wrong by an hour**

- Use the appropriate US time-zone preset when automatic daylight-saving-time changes are required.
- Use a fixed UTC offset only when a constant offset is intended.

**The ISS marker or track is unavailable**

- Confirm that the clock has internet access.
- Check the Diagnostics page for Wi-Fi and ISS status.
- The main clock and Sun/Moon display can continue even if the external ISS service is temporarily unavailable.

**The weather button does not appear**

- Confirm that a latitude and longitude have been applied and saved.
- Confirm that weather is enabled on the browser Settings page.
- The on-screen control is hidden when touch is disabled because it cannot be opened from the display; use the browser Weather page instead.

**Forecast or radar will not update**

- Confirm that Wi-Fi has internet access and working DNS.
- Open Diagnostics and review the Weather error and Radar error rows. Alpha4 reports the returned HTTP status or ESP32 transport error for failed radar-image requests instead of only displaying a generic download failure.
- Confirm that the microSD card is mounted. Forecasts can be retrieved without the card, but persistent forecast and radar caching require it.
- A failed request leaves the last successful cached data in place. Wait at least one minute before repeated retries.

## Section 2: Installing the Firmware on Your Own Device

### Supported target boards

The included board profiles support:

| `WORLDCLOCK_BOARD` value | Target hardware | Nominal flash |
|---|---|---:|
| `BOARD_HELTEC_WROOM_28` | Heltec-labeled ESP32-WROOM-32 2.8-inch display board | 8 MB |
| `BOARD_E32R28T` | E32R28T ESP32-32E 2.8-inch display board | 4 MB |
| `BOARD_ELEGOO_EL_EB_009` | Elegoo EL-EB-009 / ESP32-2432S028R CYD | 4 MB |

The profiles define the TFT pins, microSD pins, panel geometry, rotation, RGB/BGR order, display inversion, backlight polarity, BOOT/configuration-button pin, and nominal flash size.

Other ESP32 display boards may work after adding or modifying a profile in `board_profiles.h`, but they are not preconfigured.

### Hardware required

- One supported ESP32 display board
- A compatible USB data cable
- A microSD card, preferably formatted FAT32
- A 2.4 GHz Wi-Fi network with internet access for NTP, optional ISS data, and weather/radar retrieval
- A computer capable of running the Arduino IDE

Use a reliable power supply. Initial PNG-to-RGB565 cache generation performs sustained microSD reads and writes and should not be interrupted.

### Software required

Install:

1. Arduino IDE 2.x
2. The **esp32** board package by Espressif Systems through Arduino Boards Manager
3. **LovyanGFX** through Arduino Library Manager
4. **PNGdec** by Larry Bank/BitBank through Arduino Library Manager

The v5.1 development baseline supplied for this release uses **Arduino IDE 2.3.10** and **LovyanGFX 1.2.25**. Other compatible 2.x IDE and library versions may work, but these are the known project versions.

The following headers are supplied by the ESP32 Arduino core and should not normally be installed separately:

- `WiFi.h`
- `WebServer.h`
- `DNSServer.h`
- `Preferences.h`
- `HTTPClient.h`
- `WiFiClientSecure.h`
- `SPI.h`
- `SD.h`

### Project files

Keep all source files in the same Arduino sketch folder as `WorldClock_MultiBoard.ino`. The folder should include the numbered `.ino` tabs, `config.h`, `board_profiles.h`, `logo.h`, and the supplied documentation and images.

A typical source folder contains:

```text
WorldClock_MultiBoard/
├── WorldClock_MultiBoard.ino
├── 01_Hardware.ino
├── 02_SplashScreen.ino
├── 05_TouchHardware.ino
├── 06_TouchCalibration.ino
├── 10_Utilities.ino
├── 20_Storage_PNG.ino
├── 30_Map.ino
├── 35_RenderEngine.ino
├── 40_SunMoon.ino
├── 45_LocationGrid.ino
├── 50_ClockNetwork.ino
├── 55_SettingsController.ino
├── 56_NetworkController.cpp
├── 60_WiFiPortal.ino
├── 62_RuntimeWebConfig.ino
├── 65_ISS.ino
├── 66_OrbitTrack.ino
├── 67_Weather.ino
├── 70_TouchUI.ino
├── 90_Application.ino
├── XPT2046Soft.cpp
├── XPT2046Soft.h
├── board_profiles.h
├── config.h
└── logo.h
```

The numbered filenames are intentional and provide a predictable Arduino tab/build order.

### Select the target board profile

Open `config.h` and set exactly one `WORLDCLOCK_BOARD` value:

```cpp
#define WORLDCLOCK_BOARD BOARD_HELTEC_WROOM_28
```

or:

```cpp
#define WORLDCLOCK_BOARD BOARD_E32R28T
```

or:

```cpp
#define WORLDCLOCK_BOARD BOARD_ELEGOO_EL_EB_009
```

Also confirm that the firmware version is:

```cpp
static constexpr const char *FIRMWARE_VERSION =
  "5.1";
```

Selecting the wrong profile can produce a screen rotated by 90 degrees, reversed text, incorrect red/blue colors, or an inverted display. Correct the board selection before changing display-driver code.

### Arduino IDE board settings

Use these starting settings:

```text
Board: ESP32 Dev Module
CPU Frequency: 240 MHz
Upload Speed: a rate reliable for the connected board
```

Set **Flash Size** to match the physical board:

- Heltec WROOM profile: normally 8 MB
- E32R28T: normally 4 MB
- Elegoo EL-EB-009/CYD: normally 4 MB

Choose a partition scheme that provides enough application space for the compiled sketch. If compilation reports that the sketch is too large, select a partition scheme with a larger application partition rather than removing project files at random.

The serial monitor baud rate is:

```text
115200 baud
```

### Prepare the microSD card

The minimum recommended card layout is:

```text
/
├── earth_night.png
└── maps/
    └── earth_day.png
```

Additional daylight maps may be added:

```text
/
├── earth_night.png
└── maps/
    ├── earth_day.png
    ├── earth_day_political.png
    └── earth_day_relief_320x240.png
```

Do not create the cache files manually. The firmware generates files such as:

```text
/maps/earth_day.rgb565
/maps/earth_day_political.rgb565
/maps/earth_day_relief_320x240.rgb565
/earth_night.rgb565
/weather/forecast.bin
/weather/radar.png
/weather/radar.bin
/weather/osm_<zoom>_<x>_<y>.png
```

Each valid 320 × 240 RGB565 map cache is 153,600 bytes. The `/weather` directory and its files are created automatically after weather is used; they should not be preloaded or edited manually.

Older installations that contain `/earth_day.png` in the card root are migrated automatically to `/maps/earth_day.png` when possible.

### Map source requirements

Both daylight and nighttime sources must use identical geometry:

- Plate Carrée/equirectangular projection
- Longitude −180° at the left edge, 0° at the center, and +180° at the right edge
- Latitude +90° at the top, 0° at the middle, and −90° at the bottom
- Exactly 320 × 240 pixels
- North-up and not mirrored
- Greenwich at the horizontal center
- Full pole-to-pole coverage with no padding
- Non-interlaced 24-bit RGB PNG preferred

Although ordinary equirectangular maps have a 2:1 aspect ratio, this firmware requires the complete world image to be resized directly to 320 × 240 without preserving that aspect ratio and without adding top or bottom padding. The firmware's overlay calculations depend on every latitude and longitude corresponding to the expected pixel position.

The daylight source must show the whole planet in daylight with no terminator. The shared night source must show the same geography entirely at night. Coastlines, crop, poles, equator, and geographic center must align exactly or the blended terminator will show doubled coastlines.

See `Map requirements.txt` for the complete checklist.

### Compile and upload

1. Insert the prepared microSD card into the target board.
2. Connect the ESP32 board to the computer with a USB data cable.
3. Open `WorldClock_MultiBoard.ino` in Arduino IDE.
4. Select **ESP32 Dev Module** and the correct serial port.
5. Confirm the correct `WORLDCLOCK_BOARD` value in `config.h`.
6. Confirm the flash size and partition scheme.
7. Select **Sketch → Verify/Compile**.
8. Resolve any missing-library errors before uploading.
9. Select **Upload**.
10. If the board does not enter the bootloader automatically, hold **BOOT**, begin the upload, and release **BOOT** when the upload starts.
11. Open Serial Monitor at 115200 baud to observe startup, Wi-Fi, NTP, SD, map-cache, weather, radar, and web-server messages.
12. Reset or power-cycle the board after the upload if it does not restart automatically.

On first boot, the clock should display its splash screen, run integrated touch calibration when needed, and then start the `WorldClock-XXXX` setup network. Holding **BOOT** for three seconds during calibration disables touch and still allows Wi-Fi setup to continue.

### Board-profile notes

#### Heltec ESP32-WROOM-32 2.8-inch profile

The included profile uses:

- ILI9341 geometry of 240 × 320
- Rotation 3
- Normal RGB order
- Nominal 8 MB flash
- XPT2046 touch and integrated calibration verified on physical hardware

#### E32R28T profile

The included profile preserves the settings proven on this board:

- Panel declaration of 320 × 240
- Rotation 4
- Alternate RGB order
- Nominal 4 MB flash
- XPT2046 touch and integrated calibration verified on physical hardware

These unusual values are intentional.

#### Elegoo EL-EB-009 / ESP32-2432S028R CYD profile

The included profile starts with:

- Standard 240 × 320 ILI9341 geometry
- Rotation 1
- Normal RGB order
- Nominal 4 MB flash

Touch remains intentionally disabled for this profile until the controller wiring and calibration are verified on physical hardware.

Production revisions can differ. If the display is correctly landscape but upside down, change the profile rotation in `board_profiles.h` from `1` to `3`. If only red and blue are exchanged, change that profile's `rgbOrder` value.

### Installation troubleshooting

**The sketch does not compile**

- Confirm that the Espressif ESP32 board package is installed.
- Confirm that LovyanGFX and PNGdec are installed.
- Open the main `.ino` from a folder containing every project tab and header.
- Select an ESP32 board, not an AVR Arduino board.
- Use a partition scheme with sufficient application space.

**The upload does not start**

- Confirm that the USB cable supports data.
- Select the correct serial port.
- Install the USB-to-serial driver required by the board, commonly CH340/CH341 on these devices.
- Try the BOOT-button upload procedure.
- Reduce the upload speed if communication is unreliable.

**The screen is rotated, mirrored, inverted, or has incorrect colors**

- First confirm the `WORLDCLOCK_BOARD` selection in `config.h`.
- Do not compensate for a wrong board profile by changing the map images.
- Check the active profile's geometry, rotation, `rgbOrder`, and `invertDisplay` values in `board_profiles.h`.

**The microSD card does not mount**

- Format it as FAT32.
- Verify that it is fully inserted before power-up.
- Try a smaller or different microSD card.
- Confirm that the selected board profile has the correct SD SPI pins and chip-select pin.

**The map cache repeatedly fails**

- Validate both PNG files against `Map requirements.txt`.
- Remove stale `.rgb565` files and allow the firmware to regenerate them.
- Confirm that the card has free space and is writable.
- Watch the 115200-baud Serial Monitor for the failing filename and decoder message.

**Wi-Fi setup repeatedly returns after reboot**

- Confirm the SSID and password.
- ESP32 boards use 2.4 GHz Wi-Fi; a 5 GHz-only network will not work.
- Hidden networks can be entered manually in the setup form.
- Check Serial Monitor for connection-timeout messages.

**NTP or ISS data is unavailable**

- Confirm that the Wi-Fi network provides internet access and DNS.
- NTP is required to establish correct time after startup.
- The ISS overlay uses the external `wheretheiss.at` service and can be unavailable even when the rest of the clock works.

**Weather compilation or runtime problems**

- No ArduinoJson installation is required; v5.1 uses a small built-in parser and the HTTP/TLS classes supplied by the ESP32 Arduino core.
- Confirm that `HTTPClient.h` and `WiFiClientSecure.h` are available from the selected ESP32 board package.
- Confirm that the partition scheme still provides enough application space after adding `67_Weather.ino`.
- Persistent weather, radar, and basemap caching requires a writable microSD card; the firmware creates `/weather` automatically.
- Forecast retrieval uses HTTPS access to Open-Meteo. Radar metadata and imagery use HTTPS access to RainViewer. The regional basemap uses `tile.openstreetmap.org`, and optional location naming uses Nominatim at OpenStreetMap. Networks that block any of those hosts may still permit NTP and ordinary Wi-Fi operation while the corresponding weather feature fails.

### Extending the project

To support another board, add a `BoardProfile` entry in `board_profiles.h`, assign a new board identifier in `config.h`, and add the matching selection case for `ACTIVE_BOARD`. Keep application behavior separate from board-specific pins and display characteristics.

When changing display geometry or map dimensions, review all map-coordinate, overlay, cache-size, and status-layout constants together. The current renderer and PNG decoder explicitly assume a 320 × 240 source map and 16-bit RGB565 cache files.
