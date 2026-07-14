#include "config.h"

// World Clock v4.0 — behavior-preserving multi-board build.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
