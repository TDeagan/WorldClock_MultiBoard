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
bool runtimePendingTimeZoneChanged = false;
bool runtimePendingClockDisplayChanged = false;
bool runtimePendingOrientationChanged = false;
bool runtimePendingLocationGridChanged = false;
unsigned long runtimeActionAt = 0;

enum class RuntimeMapAction {
  None,
  ApplyDayMap,
  RebuildDayMap,
  RebuildNight,
  RebuildAllDayMaps,
  RebuildEverything
};

RuntimeMapAction runtimeMapActionPending =
  RuntimeMapAction::None;

unsigned long runtimeMapActionAt = 0;
String runtimeMapResultMessage;
bool runtimeMapResultError = false;
String runtimeMapActionFilename;
bool runtimeFullMapValidationRequested = false;

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


String urlEncodeRuntime(const String &input) {
  static const char hex[] = "0123456789ABCDEF";
  String output;
  output.reserve(input.length() * 3);

  for (size_t index = 0; index < input.length(); ++index) {
    const uint8_t value = static_cast<uint8_t>(input[index]);

    if (
      (value >= 'a' && value <= 'z') ||
      (value >= 'A' && value <= 'Z') ||
      (value >= '0' && value <= '9') ||
      value == '-' || value == '_' || value == '.' || value == '~'
    ) {
      output += static_cast<char>(value);
    } else {
      output += '%';
      output += hex[(value >> 4) & 0x0F];
      output += hex[value & 0x0F];
    }
  }

  return output;
}

