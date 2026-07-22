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
bool runtimePendingBrightnessChanged = false;
bool runtimePendingMapTuningChanged = false;
bool runtimePendingLocationGridChanged = false;
bool runtimePendingHomeCoordinatesChanged = false;
bool runtimePendingWeatherChanged = false;
bool runtimePendingMarketChanged = false;
unsigned long runtimeActionAt = 0;

bool runtimeTouchCalibrationPending = false;
unsigned long runtimeTouchCalibrationAt = 0;
String runtimeTouchCalibrationResultMessage;
bool runtimeTouchCalibrationResultError = false;

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
            ".note{font-size:.9rem;color:#bec8d2;line-height:1.4}.map-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:14px}.map-card{border:1px solid #40505f;border-radius:10px;padding:12px;background:#14212c}.map-card.selected{border-color:#ffd43b}.map-card h3{overflow-wrap:anywhere;margin:10px 0}.map-thumb{display:block;width:100%;aspect-ratio:4/3;object-fit:cover;background:#05080b;border-radius:7px}.badge{display:inline-block;padding:4px 7px;border-radius:999px;margin:2px 4px 2px 0;font-size:.82rem;background:#40505f}.ok{background:#28603d}.bad{background:#7a2f3b}.selected-badge{background:#7b6418}.small-button{margin-top:9px;padding:8px;font-size:.9rem}.muted{opacity:.65}.radar-image{display:block;width:256px;max-width:100%;height:auto;margin:12px auto;background:#14212c;border:1px solid #40505f;border-radius:7px}.market-chart{display:block;width:100%;height:auto;margin:12px auto;background:#101820;border:1px solid #40505f;border-radius:7px}.mood{font-weight:bold;font-size:1.1rem}</style><title>");
  page += htmlEscapeRuntime(title);
  page += F("</title></head><body><div class=\"card\"><div class=\"nav\">"
            "<a href=\"/\">Settings</a>");

  if (homeLocationIsConfigured()) {
    page += F("<a href=\"/weather\">Weather</a>");
  }

  if (marketSettings.enabled) {
    page += F("<a href=\"/market\">Market Mood</a>");
  }

  page += F("<a href=\"/diagnostics\">Diagnostics</a>"
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
  return saveUtcOffsetPreference(
    offsetMinutes
  );
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
    "Backlight brightness",
    displayBrightnessText()
  );

  summaryRow(
    "Map gamma",
    String("day ") + mapGammaText(displaySettings.dayMapGamma) +
      ", night " + mapGammaText(displaySettings.nightMapGamma)
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
    "Weather",
    !weatherSettings.enabled
      ? "Disabled"
      : !homeLocationIsConfigured()
          ? "Waiting for a saved location"
          : weatherSettings.useFahrenheit
              ? "Enabled; Fahrenheit"
              : "Enabled; Celsius"
  );

  summaryRow(
    "Market Mood",
    !marketSettings.enabled
      ? "Disabled"
      : marketDataAvailable()
          ? String(marketMoodLabel(currentMarketMoodLevel())) +
              "; " + marketPercentText(marketSnapshot.currentMoodBasisPoints)
          : "Enabled; waiting for data"
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
    "<label>Backlight brightness (16-255)</label>"
    "<input id=\"brightness\" type=\"number\" min=\"16\" max=\"255\" "
    "step=\"1\" inputmode=\"numeric\" value=\""
  );

  page += String(displaySettings.brightness);

  page += F(
    "\">"
    "<label>Day map gamma (0.50-2.00)</label>"
    "<input id=\"dayMapGamma\" type=\"number\" min=\"0.50\" max=\"2.00\" "
    "step=\"0.01\" inputmode=\"decimal\" value=\""
  );

  page += mapGammaText(displaySettings.dayMapGamma);

  page += F(
    "\">"
    "<label>Night map gamma (0.50-2.00)</label>"
    "<input id=\"nightMapGamma\" type=\"number\" min=\"0.50\" max=\"2.00\" "
    "step=\"0.01\" inputmode=\"decimal\" value=\""
  );

  page += mapGammaText(displaySettings.nightMapGamma);

  page += F(
    "\">"
    "<p class=\"note\">Gamma above 1.00 darkens map midtones; below 1.00 "
    "lifts shadow detail. Changing gamma creates new board-specific RGB565 caches.</p>"
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
    "<label>Saved-location weather</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"weatherEnabled\""
  );

  if (weatherSettings.enabled && homeLocationIsConfigured()) {
    page += F(" checked");
  }

  if (!homeLocationIsConfigured()) {
    page += F(" disabled");
  }

  page += F(
    ">Enable weather and regional radar</label>"
    "<label>Temperature units</label>"
    "<select id=\"weatherUnit\">"
    "<option value=\"F\""
  );

  if (weatherSettings.useFahrenheit) {
    page += F(" selected");
  }

  page += F(
    ">Fahrenheit / mph / inches</option>"
    "<option value=\"C\""
  );

  if (!weatherSettings.useFahrenheit) {
    page += F(" selected");
  }

  page += F(
    ">Celsius / km/h / millimeters</option>"
    "</select>"
    "<p class=\"note\">Weather uses the saved latitude and longitude. "
    "Coordinates 0,0 mean no location: weather is disabled and hidden. "
    "Saving any other valid location automatically enables weather.</p>"
    "<label>Market Mood</label>"
    "<label class=\"check\">"
    "<input type=\"checkbox\" id=\"marketEnabled\""
  );

  if (marketSettings.enabled) {
    page += F(" checked");
  }

  page += F(
    ">Enable SPY / QQQ / IWM Market Mood</label>"
    "<p class=\"note\">Uses a no-key, best-effort delayed market feed. "
    "The mood graphic is descriptive only and is not investment advice.</p>"
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
        "add('brightness',document.getElementById('brightness').value);"
        "add('dayMapGamma',document.getElementById('dayMapGamma').value);"
        "add('nightMapGamma',document.getElementById('nightMapGamma').value);"
        "add('homeLatitudeSign',document.getElementById('homeLatitudeSign').value);"
        "add('homeLatitudeMagnitude',document.getElementById('homeLatitudeMagnitude').value);"
        "add('homeLongitudeSign',document.getElementById('homeLongitudeSign').value);"
        "add('homeLongitudeMagnitude',document.getElementById('homeLongitudeMagnitude').value);"
        "add('weatherUnit',document.getElementById('weatherUnit').value);"
        "var checks=['showSeconds','showHomeMarker','showCoordinateGrid','weatherEnabled','marketEnabled',"
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
    !runtimeServer.hasArg("brightness") ||
    !runtimeServer.hasArg("dayMapGamma") ||
    !runtimeServer.hasArg("nightMapGamma") ||
    !runtimeServer.hasArg("homeLatitudeSign") ||
    !runtimeServer.hasArg("homeLatitudeMagnitude") ||
    !runtimeServer.hasArg("homeLongitudeSign") ||
    !runtimeServer.hasArg("homeLongitudeMagnitude") ||
    !runtimeServer.hasArg("weatherUnit")
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

  const int requestedBrightness =
    runtimeServer.arg("brightness").toInt();

  double requestedDayGammaValue = 0.0;
  double requestedNightGammaValue = 0.0;

  if (
    requestedBrightness < DISPLAY_BRIGHTNESS_MIN ||
    requestedBrightness > DISPLAY_BRIGHTNESS_MAX ||
    !parseCoordinateValue(
      runtimeServer.arg("dayMapGamma"),
      static_cast<double>(MAP_GAMMA_MIN) / 100.0,
      static_cast<double>(MAP_GAMMA_MAX) / 100.0,
      requestedDayGammaValue
    ) ||
    !parseCoordinateValue(
      runtimeServer.arg("nightMapGamma"),
      static_cast<double>(MAP_GAMMA_MIN) / 100.0,
      static_cast<double>(MAP_GAMMA_MAX) / 100.0,
      requestedNightGammaValue
    )
  ) {
    sendRuntimeHtml(
      400,
      runtimeConfigurationPage(
        "Brightness must be 16-255 and map gamma must be 0.50-2.00.",
        true
      )
    );
    return;
  }

  const uint16_t requestedDayMapGamma =
    static_cast<uint16_t>(lround(requestedDayGammaValue * 100.0));

  const uint16_t requestedNightMapGamma =
    static_cast<uint16_t>(lround(requestedNightGammaValue * 100.0));

  const bool requestedShowHomeMarker =
    runtimeServer.hasArg("showHomeMarker");

  const bool requestedShowCoordinateGrid =
    runtimeServer.hasArg("showCoordinateGrid");

  const bool requestedWeatherEnabled =
    runtimeServer.hasArg("weatherEnabled");

  const bool requestedWeatherFahrenheit =
    runtimeServer.arg("weatherUnit") != "C";

  const bool requestedMarketEnabled =
    runtimeServer.hasArg("marketEnabled");

  const bool previouslyConfiguredLocation =
    homeLocationIsConfigured();

  const bool requestedLocationConfigured =
    coordinatesRepresentHomeLocation(
      requestedHomeLatitude,
      requestedHomeLongitude
    );

  const bool effectiveWeatherEnabled =
    !requestedLocationConfigured
      ? false
      : !previouslyConfiguredLocation
          ? true
          : requestedWeatherEnabled;

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

  runtimePendingBrightnessChanged =
    requestedBrightness !=
      displaySettings.brightness;

  runtimePendingMapTuningChanged =
    requestedDayMapGamma !=
      displaySettings.dayMapGamma ||
    requestedNightMapGamma !=
      displaySettings.nightMapGamma;

  runtimePendingHomeCoordinatesChanged =
    fabs(
      requestedHomeLatitude -
      locationGridSettings.homeLatitude
    ) > 0.0000005 ||
    fabs(
      requestedHomeLongitude -
      locationGridSettings.homeLongitude
    ) > 0.0000005 ||
    requestedLocationConfigured !=
      previouslyConfiguredLocation;

  runtimePendingLocationGridChanged =
    runtimePendingHomeCoordinatesChanged ||
    requestedShowHomeMarker !=
      locationGridSettings.showHomeMarker ||
    requestedShowCoordinateGrid !=
      locationGridSettings.showCoordinateGrid;

  runtimePendingWeatherChanged =
    effectiveWeatherEnabled != weatherSettings.enabled ||
    requestedWeatherFahrenheit != weatherSettings.useFahrenheit ||
    runtimePendingHomeCoordinatesChanged;

  runtimePendingMarketChanged =
    requestedMarketEnabled != marketSettings.enabled;

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

  displaySettings.brightness =
    static_cast<uint8_t>(requestedBrightness);

  displaySettings.dayMapGamma =
    requestedDayMapGamma;

  displaySettings.nightMapGamma =
    requestedNightMapGamma;

  locationGridSettings.homeLatitude =
    requestedHomeLatitude;

  locationGridSettings.homeLongitude =
    requestedHomeLongitude;

  locationGridSettings.homeLocationConfigured =
    requestedLocationConfigured;

  weatherSettings.enabled = effectiveWeatherEnabled;
  weatherSettings.useFahrenheit = requestedWeatherFahrenheit;

  marketSettings.enabled = requestedMarketEnabled;

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

  const bool weatherSaved =
    saveWeatherSettings();

  const bool marketSaved =
    saveMarketSettings();

  Serial.printf(
    "Runtime web: persistence results; "
    "offset=%d time=%d overlays=%d display=%d location=%d weather=%d market=%d\n",
    offsetSaved,
    timeSaved,
    overlaysSaved,
    displaySaved,
    locationSaved,
    weatherSaved,
    marketSaved
  );

  runtimeApplyPending = true;
  runtimeActionAt = millis() + 250UL;

  const bool persistenceFailed =
    !offsetSaved ||
    !timeSaved ||
    !overlaysSaved ||
    !displaySaved ||
    !locationSaved ||
    !weatherSaved ||
    !marketSaved;

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
  const String page =
    buildDiagnosticsPage();

  runtimeTouchCalibrationResultMessage = "";
  runtimeTouchCalibrationResultError = false;

  sendRuntimeHtml(
    200,
    page
  );
}

