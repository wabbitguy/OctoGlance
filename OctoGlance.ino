/* ESP32 Dev Module 
   NO OTA 2MB APP/2MB SPIFFS
*/

#include "Settings.h"     // some basic settings (language, TimeZone)
#include "Translation.h"  // language translations
#include "Ntfy.h"         // for push nofitications if you want them
#include "NTP_Time.h"     // time server change

//#define FORMAT_LittleFS  // Wipe LittleFS and all files! Disable after use.

#define VERSION "v1.1"
#define hostNameCYD "OctoGlance"
#define CONFIG "/config.txt"

// ---------------- GAUGE IDs ----------------
#define nozzleGauge 1
#define progressGauge 2
#define bedGauge 3

// ---------------- LAYOUT Y POSITIONS ----------------

#define clockBottomY 44  // tighten clock up
#define printerNameY 60  // tighter below clock
#define headingY 77      // gauge headings
#define gaugeY 112       // arc centres (moved up 16px)
#define dataY 110        // gauge values
#define statusZoneY 146  // status zone starts earlier
#define graphicX 65      // re-centre: (240-110)/2
#define graphicY 147     // graphic starts here, ends at 257
#define endTimeY 284     // ETA/End line (257 + 4 + ~11px font)
#define filenameY 300    // filename
#define versionY 308     // version string
#define statusZoneH (versionY - 16 - statusZoneY)
#define belowClockY 44  // start of area below clock, used for fillRect clears
#define chamberX 32     // for display a chamber temp if you have one
#define chamberY 195    // ditto
// ---------------- RGB LED ----------------
#define RED_PIN 22
#define GREEN_PIN 16
#define BLUE_PIN 17

// ---------------- DISPLAY ----------------
#define SCREEN_W 240
#define SCREEN_H 320

TFT_eSPI tft = TFT_eSPI();

// ---------------- PRINTER STATE ----------------
bool foundPrinter = false;
String printerName = "";

String printState;
float progress, nozzleTemp, nozzleTarget, bedTemp, bedTarget, printDuration, totalDuration;
uint16_t progressPercent;
uint16_t lastNozzleTemp = 9999, lastBedTemp = 9999, lastProgress = 9999;

bool greenON = true;
bool greenOFF = false;

bool enablePoll = false;
uint8_t thePollTime = 10;

bool forcePoll = true;  // true at boot, true after settings update

float savedTotalDuration = 0.0;  // save the total duration so it doesn't get nop'd out

float toolheadX = 0, toolheadY = 0, toolheadZ = 0, toolheadE = 0;  // for checking against no printer movement

// ---------------- CONNECTION STATE MACHINE ----------------
typedef enum {
  CONN_BOOTING,            // "Connecting..." until the first poll
  CONN_OCTOPRINT_OFFLINE,  // OctoPrint not answering
  CONN_PRINTER_OFFLINE,    // OctoPrint up, printer not connected
  CONN_READY               // OctoPrint up and printer connected
} ConnState;

ConnState connState = CONN_BOOTING;

typedef enum {// fetchPrinterData() result
  FETCH_OK,
  FETCH_PRINTER_NOT_CONNECTED,  // HTTP 409 - no printer attached
  FETCH_UNREACHABLE             // no response, timeout, bad JSON
} FetchResult;

typedef enum {// testOctoPrintConnection() result, shown when settings are saved
  KEYTEST_OK,         // address and key OK
  KEYTEST_NOT_FOUND,  // wrong IP/port or OctoPrint not running
  KEYTEST_BAD_KEY     // 401/403 - wrong key
} KeyTestResult;

// ---------------- PRINTER STATE MACHINE ----------------
typedef enum {
  STATE_IDLE,      // printer on, doing nothing
  STATE_PREP,      // heating up
  STATE_PRINTING,
  STATE_PAUSED,
  STATE_COMPLETE,  // finished at ~100%
  STATE_FAILED     // cancelled or error
} PrinterState;

PrinterState currentState = STATE_IDLE;
PrinterState lastState = STATE_IDLE;

float lastKnownProgress = 0.0;// last progress seen while printing

bool showSleep = false;
bool showIdle = false;
bool justFinished = false;
uint32_t finishedAt = 0;
String thePrintFileRaw;  // full untruncated path, used only for thumbnail fetch
#define FINISHED_DISPLAY_MS 30000
#define HTTP_CONNECT_TIMEOUT 2000   // ms - increase if on printer is on WiFi
#define HTTP_RESPONSE_TIMEOUT 3000  // ms

// ---------------- CLOCK ----------------
uint8_t lastSecond = 99;
uint8_t lastMinute, lastHour, lastDay, lastMonth;
uint16_t lastYear, myYear;
uint8_t myHour, myMinute, mySecond, my24Hour, myDay, myMonth, myWeekDay;
bool colonBlink = false;
bool activeETA = false;
uint16_t lastETAProgress = 0;    // separate from lastProgress (gauge) — see handleETA()
bool etaShownForThisPrint = false;
uint16_t etaHH, etaMM;

// Web server
WebServer server(80);

// ---------------- FORWARD DECLARATIONS ----------------
void handlePrinterOffLine();
void drawConnecting();
void drawPrinterOffline();
void drawBmp(fs::FS &fs, const char *filename, int16_t x, int16_t y);
uint16_t read16(fs::File &f);
uint32_t read32(fs::File &f);
void handle_ClockDisplay();
void handlePolling(int8_t theSeconds);
void handleGaugeHeadings();
void handleGauge(uint8_t whichGauge, int16_t gaugeValue);
void setRGB(bool redLevel, bool greenLevel, bool blueLevel);
void handleHostName();
void handleTimeUsed();
void handlePrintFailed();
void handleETA();
void estimateTimeRemaining(float elapsedSeconds, float percentComplete, char *result);
FetchResult fetchPrinterData();
PrinterState determinePrinterState();
void updatePrinterDisplay(PrinterState state);
void handlePrinterStatus();
void configModeCallback(WiFiManager *myWiFiManager);
String SendHTML(String saveBanner = "");
void handlePrinterUpdate();
KeyTestResult testOctoPrintConnection();
String getPrinterSetup();
void writeSettings();
void readSettings();
void buildPrinterURLs();
String extractFileName(const String &path, bool withExt);
void handleWifiReset();
void redirectHome();
void drawWiFiQuality();
int8_t getWifiQuality();
void handle_OnConnect();
void drawVersionString();
bool fetchAndDrawThumbnail();
int pngDraw(PNGDRAW *pDraw);
String ntfyServerDisplay();                           // cleans up the URL for local host server
void beginHTTP(HTTPClient &http, const String &url);  // timeouts for http, adds X-Api-Key

