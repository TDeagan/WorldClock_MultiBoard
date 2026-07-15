// ============================================================
// First-boot Wi-Fi and UTC-offset captive portal
// ============================================================

namespace {
DNSServer portalDns;
WebServer portalServer(80);
Preferences portalPreferences;

bool portalSaveComplete = false;
unsigned long portalRestartAt = 0;
String portalApSsid;
String portalNetworkOptions;
unsigned long configurationButtonPressedAt = 0;
bool configurationButtonTriggered = false;

String htmlEscape(const String &input) {
  String output;
  output.reserve(input.length() + 16);

  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];

    switch (c) {
      case '&': output += F("&amp;");  break;
      case '<': output += F("&lt;");   break;
      case '>': output += F("&gt;");   break;
      case '"': output += F("&quot;"); break;
      case '\'': output += F("&#39;"); break;
      default: output += c; break;
    }
  }

  return output;
}

String offsetSignText(int offsetMinutes) {
  return offsetMinutes < 0 ? "-" : "+";
}

String offsetMagnitudeText(int offsetMinutes) {
  const int absoluteMinutes = abs(offsetMinutes);
  const int hours = absoluteMinutes / 60;
  const int minutes = absoluteMinutes % 60;

  String value = String(hours);

  if (minutes != 0) {
    value += ".";
    value += String((minutes * 100) / 60);

    while (value.endsWith("0")) {
      value.remove(value.length() - 1);
    }
  }

  return value;
}

String offsetValueText(int offsetMinutes) {
  const bool negative = offsetMinutes < 0;
  int absoluteMinutes = abs(offsetMinutes);
  const int hours = absoluteMinutes / 60;
  const int minutes = absoluteMinutes % 60;

  String value = negative ? "-" : "";
  value += String(hours);

  if (minutes != 0) {
    value += ".";
    value += String(
      (minutes * 100) / 60
    );

    while (
      value.endsWith("0")
    ) {
      value.remove(value.length() - 1);
    }
  }

  return value;
}

bool parseUtcOffset(
  const String &text,
  int &minutesOut
) {
  String cleaned = text;
  cleaned.trim();

  if (cleaned.length() == 0) {
    return false;
  }

  char *endPointer = nullptr;
  const double hours =
    strtod(
      cleaned.c_str(),
      &endPointer
    );

  if (
    endPointer == cleaned.c_str() ||
    *endPointer != '\0' ||
    !isfinite(hours)
  ) {
    return false;
  }

  const long minutes =
    lround(hours * 60.0);

  if (
    minutes < MIN_UTC_OFFSET_MINUTES ||
    minutes > MAX_UTC_OFFSET_MINUTES
  ) {
    return false;
  }

  minutesOut = static_cast<int>(minutes);
  return true;
}

String buildNetworkOptions() {
  showStatus(
    "Scanning Wi-Fi",
    portalApSsid
  );

  const int networkCount =
    WiFi.scanNetworks(false, true);

  String options;
  options.reserve(3000);

  String shownSsids[32];
  int shownCount = 0;

  for (
    int i = 0;
    i < networkCount && shownCount < 32;
    ++i
  ) {
    const String ssid = WiFi.SSID(i);

    if (ssid.length() == 0) {
      continue;
    }

    bool duplicate = false;

    for (int j = 0; j < shownCount; ++j) {
      if (shownSsids[j] == ssid) {
        duplicate = true;
        break;
      }
    }

    if (duplicate) {
      continue;
    }

    shownSsids[shownCount++] = ssid;

    options += F("<option value=\"");
    options += htmlEscape(ssid);
    options += F("\"");

    if (
      networkSettings.configured &&
      networkSettings.ssid == ssid
    ) {
      options += F(" selected");
    }

    options += F(">");
    options += htmlEscape(ssid);
    options += F(" (");
    options += String(WiFi.RSSI(i));
    options += F(" dBm");

    if (
      WiFi.encryptionType(i) ==
      WIFI_AUTH_OPEN
    ) {
      options += F(", open");
    }

    options += F(")</option>");
  }

  WiFi.scanDelete();

  if (shownCount == 0) {
    options +=
      F("<option value=\"\">No networks found</option>");
  }

  return options;
}