String pageHeader(const String &title) {
  String page;
  page.reserve(10000);
  page += F("<!doctype html><html><head><meta name=\"viewport\" "
            "content=\"width=device-width,initial-scale=1\"><meta charset=\"utf-8\">"
            "<style>body{font-family:Arial,sans-serif;background:#101820;color:#f2f4f7;"
            "margin:0;padding:20px}.card{max-width:900px;margin:auto;background:#1b2733;"
            "padding:22px;border-radius:12px}h1,h2{color:#ffd43b}a{color:#66d9ef}"
            "label{display:block;margin-top:14px;margin-bottom:6px;font-weight:bold}"
            "select,input{box-sizing:border-box;width:100%;padding:10px;border-radius:7px;"
            "border:1px solid #667;background:#fff;color:#111;font-size:1rem}"
            "button,.button{display:block;box-sizing:border-box;width:100%;margin-top:16px;"
            "padding:11px;border:0;border-radius:8px;background:#ffd43b;color:#111;"
            "font-weight:bold;text-align:center;text-decoration:none;font-size:1rem}"
            ".danger{background:#c94b50;color:white}"
            ".row{display:flex;gap:10px}.sign{width:86px;flex:0 0 86px}.hemisphere{width:125px;flex:0 0 125px}.grow{flex:1}"
            ".check{display:flex;align-items:center;gap:8px;margin-top:10px;font-weight:normal}"
            ".check input{width:auto;margin:0}.msg{padding:10px;border-radius:7px;"
            "margin-bottom:12px;background:#24435a}.err{background:#6b2632}"
            "table{width:100%;border-collapse:collapse}td{padding:7px;border-bottom:1px solid #40505f}"
            "td:first-child{color:#bec8d2}.nav{margin-bottom:16px}.nav a{margin-right:14px}"
            ".note{font-size:.9rem;color:#bec8d2;line-height:1.4}.map-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:14px}.map-card{border:1px solid #40505f;border-radius:10px;padding:12px;background:#14212c}.map-card.selected{border-color:#ffd43b}.map-card h3{overflow-wrap:anywhere;margin:10px 0}.map-thumb{display:block;width:100%;aspect-ratio:4/3;object-fit:cover;background:#05080b;border-radius:7px}.badge{display:inline-block;padding:4px 7px;border-radius:999px;margin:2px 4px 2px 0;font-size:.82rem;background:#40505f}.ok{background:#28603d}.bad{background:#7a2f3b}.selected-badge{background:#7b6418}.small-button{margin-top:9px;padding:8px;font-size:.9rem}.muted{opacity:.65}</style><title>");
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

  page += F("<h2>Active settings</h2><table>");

  auto summaryRow =
    [&](const String &name, const String &value) {
      page += F("<tr><td>");
      page += htmlEscapeRuntime(name);
      page += F("</td><td>");
      page += htmlEscapeRuntime(value);
      page += F("</td></tr>");
    };

  summaryRow(
    "Time zone",
    timeZoneDisplayName()
  );

  summaryRow(
    "Clock",
    String(clockFormatName()) +
      (timeSettings.showSeconds
        ? ", seconds shown"
        : ", seconds hidden")
  );

  summaryRow(
    "Orientation",
    displayOrientationName()
  );

  summaryRow(
    "Home marker",
    locationGridSettings.showHomeMarker
      ? formatHomeLocation()
      : "Off"
  );

  summaryRow(
    "Coordinate grid",
    locationGridSettings.showCoordinateGrid
      ? "On; 30-degree spacing"
      : "Off"
  );

  summaryRow(
    "Overlays",
    String("Sun ") +
      (overlaySettings.showSun ? "on" : "off") +
      ", Moon " +
      (overlaySettings.showMoon ? "on" : "off") +
      ", ISS " +
      (overlaySettings.showISS ? "on" : "off") +
      ", Track " +
      (overlaySettings.showIssTrack ? "on" : "off")
  );

  page += F("</table><h2>Edit settings</h2>");

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
    "<label>Time zone</label>"
    "<select id=\"timeZone\">"
  );

  for (
    uint8_t value = 0;
    value <= static_cast<uint8_t>(TimeZonePreset::USPacific);
    ++value
  ) {
    const TimeZonePreset preset =
      static_cast<TimeZonePreset>(value);

    page += F("<option value=\"");
    page += String(value);
    page += F("\"");

    if (timeSettings.timeZone == preset) {
      page += F(" selected");
    }

    page += F(">");
    page += timeZonePresetName(preset);
    page += F("</option>");
  }

  page += F(
    "</select>"
    "<label>Fixed UTC offset in hours</label>"
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
    "<p class=\"note\">The fixed offset is used only when Fixed UTC offset "
    "is selected. US presets change automatically for daylight-saving time.</p>"
    "<label>Clock format</label>"
    "<select id=\"use24Hour\">"
    "<option value=\"1\""
  );

  if (timeSettings.use24Hour) {
    page += F(" selected");
  }

  page += F(
    ">24-hour</option><option value=\"0\""
  );

  if (!timeSettings.use24Hour) {
    page += F(" selected");
  }

  page += F(
    ">12-hour</option></select>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showSeconds\""
  );

  if (timeSettings.showSeconds) {
    page += F(" checked");
  }

  page += F(
    ">Show seconds</label>"
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
    "<label>Home location and grid</label>"
    "<label>Latitude</label>"
    "<div class=\"row\">"
    "<select class=\"hemisphere\" id=\"homeLatitudeSign\">"
    "<option value=\"+\""
  );

  if (locationGridSettings.homeLatitude >= 0.0) {
    page += F(" selected");
  }

  page += F(
    ">North (+)</option><option value=\"-\""
  );

  if (locationGridSettings.homeLatitude < 0.0) {
    page += F(" selected");
  }

  page += F(
    ">South (-)</option></select>"
    "<input class=\"grow\" id=\"homeLatitudeMagnitude\" "
    "type=\"number\" min=\"0\" max=\"90\" step=\"any\" "
    "inputmode=\"decimal\" required value=\""
  );

  page += formatCoordinateInput(
    fabs(locationGridSettings.homeLatitude)
  );

  page += F(
    "\"></div>"
    "<label>Longitude</label>"
    "<div class=\"row\">"
    "<select class=\"hemisphere\" id=\"homeLongitudeSign\">"
    "<option value=\"+\""
  );

  if (locationGridSettings.homeLongitude >= 0.0) {
    page += F(" selected");
  }

  page += F(
    ">East (+)</option><option value=\"-\""
  );

  if (locationGridSettings.homeLongitude < 0.0) {
    page += F(" selected");
  }

  page += F(
    ">West (-)</option></select>"
    "<input class=\"grow\" id=\"homeLongitudeMagnitude\" "
    "type=\"number\" min=\"0\" max=\"180\" step=\"any\" "
    "inputmode=\"decimal\" required value=\""
  );

  page += formatCoordinateInput(
    fabs(locationGridSettings.homeLongitude)
  );

  page += F(
    "\"></div>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showHomeMarker\""
  );

  if (locationGridSettings.showHomeMarker) {
    page += F(" checked");
  }

  page += F(
    ">Show home-location marker</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showCoordinateGrid\""
  );

  if (locationGridSettings.showCoordinateGrid) {
    page += F(" checked");
  }

  page += F(
    ">Show 30-degree coordinate grid</label>"
    "<p class=\"note\">The Equator and Prime Meridian are emphasized. "
    "Coordinates use decimal degrees.</p>"
    "<label>Map overlays</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showSun\""
  );

  if (overlaySettings.showSun) {
    page += F(" checked");
  }

  page += F(
    ">Show Sun</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showMoon\""
  );

  if (overlaySettings.showMoon) {
    page += F(" checked");
  }

  page += F(
    ">Show Moon</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showISS\""
  );

  if (overlaySettings.showISS) {
    page += F(" checked");
  }

  page += F(
    ">Show ISS marker</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"showIssTrack\""
  );

  if (overlaySettings.showIssTrack) {
    page += F(" checked");
  }

  page += F(
    ">Show one-orbit ISS track</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"issTrackDotted\""
  );

  if (overlaySettings.issTrackDotted) {
    page += F(" checked");
  }

  page += F(
    ">Use dotted track</label>"
    "</div>"
    "<a class=\"button\" "
    "href=\"/apply-settings?source=fallback\" "
    "onclick=\"return wcApplySettings();\">"
    "Apply settings now"
    "</a>"
    "<a class=\"button danger\" href=\"/forget\">"
    "Forget Wi-Fi and start setup"
    "</a>"
    "<script>"
    "function wcApplySettings(){"
      "try{"
        "var q=[];"
        "function add(n,v){"
          "q.push(encodeURIComponent(n)+'='+encodeURIComponent(v));"
        "}"
        "add('timeZone',document.getElementById('timeZone').value);"
        "add('offsetSign',document.getElementById('offsetSign').value);"
        "add('offsetMagnitude',document.getElementById('offsetMagnitude').value);"
        "add('use24Hour',document.getElementById('use24Hour').value);"
        "add('flip180',document.getElementById('flip180').value);"
        "add('homeLatitudeSign',document.getElementById('homeLatitudeSign').value);"
        "add('homeLatitudeMagnitude',document.getElementById('homeLatitudeMagnitude').value);"
        "add('homeLongitudeSign',document.getElementById('homeLongitudeSign').value);"
        "add('homeLongitudeMagnitude',document.getElementById('homeLongitudeMagnitude').value);"
        "var checks=['showSeconds','showHomeMarker','showCoordinateGrid',"
          "'showSun','showMoon','showISS','showIssTrack','issTrackDotted'];"
        "for(var i=0;i<checks.length;i++){"
          "var e=document.getElementById(checks[i]);"
          "if(e&&e.checked){add(checks[i],'1');}"
        "}"
        "add('source','apply-link');"
        "window.location.href='/apply-settings?'+q.join('&');"
        "return false;"
      "}catch(error){return true;}"
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
    !runtimeServer.hasArg("timeZone") ||
    !runtimeServer.hasArg("offsetMagnitude") ||
    !runtimeServer.hasArg("offsetSign") ||
    !runtimeServer.hasArg("use24Hour") ||
    !runtimeServer.hasArg("flip180") ||
    !runtimeServer.hasArg("homeLatitudeSign") ||
    !runtimeServer.hasArg("homeLatitudeMagnitude") ||
    !runtimeServer.hasArg("homeLongitudeSign") ||
    !runtimeServer.hasArg("homeLongitudeMagnitude")
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "The Apply link reached the clock, but the browser did not "
        "supply all selected values.",
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
        "Invalid fixed UTC offset. Enter a value from 0 through 14 hours.",
        true
      )
    );
    return;
  }

  const int offsetMinutes =
    runtimeServer.arg("offsetSign") == "-"
      ? -magnitudeMinutes
      : magnitudeMinutes;

  const int timeZoneValue =
    runtimeServer.arg("timeZone").toInt();

  if (
    !timeZonePresetIsValid(
      static_cast<uint8_t>(timeZoneValue)
    )
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "Select a valid time zone.",
        true
      )
    );
    return;
  }

  const TimeZonePreset requestedTimeZone =
    static_cast<TimeZonePreset>(
      timeZoneValue
    );

  double requestedHomeLatitude = 0.0;
  double requestedHomeLongitude = 0.0;

  double latitudeMagnitude = 0.0;
  double longitudeMagnitude = 0.0;

  if (
    !parseCoordinateValue(
      runtimeServer.arg("homeLatitudeMagnitude"),
      0.0,
      90.0,
      latitudeMagnitude
    ) ||
    !parseCoordinateValue(
      runtimeServer.arg("homeLongitudeMagnitude"),
      0.0,
      180.0,
      longitudeMagnitude
    )
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "Enter a latitude magnitude from 0 through 90 and a longitude "
        "magnitude from 0 through 180.",
        true
      )
    );
    return;
  }

  requestedHomeLatitude =
    runtimeServer.arg("homeLatitudeSign") == "-"
      ? -latitudeMagnitude
      : latitudeMagnitude;

  requestedHomeLongitude =
    runtimeServer.arg("homeLongitudeSign") == "-"
      ? -longitudeMagnitude
      : longitudeMagnitude;

  const bool requested24Hour =
    runtimeServer.arg("use24Hour") != "0";

  const bool requestedShowSeconds =
    runtimeServer.hasArg("showSeconds");

  const bool requestedFlip180 =
    runtimeServer.arg("flip180") == "1";

  const bool requestedShowHomeMarker =
    runtimeServer.hasArg("showHomeMarker");

  const bool requestedShowCoordinateGrid =
    runtimeServer.hasArg("showCoordinateGrid");

  runtimePendingTimeZoneChanged =
    requestedTimeZone !=
      timeSettings.timeZone ||
    (
      requestedTimeZone ==
        TimeZonePreset::FixedOffset &&
      offsetMinutes !=
        networkSettings.utcOffsetMinutes
    );

  runtimePendingClockDisplayChanged =
    requested24Hour !=
      timeSettings.use24Hour ||
    requestedShowSeconds !=
      timeSettings.showSeconds;

  runtimePendingOrientationChanged =
    requestedFlip180 !=
      displaySettings.flip180;

  runtimePendingLocationGridChanged =
    fabs(
      requestedHomeLatitude -
      locationGridSettings.homeLatitude
    ) > 0.0000005 ||
    fabs(
      requestedHomeLongitude -
      locationGridSettings.homeLongitude
    ) > 0.0000005 ||
    requestedShowHomeMarker !=
      locationGridSettings.showHomeMarker ||
    requestedShowCoordinateGrid !=
      locationGridSettings.showCoordinateGrid;

  networkSettings.utcOffsetMinutes =
    offsetMinutes;

  timeSettings.timeZone =
    requestedTimeZone;

  timeSettings.use24Hour =
    requested24Hour;

  timeSettings.showSeconds =
    requestedShowSeconds;

  displaySettings.flip180 =
    requestedFlip180;

  locationGridSettings.homeLatitude =
    requestedHomeLatitude;

  locationGridSettings.homeLongitude =
    requestedHomeLongitude;

  locationGridSettings.showHomeMarker =
    requestedShowHomeMarker;

  locationGridSettings.showCoordinateGrid =
    requestedShowCoordinateGrid;

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

  const bool timeSaved =
    saveTimeSettings();

  const bool overlaysSaved =
    saveOverlaySettings();

  const bool displaySaved =
    saveDisplaySettings();

  const bool locationSaved =
    saveLocationGridSettings();

  Serial.printf(
    "Runtime web: persistence results; "
    "offset=%d time=%d overlays=%d display=%d location=%d\n",
    offsetSaved,
    timeSaved,
    overlaysSaved,
    displaySaved,
    locationSaved
  );

  runtimeApplyPending = true;
  runtimeActionAt = millis() + 250UL;

  const bool persistenceFailed =
    !offsetSaved ||
    !timeSaved ||
    !overlaysSaved ||
    !displaySaved ||
    !locationSaved;

  String message =
    "Settings accepted. The display is updating now.";

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

  runtimeFullMapValidationRequested = true;
  const String page = buildMapMaintenancePage(
    "Full PNG and CRC validation complete."
  );
  runtimeFullMapValidationRequested = false;

  sendRuntimeHtml(200, page);
}