// ============================================================
//  VERSION STRING
// ============================================================
void drawVersionString() {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(VERSION) + " @2026 - Wabbit Wanch Design", 120, versionY, 2);
}

// ============================================================
//  SETUP
// ============================================================
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);

#ifdef FORMAT_LittleFS
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Formatting LittleFS, please wait...", 120, 160, 2);
  LittleFS.format();
  ESP.restart();
#endif

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS init failed!");
    while (1) yield();
  }
  Serial.println("LittleFS ready.");

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setRGB(0, 0, 0);

  WiFiManager wifiManager;
  wifiManager.setHostname("OctoGlance");
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  if (!wifiManager.autoConnect(hostNameCYD)) {
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  WiFi.hostname(hostNameCYD);
  MDNS.begin(hostNameCYD);

  server.on("/", handle_OnConnect);
  server.on("/updatePrinterInfo", handlePrinterUpdate);
  server.on("/wifiReset", handleWifiReset);
  server.begin();
  MDNS.addService("http", "tcp", 80);

  tft.fillScreen(TFT_BLACK);
  drawVersionString();
  drawConnecting();  // shown until the first OctoPrint poll resolves

  udp.begin(localPort);
  syncTime();

  readSettings();
  if (checkTimezoneOffsets()) writeSettings();
  buildPrinterURLs();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  static time_t prevDisplay = 0;
  static uint8_t lastSyncHour = 99;

  utc = now();

  if (utc != prevDisplay) {
    prevDisplay = utc;
    handle_ClockDisplay();
  }

  uint8_t currentHour = hour(toLocal(utc));
  if (currentHour != lastSyncHour) {
    syncTime();
    if (currentHour == 2) {
      if (checkTimezoneOffsets()) writeSettings();
    }
    lastSyncHour = currentHour;
  }

  if (enablePoll == true) {
    if (printerName == "") {
      handleHostName();
      handlePrinterOffLine();
    } else {
      handlePrinterStatus();
    }
    enablePoll = false;
  }

  server.handleClient();
}

// ============================================================
//  WIFI QUALITY BAR GRAPH
// ============================================================
void drawWiFiQuality() {
  const byte numBars = 5;
  const byte barWidth = 3;
  const byte barHeight = 20;
  const byte barSpace = 1;
  const uint16_t barXPosBase = SCREEN_W - 25;
  const byte barYPosBase = 20;
  const uint16_t barColor = TFT_YELLOW;
  const uint16_t barBackColor = TFT_DARKGREY;

  int8_t quality = getWifiQuality();

  for (int8_t i = 0; i < numBars; i++) {
    byte barSpacer = i * barSpace;
    byte tempBarHeight = (barHeight / numBars) * (i + 1);
    for (int8_t j = 0; j < tempBarHeight; j++) {
      for (byte ii = 0; ii < barWidth; ii++) {
        byte nextBarThreshold = (i + 1) * (100 / numBars);
        byte currentBarThreshold = i * (100 / numBars);
        byte currentBarIncrements = (barHeight / numBars) * (i + 1);
        float rangePerBar = (100 / numBars);
        float currentBarStrength;
        if ((quality > currentBarThreshold) && (quality < nextBarThreshold)) {
          currentBarStrength = ((quality - currentBarThreshold) / rangePerBar) * currentBarIncrements;
        } else if (quality >= nextBarThreshold) {
          currentBarStrength = currentBarIncrements;
        } else {
          currentBarStrength = 0;
        }
        if (j < currentBarStrength) {
          tft.drawPixel((barXPosBase + barSpacer + ii) + (barWidth * i), barYPosBase - j, barColor);
        } else {
          tft.drawPixel((barXPosBase + barSpacer + ii) + (barWidth * i), barYPosBase - j, barBackColor);
        }
      }
    }
  }
}

int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if (dbm <= -100) return 0;
  else if (dbm >= -50) return 100;
  else return 2 * (dbm + 100);
}

// ============================================================
//  CLOCK DISPLAY  (WabbitWeather style, NotoSansBold36)
// ============================================================
void handle_ClockDisplay() {
  char buffer[24];
  uint8_t theHour;

  time_t utc = now();
  time_t localTime = toLocal(utc);

  myHour = hourFormat12(localTime);
  my24Hour = hour(localTime);
  myMinute = minute(localTime);
  mySecond = second(localTime);
  myDay = day(localTime);
  myMonth = month(localTime);
  myYear = year(localTime);

  uint8_t xpos = (SCREEN_W / 2) - 1;

  tft.loadFont(AA_FONT_LARGE, LittleFS);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  theHour = (show24HR) ? my24Hour : myHour;

  if (colonBlink == false) {
    sprintf(buffer, " %2u:%02u ", theHour, myMinute);
  } else {
    sprintf(buffer, " %2u %02u ", theHour, myMinute);
  }
  colonBlink = !colonBlink;

  tft.setTextPadding(tft.textWidth(" 44:44 "));
  tft.drawString(buffer, xpos, clockBottomY);
  tft.setTextPadding(0);
  tft.unloadFont();

  // Save for next pass
  lastSecond = mySecond;
  lastMinute = myMinute;
  lastHour = my24Hour;
  lastDay = myDay;
  lastYear = myYear;
  lastMonth = myMonth;

  handlePolling(mySecond);

  if (mySecond == (thePollTime + 5)) {
    drawWiFiQuality();
  }
}

// ============================================================
//  POLLING TRIGGER
// ============================================================
void handlePolling(int8_t theSeconds) {
  if (forcePoll) {
    enablePoll = true;
    forcePoll = false;  // clear it, back to normal timer polling
    return;
  }
  if ((theSeconds % thePollTime) == 0) {
    enablePoll = true;
  }
}

// ============================================================
//  GAUGE HEADINGS
// ============================================================
void handleGaugeHeadings() {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(strNozzle, 40, headingY, 2);
  tft.drawString(strProgress, 120, headingY, 2);
  tft.drawString(strBed, 200, headingY, 2);
}