void handleTouchRecalibrate() {
  Serial.printf(
    "Runtime web: %s %s\n",
    runtimeMethodName(),
    runtimeServer.uri().c_str()
  );

  if (
    !ACTIVE_BOARD.touchAvailable ||
    !touchHardwareIsReady()
  ) {
    sendRuntimeHtml(
      409,
      pageHeader("Touch Calibration") +
        F("<div class=\"msg err\">Touch hardware is not available "
          "for this board profile.</div>"
          "<p><a href=\"/diagnostics\">Return to Diagnostics</a></p>") +
        pageFooter()
    );
    return;
  }

  if (runtimeTouchCalibrationPending) {
    sendRuntimeHtml(
      409,
      pageHeader("Touch Calibration") +
        F("<div class=\"msg err\">Touch calibration is already scheduled.</div>"
          "<p><a href=\"/diagnostics\">Return to Diagnostics</a></p>") +
        pageFooter()
    );
    return;
  }

  runtimeTouchCalibrationPending = true;
  runtimeTouchCalibrationAt = millis() + 300UL;

  sendRuntimeHtml(
    202,
    pageHeader("Touch Calibration") +
      F("<div class=\"msg\">Calibration is starting on the World Clock display.</div>"
        "<p>Touch the four corner targets and then the center target. "
        "Hold the physical BOOT button for three seconds to cancel.</p>"
        "<p>If touch was disabled during first setup, completing this "
        "calibration re-enables all touchscreen functions.</p>"
        "<p>No Wi-Fi, map, location, overlay, clock, or other World Clock "
        "settings are erased by this operation.</p>"
        "<p>After completing or cancelling calibration, return to "
        "<a href=\"/diagnostics\">Diagnostics</a>.</p>") +
      pageFooter()
  );
}