void sendMapPngFile(
  const String &path
) {
  File image = SD.open(path.c_str(), FILE_READ);

  if (!image) {
    runtimeServer.send(404, "text/plain", "Map image not found");
    return;
  }

  runtimeServer.sendHeader(
    "Cache-Control",
    "private, max-age=60"
  );

  runtimeServer.streamFile(
    image,
    "image/png"
  );

  image.close();
}


void handleMapPreview() {
  const String filename = runtimeServer.arg("file");

  if (!isSafeDayMapFilename(filename)) {
    runtimeServer.send(400, "text/plain", "Invalid map filename");
    return;
  }

  sendMapPngFile(
    dayMapPngPath(filename)
  );
}


void handleNightMapPreview() {
  sendMapPngFile(NIGHT_PNG_FILE);
}


void scheduleMapAction(
  RuntimeMapAction action,
  const String &label,
  const String &filename = ""
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
  runtimeMapActionFilename = filename;
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
        "<p>This page will return to Maps automatically.</p>"
        "<p><a href=\"/maps\">Check status now</a></p>") +
      pageFooter()
  );
}


void handleSelectMap() {
  const String filename = runtimeServer.arg("file");

  if (
    !isSafeDayMapFilename(filename) ||
    !pngMapIsValid(
      dayMapPngPath(filename).c_str(),
      false
    )
  ) {
    sendRuntimeHtml(
      400,
      buildMapMaintenancePage(
        "The selected daylight map is missing or invalid.",
        true
      )
    );
    return;
  }

  scheduleMapAction(
    RuntimeMapAction::ApplyDayMap,
    String("Apply ") + filename,
    filename
  );
}