// ============================================================
//  GAUGE DRAW
// ============================================================
void handleGauge(uint8_t whichGauge, int16_t gaugeValue) {
  float temp;
  uint16_t theMove;
  tft.setTextDatum(MC_DATUM);

  switch (whichGauge) {
    case nozzleGauge:
      temp = constrain(float(gaugeValue) / maxNozzleTemp, 0.0f, 1.0f);// clamp or the arc wraps
      theMove = (temp * 280) + 40;
      tft.drawSmoothArc(40, gaugeY, 32, 22, 40, 320, TFT_DARKGREY, TFT_BLACK, false);
      if (theMove > 40) {
        tft.drawSmoothArc(40, gaugeY, 32, 22, 40, theMove, TFT_GREEN, TFT_DARKGREY, false);
      }
      tft.fillCircle(40, gaugeY, 20, TFT_BLACK);
      tft.setTextColor((nozzleTarget != 0) ? TFT_RED : TFT_WHITE, TFT_BLACK);
      tft.drawString(String(gaugeValue), 40, dataY, 2);
      break;

    case progressGauge:
      temp = (float(gaugeValue) / 100);
      theMove = (temp * 280) + 40;
      tft.drawSmoothArc(120, gaugeY, 32, 22, 40, 320, TFT_DARKGREY, TFT_BLACK, false);
      if (theMove > 40) {
        uint16_t arcColor = (currentState == STATE_PAUSED) ? TFT_ORANGE : TFT_GREEN;// orange when paused
        tft.drawSmoothArc(120, gaugeY, 32, 22, 40, theMove, arcColor, TFT_DARKGREY, false);
      }
      tft.fillCircle(120, gaugeY, 20, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(String(gaugeValue) + "%", 120, dataY, 2);
      break;

    case bedGauge:
      temp = constrain(float(gaugeValue) / maxBedTemp, 0.0f, 1.0f);// clamp or the arc wraps
      theMove = (temp * 280) + 40;
      tft.drawSmoothArc(200, gaugeY, 32, 22, 40, 320, TFT_DARKGREY, TFT_BLACK, false);
      if (theMove > 40) {
        tft.drawSmoothArc(200, gaugeY, 32, 22, 40, theMove, TFT_GREEN, TFT_DARKGREY, false);
      }
      tft.fillCircle(200, gaugeY, 20, TFT_BLACK);
      tft.setTextColor((bedTarget != 0) ? TFT_RED : TFT_WHITE, TFT_BLACK);
      tft.drawString(String(gaugeValue), 200, dataY, 2);
      break;
  }
}

// ============================================================
//  RGB LED
// ============================================================
void setRGB(bool redLevel, bool greenLevel, bool blueLevel) {
  digitalWrite(RED_PIN, !redLevel);
  digitalWrite(GREEN_PIN, !greenLevel);
  digitalWrite(BLUE_PIN, !blueLevel);
}

// ============================================================
//  CONNECTING SCREEN (boot only, until the first poll resolves)
// ============================================================
void drawConnecting() {
  tft.fillRect(0, belowClockY, 239, SCREEN_H - belowClockY, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(strConnecting, 120, graphicY + 55, 4);
  drawVersionString();
}

// ============================================================
//  OCTOPRINT OFFLINE SCREEN (server unreachable - no response at all)
// ============================================================
void handlePrinterOffLine() {
  if (printerName == "") {
    if (showSleep == false) {
      connState = CONN_OCTOPRINT_OFFLINE;
      tft.fillRect(0, belowClockY, 239, SCREEN_H - belowClockY, TFT_BLACK);
      drawBmp(LittleFS, OFFLINE_IMAGE, 27, belowClockY + 4);
      tft.loadFont(AA_FONT_SMALL, LittleFS);
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.setTextDatum(BC_DATUM);
      tft.drawString(strOctoOffline, 120, 260);  // between graphic and IP line
      tft.unloadFont();
      String ipaddress = "OctoGlance " + WiFi.localIP().toString();
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(BC_DATUM);
      tft.drawString(ipaddress, 120, versionY - 16, 2);
      drawVersionString();  // programmer info
      showSleep = true;
    }
  } else {
    handlePrinterStatus();// OctoPrint answered, let it decide what to draw
  }
}

// ============================================================
//  PRINTER NOT CONNECTED SCREEN (OctoPrint is up, printer isn't)
// ============================================================
void drawPrinterOffline() {
  tft.fillRect(0, belowClockY, 239, SCREEN_H - belowClockY, TFT_BLACK);
  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(printerName, 120, printerNameY);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(strOctoOnline, 120, graphicY + 30);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(strPrinterOffline, 120, graphicY + 55);
  tft.unloadFont();
  String ipaddress = "OctoGlance " + WiFi.localIP().toString();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(ipaddress, 120, versionY - 16, 2);
  drawVersionString();
}

// ============================================================
//  HTTP HELPER
// ============================================================
void beginHTTP(HTTPClient &http, const String &url) {
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT);
  http.setTimeout(HTTP_RESPONSE_TIMEOUT);
  http.begin(url);
  http.addHeader("X-Api-Key", apiKey);  // OctoPrint requires this on every request
}

// ============================================================
//  FIND PRINTER HOSTNAME
// ============================================================
void handleHostName() {
  if (WiFi.status() == WL_CONNECTED) {
    setRGB(0, greenON, 0);
    HTTPClient http;
    beginHTTP(http, printerURLInfo);  // GET /api/settings
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        String customName = doc["appearance"]["name"] | "";
        printerName = (customName != "") ? customName : "OctoPrint";// name from Settings > Appearance
        Serial.println(printerName);
      }
    } else {
      Serial.println("handleHostName failed: " + String(httpCode));
    }
    http.end();
    setRGB(0, greenOFF, 0);
    if (printerName != "") {
      buildPrinterURLs();
      lastBedTemp = 0;
      lastNozzleTemp = 0;
      lastProgress = 0;
    }
  }
}

// ============================================================
//  API KEY / ADDRESS TEST (run once, synchronously, on settings save)
// ============================================================
KeyTestResult testOctoPrintConnection() {
  if (WiFi.status() != WL_CONNECTED) return KEYTEST_NOT_FOUND;

  HTTPClient http;
  beginHTTP(http, printerURLInfo);// /api/settings works with no printer connected
  int httpCode = http.GET();
  http.end();

  if (httpCode == 200) return KEYTEST_OK;
  if (httpCode == 401 || httpCode == 403) return KEYTEST_BAD_KEY;
  return KEYTEST_NOT_FOUND;
}

