#include "config.h"

// World Clock v5.4-alpha5 — LOOP frame timeline, Market Mood, and display tuning.
// Select the target board with WORLDCLOCK_BOARD in config.h.

void setup() {
  initializeWorldClock();
}

void loop() {
  serviceWorldClock();
}
