# World Clock 5.0-alpha3 Touchscreen Maps

This patch is applied to the tested World Clock 5.0-alpha2 sketch. Copy the files into the existing `WorldClock_MultiBoard` folder and replace matching files.

## Added in this release

The touchscreen Maps page now provides:

- An alphabetically sorted daylight-map list from `/maps`
- Three large map rows per page with previous/next paging
- Active-map and staged-selection indicators
- PNG validity and same-name RGB565 cache status
- Preview rendering directly from the original daylight PNG
- Select and apply controls
- A maintenance page with quick rescan and full PNG validation
- Rebuild selected daylight cache
- Rebuild shared `/earth_night.rgb565` cache
- Rebuild every valid daylight cache plus the shared night cache
- Automatic selected-map fallback reporting when the saved map is missing

Applying a map creates its RGB565 cache when necessary, saves the selected filename, returns to the clock, and redraws with the new map.

## Touchscreen workflow

1. Tap the bottom status bar.
2. Tap **MAPS**.
3. Tap a map row to stage it.
4. Use **PREVIEW** to inspect the source PNG.
5. Tap **APPLY** to select it.
6. Use **TOOLS** for validation and cache maintenance.

The browser Maps page remains fully functional and uses the same selected-map Preference and cache files.

## Files changed

- `config.h`
- `20_Storage_PNG.ino`
- `70_TouchUI.ino`
- `WorldClock_MultiBoard.ino`
- `README.md`
- `README_V5_ALPHA3.md`

## Test checklist

Test on both verified boards:

1. Open Maps and verify all files under `/maps` appear alphabetically.
2. Page forward and backward when more than three maps exist.
3. Stage different maps and verify the yellow selection outline moves.
4. Preview each map and verify orientation and colors.
5. Apply a map with an existing cache.
6. Remove one daylight cache, apply that map, and verify the cache is generated.
7. Rebuild the selected daylight cache.
8. Rebuild the shared night cache.
9. Rebuild all caches.
10. Remove the selected daylight PNG, restart or rescan, and verify automatic fallback.
11. Confirm the browser Maps page reflects touchscreen selections.
12. Confirm the clock, web server, SD card, and touch controls remain responsive afterward.
