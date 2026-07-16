# World Clock 5.0-alpha4 Touchscreen Network and Keyboard

Apply this patch to the tested World Clock 5.0-alpha3 sketch. Copy the files into the existing `WorldClock_MultiBoard` folder and replace matching files.

## Added in this release

The touchscreen Network page now provides:

- A scan of nearby visible Wi-Fi networks
- Three large network rows per page with previous/next paging
- Signal strength and open/secured status
- Connected-network and saved-network indicators
- Manual entry for hidden SSIDs
- Password entry using a reusable on-screen keyboard
- Connection testing before credentials are saved
- Automatic restoration of the previous network after a failed test
- Confirmed removal of saved Wi-Fi credentials
- Automatic restart into the captive setup portal after credentials are removed

The keyboard follows the interaction model requested from the Zoom R4:

- A grid of selectable character and command cells
- One clearly highlighted selected cell
- Four persistent controls at the bottom: **CANCEL**, **<**, **>**, and a check mark
- Touching a cell moves the selection without entering it
- The check mark activates the selected cell
- Selection wraps between the first and last cells
- Uppercase, lowercase, numbers/symbols, space, delete, show/hide, clear, and Enter functions

## Network workflow

1. Tap the bottom status bar.
2. Tap **NETWORK**.
3. Select a scanned SSID, or choose **OTHER SSID** for a hidden network.
4. Choose **PASSWORD** and enter the password.
5. Select the keyboard **ENTER** cell and press the check mark.
6. Tap **CONNECT**.
7. The new credentials are saved only after the ESP32 successfully connects.

Forgetting Wi-Fi requires two deliberate presses: **FORGET**, followed by **CONFIRM**. The fixed UTC offset and other clock preferences remain saved.

## Files changed or added

- `config.h`
- `56_NetworkController.ino` — new
- `70_TouchUI.ino`
- `WorldClock_MultiBoard.ino`
- `README.md`
- `README_V5_ALPHA4.md` — new

## Test checklist

Test on both touch-verified boards:

1. Open Network and verify scanning completes while the clock is already connected.
2. Page through more than three visible networks.
3. Select the current saved network and confirm its password is retained.
4. Select a different secured network and confirm the password draft is cleared.
5. Enter an SSID and password with direct cell selection plus the bottom left/right/check controls.
6. Verify uppercase, lowercase, numbers/symbols, space, delete, clear, show/hide, and Enter.
7. Connect with correct credentials and confirm the displayed IP address changes when appropriate.
8. Confirm the browser control page is reachable on the new network.
9. Attempt a connection with incorrect credentials and confirm the previous network is restored.
10. Test an open network, if available.
11. Test a hidden SSID, if available.
12. Press Forget once and confirm no settings are erased yet.
13. Press Confirm and verify restart into the `WorldClock-XXXX` setup network.
14. Confirm time, display, location, overlay, and selected-map settings remain intact.
15. Confirm map rendering, SD access, touch operation, and web-server operation remain responsive.
