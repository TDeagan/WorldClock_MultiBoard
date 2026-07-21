#include "config.h"

// World Clock v5.2-alpha2 — per-board display tuning and calibration patterns.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
