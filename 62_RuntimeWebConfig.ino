// ============================================================
// Runtime configuration, diagnostics, and map maintenance
// ============================================================

namespace {
WebServer runtimeServer(RUNTIME_CONFIG_PORT);
bool runtimeServerStarted = false;

// Return HTTP responses before doing slow NTP, ISS, display, or
// restart work.
bool runtimeApplyPending = false;
bool runtimeForgetPending = false;
bool runtimePendingOffsetChanged = false;
bool runtimePendingOrientationChanged = false;
unsigned long runtimeActionAt = 0;

enum class RuntimeMapAction {
  None,
  RebuildDay,
  RebuildNight,
  RebuildBoth
};

RuntimeMapAction runtimeMapActionPending =
  RuntimeMapAction::None;

unsigned long runtimeMapActionAt = 0;
String runtimeMapResultMessage;
bool runtimeMapResultError = false;

String htmlEscapeRuntime(const String &input) {
  String output;
  output.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i) {
    switch (input[i]) {
      case '&': output += F("&amp;"); break;
      case '<': output += F("&lt;"); break;
      case '>': output += F("&gt;"); break;
      case '"': output += F("&quot;"); break;
      case '\'': output += F("&#39;"); break;
      default: output += input[i]; break;
    }
  }
  return output;
}

String pageHeader(const String &title) {
  String page;
  page.reserve(7000);
  page += F("<!doctype html><html><head><meta name=\"viewport\" "
            "content=\"width=device-width,initial-scale=1\"><meta charset=\"utf-8\">"
            "<style>body{font-family:Arial,sans-serif;background:#101820;color:#f2f4f7;"
            "margin:0;padding:20px}.card{max-width:620px;margin:auto;background:#1b2733;"
            "padding:22px;border-radius:12px}h1,h2{color:#ffd43b}a{color:#66d9ef}"
            "label{display:block;margin-top:14px;margin-bottom:6px;font-weight:bold}"
            "select,input{box-sizing:border-box;width:100%;padding:10px;border-radius:7px;"
            "border:1px solid #667;background:#fff;color:#111;font-size:1rem}"
            "button,.button{display:block;box-sizing:border-box;width:100%;margin-top:16px;"
            "padding:11px;border:0;border-radius:8px;background:#ffd43b;color:#111;"
            "font-weight:bold;text-align:center;text-decoration:none;font-size:1rem}"
            ".danger{background:#c94b50;color:white}"
            ".row{display:flex;gap:10px}.sign{width:86px;flex:0 0 86px}.grow{flex:1}"
            ".check{display:flex;align-items:center;gap:8px;margin-top:10px;font-weight:normal}"
            ".check input{width:auto;margin:0}.msg{padding:10px;border-radius:7px;"
            "margin-bottom:12px;background:#24435a}.err{background:#6b2632}"
            "table{width:100%;border-collapse:collapse}td{padding:7px;border-bottom:1px solid #40505f}"
            "td:first-child{color:#bec8d2}.nav{margin-bottom:16px}.nav a{margin-right:14px}"
            ".note{font-size:.9rem;color:#bec8d2;line-height:1.4}</style><title>");
  page += htmlEscapeRuntime(title);
  page += F("</title></head><body><div class=\"card\"><div class=\"nav\">"
            "<a href=\"/\">Settings</a><a href=\"/diagnostics\">Diagnostics</a>"
            "<a href=\"/maps\">Maps</a></div><h1>");
  page += htmlEscapeRuntime(title);
  page += F("</h1>");
  return page;
}

String pageFooter() {
  return F("</div></body></html>");
}

void sendRuntimeHtml(
  int statusCode,
  const String &page
) {
  runtimeServer.sendHeader(
    "Cache-Control",
    "no-store, no-cache, must-revalidate, max-age=0"
  );

  runtimeServer.sendHeader(
    "Pragma",
    "no-cache"
  );

  runtimeServer.send(
    statusCode,
    "text/html",
    page
  );
}