void handleDisplayTest() {
  Serial.printf(
    "Runtime web: %s %s\n",
    runtimeMethodName(),
    runtimeServer.uri().c_str()
  );

  showTouchUiDisplayTest();

  sendRuntimeHtml(
    200,
    pageHeader("Display Test") +
      F("<div class=\"msg\">The grayscale, near-black, near-white, and "
        "RGB test pattern is now shown on the World Clock display.</div>"
        "<p>Use the touchscreen LIGHT controls or the Settings page to adjust "
        "brightness. Day and night gamma are applied when map caches are built.</p>"
        "<a class=\"button\" href=\"/diagnostics/display-test/clock\">"
        "Return display to clock</a>"
        "<a class=\"button\" href=\"/diagnostics\">Return to Diagnostics</a>") +
      pageFooter()
  );
}


void handleDisplayTestClock() {
  closeTouchUi(true);

  sendRuntimeHtml(
    200,
    pageHeader("Display Test") +
      F("<div class=\"msg\">The World Clock display has returned to the clock.</div>"
        "<p><a href=\"/diagnostics\">Return to Diagnostics</a></p>") +
      pageFooter()
  );
}

String weatherRadarWebFrameLabel(time_t timestamp) {
  if (timestamp <= 0) {
    return "--";
  }

  tm localTm {};
  localtime_r(&timestamp, &localTm);
  char label[32];

  if (timeSettings.use24Hour) {
    strftime(label, sizeof(label), "%H:%M", &localTm);
  } else {
    strftime(label, sizeof(label), "%I:%M %p", &localTm);

    if (label[0] == '0') {
      memmove(label, label + 1, strlen(label));
    }
  }

  return String(label);
}