String configurationPage(
  const String &message = "",
  bool isError = false
) {
  String page;
  page.reserve(7800);

  page += F(
    "<!doctype html><html><head>"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta charset=\"utf-8\">"
    "<title>E32R28T World Clock Setup</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;background:#101820;color:#f2f4f7;"
    "margin:0;padding:20px}"
    ".card{max-width:520px;margin:auto;background:#1b2733;padding:22px;"
    "border-radius:12px;box-shadow:0 4px 18px #0008}"
    "h1{font-size:1.45rem;margin-top:0;color:#ffd43b}"
    "label{display:block;margin-top:16px;margin-bottom:6px;font-weight:bold}"
    "select,input{box-sizing:border-box;width:100%;padding:11px;border-radius:7px;"
    "border:1px solid #667;background:#fff;color:#111;font-size:1rem}"
    "button{width:100%;margin-top:22px;padding:12px;border:0;border-radius:8px;"
    "background:#ffd43b;color:#111;font-size:1.05rem;font-weight:bold}"
    ".note{font-size:.9rem;color:#bec8d2;line-height:1.4}"".row{display:flex;gap:10px}.sign{width:86px;flex:0 0 86px}"".grow{flex:1}.check{display:flex;align-items:center;gap:8px;margin-top:10px;""font-weight:normal}.check input{width:auto;margin:0}"
    ".msg{padding:10px;border-radius:7px;margin-bottom:12px;background:#24435a}"
    ".err{background:#6b2632}"
    "</style></head><body><div class=\"card\">"
    "<h1>E32R28T World Clock Setup</h1>"
  );

  if (message.length()) {
    page += F("<div class=\"msg");
    if (isError) {
      page += F(" err");
    }
    page += F("\">");
    page += htmlEscape(message);
    page += F("</div>");
  }

  page += F(
    "<form method=\"post\" action=\"/save\">"
    "<label for=\"ssid\">Wi-Fi network</label>"
    "<select id=\"ssid\" name=\"ssid\" required>"
  );

  page += portalNetworkOptions;

  page += F(
    "</select>"
    "<label for=\"manualSsid\">Or enter an SSID manually</label>"
    "<input id=\"manualSsid\" name=\"manualSsid\" maxlength=\"32\" "
    "placeholder=\"Optional hidden network name\">"
    "<label for=\"password\">Wi-Fi password</label>"
    "<input id=\"password\" name=\"password\" type=\"password\" maxlength=\"64\" "
    "placeholder=\"Leave blank for an open network\">"
    "<label class=\"check\"><input id=\"showPassword\" type=\"checkbox\" "
    "onclick=\"document.getElementById('password').type=this.checked?'text':'password'\">"
    "Show password</label>"
    "<label>UTC offset in hours</label>"
    "<div class=\"row\">"
    "<select class=\"sign\" id=\"offsetSign\" name=\"offsetSign\">"
    "<option value=\"+\""
  );

  if (networkSettings.utcOffsetMinutes >= 0) {
    page += F(" selected");
  }

  page += F(
    ">+</option><option value=\"-\""
  );

  if (networkSettings.utcOffsetMinutes < 0) {
    page += F(" selected");
  }

  page += F(
    ">-</option></select>"
    "<input class=\"grow\" id=\"offsetMagnitude\" name=\"offsetMagnitude\" "
    "type=\"number\" min=\"0\" max=\"14\" step=\"0.25\" inputmode=\"decimal\" "
    "required value=\""
  );

  page += offsetMagnitudeText(
    networkSettings.utcOffsetMinutes
  );

  page += F(
    "\"></div>"
    "<p class=\"note\">Examples: choose - and enter 5 for UTC-5; "
    "choose + and enter 5.5 for UTC+5:30. "
    "This is a fixed UTC offset and does not automatically change for "
    "daylight-saving time.</p>"
    "<label>Map overlays</label>"
    "<label class=\"check\"><input type=\"checkbox\" name=\"showSun\" value=\"1\""
  );

  if (overlaySettings.showSun) {
    page += F(" checked");
  }

  page += F(
    ">Show yellow Sun disc</label>"
    "<label class=\"check\"><input type=\"checkbox\" name=\"showMoon\" value=\"1\""
  );

  if (overlaySettings.showMoon) {
    page += F(" checked");
  }

  page += F(
    ">Show phased Moon disc</label>"
    "<label class=\"check\"><input type=\"checkbox\" name=\"showISS\" value=\"1\""
  );

  if (overlaySettings.showISS) {
    page += F(" checked");
  }

  page += F(
    ">Show small cyan ISS plus</label>"
    "<label class=\"check\"><input type=\"checkbox\" name=\"showIssTrack\" value=\"1\""
  );

  if (overlaySettings.showIssTrack) {
    page += F(" checked");
  }

  page += F(
    ">Show one complete ISS orbit track</label>"
    "<label class=\"check\"><input type=\"checkbox\" name=\"issTrackDotted\" value=\"1\""
  );

  if (overlaySettings.issTrackDotted) {
    page += F(" checked");
  }

  page += F(
    ">Use dotted orbit track</label>"
    "<button type=\"submit\">Save and connect</button>"
    "</form>"
    "<p class=\"note\">To change these settings later, hold the BOOT button "
    "for about three seconds while the clock is running.</p>"
    "</div></body></html>"
  );

  return page;
}