void handleRebuildOneMap() {
  const String filename = runtimeServer.arg("file");

  if (
    !isSafeDayMapFilename(filename) ||
    !SD.exists(dayMapPngPath(filename).c_str())
  ) {
    sendRuntimeHtml(
      400,
      buildMapMaintenancePage(
        "The requested daylight map was not found.",
        true
      )
    );
    return;
  }

  scheduleMapAction(
    RuntimeMapAction::RebuildDayMap,
    String("Rebuild cache for ") + filename,
    filename
  );
}


void handleRebuildDay() {
  scheduleMapAction(
    RuntimeMapAction::RebuildDayMap,
    String("Rebuild cache for ") + selectedDayMapFilename,
    selectedDayMapFilename
  );
}


void handleRebuildNight() {
  scheduleMapAction(
    RuntimeMapAction::RebuildNight,
    "Shared night-map cache rebuild"
  );
}


void handleRebuildAllDayMaps() {
  scheduleMapAction(
    RuntimeMapAction::RebuildAllDayMaps,
    "All daylight-map caches rebuild"
  );
}


void handleRebuildBoth() {
  scheduleMapAction(
    RuntimeMapAction::RebuildEverything,
    "All daylight and shared night caches rebuild"
  );
}
} // namespace

String buildDiagnosticsPage() {
  systemStatus.wifiConnected =
    WiFi.status() == WL_CONNECTED;

  systemStatus.issPositionValid =
    issPosition.valid;

  systemStatus.orbitTrackValid =
    issTrackValid;

  const time_t now =
    time(nullptr);

  tm localTm {};
  localtime_r(&now, &localTm);

  char zoneAbbreviation[16] = "";
  char numericOffset[16] = "";

  strftime(
    zoneAbbreviation,
    sizeof(zoneAbbreviation),
    "%Z",
    &localTm
  );

  strftime(
    numericOffset,
    sizeof(numericOffset),
    "%z",
    &localTm
  );

  String page =
    pageHeader("Diagnostics");

  page += F("<table>");

  auto row =
    [&](const String &name, const String &value) {
      page += F("<tr><td>");
      page += htmlEscapeRuntime(name);
      page += F("</td><td>");
      page += htmlEscapeRuntime(value);
      page += F("</td></tr>");
    };

  row("Board profile", ACTIVE_BOARD.name);
  row("Display dimensions", String(lcd.width()) + " x " + String(lcd.height()));
  row("Native LCD rotation", String(DISPLAY_ROTATION));
  row("Configured orientation", displayOrientationName());
  row("Effective LCD rotation", String(effectiveDisplayRotation()));
  row(
    "Home marker",
    locationGridSettings.showHomeMarker
      ? "Shown"
      : "Hidden"
  );
  row("Home latitude", formatLatitude(locationGridSettings.homeLatitude));
  row("Home longitude", formatLongitude(locationGridSettings.homeLongitude));
  row(
    "Coordinate grid",
    locationGridSettings.showCoordinateGrid
      ? "Shown; 30-degree spacing"
      : "Hidden"
  );
  row("Firmware", FIRMWARE_VERSION);
  row("Build timestamp", String(__DATE__) + " " + String(__TIME__));
  row("Uptime", formatUptime(millis()));
  row("Last reset reason", resetReasonName(esp_reset_reason()));
  row("Time zone setting", timeZoneDisplayName());
  row("Current zone", String(zoneAbbreviation) + " " + numericOffset);
  row("Clock format", clockFormatName());
  row("Seconds", timeSettings.showSeconds ? "Shown" : "Hidden");
  row("Free heap", String(ESP.getFreeHeap()) + " bytes");
  row("Minimum free heap", String(ESP.getMinFreeHeap()) + " bytes");
  row("Largest free heap block", String(ESP.getMaxAllocHeap()) + " bytes");
  row("Flash size", String(ESP.getFlashChipSize()) + " bytes");
  row("Sketch size", String(ESP.getSketchSize()) + " bytes");
  row("Free sketch space", String(ESP.getFreeSketchSpace()) + " bytes");
  row("Wi-Fi", systemStatus.wifiConnected ? "Connected" : "Disconnected");
  row("SSID", WiFi.SSID());
  row("IP", WiFi.localIP().toString());
  row("Wi-Fi signal", String(WiFi.RSSI()) + " dBm");
  row("NTP", systemStatus.ntpSynced ? "Synchronized" : "Unavailable");
  row("Last NTP sync", formatAge(systemStatus.lastNtpSync));
  row("Last full map render", formatElapsedAge(lastMapUpdate));
  row("microSD", systemStatus.sdMounted ? "Mounted" : "Unavailable");
  row("Selected daylight map", selectedDayMapFilename);
  row("Day PNG path", activeDayPngPath);
  row("Day cache path", activeDayRawPath);
  row("Day PNG", systemStatus.dayPngFound ? "Found" : "Missing");
  row("Night PNG", systemStatus.nightPngFound ? "Found" : "Missing");
  row("Day cache", systemStatus.dayCacheValid ? "Valid" : "Invalid");
  row("Night cache", systemStatus.nightCacheValid ? "Valid" : "Invalid");
  row("ISS position", issPosition.valid ? "Valid" : "Unavailable");
  row("ISS data age", formatAge(issPosition.updatedAt));
  row("Orbit track", issTrackValid ? "Valid" : "Unavailable");
  row("Sun/Moon/ISS/Track", String(overlaySettings.showSun) + "/" + String(overlaySettings.showMoon) + "/" + String(overlaySettings.showISS) + "/" + String(overlaySettings.showIssTrack));
  row("Last error", systemErrorName(systemStatus.lastError));
  row("Error detail", systemStatus.lastErrorText.length() ? systemStatus.lastErrorText : "None");

  page += F("</table><p><a href=\"/diagnostics\">Refresh</a></p>");
  page += pageFooter();
  return page;
}