// ============================================================
//  PHASE 1 — FETCH & PARSE (OctoPrint REST API)
// ============================================================
FetchResult fetchPrinterData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost");
    return FETCH_UNREACHABLE;
  }
  setRGB(0, greenON, 0);

  // ---- GET /api/printer : state flags + temperatures ----
  HTTPClient http;
  beginHTTP(http, printerURLQ);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    setRGB(0, greenOFF, 0);
    return (httpCode == 409) ? FETCH_PRINTER_NOT_CONNECTED : FETCH_UNREACHABLE;// 409 = no printer connected
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("JSON Error (printer)");
    setRGB(0, greenOFF, 0);
    return FETCH_UNREACHABLE;
  }

  bool flagOperational = doc["state"]["flags"]["operational"] | false;
  bool flagPrinting    = doc["state"]["flags"]["printing"]    | false;
  bool flagPausing     = doc["state"]["flags"]["pausing"]     | false;
  bool flagPaused      = doc["state"]["flags"]["paused"]      | false;
  bool flagCancelling  = doc["state"]["flags"]["cancelling"]  | false;
  bool flagError       = doc["state"]["flags"]["error"]       | false;

  nozzleTemp   = doc["temperature"]["tool0"]["actual"] | 0.0;
  nozzleTarget = doc["temperature"]["tool0"]["target"] | 0.0;
  bedTemp      = doc["temperature"]["bed"]["actual"]   | 0.0;
  bedTarget    = doc["temperature"]["bed"]["target"]   | 0.0;

  bool stillHeating = flagPrinting &&// printing but not up to temp yet
                       ((nozzleTarget > 0 && nozzleTemp < nozzleTarget - 3) ||
                        (bedTarget > 0 && bedTemp < bedTarget - 3));

  if (flagError) {// OctoPrint flags -> printState
    printState = "error";
  } else if (flagCancelling) {
    printState = "cancelled";
  } else if (flagPaused || flagPausing) {
    printState = "paused";
  } else if (flagPrinting) {
    printState = stillHeating ? "heating" : "printing";
  } else {
    printState = "standby";  // idle, offline or closed
  }

  toolheadX = toolheadY = toolheadZ = toolheadE = 0.0;// OctoPrint doesn't report position

  // ---- GET /api/job : progress, ETA, active file ----
  beginHTTP(http, printerURLJob);
  httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    setRGB(0, greenOFF, 0);
    return FETCH_UNREACHABLE;
  }
  payload = http.getString();
  http.end();
  setRGB(0, greenOFF, 0);

  JsonDocument jobDoc;
  error = deserializeJson(jobDoc, payload);
  if (error) {
    Serial.println("JSON Error (job)");
    return FETCH_UNREACHABLE;
  }

  float completion = jobDoc["progress"]["completion"] | 0.0;  // already 0-100
  progress = completion / 100.0;
  printDuration = jobDoc["progress"]["printTime"] | 0.0;
  totalDuration = printDuration;
  if (totalDuration > 0) savedTotalDuration = totalDuration;

  if (thePrintFile == "" && printState == "printing") {
    String rawName = jobDoc["job"]["file"]["name"] | "";
    if (rawName != "") {
      thePrintFileRaw = rawName;
      thePrintFile = extractFileName(rawName, false);
    }
  }

  return FETCH_OK;
}

// ============================================================
//  TOTAL TIME USED (shown at print end)
// ============================================================
void handleTimeUsed() {
  char buffer[30];
  int32_t totalSecs = (int32_t)savedTotalDuration;
  int hrs = totalSecs / 3600;
  int mins = (totalSecs % 3600) / 60;

  tft.fillRect(0, statusZoneY, 239, statusZoneH, TFT_BLACK);
  drawBmp(LittleFS, SUCCESS_IMAGE, graphicX, graphicY);

  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (hrs != 0) {
    sprintf(buffer, "%s: %1u:%02u %s", strTotal, hrs, mins, strHrs);
  } else {
    sprintf(buffer, "%s: %u %s", strTotal, mins, strMins);
  }
  tft.drawString(buffer, 120, endTimeY);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(thePrintFile, 120, filenameY);
  tft.unloadFont();

  justFinished = true;
  showIdle = true;
}

// ============================================================
//  PRINT FAILED (cancelled or errored out - shown at print end)
// ============================================================
void handlePrintFailed() {
  tft.fillRect(0, statusZoneY, 239, statusZoneH, TFT_BLACK);
  drawBmp(LittleFS, FAILURE_IMAGE, graphicX, graphicY);

  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString(strPrintFailed, 120, endTimeY);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(thePrintFile, 120, filenameY);
  tft.unloadFont();

  justFinished = true;
  showIdle = true;
}

// ============================================================
//  ESTIMATE TIME REMAINING
// ============================================================
void estimateTimeRemaining(float elapsedSeconds, float percentComplete, char *result) {
  if (percentComplete <= 0.0f || percentComplete > 100.0f) {
    snprintf(result, 16, "--:--");
    return;
  }
  float totalEstimated = elapsedSeconds / (percentComplete / 100.0f);
  float remainingSeconds = totalEstimated - elapsedSeconds;
  if (remainingSeconds < 0) remainingSeconds = 0;

  unsigned long remaining = (unsigned long)remainingSeconds;
  etaHH = remaining / 3600;
  etaMM = (remaining % 3600) / 60;
  snprintf(result, 16, "%02u:%02u", etaHH, etaMM);
}

