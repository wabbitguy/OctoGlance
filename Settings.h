/* ESP32 Dev Module 
   NO OTA 2MB APP/2MB SPIFFS
*/

// ── Language ───────────────────────────────────────────────
// Uncomment ONE language only. (The ready-made .bin files on GitHub
// choose their language when they're built, which skips this list.)
#ifndef LANG_FROM_BUILD
#define LANG_EN   // English  (default)
//#define LANG_FR   // French
//#define LANG_DE   // German
//#define LANG_ES   // Spanish
//#define LANG_NL   // Dutch
//#define LANG_PT   // Portuguese
//#define LANG_TR   // Turkish
#endif

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <time.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <FS.h>
#include "LittleFS.h"
#include <WebServer.h>
#include <PNGdec.h>

int32_t tzOffset = -25200;// -25200 = UTC-7 (PDT).  Updated automatically at 2am and on web save.

// IP information of your printer on the LAN
String printerIP = "192.168.1.6";  // default IP, set yours via the web page
String printerPort = "80";         // OctoPi/native default
String apiKey = "";                // OctoPrint: Settings -> Application Keys

// ── ntfy Push Notifications ────────────────────────────────
bool    ntfyEnabled  = false;
String  ntfyServer   = "http://192.168.1.82:2586";// local hosted or https://ntfy.sh
String  ntfyTopic    = "octoglance";
String  ntfyToken    = "";
String  ntfyPort     = "2586";// can be any port you want if local hosting

// -- URL query information from the printer (OctoPrint REST endpoints)
String printQuery = "/api/printer?exclude=sd,history";
uint8_t ntfyStallMin = 5;  // stall alert after this many minutes
String printerINFO = "/api/settings";  // printer name
String printerJOB = "/api/job";        // progress / ETA / active file
String printerURLQ = "";
String printerURLInfo = "";
String printerURLJob = "";
String thePrintFile = "";  // filename of the currently printing file

bool show24HR = false;// show 24hr clock

// ---------------- GRAPHICS (in LittleFS /data folder) ----------------
#define OFFLINE_IMAGE "/OctoOffLine.bmp"  // 24-bit BMP, 186 x 186 - OctoPrint not answering
#define IDLE_IMAGE "/Idle.bmp"          // 24-bit BMP, 110 x 110
#define HEATING_IMAGE "/Heating.bmp"    // 24-bit BMP, 110 x 110
#define PRINTING_IMAGE "/Printing.bmp"  // 24-bit BMP, 110 x 110
#define SUCCESS_IMAGE "/Success.bmp"    // 24-bit BMP, 110 x 110
#define FAILURE_IMAGE "/Failure.bmp"    // 24-bit BMP, 110 x 110 - cancelled or error

// ---------------- FONTS (in LittleFS /data folder) ----------------
#define AA_FONT_SMALL "NotoSansBold15"  // 15 point sans serif bold
#define AA_FONT_LARGE "NotoSansBold36"  // 36 point sans serif bold

uint8_t maxBedTemp = 180; //maximum for bed temp 
uint16_t maxNozzleTemp = 450;//maximum for nozzle temp 

// ---- setup for Klipper printers with heated chambers ----
bool hasChamber = false;
float chamberTemp = 0.0;
float chamberTarget = 0.0;
String chamberSensorName = "";// depends what the brand calls their chamber

PNG png;
HTTPClient httpThumb;
uint8_t *thumbBuffer = nullptr;
int thumbBufferSize = 0;

// ---- Thumbnail scaling ----
#define THUMB_BOX_SIZE 110          // thumbnails are scaled to fit this box
#define THUMB_UPSCALE_THRESHOLD 32  // this size or smaller gets pixel-doubled
#define THUMB_UPSCALE_FACTOR 2
#define THUMB_MAX_SRC_WIDTH 512     // wider thumbnails are skipped
int thumbSrcW = 0, thumbSrcH = 0;        // size of the PNG
int thumbOutW = 0, thumbOutH = 0;        // size drawn on screen
int thumbOffsetX = 0, thumbOffsetY = 0;  // centres it in the box
bool thumbIsUpscale = false;
int thumbLastDstRow = -1;

// ---------------- WEB PAGE TEMPLATE ----------------
const char printer_Info[] PROGMEM = R"rawliteral(
<form action="/updatePrinterInfo">
  <div class="field">
    <label>%LBL_PRINTER_IP%</label>
    <input type="text" name="printerIP" value="%IP%">
  </div>
  <div class="field">
    <label>%LBL_PRINTER_PORT%</label>
    <input type="text" name="printerPort" value="%PORT%">
  </div>
  <div class="field">
    <label>%LBL_API_KEY%</label>
    <input type="text" name="apiKey" value="%APIKEY%">
  </div>
  <div class="field">
    <label>%LBL_CLOCK24%</label>
    <label class="toggle">
      <input type="checkbox" name="show24HR" %boxState%>
      <span class="slider"></span>
    </label>
  </div>
)rawliteral";