const char *runtimeMethodName() {
  switch (runtimeServer.method()) {
    case HTTP_GET:
      return "GET";

    case HTTP_POST:
      return "POST";

    default:
      return "OTHER";
  }
}

void handleRuntimeNotFound() {
  Serial.printf(
    "Runtime web: unhandled %s %s\n",
    runtimeMethodName(),
    runtimeServer.uri().c_str()
  );

  sendRuntimeHtml(
    404,
    pageHeader("Not found") +
      F("<p>The requested control-panel action was not recognized.</p>"
        "<p><a href=\"/\">Return to Settings</a></p>") +
      pageFooter()
  );
}

String offsetMagnitudeTextRuntime(int offsetMinutes) {
  int absoluteMinutes = abs(offsetMinutes);
  String value = String(absoluteMinutes / 60);
  int minutes = absoluteMinutes % 60;
  if (minutes) {
    value += ".";
    value += String((minutes * 100) / 60);
    while (value.endsWith("0")) value.remove(value.length() - 1);
  }
  return value;
}

bool parseOffsetMagnitudeRuntime(const String &text, int &minutesOut) {
  String cleaned = text;
  cleaned.trim();
  char *endPointer = nullptr;
  double hours = strtod(cleaned.c_str(), &endPointer);
  if (cleaned.length() == 0 || endPointer == cleaned.c_str() ||
      *endPointer != '\0' || !isfinite(hours) || hours < 0.0 || hours > 14.0) {
    return false;
  }
  minutesOut = static_cast<int>(lround(hours * 60.0));
  return true;
}

bool saveRuntimeUtcOffset(
  int offsetMinutes
) {
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
    preferences.putInt(
      PREF_KEY_OFFSET_MINUTES,
      offsetMinutes
    ) > 0;

  preferences.end();
  return ok;
}


String formatAge(time_t stamp) {
  if (stamp <= 0) return "Never";
  long age = static_cast<long>(time(nullptr) - stamp);
  if (age < 0) age = 0;
  if (age < 60) return String(age) + " sec";
  if (age < 3600) return String(age / 60) + " min";
  return String(age / 3600) + " hr " + String((age % 3600) / 60) + " min";
}

