// ============================================================
// Runtime configuration, diagnostics, and map maintenance
// ============================================================

namespace {
WebServer runtimeServer(RUNTIME_CONFIG_PORT);
bool runtimeServerStarted = false;

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
            "button{width:100%;margin-top:16px;padding:11px;border:0;border-radius:8px;"
            "background:#ffd43b;color:#111;font-weight:bold}.danger{background:#c94b50;color:white}"
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

String formatAge(time_t stamp) {
  if (stamp <= 0) return "Never";
  long age = static_cast<long>(time(nullptr) - stamp);
  if (age < 0) age = 0;
  if (age < 60) return String(age) + " sec";
  if (age < 3600) return String(age / 60) + " min";
  return String(age / 3600) + " hr " + String((age % 3600) / 60) + " min";
}

String runtimeConfigurationPage(const String &message = "", bool error = false) {
  String page = pageHeader("World Clock Configuration");
  if (message.length()) {
    page += F("<div class=\"msg"); if (error) page += F(" err"); page += F("\">");
    page += htmlEscapeRuntime(message); page += F("</div>");
  }
  page += F("<p class=\"note\">Connected to <strong>");
  page += htmlEscapeRuntime(WiFi.SSID());
  page += F("</strong> at <strong>http://"); page += WiFi.localIP().toString();
  page += F("</strong></p><form method=\"post\" action=\"/save\">"
            "<label>UTC offset in hours</label><div class=\"row\">"
            "<select class=\"sign\" name=\"offsetSign\"><option value=\"+\"");
  if (networkSettings.utcOffsetMinutes >= 0) page += F(" selected");
  page += F(">+</option><option value=\"-\"");
  if (networkSettings.utcOffsetMinutes < 0) page += F(" selected");
  page += F(">-</option></select><input class=\"grow\" name=\"offsetMagnitude\" "
            "type=\"number\" min=\"0\" max=\"14\" step=\"0.25\" inputmode=\"decimal\" required value=\"");
  page += offsetMagnitudeTextRuntime(networkSettings.utcOffsetMinutes);
  page += F("\"></div><label>Map overlays</label>"
            "<label class=\"check\"><input type=\"checkbox\" name=\"showSun\" value=\"1\"");
  if (overlaySettings.showSun) page += F(" checked");
  page += F(">Show Sun</label><label class=\"check\"><input type=\"checkbox\" name=\"showMoon\" value=\"1\"");
  if (overlaySettings.showMoon) page += F(" checked");
  page += F(">Show Moon</label><label class=\"check\"><input type=\"checkbox\" name=\"showISS\" value=\"1\"");
  if (overlaySettings.showISS) page += F(" checked");
  page += F(">Show ISS marker</label><label class=\"check\"><input type=\"checkbox\" name=\"showIssTrack\" value=\"1\"");
  if (overlaySettings.showIssTrack) page += F(" checked");
  page += F(">Show one-orbit ISS track</label><label class=\"check\"><input type=\"checkbox\" name=\"issTrackDotted\" value=\"1\"");
  if (overlaySettings.issTrackDotted) page += F(" checked");
  page += F(">Use dotted track</label><button type=\"submit\">Apply settings now</button></form>"
            "<form method=\"post\" action=\"/forget\"><button class=\"danger\" type=\"submit\">"
            "Forget Wi-Fi and start setup</button></form>");
  page += pageFooter(); return page;
}

void sendRuntimePage() { runtimeServer.send(200, "text/html", runtimeConfigurationPage()); }

void handleRuntimeSave() {
  int magnitudeMinutes = 0;
  if (!parseOffsetMagnitudeRuntime(runtimeServer.arg("offsetMagnitude"), magnitudeMinutes)) {
    runtimeServer.send(400, "text/html", runtimeConfigurationPage("Invalid UTC offset.", true)); return;
  }
  int offsetMinutes = runtimeServer.arg("offsetSign") == "-" ? -magnitudeMinutes : magnitudeMinutes;
  if (offsetMinutes < MIN_UTC_OFFSET_MINUTES || offsetMinutes > MAX_UTC_OFFSET_MINUTES) {
    runtimeServer.send(400, "text/html", runtimeConfigurationPage("UTC offset is outside the allowed range.", true)); return;
  }
  overlaySettings.showSun = runtimeServer.hasArg("showSun");
  overlaySettings.showMoon = runtimeServer.hasArg("showMoon");
  overlaySettings.showISS = runtimeServer.hasArg("showISS");
  overlaySettings.showIssTrack = runtimeServer.hasArg("showIssTrack");
  overlaySettings.issTrackDotted = runtimeServer.hasArg("issTrackDotted");
  bool offsetChanged = offsetMinutes != networkSettings.utcOffsetMinutes;
  bool saved = saveNetworkSettings(networkSettings.ssid, networkSettings.password, offsetMinutes) && saveOverlaySettings();
  if (!saved) { runtimeServer.send(500, "text/html", runtimeConfigurationPage("Settings could not be saved.", true)); return; }
  if (offsetChanged) retryNtpSync();
  if ((overlaySettings.showISS || overlaySettings.showIssTrack) && !issPosition.valid) fetchIssPosition();
  if (overlaySettings.showIssTrack && issPosition.valid) calculateIssOrbitTrack();
  redrawWorldClock();
  runtimeServer.send(200, "text/html", runtimeConfigurationPage("Settings applied without restarting."));
}

void handleForgetWifi() {
  runtimeServer.send(200, "text/html", pageHeader("Wi-Fi cleared") +
    F("<p>The clock is restarting into captive-portal setup.</p>") + pageFooter());
  clearNetworkSettings(); delay(900); ESP.restart();
}

void handleDiagnostics() { runtimeServer.send(200, "text/html", buildDiagnosticsPage()); }
void handleMaps() { runtimeServer.send(200, "text/html", buildMapMaintenancePage()); }
void handleValidateMaps() { refreshStorageStatus(); runtimeServer.send(200, "text/html", buildMapMaintenancePage("Validation complete.")); }
void handleRebuildDay() { bool ok = rebuildMapCache(true, false); runtimeServer.send(ok ? 200 : 500, "text/html", buildMapMaintenancePage(ok ? "Day cache rebuilt." : "Day cache rebuild failed.", !ok)); }
void handleRebuildNight() { bool ok = rebuildMapCache(false, true); runtimeServer.send(ok ? 200 : 500, "text/html", buildMapMaintenancePage(ok ? "Night cache rebuilt." : "Night cache rebuild failed.", !ok)); }
void handleRebuildBoth() { bool ok = rebuildMapCache(true, true); runtimeServer.send(ok ? 200 : 500, "text/html", buildMapMaintenancePage(ok ? "Both caches rebuilt." : "Cache rebuild failed.", !ok)); }
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
  row("Firmware", FIRMWARE_VERSION);
  row("Compiled", String(__DATE__) + " " + String(__TIME__));
  row("Uptime", String(millis() / 1000UL) + " sec");
  row("Free heap", String(ESP.getFreeHeap()) + " bytes");
  row("Minimum free heap", String(ESP.getMinFreeHeap()) + " bytes");
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
  page += F("</table><form method=\"post\" action=\"/maps/validate\"><button>Validate files</button></form>"
            "<form method=\"post\" action=\"/maps/rebuild-day\"><button>Rebuild day cache</button></form>"
            "<form method=\"post\" action=\"/maps/rebuild-night\"><button>Rebuild night cache</button></form>"
            "<form method=\"post\" action=\"/maps/rebuild-both\"><button>Rebuild both caches</button></form>"
            "<p class=\"note\">Rebuilding closes the active map files, converts the PNG, validates the 153,600-byte cache, reopens both maps, and redraws the clock.</p>");
  page += pageFooter(); return page;
}

