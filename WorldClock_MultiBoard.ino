#include "config.h"

// World Clock v5.0-alpha3 — touchscreen maps and settings integration.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