String runtimeConfigurationPage(
  const String &message = "",
  bool error = false
) {
  String page =
    pageHeader(
      "World Clock Configuration"
    );

  if (message.length()) {
    page += F("<div class=\"msg");

    if (error) {
      page += F(" err");
    }

    page += F("\">");
    page += htmlEscapeRuntime(message);
    page += F("</div>");
  }

  page +=
    F("<p class=\"note\">Connected to <strong>");

  page +=
    htmlEscapeRuntime(
      WiFi.SSID()
    );

  page += F(
    "</strong> at <strong>http://"
  );

  page +=
    WiFi.localIP().toString();

  page += F(
    "</strong></p>"
    "<div id=\"settingsControls\">"
    "<label>UTC offset in hours</label>"
    "<div class=\"row\">"
    "<select class=\"sign\" id=\"offsetSign\">"
    "<option value=\"+\""
  );

  if (
    networkSettings.utcOffsetMinutes >= 0
  ) {
    page += F(" selected");
  }

  page += F(
    ">+</option>"
    "<option value=\"-\""
  );

  if (
    networkSettings.utcOffsetMinutes < 0
  ) {
    page += F(" selected");
  }

  page += F(
    ">-</option>"
    "</select>"
    "<input class=\"grow\" "
    "id=\"offsetMagnitude\" "
    "type=\"text\" "
    "inputmode=\"decimal\" "
    "maxlength=\"6\" "
    "value=\""
  );

  page +=
    offsetMagnitudeTextRuntime(
      networkSettings.utcOffsetMinutes
    );

  page += F(
    "\">"
    "</div>"
    "<label>Display orientation</label>"
    "<select id=\"flip180\">"
    "<option value=\"0\""
  );

  if (!displaySettings.flip180) {
    page += F(" selected");
  }

  page += F(
    ">Normal</option>"
    "<option value=\"1\""
  );

  if (displaySettings.flip180) {
    page += F(" selected");
  }

  page += F(
    ">Flipped 180 degrees</option>"
    "</select>"
    "<label>Map overlays</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" "
    "id=\"showSun\""
  );

  if (overlaySettings.showSun) {
    page += F(" checked");
  }

  page += F(
    ">Show Sun</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" "
    "id=\"showMoon\""
  );

  if (overlaySettings.showMoon) {
    page += F(" checked");
  }

  page += F(
    ">Show Moon</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" "
    "id=\"showISS\""
  );

  if (overlaySettings.showISS) {
    page += F(" checked");
  }

  page += F(
    ">Show ISS marker</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" "
    "id=\"showIssTrack\""
  );

  if (overlaySettings.showIssTrack) {
    page += F(" checked");
  }

  page += F(
    ">Show one-orbit ISS track</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" "
    "id=\"issTrackDotted\""
  );

  if (overlaySettings.issTrackDotted) {
    page += F(" checked");
  }

  page += F(
    ">Use dotted track</label>"
    "</div>"

    // This is deliberately an ordinary link, not a form submit
    // control. The other link-based controls are known to work.
    "<a class=\"button\" "
    "href=\"/apply-settings?source=fallback\" "
    "onclick=\"return wcApplySettings();\">"
    "Apply settings now"
    "</a>"

    "<a class=\"button danger\" "
    "href=\"/forget\">"
    "Forget Wi-Fi and start setup"
    "</a>"

    "<script>"
    "function wcApplySettings(){"
      "try{"
        "var q=[];"
        "function add(n,v){"
          "q.push(encodeURIComponent(n)+'='+encodeURIComponent(v));"
        "}"
        "add('offsetSign',"
          "document.getElementById('offsetSign').value);"
        "add('offsetMagnitude',"
          "document.getElementById('offsetMagnitude').value);"
        "add('flip180',"
          "document.getElementById('flip180').value);"

        "var checks=["
          "'showSun',"
          "'showMoon',"
          "'showISS',"
          "'showIssTrack',"
          "'issTrackDotted'"
        "];"

        "for(var i=0;i<checks.length;i++){"
          "var e=document.getElementById(checks[i]);"
          "if(e&&e.checked){"
            "add(checks[i],'1');"
          "}"
        "}"

        "add('source','apply-link');"
        "window.location.href='/apply-settings?'+q.join('&');"
        "return false;"
      "}catch(error){"
        // Returning true follows the fallback href, which will still
        // reach the ESP32 and create Serial output.
        "return true;"
      "}"
    "}"
    "</script>"
  );

  page += pageFooter();
  return page;
}

void sendRuntimePage() {
  sendRuntimeHtml(
    200,
    runtimeConfigurationPage()
  );
}