String buildWeatherPage(
  const String &message = "",
  bool error = false
) {
  String pageTitle = "Weather - ";
  pageTitle += weatherLocationName();
  String page = pageHeader(pageTitle);

  if (message.length()) {
    page += F("<div class=\"msg");

    if (error) {
      page += F(" err");
    }

    page += F("\">");
    page += htmlEscapeRuntime(message);
    page += F("</div>");
  }

  page += F("<table>");

  auto row =
    [&](const String &name, const String &value) {
      page += F("<tr><td>");
      page += htmlEscapeRuntime(name);
      page += F("</td><td>");
      page += htmlEscapeRuntime(value);
      page += F("</td></tr>");
    };

  row(
    "Location",
    homeLocationIsConfigured()
      ? weatherLocationName()
      : "Not configured"
  );

  row(
    "Coordinates",
    homeLocationIsConfigured()
      ? formatHomeLocation()
      : "Not configured"
  );

  row(
    "Weather feature",
    !homeLocationIsConfigured()
      ? "Disabled - no saved location"
      : weatherSettings.enabled
          ? "Enabled"
          : "Disabled"
  );

  row(
    "Units",
    weatherSettings.useFahrenheit
      ? "Fahrenheit, mph, inches"
      : "Celsius, km/h, millimeters"
  );

  row("Forecast cache", weatherForecastAgeText());
  row("Radar cache", weatherRadarAgeText());
  page += F("</table>");

  if (!homeLocationIsConfigured()) {
    page += F(
      "<p>Coordinates 0,0 mean no saved location. Enter and apply a "
      "home latitude and longitude on the <a href=\"/\">Settings</a> "
      "page. Weather will be enabled automatically.</p>"
    );
    page += pageFooter();
    return page;
  }

  if (!weatherSettings.enabled) {
    page += F(
      "<p>Weather is disabled. Enable it on the "
      "<a href=\"/\">Settings</a> page.</p>"
    );
    page += pageFooter();
    return page;
  }

  page += F("<h2>Current conditions for ");
  page += htmlEscapeRuntime(weatherLocationName());
  page += F("</h2>");

  if (weatherForecastAvailableForSavedLocation()) {
    page += F("<table>");
    row(
      "Conditions",
      weatherConditionText(weatherForecast.weatherCode)
    );
    row(
      "Temperature",
      weatherTemperatureText(weatherForecast.temperature)
    );
    row(
      "Feels like",
      weatherTemperatureText(weatherForecast.apparentTemperature)
    );
    row(
      "Humidity",
      String(weatherForecast.relativeHumidity) + "%"
    );
    row(
      "Precipitation",
      String(weatherForecast.precipitation, 2) +
        (weatherSettings.useFahrenheit ? " in" : " mm")
    );
    row(
      "Wind",
      String(weatherWindDirectionText(weatherForecast.windDirection)) +
        " " + String(weatherForecast.windSpeed, 1) +
        (weatherSettings.useFahrenheit ? " mph" : " km/h")
    );
    row(
      "Gusts",
      String(weatherForecast.windGust, 1) +
        (weatherSettings.useFahrenheit ? " mph" : " km/h")
    );
    page += F("</table><h2>Three-day forecast</h2><table>");

    for (size_t index = 0; index < WEATHER_FORECAST_DAYS; ++index) {
      const WeatherForecastDay &day = weatherForecast.days[index];
      String summary = weatherConditionText(day.weatherCode);
      summary += "; high ";
      summary += weatherTemperatureText(day.highTemperature);
      summary += ", low ";
      summary += weatherTemperatureText(day.lowTemperature);
      summary += ", rain ";
      summary += String(day.precipitationProbability);
      summary += "%";
      row(weatherDayName(index), summary);
    }

    page += F("</table>");
  } else {
    page += F("<p>No forecast is cached for the saved location.</p>");

    if (weatherForecastLastError().length()) {
      page += F("<div class=\"msg err\">");
      page += htmlEscapeRuntime(weatherForecastLastError());
      page += F("</div>");
    }
  }

  page += F(
    "<a class=\"button\" href=\"/weather/refresh\">"
    "Refresh forecast</a>"
    "<h2>Regional precipitation radar</h2>"
  );

  if (weatherRadarAvailableForSavedLocation()) {
    const size_t frameCount = weatherRadarFrameCount();
    const size_t latestFrameIndex = frameCount - 1;
    page += F(
      "<img id=\"radar-animation\" class=\"radar-image\" "
      "src=\"/weather/radar.png?frame="
    );
    page += String(latestFrameIndex);
    page += F("&t=");
    page += String(
      static_cast<unsigned long>(weatherRadarFrameTime(latestFrameIndex))
    );
    page += F(
      "\" width=\"256\" height=\"256\" "
      "alt=\"Cached RainViewer radar overlay\">"
      "<p id=\"radar-frame-label\" class=\"note\">"
    );
    page += htmlEscapeRuntime(
      String("LATEST - ") +
      weatherRadarWebFrameLabel(weatherRadarFrameTime(latestFrameIndex))
    );
    page += F("</p>");

    if (frameCount > 1) {
      page += F(
        "<button id=\"radar-latest\" class=\"button\" type=\"button\" onclick=\"radarLatest()\">LATEST</button>"
        "<button id=\"radar-loop\" class=\"button\" type=\"button\" onclick=\"radarLoop()\">LOOP</button>"
        "<button class=\"button\" type=\"button\" onclick=\"radarPrevious()\">Previous frame</button>"
        "<button class=\"button\" type=\"button\" onclick=\"radarNext()\">Next frame</button>"
        "<script>const radarFrames=["
      );

      for (size_t index = 0; index < frameCount; ++index) {
        if (index > 0) {
          page += ',';
        }

        page += F("{u:'/weather/radar.png?frame=");
        page += String(index);
        page += F("&t=");
        page += String(
          static_cast<unsigned long>(weatherRadarFrameTime(index))
        );
        page += F("',l:'Frame ");
        page += String(index + 1);
        page += '/';
        page += String(frameCount);
        page += F(" - ");
        const String frameTimeLabel =
          weatherRadarWebFrameLabel(weatherRadarFrameTime(index));
        page += frameTimeLabel;
        page += F("',t:'");
        page += frameTimeLabel;
        page += F("'}");
      }

      page += F(
        "];let radarIndex=radarFrames.length-1;let radarMode='latest';let radarNextAt=0;"
        "function radarButtons(){document.getElementById('radar-latest').textContent=radarMode==='latest'?'LATEST (active)':'LATEST';"
        "document.getElementById('radar-loop').textContent=radarMode==='loop'?'LOOP (active)':'LOOP';}"
        "function radarShow(){document.getElementById('radar-animation').src=radarFrames[radarIndex].u;"
        "let label=radarFrames[radarIndex].l;if(radarMode==='latest')label='LATEST - '+radarFrames[radarIndex].t;"
        "else if(radarMode==='loop')label='LOOP - '+label;document.getElementById('radar-frame-label').textContent=label;}"
        "function radarLatest(){radarMode='latest';radarIndex=radarFrames.length-1;radarShow();radarButtons();}"
        "function radarLoop(){radarMode='loop';radarIndex=0;radarShow();radarButtons();radarNextAt=Date.now()+900;}"
        "function radarNext(){radarMode='manual';radarIndex=(radarIndex+1)%radarFrames.length;radarShow();radarButtons();}"
        "function radarPrevious(){radarMode='manual';radarIndex=(radarIndex+radarFrames.length-1)%radarFrames.length;radarShow();radarButtons();}"
        "setInterval(function(){if(radarMode!=='loop'||Date.now()<radarNextAt)return;radarIndex=(radarIndex+1)%radarFrames.length;radarShow();"
        "radarNextAt=Date.now()+(radarIndex===radarFrames.length-1?1800:900);},100);radarButtons();</script>"
      );
    }

    page += F(
      "<p class=\"note\">The device display places these transparent radar "
      "frames over cached OpenStreetMap raster tiles and marks the saved "
      "location at the center.</p>"
    );
  } else {
    page += F("<p>No radar image is cached for the saved location.</p>");

    if (weatherRadarLastError().length()) {
      page += F("<div class=\"msg err\">");
      page += htmlEscapeRuntime(weatherRadarLastError());
      page += F("</div>");
    }
  }

  page += F(
    "<a class=\"button\" href=\"/weather/refresh-radar\">"
    "Refresh radar</a>"
    "<p class=\"note\">Forecast data: "
    "<a href=\"https://open-meteo.com/\">Open-Meteo</a>. "
    "Radar data: <a href=\"https://www.rainviewer.com/\">RainViewer</a>. "
    "Map and location-name data: &copy; "
    "<a href=\"https://www.openstreetmap.org/copyright\">"
    "OpenStreetMap contributors</a>."
    "</p>"
  );

  page += pageFooter();
  return page;
}

