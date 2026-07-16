#include "config.h"

// ============================================================
// Shared runtime Wi-Fi settings controller
// ============================================================
//
// The browser setup portal and touchscreen network page use the same saved
// NetworkSettings structure and Preferences keys. The touchscreen tests new
// credentials before saving them and restores the previous connection when a
// test fails.
// ============================================================

namespace {

bool waitForTouchNetworkConnection(
  unsigned long timeoutMs
) {
  const unsigned long startedAt =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startedAt < timeoutMs
  ) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}

void restorePreviousTouchNetwork(
  const NetworkSettings &previous,
  bool keepSoftAp
) {
  WiFi.mode(
    keepSoftAp
      ? WIFI_AP_STA
      : WIFI_STA
  );

  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.disconnect(false, false);
  delay(100);

  if (
    !previous.configured ||
    previous.ssid.length() == 0
  ) {
    return;
  }

  WiFi.begin(
    previous.ssid.c_str(),
    previous.password.c_str()
  );

  waitForTouchNetworkConnection(
    8000UL
  );
}
} // namespace


bool applyTouchNetworkSettings(
  const String &requestedSsid,
  const String &requestedPassword,
  String &messageOut
) {
  String ssid = requestedSsid;
  ssid.trim();

  if (
    ssid.length() == 0 ||
    ssid.length() > 32 ||
    requestedPassword.length() > 64
  ) {
    messageOut =
      "SSID must be 1-32 characters; password maximum is 64";
    return false;
  }

  const NetworkSettings previous =
    networkSettings;

  const wifi_mode_t startingMode =
  WiFi.getMode();

  const bool keepSoftAp =
    startingMode == WIFI_AP ||
    startingMode == WIFI_AP_STA;

  WiFi.mode(
  keepSoftAp
    ? WIFI_AP_STA
    : WIFI_STA
  );
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  WiFi.disconnect(false, false);
  delay(120);

  WiFi.begin(
    ssid.c_str(),
    requestedPassword.c_str()
  );

  if (
    !waitForTouchNetworkConnection(
      TOUCH_NETWORK_CONNECT_TIMEOUT_MS
    )
  ) {
    messageOut =
      "Connection failed; restoring previous Wi-Fi";

    setSystemError(
      SystemError::WifiDisconnected,
      "Touchscreen Wi-Fi test failed"
    );

    restorePreviousTouchNetwork(
      previous,
      keepSoftAp
    );

    WiFi.setAutoReconnect(true);
    systemStatus.wifiConnected =
      WiFi.status() == WL_CONNECTED;

    return false;
  }

  if (
    !saveNetworkSettings(
      ssid,
      requestedPassword,
      networkSettings.utcOffsetMinutes
    )
  ) {
    messageOut =
      "Connected, but credentials could not be saved";

    WiFi.disconnect(false, false);

    restorePreviousTouchNetwork(
      previous,
      keepSoftAp
    );
    WiFi.setAutoReconnect(true);
    systemStatus.wifiConnected =
      WiFi.status() == WL_CONNECTED;

    return false;
  }

  WiFi.setAutoReconnect(true);
  systemStatus.wifiConnected = true;

  if (
    systemStatus.lastError ==
      SystemError::WifiDisconnected
  ) {
    systemStatus.lastError =
      SystemError::None;
    systemStatus.lastErrorText = "";
  }

  if (!timeValid) {
    timeValid = retryNtpSync();
  }

  messageOut =
    String("Connected: http://") +
    WiFi.localIP().toString();

  return true;
}


bool clearSavedWifiCredentials() {
  Preferences preferences;

  if (
    !preferences.begin(
      PREF_NAMESPACE,
      false
    )
  ) {
    return false;
  }

  const bool ssidRemoved =
    preferences.remove(
      PREF_KEY_SSID
    );

  const bool passwordRemoved =
    preferences.remove(
      PREF_KEY_PASSWORD
    );

  const bool configuredRemoved =
    preferences.remove(
      PREF_KEY_CONFIGURED
    );

  preferences.end();

  networkSettings.ssid = "";
  networkSettings.password = "";
  networkSettings.configured = false;

  // A key may already be absent, so the Preferences remove() return values are
  // diagnostic rather than a requirement for success. Re-open read-only and
  // verify that the configured flag is gone.
  Preferences verify;

  if (
    !verify.begin(
      PREF_NAMESPACE,
      true
    )
  ) {
    return false;
  }

  const bool stillConfigured =
    verify.getBool(
      PREF_KEY_CONFIGURED,
      false
    );

  verify.end();

  Serial.printf(
    "Touch Wi-Fi forget: ssid=%d password=%d configured=%d verified=%d\n",
    ssidRemoved,
    passwordRemoved,
    configuredRemoved,
    !stillConfigured
  );

  return !stillConfigured;
}