void handleRuntimeSave() {
  Serial.printf(
    "Runtime web: %s %s; %d argument(s)\n",
    runtimeMethodName(),
    runtimeServer.uri().c_str(),
    runtimeServer.args()
  );

  for (
    int index = 0;
    index < runtimeServer.args();
    ++index
  ) {
    Serial.printf(
      "  arg[%d] %s=%s\n",
      index,
      runtimeServer.argName(index).c_str(),
      runtimeServer.arg(index).c_str()
    );
  }

  if (
    !runtimeServer.hasArg("offsetMagnitude") ||
    !runtimeServer.hasArg("offsetSign") ||
    !runtimeServer.hasArg("flip180")
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "The Apply link reached the clock, but the browser did not "
        "supply the selected values. Please report this message.",
        true
      )
    );
    return;
  }

  int magnitudeMinutes = 0;

  if (
    !parseOffsetMagnitudeRuntime(
      runtimeServer.arg("offsetMagnitude"),
      magnitudeMinutes
    )
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "Invalid UTC offset. Enter a value from 0 through 14 hours.",
        true
      )
    );
    return;
  }

  const int offsetMinutes =
    runtimeServer.arg("offsetSign") == "-"
      ? -magnitudeMinutes
      : magnitudeMinutes;

  if (
    offsetMinutes < MIN_UTC_OFFSET_MINUTES ||
    offsetMinutes > MAX_UTC_OFFSET_MINUTES
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "UTC offset is outside the allowed range.",
        true
      )
    );
    return;
  }

  const bool requestedFlip180 =
    runtimeServer.arg("flip180") == "1";

  runtimePendingOrientationChanged =
    requestedFlip180 !=
      displaySettings.flip180;

  runtimePendingOffsetChanged =
    offsetMinutes !=
      networkSettings.utcOffsetMinutes;

  // Apply valid submitted values to live state before attempting
  // persistence. A preference error must not block this session.
  networkSettings.utcOffsetMinutes =
    offsetMinutes;

  displaySettings.flip180 =
    requestedFlip180;

  overlaySettings.showSun =
    runtimeServer.hasArg("showSun");

  overlaySettings.showMoon =
    runtimeServer.hasArg("showMoon");

  overlaySettings.showISS =
    runtimeServer.hasArg("showISS");

  overlaySettings.showIssTrack =
    runtimeServer.hasArg("showIssTrack");

  overlaySettings.issTrackDotted =
    runtimeServer.hasArg("issTrackDotted");

  const bool offsetSaved =
    saveRuntimeUtcOffset(
      offsetMinutes
    );

  const bool overlaysSaved =
    saveOverlaySettings();

  const bool displaySaved =
    saveDisplaySettings();

  Serial.printf(
    "Runtime web: persistence results; "
    "offset=%d overlays=%d display=%d\n",
    offsetSaved,
    overlaysSaved,
    displaySaved
  );

  // Always apply valid submitted settings now.
  runtimeApplyPending = true;
  runtimeActionAt = millis() + 250UL;

  String message =
    "Settings accepted. The display is updating now.";

  const bool persistenceFailed =
    !offsetSaved ||
    !overlaysSaved ||
    !displaySaved;

  if (persistenceFailed) {
    message +=
      " One or more settings could not be stored permanently; "
      "see Serial output for details.";
  }

  sendRuntimeHtml(
    200,
    runtimeConfigurationPage(
      message,
      persistenceFailed
    )
  );
}

void handleForgetWifi() {
  Serial.printf(
    "Runtime web: %s /forget; confirm=%s\n",
    runtimeMethodName(),
    runtimeServer.arg("confirm").c_str()
  );

  if (
    runtimeServer.arg("confirm") != "1"
  ) {
    sendRuntimeHtml(
      200,
      pageHeader("Confirm Wi-Fi reset") +
        F("<p>This will erase the saved Wi-Fi network and restart "
          "the clock in setup mode.</p>"
          "<a class=\"button danger\" href=\"/forget?confirm=1\">"
          "Yes, forget Wi-Fi and restart</a>"
          "<a class=\"button\" href=\"/\">Cancel</a>") +
        pageFooter()
    );
    return;
  }

  clearNetworkSettings();

  runtimeForgetPending = true;
  runtimeActionAt = millis() + 1800UL;

  sendRuntimeHtml(
    200,
    pageHeader("Wi-Fi cleared") +
      F("<p>The saved Wi-Fi network has been cleared.</p>"
        "<p>The clock will restart into captive-portal setup "
        "in a moment.</p>") +
      pageFooter()
  );
}

void handleDiagnostics() {
  sendRuntimeHtml(
    200,
    buildDiagnosticsPage()
  );
}

void handleMaps() {
  const String message =
    runtimeMapResultMessage;

  const bool error =
    runtimeMapResultError;

  runtimeMapResultMessage = "";
  runtimeMapResultError = false;

  sendRuntimeHtml(
    200,
    buildMapMaintenancePage(
      message,
      error
    )
  );
}