void handleWeather() {
  if (weatherFeatureAvailable() && weatherRadarIsStale()) {
    requestWeatherRadarRefresh();
  }

  sendRuntimeHtml(200, buildWeatherPage());
}

void handleWeatherRefresh() {
  if (!weatherFeatureAvailable()) {
    sendRuntimeHtml(
      409,
      buildWeatherPage(
        "Enable weather and save a location before refreshing.",
        true
      )
    );
    return;
  }

  requestWeatherForecastRefresh();
  runtimeServer.sendHeader("Refresh", "2; url=/weather");
  sendRuntimeHtml(
    202,
    buildWeatherPage("Forecast refresh scheduled.")
  );
}

void handleWeatherRadarRefresh() {
  if (!weatherFeatureAvailable()) {
    sendRuntimeHtml(
      409,
      buildWeatherPage(
        "Enable weather and save a location before refreshing radar.",
        true
      )
    );
    return;
  }

  requestWeatherRadarRefresh();
  runtimeServer.sendHeader("Refresh", "3; url=/weather");
  sendRuntimeHtml(
    202,
    buildWeatherPage("Radar refresh scheduled.")
  );
}

void handleWeatherRadarImage() {
  if (!weatherRadarAvailableForSavedLocation()) {
    runtimeServer.send(404, "text/plain", "Radar image not available");
    return;
  }

  size_t frameIndex = weatherRadarFrameCount() - 1;

  if (runtimeServer.hasArg("frame")) {
    const int requested = runtimeServer.arg("frame").toInt();

    if (
      requested >= 0 &&
      static_cast<size_t>(requested) < weatherRadarFrameCount()
    ) {
      frameIndex = static_cast<size_t>(requested);
    }
  }

  const String framePath = weatherRadarFramePath(frameIndex);
  File image = SD.open(framePath.c_str(), FILE_READ);

  if (!image) {
    runtimeServer.send(404, "text/plain", "Radar image not found");
    return;
  }

  runtimeServer.sendHeader("Cache-Control", "private, max-age=60");
  runtimeServer.streamFile(image, "image/png");
  image.close();
}


String marketMoodEmojiHtml(MarketMoodLevel level) {
  switch (level) {
    case MarketMoodLevel::VeryUpbeat: return "&#x1F600;";
    case MarketMoodLevel::Upbeat: return "&#x1F642;";
    case MarketMoodLevel::Neutral: return "&#x1F610;";
    case MarketMoodLevel::Uneasy: return "&#x1F61F;";
    case MarketMoodLevel::Distressed: return "&#x1F623;";
    case MarketMoodLevel::Unknown:
    default: return "&#x2753;";
  }
}

String marketWebHorizontalLabel(time_t timestamp, bool intraday) {
  if (timestamp <= 0) {
    return "--";
  }

  tm localTm {};
  localtime_r(&timestamp, &localTm);
  char label[32];

  if (intraday) {
    if (timeSettings.use24Hour) {
      snprintf(
        label,
        sizeof(label),
        "%02d:%02d",
        localTm.tm_hour,
        localTm.tm_min
      );
    } else {
      int hour = localTm.tm_hour % 12;
      if (hour == 0) {
        hour = 12;
      }

      snprintf(
        label,
        sizeof(label),
        "%d:%02d",
        hour,
        localTm.tm_min
      );
    }
  } else {
    snprintf(
      label,
      sizeof(label),
      "%d/%d",
      localTm.tm_mon + 1,
      localTm.tm_mday
    );
  }

  return String(label);
}

String marketWebScaleLabel(int16_t basisPoints) {
  const float percent = basisPoints / 100.0f;
  String label;

  if (percent > 0.0001f) {
    label += '+';
  }

  label += String(percent, 1);
  label += '%';
  return label;
}