// ============================================================
//  ETA
// ============================================================
void handleETA() {
  if (currentState != STATE_PRINTING) {
    activeETA = false;
    return;
  }
  char timeLeft[24];
  uint16_t pct = uint16_t(progress * 100);
  if (pct > lastETAProgress || !etaShownForThisPrint) {// own tracker, not lastProgress
    lastETAProgress = pct;
    etaShownForThisPrint = true;
    estimateTimeRemaining(printDuration, pct, timeLeft);

    // Build the end time string
    time_t utc = now();
    time_t localTime = toLocal(utc);
    time_t addSeconds = ((time_t)etaHH * 3600) + ((time_t)etaMM * 60);
    time_t futureTime = localTime + addSeconds;
    uint8_t theHour = (show24HR) ? hour(futureTime) : hourFormat12(futureTime);
    uint8_t theMinute = minute(futureTime);
    bool nextDay = (hour(futureTime) < hour(localTime) && etaHH > 0);

    char endStr[50];

    if (!show24HR) {
      const char *ampm = (hour(futureTime) >= 12) ? "PM" : "AM";
      if (nextDay) {
        sprintf(endStr, "ETA: %s  -  FPT: %u:%02u %s", timeLeft, theHour, theMinute, ampm);
      } else {
        sprintf(endStr, "ETA: %s  -  FPT: %u:%02u %s", timeLeft, theHour, theMinute, ampm);
      }
    } else {
      sprintf(endStr, "ETA: %s  -  FPT: %u:%02u", timeLeft, theHour, theMinute);
    }

    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextDatum(BC_DATUM);
    tft.fillRect(0, endTimeY - 16, 239, 18, TFT_BLACK);

    if (nextDay) {
      // Draw ETA part in white
      char etaPart[40];
      sprintf(etaPart, "ETA: %s  -  FPT: ", timeLeft);
      char endPart[16];
      if (!show24HR) {
        const char *ampm = (hour(futureTime) >= 12) ? "PM" : "AM";
        sprintf(endPart, "%u:%02u %s", theHour, theMinute, ampm);
      } else {
        sprintf(endPart, "%u:%02u", theHour, theMinute);
      }
      // Measure ETA part width to position End part
      int16_t etaWidth = tft.textWidth(etaPart);
      int16_t totalWidth = etaWidth + tft.textWidth(endPart);
      int16_t startX = 120 - (totalWidth / 2);
      tft.setTextDatum(BL_DATUM);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(etaPart, startX, endTimeY);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString(endPart, startX + etaWidth, endTimeY);
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(endStr, 120, endTimeY);
    }

    tft.unloadFont();
    activeETA = true;
  }
}
// ============================================================
//  PHASE 2 — DETERMINE STATE
// ============================================================
PrinterState determinePrinterState() {
  lastState = currentState;

  if (printState == "canceled" || printState == "cancelled" || printState == "error") {// both spellings
    return STATE_FAILED;
  }

  if (printState == "paused") {
    return STATE_PAUSED;
  }

  if (printState == "standby") {
    if (lastState == STATE_PRINTING || lastState == STATE_PREP || lastState == STATE_PAUSED) {// print just stopped
      float endProgress = max(lastKnownProgress, progress);// also use OctoPrint's own %
      uint16_t lastPct = (uint16_t)(endProgress * 100.0);
      return (lastPct >= 99) ? STATE_COMPLETE : STATE_FAILED;// under 99% = cancelled
    }
    return STATE_IDLE;
  }

  if (printState == "heating") {
    return (lastState == STATE_PRINTING) ? STATE_PRINTING : STATE_PREP;// ignore mid-print temp dips
  }

  return STATE_PRINTING;  // printState == "printing"
}

