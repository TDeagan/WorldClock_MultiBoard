# World Clock 5.0-alpha2 Touchscreen Settings

This package contains only files that are new or changed from the working
World Clock 5.0-alpha1 touchscreen build. Copy them into the existing
`WorldClock_MultiBoard` sketch folder and replace files with matching names.

## What this alpha adds

The touchscreen can now edit and persist the first groups of World Clock
settings. The browser interface remains available and displays the same saved
values because both interfaces use the same application state and Preferences
keys.

### Time / Display

- Cycle through Fixed UTC offset, UTC, US Eastern, US Central, US Mountain,
  and US Pacific.
- Adjust the fixed UTC offset in 15-minute increments from UTC-12:00 through
  UTC+14:00.
- Select 12-hour or 24-hour clock display.
- Show or hide seconds.
- Select normal or 180-degree-flipped display orientation.
- APPLY saves the staged values and returns to the clock.
- CANCEL discards staged changes and returns to the touch menu.

The fixed-offset value remains editable even while a named time-zone preset is
selected. It is retained for the next time Fixed UTC offset is selected.

### Location

- Adjust latitude magnitude from 0.0 through 90.0 degrees.
- Select North or South independently of the magnitude.
- Adjust longitude magnitude from 0.0 through 180.0 degrees.
- Select East or West independently of the magnitude.
- Cycle the adjustment step through 0.1, 1, and 10 degrees.
- Show or hide the home-location marker.
- Show or hide the coordinate grid.
- APPLY saves the staged values and returns to the clock.
- CANCEL discards staged changes and returns to the touch menu.

### Overlays

- Show or hide the Sun.
- Show or hide the Moon.
- Show or hide the ISS marker.
- Show or hide the ISS orbit track.
- Select a solid or dotted orbit track.
- APPLY saves the staged values and returns to the clock.
- CANCEL discards staged changes and returns to the touch menu.

Enabling the ISS marker or orbit track may briefly delay the return to the
clock while the current ISS position is fetched.

## Shared settings controller

`55_SettingsController.ino` contains the shared validation and persistence
entry points used by the touchscreen. The runtime web page now uses the same
UTC-offset persistence helper. Later alphas can move more browser operations
through this controller without changing the stored settings format.

## Still placeholders

The touchscreen Maps and Network pages remain placeholders in this alpha.
Their complete browser equivalents continue to work normally.

## Test checklist

Test on both verified boards:

1. Open the touch menu by tapping the status bar.
2. Change every Time / Display option and press CANCEL. Verify nothing changes.
3. Change every Time / Display option and press APPLY. Verify the clock and
   browser page show the new values.
4. Flip the display from the touchscreen, then verify touch coordinates remain
   correct and the menu can be opened again.
5. Restart the board and verify Time / Display values persist.
6. Change latitude, longitude, hemispheres, adjustment step, home marker, and
   grid. Test both CANCEL and APPLY.
7. Verify the home marker and coordinate grid move or appear as requested.
8. Restart and verify location settings persist.
9. Toggle each overlay and track style. Test both CANCEL and APPLY.
10. Verify ISS fetching and track generation do not freeze touch, Wi-Fi, or SD
    operation.
11. Open the browser configuration page after touchscreen changes and confirm
    it reports the same settings.
12. Change settings in the browser, reopen the corresponding touch page, and
    confirm it starts with the browser-selected values.
13. Leave each settings page untouched for 60 seconds and verify it returns to
    the clock without applying staged changes.

## Board status

- `BOARD_HELTEC_WROOM_28`: touchscreen verified
- `BOARD_E32R28T`: touchscreen verified; requires firmer physical pressure
- `BOARD_ELEGOO_EL_EB_009`: touch remains disabled until hardware-tested

## Files in this patch

Changed:

- `config.h`
- `62_RuntimeWebConfig.ino`
- `70_TouchUI.ino`

New:

- `55_SettingsController.ino`
- `README_V5_ALPHA2.md`
