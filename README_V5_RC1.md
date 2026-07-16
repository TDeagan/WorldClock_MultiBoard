# World Clock 5.0-rc1 Release Candidate

World Clock 5.0-rc1 is based on the confirmed-working 5.0-alpha5 project. It is intended as the final stabilization build before 5.0.

## Changes from 5.0-alpha5

- Adds a BOOT-button escape from integrated touchscreen calibration.
- Holding BOOT for three seconds when no calibration exists stores a board-specific touch-disabled flag in the existing `touchtest` Preferences namespace.
- After BOOT is released, startup continues automatically into the existing captive-portal Wi-Fi setup.
- Touch remains disabled on later boots until calibration succeeds.
- Adds **Recalibrate Touch** to the browser Diagnostics page so a clock with disabled touch can recover without using the touchscreen.
- Cancelling recalibration when a valid calibration already exists retains the previous calibration.
- Successful calibration clears the touch-disabled flag and saves the existing `TouchCalibration` record format.
- Wi-Fi credentials and every other World Clock preference remain untouched by calibration, cancellation, or touch disablement.
- Firmware version is `5.0-rc1`.

## Verified boards

- `BOARD_HELTEC_WROOM_28` — touchscreen and World Clock operation tested.
- `BOARD_E32R28T` — touchscreen and World Clock operation tested.
- `BOARD_ELEGOO_EL_EB_009` — touch remains intentionally disabled until hardware-tested.

## Recovery procedure

1. During first-boot calibration, hold the physical **BOOT** button for three seconds.
2. Release BOOT when the display asks.
3. Complete captive-portal Wi-Fi setup normally.
4. Open the World Clock web address on the configured network.
5. Open **Diagnostics** and select **Recalibrate Touch**.
6. Complete the four corner targets and center verification on the display.

## Release-candidate acceptance checks

- Erased NVS: calibration runs before captive-portal setup.
- BOOT bypass: touch is disabled, Wi-Fi setup continues, and the disabled state survives reboot.
- Browser Diagnostics: recalibration can be launched while touch is disabled.
- Successful recalibration: touch is re-enabled and remains calibrated after reboot.
- Existing calibration: cancelling recalibration retains the old calibration.
- Existing installation upgrade: Wi-Fi, maps, location, overlays, display, and clock settings remain unchanged.
- Both verified board profiles compile and operate with their correct `WORLDCLOCK_BOARD` selection.

The project continues to use `56_NetworkController.cpp`. Do not restore the obsolete `56_NetworkController.ino`.