String buildMapMaintenancePage(
  const String &message,
  bool error
) {
  refreshStorageStatus();

  DaylightMapInfo maps[MAX_DAYLIGHT_MAPS];
  size_t totalMapCount = 0;
  const size_t mapCount = scanDaylightMaps(
    maps,
    MAX_DAYLIGHT_MAPS,
    runtimeFullMapValidationRequested,
    &totalMapCount
  );

  const bool nightPngExists =
    SD.exists(NIGHT_PNG_FILE);

  const bool nightPngValid =
    nightPngExists &&
    pngMapIsValid(
      NIGHT_PNG_FILE,
      runtimeFullMapValidationRequested
    );

  const bool nightCacheValid =
    rawMapIsValid(NIGHT_RAW_FILE);

  String page = pageHeader("Maps");

  if (message.length()) {
    page += F("<div class=\"msg");

    if (error) {
      page += F(" err");
    }

    page += F("\">");
    page += htmlEscapeRuntime(message);
    page += F("</div>");
  }

  page += F(
    "<p class=\"note\">Daylight maps are discovered from <strong>/maps/</strong>. "
    "Each PNG uses a same-name RGB565 cache. The night image and cache are shared "
    "by every daylight map.</p>"
    "<table>"
  );

  auto row = [&](const String &name, const String &value) {
    page += F("<tr><td>");
    page += htmlEscapeRuntime(name);
    page += F("</td><td>");
    page += htmlEscapeRuntime(value);
    page += F("</td></tr>");
  };

  row("Selected daylight map", selectedDayMapFilename);
  row("Expected cache size", String(RAW_MAP_BYTES) + " bytes");
  row("Maps discovered", String(totalMapCount));
  row(
    "Shared night source",
    nightPngExists
      ? String(fileSizeOrZero(NIGHT_PNG_FILE)) + " bytes"
      : "Missing"
  );
  row(
    "Shared night cache",
    nightCacheValid
      ? String(fileSizeOrZero(NIGHT_RAW_FILE)) + " bytes; valid"
      : "Invalid or missing"
  );

  page += F(
    "</table>"
    "<a class=\"button\" href=\"/maps/validate\">Validate every PNG with CRC</a>"
    "<a class=\"button\" href=\"/maps/rebuild-all\">Rebuild all daylight caches</a>"
    "<a class=\"button\" href=\"/maps/rebuild-both\">Rebuild every cache</a>"
    "<h2>Daylight maps</h2>"
  );

  if (mapCount == 0) {
    page += F(
      "<div class=\"msg err\">No .png files were found in /maps, or none could "
      "be read from the microSD card.</div>"
    );
  } else {
    page += F("<div class=\"map-grid\">");

    for (size_t index = 0; index < mapCount; ++index) {
      const DaylightMapInfo &map = maps[index];
      const String encodedFilename =
        urlEncodeRuntime(map.filename);

      page += F("<div class=\"map-card");

      if (map.selected) {
        page += F(" selected");
      }

      page += F("\"><img class=\"map-thumb\" loading=\"lazy\" src=\"");
      page += MAP_PREVIEW_PATH;
      page += F("?file=");
      page += encodedFilename;
      page += F("\" alt=\"");
      page += htmlEscapeRuntime(map.filename);
      page += F(" preview\"><h3>");
      page += htmlEscapeRuntime(map.filename);
      page += F("</h3>");

      if (map.selected) {
        page += F("<span class=\"badge selected-badge\">Selected</span>");
      }

      if (map.pngValid) {
        page += runtimeFullMapValidationRequested
          ? F("<span class=\"badge ok\">PNG + CRC valid</span>")
          : F("<span class=\"badge ok\">PNG geometry valid</span>");
      } else {
        page += F("<span class=\"badge bad\">PNG invalid</span>");
      }

      page += map.cacheValid
        ? F("<span class=\"badge ok\">Cache valid</span>")
        : F("<span class=\"badge bad\">Cache missing/invalid</span>");

      page += F("<table><tr><td>PNG</td><td>");
      page += String(map.pngBytes);
      page += F(" bytes</td></tr><tr><td>Cache</td><td>");

      if (map.rawBytes > 0) {
        page += String(map.rawBytes);
        page += F(" bytes");
      } else {
        page += F("Not present");
      }

      page += F("</td></tr></table>");

      if (map.pngValid) {
        if (!map.selected) {
          page += F("<a class=\"button small-button\" href=\"");
          page += MAP_SELECT_PATH;
          page += F("?file=");
          page += encodedFilename;
          page += F("\">Select and apply</a>");
        }

        page += F("<a class=\"button small-button\" href=\"");
        page += MAP_REBUILD_ONE_PATH;
        page += F("?file=");
        page += encodedFilename;
        page += F("\">Rebuild this cache</a>");
      } else {
        page += F(
          "<p class=\"note\">Not selectable. The PNG must decode successfully, "
          "be exactly 320 × 240, and be non-interlaced.</p>"
        );
      }

      page += F("</div>");
    }

    page += F("</div>");
  }

  if (totalMapCount > mapCount) {
    page += F(
      "<p class=\"note\">The page displays the first "
    );
    page += String(MAX_DAYLIGHT_MAPS);
    page += F(
      " PNG files alphabetically. Remove unused files if more are present.</p>"
    );
  }

  page += F(
    "<h2>Shared night map</h2>"
    "<div class=\"map-card\">"
    "<img class=\"map-thumb\" loading=\"lazy\" src=\"/maps/night-preview\" "
    "alt=\"earth_night.png preview\">"
    "<h3>earth_night.png</h3>"
  );

  if (nightPngValid) {
    page += runtimeFullMapValidationRequested
      ? F("<span class=\"badge ok\">PNG + CRC valid</span>")
      : F("<span class=\"badge ok\">PNG geometry valid</span>");
  } else {
    page += F("<span class=\"badge bad\">PNG invalid/missing</span>");
  }

  page += nightCacheValid
    ? F("<span class=\"badge ok\">Cache valid</span>")
    : F("<span class=\"badge bad\">Cache missing/invalid</span>");

  page += F(
    "<a class=\"button small-button\" href=\"/maps/rebuild-night\">"
    "Rebuild shared night cache</a></div>"
    "<p class=\"note\">Selecting a daylight map stores only its filename in "
    "Preferences. If that file is later missing or invalid, the firmware falls "
    "back to earth_day.png when available, otherwise to the alphabetically first "
    "valid PNG in /maps.</p>"
  );

  page += pageFooter();
  return page;
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
    MAP_PREVIEW_PATH,
    HTTP_GET,
    handleMapPreview
  );
  runtimeServer.on(
    MAP_NIGHT_PREVIEW_PATH,
    HTTP_GET,
    handleNightMapPreview
  );
  runtimeServer.on(
    MAP_SELECT_PATH,
    HTTP_GET,
    handleSelectMap
  );
  runtimeServer.on(
    MAP_REBUILD_ONE_PATH,
    HTTP_GET,
    handleRebuildOneMap
  );
  runtimeServer.on(
    MAP_REBUILD_ALL_PATH,
    HTTP_GET,
    handleRebuildAllDayMaps
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

    const bool timeZoneChanged =
      runtimePendingTimeZoneChanged;

    const bool clockDisplayChanged =
      runtimePendingClockDisplayChanged;

    const bool orientationChanged =
      runtimePendingOrientationChanged;

    const bool locationGridChanged =
      runtimePendingLocationGridChanged;

    runtimePendingTimeZoneChanged = false;
    runtimePendingClockDisplayChanged = false;
    runtimePendingOrientationChanged = false;
    runtimePendingLocationGridChanged = false;

    Serial.printf(
      "Runtime web: applying settings; "
      "timeZoneChanged=%d clockDisplayChanged=%d "
      "orientationChanged=%d locationGridChanged=%d\n",
      timeZoneChanged,
      clockDisplayChanged,
      orientationChanged,
      locationGridChanged
    );

    if (timeZoneChanged) {
      applyConfiguredTimeZone();
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

    const String actionFilename =
      runtimeMapActionFilename;

    runtimeMapActionFilename = "";

    switch (action) {
      case RuntimeMapAction::ApplyDayMap:
        Serial.printf(
          "Runtime web: applying daylight map %s\n",
          actionFilename.c_str()
        );

        ok = activateDayMap(
          actionFilename,
          false
        );

        runtimeMapResultMessage =
          ok
            ? actionFilename + " is now selected."
            : "The daylight map could not be applied.";
        break;

      case RuntimeMapAction::RebuildDayMap:
        Serial.printf(
          "Runtime web: rebuilding cache for %s\n",
          actionFilename.c_str()
        );

        ok = rebuildDayMapCache(
          actionFilename
        );

        runtimeMapResultMessage =
          ok
            ? String("Cache rebuilt for ") + actionFilename + "."
            : String("Cache rebuild failed for ") + actionFilename + ".";
        break;

      case RuntimeMapAction::RebuildNight:
        Serial.println(
          "Runtime web: rebuilding shared night cache"
        );

        ok = rebuildMapCache(
          false,
          true
        );

        runtimeMapResultMessage =
          ok
            ? "Shared night cache rebuilt."
            : "Shared night cache rebuild failed.";
        break;

      case RuntimeMapAction::RebuildAllDayMaps:
        Serial.println(
          "Runtime web: rebuilding all daylight caches"
        );

        ok = rebuildAllDayMapCaches();

        runtimeMapResultMessage =
          ok
            ? "All valid daylight-map caches were rebuilt."
            : "One or more daylight caches could not be rebuilt.";
        break;

      case RuntimeMapAction::RebuildEverything:
        Serial.println(
          "Runtime web: rebuilding every map cache"
        );

        ok = rebuildAllDayMapCaches();

        if (ok) {
          ok = rebuildMapCache(
            false,
            true
          );
        }

        runtimeMapResultMessage =
          ok
            ? "All daylight and shared night caches were rebuilt."
            : "One or more map caches could not be rebuilt.";
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
