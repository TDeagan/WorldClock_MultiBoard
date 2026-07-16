#include "config.h"

// World Clock v5.0-alpha5 — integrated touchscreen calibration.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
