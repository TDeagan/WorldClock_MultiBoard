#include "config.h"

// World Clock v5.1-rc2 — saved-location weather and regional radar.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