// ============================================================
//  PHASE 3 — UPDATE DISPLAY
// ============================================================
void updatePrinterDisplay(PrinterState state) {
  tft.setTextDatum(MC_DATUM);

  // Always update temperature gauges
  if (round(nozzleTemp) != lastNozzleTemp) {
    lastNozzleTemp = round(nozzleTemp);
    handleGauge(nozzleGauge, lastNozzleTemp);
  }
  if (round(bedTemp) != lastBedTemp) {
    lastBedTemp = round(bedTemp);
    handleGauge(bedGauge, lastBedTemp);
  }

  switch (state) {

    case STATE_IDLE:
      tft.drawSmoothArc(120, gaugeY, 32, 22, 40, 320, TFT_DARKGREY, TFT_BLACK, false);
      tft.fillCircle(120, gaugeY, 20, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(strIdle, 120, dataY, 2);

      if (lastState == STATE_PRINTING || lastState == STATE_PREP || lastState == STATE_PAUSED || lastState == STATE_FAILED) {// clear old filename, redraw idle
        thePrintFile = "";
        thePrintFileRaw = "";
        showIdle = false;
      }

      if (!showIdle && !justFinished) {
        tft.fillRect(0, statusZoneY, 239, statusZoneH, TFT_BLACK);
        drawBmp(LittleFS, IDLE_IMAGE, graphicX + 7, graphicY + 7);
        showIdle = true;
      }
      activeETA = false;
      break;

    case STATE_PREP:
      if (lastState != STATE_PREP) showIdle = false;// so the heating graphic draws
      tft.drawSmoothArc(120, gaugeY, 32, 22, 40, 320, TFT_DARKGREY, TFT_BLACK, false);
      tft.fillCircle(120, gaugeY, 20, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawString(strPrep, 120, dataY, 2);
      if (!showIdle) {
        tft.fillRect(0, statusZoneY, 239, statusZoneH, TFT_BLACK);
        drawBmp(LittleFS, HEATING_IMAGE, graphicX, graphicY);
        showIdle = true;
      }
      activeETA = false;
      break;

    case STATE_PRINTING:
      showIdle = false;
      justFinished = false;
      // Serial.println("STATE_PRINTING - lastState: " + String(lastState) + " thePrintFile: " + thePrintFile);
      if (thePrintFile != "" && lastState != STATE_PRINTING && lastState != STATE_PAUSED) {
        // Serial.println("Thumbnail block firing - lastState: " + String(lastState));
        // Serial.println("thePrintFile on reconnect: " + thePrintFile);
        ntfyResetForNewPrint();
        tft.fillRect(0, statusZoneY, 239, statusZoneH, TFT_BLACK);
        if (!fetchAndDrawThumbnail()) {
          drawBmp(LittleFS, PRINTING_IMAGE, graphicX + 7, graphicY + 7);
        }
        tft.loadFont(AA_FONT_SMALL, LittleFS);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(BC_DATUM);
        tft.fillRect(0, filenameY - 14, 239, 16, TFT_BLACK);  // clear filename line
        tft.drawString(thePrintFile, 120, filenameY);
        tft.unloadFont();
        handleGauge(progressGauge, 0);  // start at 0%
        lastProgress = 0;
        lastKnownProgress = 0.0;        // reset for the new print
        lastETAProgress = 0;
        etaShownForThisPrint = false;   // draw ETA at least once
      }

      progressPercent = uint16_t(progress * 100.0);
      if (progressPercent != lastProgress) {
        lastProgress = progressPercent;
        handleGauge(progressGauge, lastProgress);
      }
      lastKnownProgress = progress;
     ntfyCheckStall(progress, toolheadX, toolheadY, toolheadZ, toolheadE);

      // ── Chamber temp ──────────────────────────────────────
      if (hasChamber) {
        tft.loadFont(AA_FONT_SMALL, LittleFS);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("CHMBR", chamberX, chamberY);
        tft.fillRect(0, chamberY + 6, 63, 18, TFT_BLACK);  // clear number line only
        tft.setTextColor((chamberTarget > 0) ? TFT_RED : TFT_WHITE, TFT_BLACK);
        tft.drawString(String((int)chamberTemp) + "C", chamberX, chamberY + 18);
        tft.unloadFont();
      }
      break;

    case STATE_PAUSED:
      progressPercent = uint16_t(progress * 100.0);
      handleGauge(progressGauge, progressPercent);// arc turns orange
      tft.loadFont(AA_FONT_SMALL, LittleFS);
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.setTextDatum(BC_DATUM);
      tft.fillRect(0, endTimeY - 16, 239, 18, TFT_BLACK);
      tft.drawString(strPrinterPaused, 120, endTimeY);// replaces the ETA line
      tft.unloadFont();
      activeETA = false;
      break;

    case STATE_COMPLETE:
      lastProgress = 0;
      handleGauge(progressGauge, 0);

      if (thePrintFile != "") {
        ntfyPrintComplete(thePrintFileRaw, savedTotalDuration);  // send notification
        tft.fillRect(0, filenameY - 14, 239, 16, TFT_BLACK);
        handleTimeUsed();
      }
      thePrintFile = "";
      showIdle = true;
      activeETA = false;
      break;

    case STATE_FAILED:
      lastProgress = 0;
      handleGauge(progressGauge, 0);

      if (thePrintFile != "") {
        tft.fillRect(0, filenameY - 14, 239, 16, TFT_BLACK);
        handlePrintFailed();
      }
      thePrintFile = "";
      showIdle = true;
      activeETA = false;
      break;
  }
}

// ============================================================
//  handlePrinterStatus
// ============================================================
void handlePrinterStatus() {
  static uint8_t failCount = 0;

  FetchResult result = fetchPrinterData();

  if (result != FETCH_OK) {
    failCount++;
    Serial.println("fetchPrinterData failed - count: " + String(failCount));

    if (failCount >= 3) {  // 3 failures in a row (~30 s)
      failCount = 0;

      if (result == FETCH_PRINTER_NOT_CONNECTED) {
        if (connState != CONN_PRINTER_OFFLINE) {// draw once, no flicker
          connState = CONN_PRINTER_OFFLINE;
          drawPrinterOffline();
        }
      } else {
        connState = CONN_OCTOPRINT_OFFLINE;// OctoPrint gone, start over via handleHostName()
        printerName = "";
        showSleep = false;
        showIdle = false;
      }
    }
    return;
  }

  failCount = 0;  // reset on success

  if (connState != CONN_READY) {// back online, redraw name and headings
    connState = CONN_READY;
    tft.fillRect(0, belowClockY, 239, SCREEN_H - belowClockY, TFT_BLACK);
    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextDatum(BC_DATUM);
    tft.drawString(printerName, 120, printerNameY);
    tft.unloadFont();
    handleGaugeHeadings();
    drawVersionString();
    showIdle = false;
    showSleep = false;
    justFinished = false;// or the idle graphic never draws
    lastNozzleTemp = lastBedTemp = lastProgress = 9999;  // force gauge redraw
  }

  currentState = determinePrinterState();
  updatePrinterDisplay(currentState);
  handleETA();
}

// ============================================================
//  WIFI CONFIG AP SCREEN
// ============================================================
void configModeCallback(WiFiManager *myWiFiManager) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(String(hostNameCYD), SCREEN_W / 2, SCREEN_H / 2, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.loadFont(AA_FONT_SMALL, LittleFS);
  tft.drawString(strAPActive, SCREEN_W / 2, (SCREEN_H / 2) + 36, 2);
  tft.unloadFont();
  delay(2000);
}

// ============================================================
//  nfty setup
// ============================================================

String ntfyServerDisplay() {
  String s = ntfyServer;
  s.replace("https://", "");
  s.replace("http://", "");
  // Strip port if present
  int colonIdx = s.lastIndexOf(':');
  if (colonIdx > 0) s = s.substring(0, colonIdx);
  return s;
}

// ============================================================
//  WEB PAGE — dark theme
// ============================================================
String SendHTML(String saveBanner) {
  String page = String(HTML_TEMPLATE);

  // Header
  page.replace("%VERSION%", String(VERSION));
  page.replace("%WIFI_QUALITY%", String(getWifiQuality()));

  page.replace("%SAVE_BANNER%", saveBanner);// empty unless just saved

  // WiFi reset modal
  page.replace("%WIFI_RESET_TITLE%", String(wcWifiResetTitle));
  page.replace("%WIFI_RESET_BODY%", String(wcWifiResetBody));
  page.replace("%WIFI_RESET_YES%", String(wcWifiResetYes));
  page.replace("%WIFI_RESET_CANCEL%", String(wcWifiResetCancel));
  page.replace("%WIFI_RESET_BTN%", String(wcWifiResetBtn));

  // Section headings
  page.replace("%SEC_PRINTER%", String(wcSecPrinter));
  page.replace("%SEC_NTFY%", String(wcSecNtfy));
  page.replace("%SEC_WIFI%", String(wcSecWifi));

  // Printer setup block
  page.replace("%PRINTER_SETUP%", getPrinterSetup());

  // Ntfy labels
  page.replace("%NTFY_ENABLED%", String(wcNtfyEnabled));
  page.replace("%NTFY_SERVER%", String(wcNtfyServer));
  page.replace("%NTFY_PORT%", String(wcNtfyPort));
  page.replace("%NTFY_TOPIC%", String(wcNtfyTopic));
  page.replace("%NTFY_TOKEN%", String(wcNtfyToken));
  page.replace("%NTFY_STALL_MIN%", String(wcNtfyStallMin));

  // Ntfy values
  page.replace("%NTFY_ENABLED_CHECKED%", ntfyEnabled ? " checked" : "");
  page.replace("%NTFY_SERVER_VAL%", ntfyServerDisplay());
  page.replace("%NTFY_PORT_VAL%", ntfyPort);
  page.replace("%NTFY_TOPIC_VAL%", ntfyTopic);
  page.replace("%NTFY_TOKEN_VAL%", ntfyToken);
  page.replace("%NTFY_STALL_MIN_VAL%", String(ntfyStallMin));

  // Save button
  page.replace("%SAVE_BTN%", String(wcSaveBtn));

  return page;
}

String buildSaveBanner(KeyTestResult result) {// green "saved" or red reason after a save
  String cssClass, text;
  switch (result) {
    case KEYTEST_OK:
      cssClass = "ok";
      text = String(wcSaved);
      break;
    case KEYTEST_BAD_KEY:
      cssClass = "err";
      text = String(wcSaveFailedBadKey);
      break;
    case KEYTEST_NOT_FOUND:
    default:
      cssClass = "err";
      text = String(wcSaveFailedNotFound);
      break;
  }
  return "<div class=\"save-banner " + cssClass + "\">" + text + "</div>";
}

void handlePrinterUpdate() {
  if (server.hasArg("printerIP")) printerIP = server.arg("printerIP");
  if (server.hasArg("printerPort")) printerPort = server.arg("printerPort");
  if (server.hasArg("apiKey")) {
    apiKey = server.arg("apiKey");
    apiKey.trim();
  }
  show24HR = server.hasArg("show24HR");
  ntfyEnabled = server.hasArg("ntfyEnabled");
  if (server.hasArg("ntfyServer")) {
    ntfyServer = server.arg("ntfyServer");
    ntfyServer.trim();
  }
  if (server.hasArg("ntfyPort")) {
    ntfyPort = server.arg("ntfyPort");
    ntfyPort.trim();
  }
  if (server.hasArg("ntfyTopic")) {
    ntfyTopic = server.arg("ntfyTopic");
    ntfyTopic.trim();
  }
  if (server.hasArg("ntfyToken")) {
    ntfyToken = server.arg("ntfyToken");
    ntfyToken.trim();
  }
  if (server.hasArg("ntfyStallMin")) ntfyStallMin = server.arg("ntfyStallMin").toInt();

  // Build the full ntfy URL
  if (!ntfyServer.startsWith("http://") && !ntfyServer.startsWith("https://")) {
    bool isIP = true;
    for (char c : ntfyServer) {
      if (!isDigit(c) && c != '.') {
        isIP = false;
        break;
      }
    }
    ntfyServer = isIP ? "http://" + ntfyServer + ":" + ntfyPort
                      : "https://" + ntfyServer;
  }

  writeSettings();  // ← everything is set before we save

  // Reset display
  tft.fillRect(0, clockBottomY, 239, SCREEN_H - clockBottomY, TFT_BLACK);
  drawVersionString();
  drawConnecting();
  printerName = "";
  showSleep = false;
  showIdle = false;
  justFinished = false;
  currentState = STATE_IDLE;
  lastState = STATE_IDLE;
  connState = CONN_BOOTING;
  forcePoll = true;
  buildPrinterURLs();

  KeyTestResult testResult = testOctoPrintConnection();// test the new address and key now
  server.send(200, "text/html", SendHTML(buildSaveBanner(testResult)));
}

String getPrinterSetup() {
  String printerForm = String(printer_Info);
  printerForm.replace("%LBL_PRINTER_IP%", String(wcPrinterIP));
  printerForm.replace("%LBL_PRINTER_PORT%", String(wcPrinterPort));
  printerForm.replace("%LBL_API_KEY%", String(wcApiKey));
  printerForm.replace("%LBL_CLOCK24%", String(wcClock24));
  printerForm.replace("%IP%", printerIP);
  printerForm.replace("%PORT%", printerPort);
  printerForm.replace("%APIKEY%", apiKey);
  printerForm.replace("%boxState%", show24HR ? "checked" : "unchecked");
  return printerForm;
}

void writeSettings() {
  File f = LittleFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("Settings write failed!");
    return;
  }
  f.println("printerIP=" + printerIP);
  f.println("printerPort=" + printerPort);
  f.println("apiKey=" + apiKey);
  f.println("show24HR=" + String(show24HR));
  f.println("ntfyPort=" + ntfyPort);  // write the port number if there is one
  f.println("ntfyEnabled=" + String(ntfyEnabled));
  f.println("ntfyServer=" + ntfyServer);
  f.println("ntfyTopic=" + ntfyTopic);
  f.println("ntfyToken=" + ntfyToken);
  f.println("ntfyStallMin=" + String(ntfyStallMin));
  f.println("tzOffset=" + String(tzOffset));
  f.close();
}

void readSettings() {
  if (!LittleFS.exists(CONFIG)) {
    writeSettings();
    return;
  }
  File fr = LittleFS.open(CONFIG, "r");
  String line;
  while (fr.available()) {
    line = fr.readStringUntil('\n');
    if (line.indexOf("printerIP=") >= 0) {
      printerIP = line.substring(10);
      printerIP.trim();
    }
    if (line.indexOf("printerPort=") >= 0) {
      printerPort = line.substring(12);
      printerPort.trim();
    }
    if (line.indexOf("apiKey=") >= 0) {
      apiKey = line.substring(7);
      apiKey.trim();
    }
    if (line.indexOf("show24HR=") >= 0) show24HR = line.substring(9).toInt();
    if (line.indexOf("ntfyPort=") >= 0) {
      ntfyPort = line.substring(9);
      ntfyPort.trim();
    };  // read custom port local host
    if (line.indexOf("ntfyEnabled=") >= 0) ntfyEnabled = line.substring(12).toInt();
    if (line.indexOf("ntfyServer=") >= 0) {
      ntfyServer = line.substring(11);
      ntfyServer.trim();
    }
    if (line.indexOf("ntfyTopic=") >= 0) {
      ntfyTopic = line.substring(10);
      ntfyTopic.trim();
    }
    if (line.indexOf("ntfyToken=") >= 0) {
      ntfyToken = line.substring(10);
      ntfyToken.trim();
    }
    if (line.indexOf("ntfyStallMin=") >= 0) ntfyStallMin = line.substring(13).toInt();
    if (line.indexOf("tzOffset=") >= 0) tzOffset = line.substring(9).toInt();
  }
  fr.close();
}

void buildPrinterURLs() {
  printerURLInfo = "http://" + printerIP + ":" + printerPort + printerINFO;
  printerURLQ = "http://" + printerIP + ":" + printerPort + printQuery;
  printerURLJob = "http://" + printerIP + ":" + printerPort + printerJOB;
  // Serial.println("printerURLQ: " + printerURLQ);
  // Serial.println("printerURLJob: " + printerURLJob);
}

// ============================================================
//  BMP DRAW FROM LittleFS
// ============================================================
void drawBmp(fs::FS &fs, const char *filename, int16_t x, int16_t y) {
  if ((x >= tft.width()) || (y >= tft.height())) return;

  File bmpFS = fs.open(filename, "r");
  if (!bmpFS) {
    Serial.print("BMP not found: ");
    Serial.println(filename);
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t r, g, b;

  if (read16(bmpFS) == 0x4D42) {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)) {
      y += h - 1;
      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        uint16_t *tptr = (uint16_t *)lineBuffer;
        for (uint16_t col = 0; col < w; col++) {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }
        tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    } else {
      Serial.println("BMP must be 24-bit uncompressed.");
    }
  }
  bmpFS.close();
}

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}