// ============================================================
//  MAIN WEB PAGE TEMPLATE — stored in flash
// ============================================================
const char HTML_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>OctoGlance</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: Helvetica, sans-serif; background: #1a1a1a; color: #ccc; }
.container { max-width: 800px; margin: 0 auto; padding: 20px; }
header { text-align: center; padding: 30px 0 20px; }
header h1 { font-size: 2em; color: #00bcd4; margin-bottom: 6px; }
header h3 { font-size: 1em; color: #666; font-weight: normal; }
header h4 { font-size: 0.9em; color: #555; font-weight: normal; margin-top: 4px; }
.card { background: #252525; border-radius: 12px; padding: 24px; margin-bottom: 20px; border: 1px solid #333; }
.card h2 { font-size: 0.8em; color: #00bcd4; text-transform: uppercase; letter-spacing: 2px; margin-bottom: 16px; }
.field { display: flex; align-items: center; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #333; }
.field:last-of-type { border-bottom: none; }
.field label { font-size: 0.95em; color: #ccc; }
.field input[type=text] { width: 180px; padding: 8px 12px; background: #333; border: 1px solid #444; border-radius: 6px; font-size: 0.95em; color: #fff; }
.field input[type=text]:focus { outline: none; border-color: #00bcd4; }
.field input[name=apiKey] { width: 380px; font-family: monospace; }
.toggle { position: relative; width: 46px; height: 26px; }
.toggle input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #444; border-radius: 26px; transition: 0.3s; }
.slider:before { position: absolute; content: ''; height: 20px; width: 20px; left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: 0.3s; }
input:checked + .slider { background: #00bcd4; }
input:checked + .slider:before { transform: translateX(20px); }
.btn { display: block; width: 100%; padding: 14px; border: none; border-radius: 8px; font-size: 1em; font-weight: bold; cursor: pointer; margin-top: 16px; letter-spacing: 1px; }
.btn-update { background: #00bcd4; color: #111; }
.btn-update:hover { background: #00acc1; }
.btn-reset { background: #c0392b; color: white; }
.btn-reset:hover { background: #a93226; }
.mbg{display:none;position:fixed;inset:0;background:rgba(0,0,0,.75);z-index:200;align-items:center;justify-content:center}
.mbg.open{display:flex}
.modal{background:#252525;border:1px solid #c0392b;border-radius:12px;padding:28px 24px;max-width:340px;width:90%;text-align:center}
.modal h3{color:#c0392b;margin-bottom:12px;font-size:17px}
.modal p{color:#888;font-size:14px;margin-bottom:20px;line-height:1.5}
.mbtns{display:flex;gap:10px;justify-content:center}
.mok{background:#c0392b;border:none;color:#fff;padding:9px 22px;border-radius:7px;font-size:14px;font-weight:600;cursor:pointer}
.mno{background:transparent;border:1px solid #888;color:#888;padding:9px 22px;border-radius:7px;font-size:14px;cursor:pointer}
.save-banner{margin-bottom:20px;padding:14px 18px;border-radius:10px;font-size:0.9em;border:1px solid;background:#252525}
.save-banner.ok{border-color:#00bcd4;color:#00bcd4}
.save-banner.err{border-color:#c0392b;color:#c0392b}
</style></head>
<body><div class="container">
<div class='mbg' id='wm'><div class='modal'>
<h3>%WIFI_RESET_TITLE%</h3>
<p>%WIFI_RESET_BODY%</p>
<div class='mbtns'>
<button class='mok' onclick='window.location="/wifiReset"'>%WIFI_RESET_YES%</button>
<button class='mno' onclick='document.getElementById("wm").classList.remove("open")'>%WIFI_RESET_CANCEL%</button>
</div></div></div>
<header>
<h1>OctoGlance</h1>
<h3>WabbitWanch Design</h3>
<h4>Version %VERSION% &nbsp;&bull;&nbsp; WiFi %WIFI_QUALITY%%</h4>
</header>
%SAVE_BANNER%
<div class="card">
<h2>%SEC_PRINTER%</h2>
%PRINTER_SETUP%
</div>
<div class="card">
<h2>%SEC_NTFY%</h2>
<div class="field"><label>%NTFY_ENABLED%</label>
<label class="toggle"><input type="checkbox" name="ntfyEnabled"%NTFY_ENABLED_CHECKED%><span class="slider"></span></label></div>
<div class="field"><label>%NTFY_SERVER%</label>
<input type="text" name="ntfyServer" value="%NTFY_SERVER_VAL%"></div>
<div class="field"><label>%NTFY_PORT%</label>
<input type="text" name="ntfyPort" value="%NTFY_PORT_VAL%"></div>
<div class="field"><label>%NTFY_TOPIC%</label>
<input type="text" name="ntfyTopic" value="%NTFY_TOPIC_VAL%"></div>
<div class="field"><label>%NTFY_TOKEN%</label>
<input type="text" name="ntfyToken" value="%NTFY_TOKEN_VAL%"></div>
<div class="field"><label>%NTFY_STALL_MIN%</label>
<input type="text" name="ntfyStallMin" value="%NTFY_STALL_MIN_VAL%"></div>
</div>
<button class="btn btn-update" type="submit">%SAVE_BTN%</button>
</form>
<div class="card">
<h2>%SEC_WIFI%</h2>
<button class="btn btn-reset" onclick="document.getElementById('wm').classList.add('open')">%WIFI_RESET_BTN%</button>
</div>
</div></body></html>
)rawliteral";