String buildMarketSvg(
  const MarketGraphPoint *points,
  size_t pointCount,
  const String &label,
  bool intraday
) {
  if (pointCount < 2) {
    return String("<div class=\"note\">") +
      htmlEscapeRuntime(label) +
      ": not enough cached points yet.</div>";
  }

  int16_t minimum = 0;
  int16_t maximum = 0;

  for (size_t index = 0; index < pointCount; ++index) {
    minimum = min(minimum, points[index].moodBasisPoints);
    maximum = max(maximum, points[index].moodBasisPoints);
  }

  if (maximum - minimum < 40) {
    const int16_t middle = static_cast<int16_t>((maximum + minimum) / 2);
    minimum = middle - 20;
    maximum = middle + 20;
  }

  static constexpr int LEFT = 58;
  static constexpr int RIGHT = 584;
  static constexpr int TOP = 28;
  static constexpr int BOTTOM = 146;

  const int valueRange = maximum - minimum;
  const int plotHeight = BOTTOM - TOP;

  auto valueToY = [&](int16_t value) {
    return valueRange <= 0
      ? (TOP + BOTTOM) / 2
      : BOTTOM - static_cast<int>(
          (value - minimum) * plotHeight / valueRange
        );
  };

  const size_t middleIndex = pointCount / 2;
  const int middleX = (LEFT + RIGHT) / 2;

  String svg;
  svg.reserve(2600);
  svg += F("<svg class=\"market-chart\" viewBox=\"0 0 600 180\" "
           "role=\"img\" aria-label=\"");
  svg += htmlEscapeRuntime(label);
  svg += F(" Market Mood graph\">"
           "<rect x=\"0\" y=\"0\" width=\"600\" height=\"180\" "
           "rx=\"10\" fill=\"#101820\"/>"
           "<text x=\"16\" y=\"19\" fill=\"#bec8d2\" "
           "font-size=\"14\">");
  svg += htmlEscapeRuntime(label);
  svg += F("</text>"
           "<line x1=\"58\" y1=\"28\" x2=\"58\" y2=\"146\" "
           "stroke=\"#536473\"/>"
           "<line x1=\"58\" y1=\"146\" x2=\"584\" y2=\"146\" "
           "stroke=\"#536473\"/>");

  auto appendValueTick = [&](int y, int16_t value) {
    svg += F("<line x1=\"54\" y1=\"");
    svg += String(y);
    svg += F("\" x2=\"62\" y2=\"");
    svg += String(y);
    svg += F("\" stroke=\"#718291\"/>"
             "<text x=\"50\" y=\"");
    svg += String(y + 4);
    svg += F("\" fill=\"#9aa8b3\" font-size=\"11\" "
             "text-anchor=\"end\">");
    svg += marketWebScaleLabel(value);
    svg += F("</text>");
  };

  appendValueTick(TOP, maximum);
  appendValueTick(BOTTOM, minimum);

  if (minimum < 0 && maximum > 0 && valueRange > 0) {
    const int zeroY = valueToY(0);

    if (zeroY - TOP >= 12 && BOTTOM - zeroY >= 12) {
      appendValueTick(zeroY, 0);
    }
  }

  const int tickXs[3] = {LEFT, middleX, RIGHT};
  const size_t labelIndexes[3] = {0, middleIndex, pointCount - 1};
  const char *anchors[3] = {"start", "middle", "end"};

  for (size_t tick = 0; tick < 3; ++tick) {
    svg += F("<line x1=\"");
    svg += String(tickXs[tick]);
    svg += F("\" y1=\"142\" x2=\"");
    svg += String(tickXs[tick]);
    svg += F("\" y2=\"151\" stroke=\"#718291\"/>"
             "<text x=\"");
    svg += String(tickXs[tick]);
    svg += F("\" y=\"168\" fill=\"#9aa8b3\" font-size=\"11\" "
             "text-anchor=\"");
    svg += anchors[tick];
    svg += F("\">");
    svg += marketWebHorizontalLabel(
      points[labelIndexes[tick]].timestamp,
      intraday
    );
    svg += F("</text>");
  }

  svg += F("<polyline fill=\"none\" stroke=\"#5ed38b\" "
           "stroke-width=\"4\" stroke-linecap=\"round\" "
           "stroke-linejoin=\"round\" points=\"");

  for (size_t index = 0; index < pointCount; ++index) {
    const int x = LEFT + static_cast<int>(
      index * (RIGHT - LEFT) / (pointCount - 1)
    );
    const int y = valueToY(points[index].moodBasisPoints);

    if (index > 0) {
      svg += ' ';
    }
    svg += String(x);
    svg += ',';
    svg += String(y);
  }

  svg += F("\"/></svg>");
  return svg;
}

String buildMarketPage(
  const String &message = "",
  bool error = false
) {
  String page = pageHeader("Market Mood");

  if (message.length()) {
    page += F("<div class=\"msg");
    if (error) {
      page += F(" err");
    }
    page += F("\">");
    page += htmlEscapeRuntime(message);
    page += F("</div>");
  }

  page += F("<table>");

  auto row = [&](const String &name, const String &value) {
    page += F("<tr><td>");
    page += htmlEscapeRuntime(name);
    page += F("</td><td>");
    page += value;
    page += F("</td></tr>");
  };

  row("Feature", marketSettings.enabled ? "Enabled" : "Disabled");
  row("Symbols", "SPY, QQQ, IWM");
  row("Refresh", "Approximately every 10 minutes");

  if (!marketSettings.enabled) {
    page += F("</table>"
      "<p>Enable Market Mood on the <a href=\"/\">Settings</a> page. "
      "The feature is disabled by default.</p>"
      "<p class=\"note\">Market Mood is a descriptive broad-market summary, "
      "not an economic-health measure or investment advice.</p>");
    page += pageFooter();
    return page;
  }

  if (marketDataAvailable()) {
    const MarketMoodLevel mood = currentMarketMoodLevel();
    String moodValue = "<span class=\"mood\">";
    moodValue += marketMoodEmojiHtml(mood);
    moodValue += ' ';
    moodValue += htmlEscapeRuntime(marketMoodLabel(mood));
    moodValue += " &nbsp; ";
    moodValue += htmlEscapeRuntime(
      marketPercentText(marketSnapshot.currentMoodBasisPoints)
    );
    moodValue += "</span>";

    row("Current mood", moodValue);
    row("Session", marketSnapshot.marketOpen ? "Market open" : "Market closed");
    row("Latest data", htmlEscapeRuntime(marketAgeText()));
    row(
      "30-session trend",
      htmlEscapeRuntime(
        marketPercentText(marketSnapshot.thirtyDayMoodBasisPoints)
      )
    );
  } else {
    row("Current mood", "Waiting for market data");
    row("Latest data", htmlEscapeRuntime(marketAgeText()));
  }

  row(
    "Refresh state",
    marketRefreshIsPending() ? "Pending" : "Idle"
  );
  page += F("</table>");

  if (marketDataAvailable()) {
    page += F("<h2>Broad-market inputs</h2><table>");
    for (size_t index = 0; index < 3; ++index) {
      const MarketQuote &quote = marketSnapshot.quotes[index];
      String value = String(quote.price, 2);
      value += " &nbsp; ";
      if (quote.changePercent > 0.0001f) {
        value += '+';
      }
      value += String(quote.changePercent, 2);
      value += '%';
      row(quote.symbol, value);
    }
    page += F("</table><h2>Today</h2>");
    page += buildMarketSvg(
      marketSnapshot.today,
      marketSnapshot.todayCount,
      "TODAY",
      true
    );
    page += F("<h2>30 trading sessions</h2>");
    page += buildMarketSvg(
      marketSnapshot.thirtyDay,
      marketSnapshot.thirtyDayCount,
      "30 SESSIONS",
      false
    );
  } else {
    page += F("<p>No market snapshot is cached yet.</p>");
  }

  if (marketLastError().length()) {
    page += F("<div class=\"msg err\">");
    page += htmlEscapeRuntime(marketLastError());
    page += F("</div>");
  }

  page += F(
    "<a class=\"button\" href=\"/market/refresh\">Refresh Market Mood</a>"
    "<p class=\"note\">The TODAY line is the average percentage movement "
    "of SPY, QQQ, and IWM relative to their previous closes. The 30-session "
    "line compounds the same equal-weight daily composite. Data is delayed, "
    "best effort, and cached on the microSD card. Source: Yahoo Finance "
    "chart data through an unofficial integration. It is not investment advice.</p>"
  );

  page += pageFooter();
  return page;
}

