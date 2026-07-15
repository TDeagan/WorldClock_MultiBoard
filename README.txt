World Clock v4.3 rendering-engine patch

Replace these existing files in the working v4.2 project:

  config.h
  30_Map.ino
  40_SunMoon.ino
  65_ISS.ino
  90_Application.ino

Add this new Arduino tab:

  35_RenderEngine.ino

What changed:

  - Firmware version is now 4.3.
  - One module owns the complete full-display rendering order.
  - 90_Application.ino no longer calls individual map/overlay functions.
  - The Sun and Moon have separate overlay renderers.
  - ISS track and marker have rendering wrappers.
  - Status-only clock updates call renderStatusBar().
  - redrawWorldClock() remains as a compatibility entry point for the
    runtime web configuration, map maintenance, and recovery code.
  - A RenderState structure records which layers completed during the
    most recent full redraw.

Rendering order:

  1. Earth map and terminator
  2. ISS orbit track
  3. Sun
  4. Moon
  5. ISS marker
  6. Clock/status bar and IP address

No changes were made to:

  - the v4.2 unified scheduler;
  - astronomy calculations;
  - ISS retrieval or orbit calculations;
  - Wi-Fi or captive portal behavior;
  - SD cache behavior;
  - diagnostics or map maintenance;
  - board profiles;
  - splash screen.