void startRuntimeConfigServer() {
  if (runtimeServerStarted || WiFi.status() != WL_CONNECTED) return;
  runtimeServer.on(RUNTIME_CONFIG_PATH, HTTP_GET, sendRuntimePage);
  runtimeServer.on("/save", HTTP_POST, handleRuntimeSave);
  runtimeServer.on("/forget", HTTP_POST, handleForgetWifi);
  runtimeServer.on(DIAGNOSTICS_PATH, HTTP_GET, handleDiagnostics);
  runtimeServer.on(MAP_MAINTENANCE_PATH, HTTP_GET, handleMaps);
  runtimeServer.on(MAP_VALIDATE_PATH, HTTP_POST, handleValidateMaps);
  runtimeServer.on(MAP_REBUILD_DAY_PATH, HTTP_POST, handleRebuildDay);
  runtimeServer.on(MAP_REBUILD_NIGHT_PATH, HTTP_POST, handleRebuildNight);
  runtimeServer.on(MAP_REBUILD_BOTH_PATH, HTTP_POST, handleRebuildBoth);
  runtimeServer.onNotFound(sendRuntimePage);
  runtimeServer.begin(); runtimeServerStarted = true;
  Serial.print("Runtime configuration: http://"); Serial.println(WiFi.localIP());
}

void serviceRuntimeConfigServer() {
  if (!runtimeServerStarted && WiFi.status() == WL_CONNECTED) startRuntimeConfigServer();
  if (runtimeServerStarted) runtimeServer.handleClient();
}