void handleMarket() {
  if (marketFeatureAvailable() && marketDataIsStale()) {
    requestMarketRefresh(marketSnapshot.thirtyDayCount == 0);
  }

  sendRuntimeHtml(200, buildMarketPage());
}

void handleMarketRefresh() {
  if (!marketFeatureAvailable()) {
    sendRuntimeHtml(
      409,
      buildMarketPage(
        "Enable Market Mood before refreshing.",
        true
      )
    );
    return;
  }

  requestMarketRefresh(true);
  runtimeServer.sendHeader("Refresh", "4; url=/market");
  sendRuntimeHtml(
    202,
    buildMarketPage("Market Mood refresh scheduled.")
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

  if (
    runtimeTouchCalibrationResultMessage.length() > 0
  ) {
    page += F("<div class=\"msg");

    if (runtimeTouchCalibrationResultError) {
      page += F(" err");
    }

    page += F("\">");
    page += htmlEscapeRuntime(
      runtimeTouchCalibrationResultMessage
    );
    page += F("</div>");
  }

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
  row("Backlight brightness", displayBrightnessText());
  row("Day map gamma", mapGammaText(displaySettings.dayMapGamma));
  row("Night map gamma", mapGammaText(displaySettings.nightMapGamma));
  row("Map cache format", String(MAP_CACHE_FORMAT_VERSION));
  row(
    "Legacy night cache",
    SD.exists(LEGACY_NIGHT_RAW_FILE)
      ? "Present but unused"
      : "Absent"
  );

  const TouchDiagnostics &touch =
    getTouchDiagnostics();

  row(
    "Touch hardware",
    !touch.hardwareAvailable
      ? "Disabled for board profile"
      : touch.initialized
          ? "Initialized"
          : "Unavailable"
  );

  row(
    "Touch calibration",
    touchCalibrationIsReady()
      ? "Ready"
      : touchCalibrationWasBypassed()
          ? "Disabled until recalibrated"
          : "Missing"
  );

  row("Touch state", touch.state);
  row("Touch display rotation", String(touch.activeRotation));
  row("Touch pressure threshold", String(touch.pressureMinimum));
  row(
    "Home marker",
    locationGridSettings.showHomeMarker
      ? "Shown"
      : "Hidden"
  );
  row("Home latitude", formatLatitude(locationGridSettings.homeLatitude));
  row("Home longitude", formatLongitude(locationGridSettings.homeLongitude));
  row(
    "Home location configured",
    homeLocationIsConfigured() ? "Yes" : "No"
  );
  row(
    "Weather",
    !homeLocationIsConfigured()
      ? "Disabled - no saved location"
      : weatherSettings.enabled
          ? "Enabled"
          : "Disabled"
  );
  row(
    "Weather units",
    weatherSettings.useFahrenheit ? "Fahrenheit" : "Celsius"
  );
  row("Forecast cache", weatherForecastAgeText());
  row("Radar cache", weatherRadarAgeText());
  row(
    "Radar animation frames",
    String(weatherRadarFrameCount()) + "/" +
      String(WEATHER_RADAR_ANIMATION_FRAME_COUNT)
  );
  row(
    "Forecast refresh",
    weatherForecastRefreshIsPending() ? "Pending" : "Idle"
  );
  row(
    "Radar refresh",
    weatherRadarRefreshInProgress()
      ? String("Downloading ") +
          String(weatherRadarRefreshCompletedCount()) + "/" +
          String(weatherRadarRefreshTargetCount())
      : weatherRadarRefreshIsPending()
          ? "Pending"
          : "Idle"
  );
  row(
    "Weather error",
    weatherForecastLastError().length()
      ? weatherForecastLastError()
      : "None"
  );
  row(
    "Radar/map error",
    weatherRadarLastError().length()
      ? weatherRadarLastError()
      : "None"
  );
  row(
    "Market Mood",
    marketSettings.enabled ? "Enabled" : "Disabled"
  );
  row(
    "Market mood",
    marketDataAvailable()
      ? String(marketMoodLabel(currentMarketMoodLevel())) +
          " " + marketPercentText(marketSnapshot.currentMoodBasisPoints)
      : "No cached data"
  );
  row(
    "Market session",
    marketDataAvailable()
      ? marketSnapshot.marketOpen ? "Open" : "Closed"
      : "Unknown"
  );
  row(
    "Market graph points",
    String(marketSnapshot.todayCount) + " today; " +
      String(marketSnapshot.thirtyDayCount) + " of 30 sessions"
  );
  row(
    "Market refresh",
    marketRefreshIsPending() ? "Pending" : "Idle"
  );
  row(
    "Market error",
    marketLastError().length() ? marketLastError() : "None"
  );
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
  row("Night cache path", activeNightRawPath);
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

  if (
    ACTIVE_BOARD.touchAvailable &&
    touchHardwareIsReady()
  ) {
    page += F(
      "<h2>Touch calibration</h2>"
      "<p class=\"note\">Calibration changes only the touch record in the "
      "touchtest Preferences namespace. Wi-Fi and all other World Clock "
      "settings are preserved.</p>"
      "<a class=\"button\" href=\""
    );
    page += TOUCH_RECALIBRATE_PATH;
    page += F("\">");
    page +=
      touchCalibrationIsReady()
        ? "Recalibrate Touch"
        : "Enable / Calibrate Touch";
    page += F("</a>");
  }

  page += F(
    "<h2>Display calibration</h2>"
    "<p class=\"note\">Show grayscale, near-black, near-white, and RGB "
    "gradients on the physical display. The pattern remains until CLOCK is "
    "selected on the touchscreen or the browser return control is used.</p>"
    "<a class=\"button\" href=\""
  );
  page += DISPLAY_TEST_PATH;
  page += F("\">Show Display Test Pattern</a>");

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
    rawMapIsValid(activeNightRawPath.c_str());

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
    "Each PNG uses a board- and gamma-specific RGB565 cache. The night image "
    "source is shared, while each board/tuning combination gets its own cache.</p>"
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
  row("Cache format", String(MAP_CACHE_FORMAT_VERSION));
  row("Day gamma", mapGammaText(displaySettings.dayMapGamma));
  row("Night gamma", mapGammaText(displaySettings.nightMapGamma));
  row(
    "Legacy /earth_night.rgb565",
    SD.exists(LEGACY_NIGHT_RAW_FILE)
      ? "Present but unused"
      : "Absent"
  );
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
      ? String(fileSizeOrZero(activeNightRawPath.c_str())) + " bytes; valid"
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
  runtimeServer.on(WEATHER_PATH, HTTP_GET, handleWeather);
  runtimeServer.on(
    WEATHER_REFRESH_PATH,
    HTTP_GET,
    handleWeatherRefresh
  );
  runtimeServer.on(
    WEATHER_RADAR_REFRESH_PATH,
    HTTP_GET,
    handleWeatherRadarRefresh
  );
  runtimeServer.on(
    WEATHER_RADAR_IMAGE_PATH,
    HTTP_GET,
    handleWeatherRadarImage
  );
  runtimeServer.on(MARKET_PATH, HTTP_GET, handleMarket);
  runtimeServer.on(
    MARKET_REFRESH_PATH,
    HTTP_GET,
    handleMarketRefresh
  );
  runtimeServer.on(DIAGNOSTICS_PATH, HTTP_GET, handleDiagnostics);
  runtimeServer.on(
    TOUCH_RECALIBRATE_PATH,
    HTTP_GET,
    handleTouchRecalibrate
  );
  runtimeServer.on(
    DISPLAY_TEST_PATH,
    HTTP_GET,
    handleDisplayTest
  );
  runtimeServer.on(
    DISPLAY_TEST_CLOCK_PATH,
    HTTP_GET,
    handleDisplayTestClock
  );
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

    const bool brightnessChanged =
      runtimePendingBrightnessChanged;

    const bool mapTuningChanged =
      runtimePendingMapTuningChanged;

    const bool locationGridChanged =
      runtimePendingLocationGridChanged;

    const bool homeCoordinatesChanged =
      runtimePendingHomeCoordinatesChanged;

    const bool weatherChanged =
      runtimePendingWeatherChanged;

    const bool marketChanged =
      runtimePendingMarketChanged;

    runtimePendingTimeZoneChanged = false;
    runtimePendingClockDisplayChanged = false;
    runtimePendingOrientationChanged = false;
    runtimePendingBrightnessChanged = false;
    runtimePendingMapTuningChanged = false;
    runtimePendingLocationGridChanged = false;
    runtimePendingHomeCoordinatesChanged = false;
    runtimePendingWeatherChanged = false;
    runtimePendingMarketChanged = false;

    Serial.printf(
      "Runtime web: applying settings; "
      "timeZoneChanged=%d clockDisplayChanged=%d "
      "orientationChanged=%d brightnessChanged=%d mapTuningChanged=%d "
      "locationGridChanged=%d homeCoordinatesChanged=%d weatherChanged=%d marketChanged=%d\n",
      timeZoneChanged,
      clockDisplayChanged,
      orientationChanged,
      brightnessChanged,
      mapTuningChanged,
      locationGridChanged,
      homeCoordinatesChanged,
      weatherChanged,
      marketChanged
    );

    if (timeZoneChanged) {
      applyConfiguredTimeZone();
    }

    if (orientationChanged) {
      applyDisplayRotation();
      lcd.fillScreen(TFT_BLACK);
    }

    if (brightnessChanged) {
      applyBacklightBrightness();
    }

    if (mapTuningChanged) {
      applyMapDisplayTuning();
    }

    if (weatherChanged) {
      weatherForecast.valid = false;

      if (homeCoordinatesChanged && sdReady) {
        initializeWeatherService();
      } else if (homeCoordinatesChanged) {
        weatherRadarInfo.valid = false;
      }

      if (weatherFeatureAvailable()) {
        requestWeatherForecastRefresh();
      }
    }

    if (marketChanged && marketFeatureAvailable()) {
      initializeMarketService();
      requestMarketRefresh(true);
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
    runtimeTouchCalibrationPending &&
    static_cast<long>(
      now - runtimeTouchCalibrationAt
    ) >= 0
  ) {
    runtimeTouchCalibrationPending = false;

    const bool hadCalibration =
      touchCalibrationIsReady();

    Serial.println(
      "Runtime web: starting touch calibration"
    );

    const bool calibrated =
      runIntegratedTouchCalibration(false);

    if (calibrated) {
      runtimeTouchCalibrationResultMessage =
        "Touch calibration was saved and the touchscreen is enabled.";
      runtimeTouchCalibrationResultError = false;
    } else if (touchCalibrationWasBypassed()) {
      runtimeTouchCalibrationResultMessage =
        "Touch calibration was cancelled. Touch remains disabled until recalibrated.";
      runtimeTouchCalibrationResultError = false;
    } else if (
      hadCalibration &&
      touchCalibrationIsReady()
    ) {
      runtimeTouchCalibrationResultMessage =
        "Touch recalibration was cancelled. The previous calibration was retained.";
      runtimeTouchCalibrationResultError = false;
    } else {
      runtimeTouchCalibrationResultMessage =
        "Touch calibration did not complete.";
      runtimeTouchCalibrationResultError = true;
    }

    if (
      timeValid &&
      sdReady
    ) {
      redrawWorldClock();
    } else {
      showStatus(
        "World Clock",
        timeValid
          ? "Waiting for map storage"
          : "Waiting for network time"
      );
    }

    Serial.printf(
      "Runtime web: touch calibration complete; ok=%d ready=%d bypassed=%d\n",
      calibrated,
      touchCalibrationIsReady(),
      touchCalibrationWasBypassed()
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