void handleValidateMaps() {
  Serial.printf(
    "Runtime web: %s /maps/validate\n",
    runtimeMethodName()
  );

  refreshStorageStatus();

  sendRuntimeHtml(
    200,
    buildMapMaintenancePage(
      "Validation complete."
    )
  );
}


void scheduleMapAction(
  RuntimeMapAction action,
  const String &label
) {
  Serial.printf(
    "Runtime web: %s %s\n",
    runtimeMethodName(),
    runtimeServer.uri().c_str()
  );

  if (
    runtimeMapActionPending !=
      RuntimeMapAction::None
  ) {
    sendRuntimeHtml(
      409,
      buildMapMaintenancePage(
        "Another map operation is already running.",
        true
      )
    );
    return;
  }

  runtimeMapActionPending = action;
  runtimeMapActionAt = millis() + 250UL;

  runtimeServer.sendHeader(
    "Refresh",
    "2; url=/maps"
  );

  sendRuntimeHtml(
    202,
    pageHeader("Map Maintenance") +
      F("<p>") +
      htmlEscapeRuntime(label) +
      F(" has been scheduled.</p>"
        "<p>This page will return to Map Maintenance automatically.</p>"
        "<p><a href=\"/maps\">Check status now</a></p>") +
      pageFooter()
  );
}


void handleRebuildDay() {
  scheduleMapAction(
    RuntimeMapAction::RebuildDay,
    "Day-map cache rebuild"
  );
}


void handleRebuildNight() {
  scheduleMapAction(
    RuntimeMapAction::RebuildNight,
    "Night-map cache rebuild"
  );
}


void handleRebuildBoth() {
  scheduleMapAction(
    RuntimeMapAction::RebuildBoth,
    "Day and night cache rebuild"
  );
}
} // namespace

String buildDiagnosticsPage() {
  systemStatus.wifiConnected = WiFi.status() == WL_CONNECTED;
  systemStatus.issPositionValid = issPosition.valid;
  systemStatus.orbitTrackValid = issTrackValid;
  String page = pageHeader("Diagnostics");
  page += F("<table>");
  auto row = [&](const String &name, const String &value) {
    page += F("<tr><td>"); page += name; page += F("</td><td>"); page += htmlEscapeRuntime(value); page += F("</td></tr>");
  };
  row("Board profile", ACTIVE_BOARD.name);
  row("Display dimensions", String(lcd.width()) + " x " + String(lcd.height()));
  row("Native LCD rotation", String(DISPLAY_ROTATION));
  row("Configured orientation", displayOrientationName());
  row("Effective LCD rotation", String(effectiveDisplayRotation()));
  row("Firmware", FIRMWARE_VERSION);
  row("Build timestamp", String(__DATE__) + " " + String(__TIME__));
  row("Uptime", formatUptime(millis()));
  row("Last reset reason", resetReasonName(esp_reset_reason()));
  row("Free heap", String(ESP.getFreeHeap()) + " bytes");
  row("Minimum free heap", String(ESP.getMinFreeHeap()) + " bytes");
  row("Largest free heap block", String(ESP.getMaxAllocHeap()) + " bytes");
  row("Flash size", String(ESP.getFlashChipSize()) + " bytes");
  row("Sketch size", String(ESP.getSketchSize()) + " bytes");
  row("Free sketch space", String(ESP.getFreeSketchSpace()) + " bytes");
  row("Wi-Fi", systemStatus.wifiConnected ? "Connected" : "Disconnected");
  row("SSID", WiFi.SSID());
  row("IP", WiFi.localIP().toString());
  row("RSSI", String(WiFi.RSSI()) + " dBm");
  row("NTP", systemStatus.ntpSynced ? "Synchronized" : "Unavailable");
  row("Last NTP sync age", formatAge(systemStatus.lastNtpSync));
  row("microSD", systemStatus.sdMounted ? "Mounted" : "Unavailable");
  row("Day PNG", systemStatus.dayPngFound ? "Found" : "Missing");
  row("Night PNG", systemStatus.nightPngFound ? "Found" : "Missing");
  row("Day cache", systemStatus.dayCacheValid ? "Valid" : "Invalid");
  row("Night cache", systemStatus.nightCacheValid ? "Valid" : "Invalid");
  row("ISS position", issPosition.valid ? "Valid" : "Unavailable");
  row("ISS age", formatAge(issPosition.updatedAt));
  row("Orbit track", issTrackValid ? "Valid" : "Unavailable");
  row("Sun/Moon/ISS/Track", String(overlaySettings.showSun) + "/" + String(overlaySettings.showMoon) + "/" + String(overlaySettings.showISS) + "/" + String(overlaySettings.showIssTrack));
  row("Last error", systemErrorName(systemStatus.lastError));
  row("Error detail", systemStatus.lastErrorText.length() ? systemStatus.lastErrorText : "None");
  page += F("</table><p><a href=\"/diagnostics\">Refresh</a></p>");
  page += pageFooter(); return page;
}

