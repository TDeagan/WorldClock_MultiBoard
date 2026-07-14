World Clock v4.2 unified-scheduler patch

Replace these four files in the working v4.1 project:

  config.h
  30_Map.ino
  50_ClockNetwork.ino
  90_Application.ino

Behavior:

  Clock/status update:
    every 30 seconds, aligned to :00 and :30

  Full astronomy update:
    every 5 minutes, aligned to :00, :05, :10, etc.

Each full update performs one coordinated operation:

  1. Fetch current ISS position, when ISS or track is enabled
  2. Recalculate ISS track
  3. Recalculate and draw the terminator
  4. Draw ISS track
  5. Recalculate and draw Sun and Moon
  6. Draw ISS marker
  7. Draw clock and IP address

The prior independent 20-second ISS and 60-second map redraw schedules are no
longer used by the application loop.

Wi-Fi, NTP, and storage recovery retries remain on their original schedules.
This allows failures to recover promptly without waiting for a five-minute
display boundary.
