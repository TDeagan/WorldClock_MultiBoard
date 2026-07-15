World Clock v4.6 patch

Starting point:
  Working v4.5 firmware

Replace:
  config.h
  01_Hardware.ino
  10_Utilities.ino
  35_RenderEngine.ino
  60_WiFiPortal.ino
  62_RuntimeWebConfig.ino
  90_Application.ino

Add as a new Arduino tab:
  45_LocationGrid.ino

Reviewed but unchanged:
  30_Map.ino

New v4.6 features:
  - Persistent home latitude and longitude
  - Optional magenta home-location target marker
  - Optional 30-degree latitude/longitude grid
  - Emphasized Equator and Prime Meridian
  - Location and grid controls in first-time setup
  - Location and grid controls in the normal web Settings page
  - Active-settings summary entries
  - Diagnostics entries for marker, latitude, longitude, and grid
  - Immediate full redraw when the settings are applied
  - Startup Serial report for the stored location/grid settings

Defaults after upgrading:
  - Home marker: off
  - Coordinate grid: off
  - Home latitude: 0
  - Home longitude: 0

Layer order:
  1. Earth map and terminator
  2. Coordinate grid
  3. ISS orbit track
  4. Sun
  5. Moon
  6. ISS marker
  7. Home-location marker
  8. Status bar

The home marker is drawn last among map overlays so it remains visible.
Coordinates use decimal degrees:
  Latitude:  -90 through +90
  Longitude: -180 through +180

No brightness, PWM, backlight-level, or scheduled-dimming changes are
included in this release.
