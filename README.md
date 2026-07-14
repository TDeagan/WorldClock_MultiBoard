# World Clock Multi-Board v4.0 — Conservative Port

This package is built directly from the uploaded, working v3.4 project.

Its only functional purpose is board independence. It deliberately preserves:

- all update schedules;
- captive-portal behavior;
- runtime web configuration;
- diagnostics and map maintenance;
- SD map caching;
- Sun, Moon, ISS, and orbit-track behavior;
- `CONFIG_BUTTON_HOLD_MS`;
- application state and redraw flow.

## Select a board

Change one line in `config.h`:

```cpp
#define WORLDCLOCK_BOARD BOARD_HELTEC_WROOM_28
```

Supported selections:

```cpp
BOARD_HELTEC_WROOM_28
BOARD_E32R28T
BOARD_ELEGOO_EL_EB_009
```

## Board profiles

All board-specific values are in `board_profiles.h`:

- TFT pins;
- SD pins;
- panel native geometry;
- display rotation;
- RGB/BGR order;
- display inversion;
- backlight pin and active level;
- configuration-button pin;
- nominal flash size.

The application continues to use its existing constant names through aliases in
`config.h`. This minimizes the code diff and avoids changing behavior.

## Arduino IDE

Use:

```text
Board: ESP32 Dev Module
CPU Frequency: 240 MHz
```

Set Flash Size to match the physical board:

- Heltec WROOM profile: normally 8 MB
- E32R28T: normally 4 MB
- Elegoo EL-EB-009 CYD: normally 4 MB

Use a partition scheme with enough application space for the compiled sketch.

## Profile notes

### Heltec WROOM

The default profile exactly reproduces the working v3.4 settings:

- rotation 3;
- standard 240×320 ILI9341 geometry;
- `rgb_order = false`.

### E32R28T

Preserves the proven unusual settings:

- rotation 4;
- 320×240 panel declaration;
- `rgb_order = true`.

### Elegoo EL-EB-009 CYD

Starts with standard CYD settings:

- rotation 1;
- standard 240×320 geometry;
- `rgb_order = false`.

If the Elegoo display is landscape but upside down, change its rotation in
`board_profiles.h` from `1` to `3`. If only red and blue are exchanged, change
its `rgbOrder`.

## Future v4.1 work

The included `deagan_logo_128.png` is an unused future asset. It is not shown by
v4.0. The planned v4.1 changes are:

- centered startup logo for three seconds;
- one unified five-minute map/terminator/Sun/Moon/ISS update schedule;
- clock/status updates kept separate.

Those changes are intentionally deferred so v4.0 remains a pure board port.
