# ESP32 World Clock v5.0-alpha3

ESP32 World Clock displays a live day/night world map on a 320 × 240 ILI9341 screen. It can show local and UTC time, the Sun, the Moon and its phase, the current International Space Station position, an approximate one-orbit ISS ground track, a home-location marker, and a latitude/longitude grid.

The clock can be configured from its resistive touchscreen or from a phone, tablet, or computer through its built-in web interface. Daylight maps are stored on a microSD card and can be previewed, validated, selected, and cached without recompiling the firmware.

## 1. Using an Installed World Clock

### What appears on the display

The main display contains:

- A world map blended between daylight and nighttime imagery according to the current position of the Sun.
- Optional Sun, Moon, ISS, ISS orbit-track, home-location, and coordinate-grid overlays.
- A bottom status area showing local time, UTC time, and the local date.
- The clock's web address, such as `http://192.168.1.42`, at the bottom center.

The address is available only while the clock is connected to Wi-Fi. Open it from a device connected to the same local network.

### First-time setup

When the clock has no saved Wi-Fi configuration, it starts its own temporary setup network.

1. Power on the clock with the microSD card installed.
2. On a phone, tablet, or computer, open the Wi-Fi settings.
3. Connect to the open network named `WorldClock-XXXX`, where `XXXX` is unique to the clock.
4. The World Clock Setup page should open automatically. If it does not, open a browser and enter the address shown on the clock display. This is normally `http://192.168.4.1`.
5. Select a detected Wi-Fi network, or type the name of a hidden network manually.
6. Enter the Wi-Fi password. Leave it blank only for an open network.
7. Select a time zone:
   - Fixed UTC offset
   - UTC
   - US Eastern
   - US Central
   - US Mountain
   - US Pacific
8. When using **Fixed UTC offset**, select `+` or `−` and enter the offset magnitude in hours. Quarter-hour offsets are supported.
9. Choose 12-hour or 24-hour time and whether seconds are shown.
10. Enter the home location in decimal degrees:
    - Choose North or South and enter a latitude magnitude from 0 through 90.
    - Choose East or West and enter a longitude magnitude from 0 through 180.
11. Enable any desired map overlays.
12. Select **Save and connect**.

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
- Touch, Wi-Fi, SD-card, NTP, and selected-map diagnostics

Settings changed on the touchscreen use the same saved Preferences values as the browser controls. The menu returns to the clock automatically after one minute without activity.

### Opening the control panel later

While the clock is operating normally:

1. Read the `http://` address displayed at the bottom center of the screen.
2. Connect the phone, tablet, or computer to the same Wi-Fi network as the clock.
3. Enter the complete address in a browser, including `http://`.

The control panel has three pages: **Settings**, **Diagnostics**, and **Maps**.

#### Settings

The Settings page shows the active configuration and permits changes to:

- Time zone or fixed UTC offset
- 12-hour or 24-hour clock format
- Seconds display
- Normal or 180-degree display orientation
- Home latitude and longitude
- Home-location marker
- 30-degree coordinate grid
- Sun marker
- Moon marker and phase
- ISS marker
- One-orbit ISS ground track
- Solid or dotted ISS track

Select **Apply settings now** after making changes. Most changes are applied immediately without rebooting.

The **Forget Wi-Fi and start setup** control erases the saved network credentials and restarts the clock in setup mode. It asks for confirmation before doing so.

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
- ISS data and orbit-track status
- Last system error and error detail

Use the browser's **Refresh** link to update the values.

### Normal update behavior

- The clock display updates on wall-clock boundaries.
- With seconds hidden, the status line normally updates every 30 seconds.
- With seconds shown, the clock updates every second.
- The map, terminator, astronomy overlays, and ISS data normally update together every five minutes.
- Wi-Fi, NTP, and microSD failures are retried automatically.
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

## 2. Installing the Firmware on Your Own Device

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
- A 2.4 GHz Wi-Fi network with internet access for NTP and optional ISS data
- A computer capable of running the Arduino IDE

Use a reliable power supply. Initial PNG-to-RGB565 cache generation performs sustained microSD reads and writes and should not be interrupted.

### Software required

Install:

1. Arduino IDE 2.x
2. The **esp32** board package by Espressif Systems through Arduino Boards Manager
3. **LovyanGFX** through Arduino Library Manager
4. **PNGdec** by Larry Bank/BitBank through Arduino Library Manager

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
├── 10_Utilities.ino
├── 20_Storage_PNG.ino
├── 30_Map.ino
├── 35_RenderEngine.ino
├── 40_SunMoon.ino
├── 45_LocationGrid.ino
├── 50_ClockNetwork.ino
├── 60_WiFiPortal.ino
├── 62_RuntimeWebConfig.ino
├── 65_ISS.ino
├── 66_OrbitTrack.ino
├── 90_Application.ino
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
  "4.7.1";
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
```

Each valid 320 × 240 RGB565 cache is 153,600 bytes.

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
11. Open Serial Monitor at 115200 baud to observe startup, Wi-Fi, NTP, SD, map-cache, and web-server messages.
12. Reset or power-cycle the board after the upload if it does not restart automatically.

On first boot, the clock should display its splash screen and then start the `WorldClock-XXXX` setup network.

### Board-profile notes

#### Heltec ESP32-WROOM-32 2.8-inch profile

The included profile uses:

- ILI9341 geometry of 240 × 320
- Rotation 3
- Normal RGB order
- Nominal 8 MB flash

#### E32R28T profile

The included profile preserves the settings proven on this board:

- Panel declaration of 320 × 240
- Rotation 4
- Alternate RGB order
- Nominal 4 MB flash

These unusual values are intentional.

#### Elegoo EL-EB-009 / ESP32-2432S028R CYD profile

The included profile starts with:

- Standard 240 × 320 ILI9341 geometry
- Rotation 1
- Normal RGB order
- Nominal 4 MB flash

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

### Extending the project

To support another board, add a `BoardProfile` entry in `board_profiles.h`, assign a new board identifier in `config.h`, and add the matching selection case for `ACTIVE_BOARD`. Keep application behavior separate from board-specific pins and display characteristics.

When changing display geometry or map dimensions, review all map-coordinate, overlay, cache-size, and status-layout constants together. The current renderer and PNG decoder explicitly assume a 320 × 240 source map and 16-bit RGB565 cache files.
