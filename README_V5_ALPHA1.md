# World Clock 5.0-alpha1 Touchscreen Integration

This package contains only the files that are new or changed from World Clock
4.7.1. Copy them into the existing `WorldClock_MultiBoard` sketch folder,
replacing files with the same names.

## Purpose of this alpha

This build integrates the touch hardware and finger-oriented event handling
proven by `ESP32_MultiBoard_TouchTest v0.2.2-alpha2`, without yet changing any
World Clock settings from the LCD.

It is intended to prove that all of these can operate together:

- World map rendering and scheduled redraws
- microSD access
- Wi-Fi and the runtime web server
- XPT2046 touch polling
- Full-screen touchscreen navigation
- Normal and 180-degree-flipped display orientations

## Prerequisite

Run and calibrate the standalone touch-test application on each physical board
before installing this alpha. Calibration and pressure data are read from the
same `touchtest` Preferences namespace, so the values normally survive a
standard Arduino upload.

If calibration is not found, the World Clock and browser interface continue to
work normally, but the touchscreen menu remains unavailable. The Serial
Monitor will report the missing calibration.

## Using the touchscreen

1. Let the normal World Clock screen appear.
2. Tap anywhere in the bottom status bar.
3. The main touch menu should open.
4. Test every menu button, Back, and Return to Clock.
5. Open **Diagnostics** to view raw coordinates, mapped coordinates, pressure,
   calibration source, Wi-Fi, SD, NTP, and selected-map status.
6. Use **PRESS -** or **PRESS +** to adjust the saved pressure threshold in
   steps of 20.
7. Leave a menu untouched for 60 seconds to test automatic return to the clock.

The Time/Display, Location, Overlays, Maps, and Network pages are placeholders
in this alpha. The existing browser interface remains fully functional.

## Board status

- `BOARD_HELTEC_WROOM_28`: touch verified
- `BOARD_E32R28T`: touch verified; default pressure threshold reduced to 60
- `BOARD_ELEGOO_EL_EB_009`: touch disabled until hardware-tested

## Test checklist

For both verified boards, test:

- Opening the menu from the status bar
- Every main-menu button
- Back and Clock buttons
- Diagnostics values while touching and after release
- Pressure adjustment
- Automatic 60-second timeout
- Browser access while a touch page is open
- SD/map operation after returning to the clock
- Normal display orientation
- 180-degree-flipped display orientation
- Long operation without display corruption or freezes

## Files in this patch

Changed:

- `WorldClock_MultiBoard.ino`
- `board_profiles.h`
- `config.h`
- `10_Utilities.ino`
- `35_RenderEngine.ino`
- `90_Application.ino`

New:

- `XPT2046Soft.h`
- `XPT2046Soft.cpp`
- `05_TouchHardware.ino`
- `70_TouchUI.ino`
- `README_V5_ALPHA1.md`
