#include "config.h"

// World Clock v5.0-alpha4 — touchscreen Wi-Fi and keyboard integration.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