void sendConfigurationPage() {
  portalServer.send(
    200,
    "text/html",
    configurationPage()
  );
}

void handleConfigurationSave() {
  String ssid =
    portalServer.arg("manualSsid");

  ssid.trim();

  if (ssid.length() == 0) {
    ssid = portalServer.arg("ssid");
    ssid.trim();
  }

  const String password =
    portalServer.arg("password");

  int offsetMinutes = 0;

  if (ssid.length() == 0) {
    portalServer.send(
      400,
      "text/html",
      configurationPage(
        "Select or enter a Wi-Fi network.",
        true
      )
    );
    return;
  }

  String magnitudeText =
    portalServer.arg("offsetMagnitude");

  magnitudeText.trim();

  int magnitudeMinutes = 0;

  if (
    !parseUtcOffset(
      magnitudeText,
      magnitudeMinutes
    ) ||
    magnitudeMinutes < 0
  ) {
    portalServer.send(
      400,
      "text/html",
      configurationPage(
        "Enter a positive UTC-offset magnitude from 0 through 14.",
        true
      )
    );
    return;
  }

  const String sign =
    portalServer.arg("offsetSign");

  offsetMinutes =
    sign == "-"
    ? -magnitudeMinutes
    : magnitudeMinutes;

  if (
    offsetMinutes < MIN_UTC_OFFSET_MINUTES ||
    offsetMinutes > MAX_UTC_OFFSET_MINUTES
  ) {
    portalServer.send(
      400,
      "text/html",
      configurationPage(
        "The selected UTC offset is outside the allowed range.",
        true
      )
    );
    return;
  }

  if (
    !saveNetworkSettings(
      ssid,
      password,
      offsetMinutes
    )
  ) {
    portalServer.send(
      500,
      "text/html",
      configurationPage(
        "The settings could not be saved.",
        true
      )
    );
    return;
  }

  overlaySettings.showSun =
    portalServer.hasArg("showSun");

  overlaySettings.showMoon =
    portalServer.hasArg("showMoon");

  overlaySettings.showISS =
    portalServer.hasArg("showISS");

  overlaySettings.showIssTrack =
    portalServer.hasArg("showIssTrack");

  overlaySettings.issTrackDotted =
    portalServer.hasArg("issTrackDotted");

  if (!saveOverlaySettings()) {
    portalServer.send(
      500,
      "text/html",
      configurationPage(
        "The overlay settings could not be saved.",
        true
      )
    );
    return;
  }

  portalServer.send(
    200,
    "text/html",
    F(
      "<!doctype html><html><head>"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<meta charset=\"utf-8\"><title>Saved</title></head>"
      "<body style=\"font-family:Arial;background:#101820;color:white;"
      "text-align:center;padding:35px\">"
      "<h2 style=\"color:#ffd43b\">Settings saved</h2>"
      "<p>The clock is restarting and will connect to the selected network.</p>"
      "</body></html>"
    )
  );

  showStatus(
    "Settings saved",
    "Restarting..."
  );

  portalSaveComplete = true;
  portalRestartAt =
    millis() + PORTAL_RESTART_DELAY_MS;
}

void handleCaptiveProbe() {
  portalServer.sendHeader(
    "Location",
    String("http://") +
    WiFi.softAPIP().toString() +
    "/",
    true
  );

  portalServer.send(
    302,
    "text/plain",
    ""
  );
}

void stopConfigurationPortal() {
  portalServer.stop();
  portalDns.stop();
  WiFi.softAPdisconnect(true);
}
} // namespace

