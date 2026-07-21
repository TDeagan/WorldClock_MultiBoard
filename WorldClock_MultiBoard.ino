#include "config.h"

// World Clock v5.3-alpha4 — weather, Market Mood, and per-board display tuning.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