// ============================================================
//  FILENAME HELPER
// ============================================================
String extractFileName(const String &path, bool withExt) {
  int slashIdx = path.lastIndexOf('/');
  int start = (slashIdx >= 0) ? slashIdx + 1 : 0;

  String result;
  if (!withExt) {
    int dotIdx = path.lastIndexOf('.');
    int end = (dotIdx > start) ? dotIdx : path.length();
    result = path.substring(start, end);
  } else {
    result = path.substring(start);
  }

  if (result.length() > 28) result = result.substring(0, 28) + "~";
  return result;
}

// ============================================================
//  WIFI RESET
// ============================================================
void handleWifiReset() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(strWifiReset, SCREEN_W / 2, SCREEN_H / 2, 2);
  delay(5000);
  redirectHome();
  delay(1000);
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

void redirectHome() {
  server.sendHeader("Location", String("/"), true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");
  server.client().stop();
}

int pngDraw(PNGDRAW *pDraw) {// PNGdec callback, one row at a time, scaled to fit the box
  static uint16_t srcLine[THUMB_MAX_SRC_WIDTH];
  static uint16_t outLine[THUMB_MAX_SRC_WIDTH];
  png.getLineAsRGB565(pDraw, srcLine, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

  if (thumbIsUpscale) {// small thumbnail, pixel-doubled
    for (int x = 0; x < thumbSrcW; x++) {
      for (int rep = 0; rep < THUMB_UPSCALE_FACTOR; rep++) {
        outLine[x * THUMB_UPSCALE_FACTOR + rep] = srcLine[x];
      }
    }
    int baseY = graphicY + thumbOffsetY + (pDraw->y * THUMB_UPSCALE_FACTOR);
    for (int rep = 0; rep < THUMB_UPSCALE_FACTOR; rep++) {
      tft.pushImage(graphicX + thumbOffsetX, baseY + rep, thumbOutW, 1, outLine);
    }
  } else {// large thumbnail, scaled down
    int dstY = (pDraw->y * thumbOutH) / thumbSrcH;
    if (dstY == thumbLastDstRow) return 1;// row already drawn
    thumbLastDstRow = dstY;

    for (int dx = 0; dx < thumbOutW; dx++) {
      int sx = (dx * thumbSrcW) / thumbOutW;
      outLine[dx] = srcLine[sx];
    }
    tft.pushImage(graphicX + thumbOffsetX, graphicY + thumbOffsetY + dstY, thumbOutW, 1, outLine);
  }
  return 1;  // return 1 to continue decoding
}

bool fetchAndDrawThumbnail() {
  String thumbFile = thePrintFileRaw;
  int slashIdx = thumbFile.lastIndexOf('/');
  if (slashIdx >= 0) thumbFile = thumbFile.substring(slashIdx + 1);
  int dotIdx = thumbFile.lastIndexOf('.');
  if (dotIdx > 0) thumbFile = thumbFile.substring(0, dotIdx);

  String encodedFile = thumbFile;
  encodedFile.replace(" ", "%20");
  encodedFile.replace("(", "%28");
  encodedFile.replace(")", "%29");
  encodedFile.replace("[", "%5B");
  encodedFile.replace("]", "%5D");
  encodedFile.replace("&", "%26");
  encodedFile.replace("+", "%2B");

  String thumbURL = "http://" + printerIP + ":" + printerPort + "/plugin/prusaslicerthumbnails/thumbnail/" + encodedFile + ".png";// Slicer Thumbnails plugin

  httpThumb.begin(thumbURL);
  httpThumb.addHeader("X-Api-Key", apiKey);
  int httpCode = httpThumb.GET();

  if (httpCode != 200) {
    Serial.println("Thumbnail fetch failed (" + String(httpCode) + "): " + thumbURL);
    httpThumb.end();
    return false;
  }

  thumbBufferSize = httpThumb.getSize();
  if (thumbBufferSize <= 0) {
    Serial.println("Thumb: invalid content length, aborting");
    httpThumb.end();
    return false;
  }

  thumbBuffer = (uint8_t *)malloc(thumbBufferSize);
  if (!thumbBuffer) {
    Serial.println("malloc failed!");
    httpThumb.end();
    return false;
  }

  WiFiClient *stream = httpThumb.getStreamPtr();
  int bytesRead = 0;
  while (httpThumb.connected() && bytesRead < thumbBufferSize) {
    if (stream->available()) {
      thumbBuffer[bytesRead++] = stream->read();
    }
  }
  httpThumb.end();

  Serial.println("Bytes read: " + String(bytesRead));

  int rc = png.openRAM(thumbBuffer, bytesRead, pngDraw);
  if (rc == PNG_SUCCESS) {
    thumbSrcW = png.getWidth();
    thumbSrcH = png.getHeight();
    thumbLastDstRow = -1;

    if (thumbSrcW > THUMB_MAX_SRC_WIDTH) {// too wide for the row buffers
      Serial.println("Thumbnail too wide (" + String(thumbSrcW) + "px, max " + String(THUMB_MAX_SRC_WIDTH) + "px), skipping");
      png.close();
      free(thumbBuffer);
      thumbBuffer = nullptr;
      return false;
    }

    if (thumbSrcW <= THUMB_UPSCALE_THRESHOLD) {// e.g. 16x16
      thumbIsUpscale = true;
      thumbOutW = thumbSrcW * THUMB_UPSCALE_FACTOR;
      thumbOutH = thumbSrcH * THUMB_UPSCALE_FACTOR;
    } else {// e.g. 200x200
      thumbIsUpscale = false;
      thumbOutW = THUMB_BOX_SIZE;
      thumbOutH = THUMB_BOX_SIZE;
    }
    thumbOffsetX = (THUMB_BOX_SIZE - thumbOutW) / 2;
    thumbOffsetY = (THUMB_BOX_SIZE - thumbOutH) / 2;

    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();
    png.close();
  } else {
    Serial.println("PNG open failed: " + String(rc));
  }

  free(thumbBuffer);
  thumbBuffer = nullptr;

  return (rc == PNG_SUCCESS);
}