bool loadNetworkSettings() {
  NetworkSettings loaded;

  if (
    !portalPreferences.begin(
      PREF_NAMESPACE,
      true
    )
  ) {
    networkSettings = loaded;
    return false;
  }

  loaded.configured =
    portalPreferences.getBool(
      PREF_KEY_CONFIGURED,
      false
    );

  loaded.ssid =
    portalPreferences.getString(
      PREF_KEY_SSID,
      ""
    );

  loaded.password =
    portalPreferences.getString(
      PREF_KEY_PASSWORD,
      ""
    );

  loaded.utcOffsetMinutes =
    portalPreferences.getInt(
      PREF_KEY_OFFSET_MINUTES,
      0
    );

  portalPreferences.end();

  if (
    loaded.utcOffsetMinutes <
      MIN_UTC_OFFSET_MINUTES ||
    loaded.utcOffsetMinutes >
      MAX_UTC_OFFSET_MINUTES
  ) {
    loaded.configured = false;
  }

  if (loaded.ssid.length() == 0) {
    loaded.configured = false;
  }

  networkSettings = loaded;
  return networkSettings.configured;
}

bool saveNetworkSettings(
  const String &ssid,
  const String &password,
  int utcOffsetMinutes
) {
  if (
    ssid.length() == 0 ||
    utcOffsetMinutes <
      MIN_UTC_OFFSET_MINUTES ||
    utcOffsetMinutes >
      MAX_UTC_OFFSET_MINUTES
  ) {
    return false;
  }

  if (
    !portalPreferences.begin(
      PREF_NAMESPACE,
      false
    )
  ) {
    return false;
  }

  const bool ok =
    portalPreferences.putString(
      PREF_KEY_SSID,
      ssid
    ) > 0 &&
    portalPreferences.putString(
      PREF_KEY_PASSWORD,
      password
    ) >= 0 &&
    portalPreferences.putInt(
      PREF_KEY_OFFSET_MINUTES,
      utcOffsetMinutes
    ) > 0 &&
    portalPreferences.putBool(
      PREF_KEY_CONFIGURED,
      true
    ) > 0;

  portalPreferences.end();

  if (ok) {
    networkSettings.ssid = ssid;
    networkSettings.password = password;
    networkSettings.utcOffsetMinutes =
      utcOffsetMinutes;
    networkSettings.configured = true;
  }

  return ok;
}

void clearNetworkSettings() {
  portalPreferences.begin(
    PREF_NAMESPACE,
    false
  );

  portalPreferences.remove(
    PREF_KEY_SSID
  );

  portalPreferences.remove(
    PREF_KEY_PASSWORD
  );

  portalPreferences.remove(
    PREF_KEY_OFFSET_MINUTES
  );

  portalPreferences.remove(
    PREF_KEY_CONFIGURED
  );

  portalPreferences.end();

  networkSettings = NetworkSettings();
}

void serviceConfigurationButton() {
  pinMode(
    CONFIG_BUTTON_PIN,
    INPUT_PULLUP
  );

  if (
    digitalRead(CONFIG_BUTTON_PIN) == LOW
  ) {
    if (configurationButtonPressedAt == 0) {
      configurationButtonPressedAt = millis();
    }

    if (
      !configurationButtonTriggered &&
      millis() - configurationButtonPressedAt >=
      CONFIG_BUTTON_HOLD_MS
    ) {
      configurationButtonTriggered = true;

      showStatus(
        "Clearing settings",
        "Restarting setup portal"
      );

      clearNetworkSettings();
      delay(700);
      ESP.restart();
    }
  } else {
    configurationButtonPressedAt = 0;
    configurationButtonTriggered = false;
  }
}

String makeConfigurationApSsid() {
  const uint64_t chipId =
    ESP.getEfuseMac();

  char suffix[5];

  snprintf(
    suffix,
    sizeof(suffix),
    "%04X",
    static_cast<unsigned int>(
      chipId & 0xFFFF
    )
  );

  return String(CONFIG_AP_PREFIX) +
    "-" +
    suffix;
}