String buildMapMaintenancePage(const String &message, bool error) {
  refreshStorageStatus();
  String page = pageHeader("Map Maintenance");
  if (message.length()) { page += F("<div class=\"msg"); if (error) page += F(" err"); page += F("\">"); page += htmlEscapeRuntime(message); page += F("</div>"); }
  page += F("<table>");
  auto row = [&](const String &name, const String &value) {
    page += F("<tr><td>"); page += name; page += F("</td><td>"); page += htmlEscapeRuntime(value); page += F("</td></tr>");
  };
  row("Expected cache size", String(RAW_MAP_BYTES) + " bytes");
  row("Day PNG", systemStatus.dayPngFound ? String(fileSizeOrZero(DAY_PNG_FILE)) + " bytes" : "Missing");
  row("Night PNG", systemStatus.nightPngFound ? String(fileSizeOrZero(NIGHT_PNG_FILE)) + " bytes" : "Missing");
  row("Day RGB565", systemStatus.dayCacheValid ? String(fileSizeOrZero(DAY_RAW_FILE)) + " bytes; valid" : "Invalid or missing");
  row("Night RGB565", systemStatus.nightCacheValid ? String(fileSizeOrZero(NIGHT_RAW_FILE)) + " bytes; valid" : "Invalid or missing");
  page += F("</table>"
            "<a class=\"button\" href=\"/maps/validate\">Validate files</a>"
            "<a class=\"button\" href=\"/maps/rebuild-day\">Rebuild day cache</a>"
            "<a class=\"button\" href=\"/maps/rebuild-night\">Rebuild night cache</a>"
            "<a class=\"button\" href=\"/maps/rebuild-both\">Rebuild both caches</a>"
            "<p class=\"note\">Rebuilding closes the active map files, converts the PNG, validates the 153,600-byte cache, reopens both maps, and redraws the clock.</p>");
  page += pageFooter(); return page;
}

void startRuntimeConfigServer() {
  if (runtimeServerStarted || WiFi.status() != WL_CONNECTED) return;
  runtimeServer.on(
    RUNTIME_CONFIG_PATH,
    sendRuntimePage
  );

  runtimeServer.on(
    "/apply-settings",
    HTTP_GET,
    handleRuntimeSave
  );

  // Backward-compatible path for a browser displaying a cached page.
  runtimeServer.on(
    "/save",
    HTTP_GET,
    handleRuntimeSave
  );

  runtimeServer.on(
    "/forget",
    HTTP_GET,
    handleForgetWifi
  );
  runtimeServer.on(DIAGNOSTICS_PATH, HTTP_GET, handleDiagnostics);
  runtimeServer.on(MAP_MAINTENANCE_PATH, HTTP_GET, handleMaps);
  runtimeServer.on(
    MAP_VALIDATE_PATH,
    HTTP_GET,
    handleValidateMaps
  );

  runtimeServer.on(
    MAP_REBUILD_DAY_PATH,
    HTTP_GET,
    handleRebuildDay
  );

  runtimeServer.on(
    MAP_REBUILD_NIGHT_PATH,
    HTTP_GET,
    handleRebuildNight
  );

  runtimeServer.on(
    MAP_REBUILD_BOTH_PATH,
    HTTP_GET,
    handleRebuildBoth
  );
  runtimeServer.onNotFound(
    handleRuntimeNotFound
  );
  runtimeServer.begin(); runtimeServerStarted = true;
  Serial.print("Runtime configuration: http://"); Serial.println(WiFi.localIP());
}

