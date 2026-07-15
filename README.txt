World Clock v4.7 patch

Starting point:
  Working v4.6 firmware

New map-library features:
  - Scans /maps for daylight .png files
  - Uses one shared /earth_night.png
  - Creates /maps/<day-map-name>.rgb565 for each daylight map
  - Stores the selected daylight filename in Preferences
  - Falls back to earth_day.png, then the first valid map, if selected file is missing
  - Streams thumbnail previews from the original PNG files
  - Adds per-map Select and apply controls
  - Shows PNG validation and RGB565 cache status for each map
  - Adds per-map, all-daylight, shared-night, and all-cache rebuild controls
  - Migrates legacy /earth_day.png into /maps/earth_day.png when needed

Location-entry changes:
  - Latitude selector: North (+) or South (-)
  - Longitude selector: East (+) or West (-)
  - Magnitude-only numeric fields optimized for mobile decimal keyboards
  - Same controls in first-time setup and normal runtime Settings

SD-card layout:
  /maps/earth_day.png
  /maps/earth_day_political.png
  /earth_night.png

Generated automatically:
  /maps/earth_day.rgb565
  /maps/earth_day_political.rgb565
  /earth_night.rgb565

All map PNGs must be non-interlaced 320 x 240 equirectangular images.