void runConfigurationPortal() {
  portalSaveComplete = false;
  portalRestartAt = 0;
  portalApSsid = makeConfigurationApSsid();

  WiFi.disconnect(false, false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);

  // Open setup network: no password is required.
  const bool apStarted =
    WiFi.softAP(
      portalApSsid.c_str(),
      nullptr,
      CONFIG_AP_CHANNEL,
      false,
      4
    );

  if (!apStarted) {
    showStatus(
      "Setup AP failed",
      "Reset to retry"
    );

    while (true) {
      delay(1000);
    }
  }

  const IPAddress portalIp =
    WiFi.softAPIP();

  showStatus(
    portalApSsid,
    portalIp.toString()
  );

  portalNetworkOptions = buildNetworkOptions();

  // Return the screen to the AP name after scanning.
  showStatus(
    portalApSsid,
    portalIp.toString()
  );

  portalDns.start(
    CONFIG_DNS_PORT,
    "*",
    portalIp
  );

  portalServer.on(
    "/",
    HTTP_GET,
    sendConfigurationPage
  );

  portalServer.on(
    "/save",
    HTTP_POST,
    handleConfigurationSave
  );

  // Common captive-portal detection URLs.
  portalServer.on(
    "/generate_204",
    HTTP_ANY,
    handleCaptiveProbe
  );

  portalServer.on(
    "/gen_204",
    HTTP_ANY,
    handleCaptiveProbe
  );

  portalServer.on(
    "/hotspot-detect.html",
    HTTP_ANY,
    sendConfigurationPage
  );

  portalServer.on(
    "/connecttest.txt",
    HTTP_ANY,
    sendConfigurationPage
  );

  portalServer.on(
    "/ncsi.txt",
    HTTP_ANY,
    sendConfigurationPage
  );

  portalServer.onNotFound(
    sendConfigurationPage
  );

  portalServer.begin();

  while (true) {
    portalDns.processNextRequest();
    portalServer.handleClient();

    if (
      portalSaveComplete &&
      static_cast<long>(
        millis() - portalRestartAt
      ) >= 0
    ) {
      stopConfigurationPortal();
      delay(100);
      ESP.restart();
    }

    delay(2);
  }
}

bool ensureNetworkConfigured() {
  loadOverlaySettings();
  loadNetworkSettings();

  if (!networkSettings.configured) {
    runConfigurationPortal();
    return false;
  }

  if (!connectConfiguredWiFi()) {
    showStatus(
      "Wi-Fi connection failed",
      "Starting setup portal"
    );

    delay(700);
    runConfigurationPortal();
    return false;
  }

  return true;
}


bool loadOverlaySettings() {
  Preferences preferences;

  if (
    !preferences.begin(
      PREF_NAMESPACE,
      true
    )
  ) {
    return false;
  }

  overlaySettings.showSun =
    preferences.getBool(
      PREF_KEY_SHOW_SUN,
      true
    );

  overlaySettings.showMoon =
    preferences.getBool(
      PREF_KEY_SHOW_MOON,
      true
    );

  overlaySettings.showISS =
    preferences.getBool(
      PREF_KEY_SHOW_ISS,
      false
    );

  overlaySettings.showIssTrack =
    preferences.getBool(
      PREF_KEY_SHOW_ISS_TRACK,
      false
    );

  overlaySettings.issTrackDotted =
    preferences.getBool(
      PREF_KEY_ISS_TRACK_DOTTED,
      false
    );

  preferences.end();
  return true;
}

bool saveOverlaySettings() {
  Preferences preferences;

  if (
    !preferences.begin(
      PREF_NAMESPACE,
      false
    )
  ) {
    return false;
  }

  const bool ok =
    preferences.putBool(
      PREF_KEY_SHOW_SUN,
      overlaySettings.showSun
    ) > 0 &&
    preferences.putBool(
      PREF_KEY_SHOW_MOON,
      overlaySettings.showMoon
    ) > 0 &&
    preferences.putBool(
      PREF_KEY_SHOW_ISS,
      overlaySettings.showISS
    ) > 0 &&
    preferences.putBool(
      PREF_KEY_SHOW_ISS_TRACK,
      overlaySettings.showIssTrack
    ) > 0 &&
    preferences.putBool(
      PREF_KEY_ISS_TRACK_DOTTED,
      overlaySettings.issTrackDotted
    ) > 0;

  preferences.end();
  return ok;
}


bool loadDisplaySettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, true)) {
    displaySettings.flip180 = false;
    return false;
  }

  displaySettings.flip180 =
    preferences.getBool(
      PREF_KEY_DISPLAY_FLIP_180,
      false
    );

  preferences.end();
  return true;
}


bool saveDisplaySettings() {
  Preferences preferences;

  if (!preferences.begin(PREF_NAMESPACE, false)) {
    return false;
  }

  const bool ok =
    preferences.putBool(
      PREF_KEY_DISPLAY_FLIP_180,
      displaySettings.flip180
    ) > 0;

  preferences.end();
  return ok;
}