void serviceRuntimeConfigServer() {
  if (
    !runtimeServerStarted &&
    WiFi.status() == WL_CONNECTED
  ) {
    startRuntimeConfigServer();
  }

  if (runtimeServerStarted) {
    runtimeServer.handleClient();
  }

  const unsigned long now =
    millis();

  if (
    runtimeForgetPending &&
    static_cast<long>(
      now - runtimeActionAt
    ) >= 0
  ) {
    runtimeForgetPending = false;

    Serial.println(
      "Runtime web: restarting into setup portal"
    );

    delay(50);
    ESP.restart();
  }

  if (
    runtimeApplyPending &&
    static_cast<long>(
      now - runtimeActionAt
    ) >= 0
  ) {
    runtimeApplyPending = false;

    const bool offsetChanged =
      runtimePendingOffsetChanged;

    const bool orientationChanged =
      runtimePendingOrientationChanged;

    runtimePendingOffsetChanged = false;
    runtimePendingOrientationChanged = false;

    Serial.printf(
      "Runtime web: applying settings; "
      "offsetChanged=%d orientationChanged=%d\n",
      offsetChanged,
      orientationChanged
    );

    if (offsetChanged) {
      retryNtpSync();
    }

    if (orientationChanged) {
      applyDisplayRotation();
      lcd.fillScreen(TFT_BLACK);
    }

    if (
      (
        overlaySettings.showISS ||
        overlaySettings.showIssTrack
      ) &&
      !issPosition.valid
    ) {
      fetchIssPosition();
    }

    if (
      overlaySettings.showIssTrack &&
      issPosition.valid
    ) {
      calculateIssOrbitTrack();
    }

    redrawWorldClock();

    Serial.println(
      "Runtime web: settings applied"
    );
  }


  if (
    runtimeMapActionPending !=
      RuntimeMapAction::None &&
    static_cast<long>(
      now - runtimeMapActionAt
    ) >= 0
  ) {
    const RuntimeMapAction action =
      runtimeMapActionPending;

    runtimeMapActionPending =
      RuntimeMapAction::None;

    bool ok = false;

    switch (action) {
      case RuntimeMapAction::RebuildDay:
        Serial.println(
          "Runtime web: rebuilding day cache"
        );

        ok = rebuildMapCache(
          true,
          false
        );

        runtimeMapResultMessage =
          ok
            ? "Day cache rebuilt."
            : "Day cache rebuild failed.";
        break;

      case RuntimeMapAction::RebuildNight:
        Serial.println(
          "Runtime web: rebuilding night cache"
        );

        ok = rebuildMapCache(
          false,
          true
        );

        runtimeMapResultMessage =
          ok
            ? "Night cache rebuilt."
            : "Night cache rebuild failed.";
        break;

      case RuntimeMapAction::RebuildBoth:
        Serial.println(
          "Runtime web: rebuilding both caches"
        );

        ok = rebuildMapCache(
          true,
          true
        );

        runtimeMapResultMessage =
          ok
            ? "Both caches rebuilt."
            : "Cache rebuild failed.";
        break;

      case RuntimeMapAction::None:
      default:
        break;
    }

    runtimeMapResultError = !ok;

    Serial.printf(
      "Runtime web: map operation complete; ok=%d\n",
      ok
    );
  }
}
