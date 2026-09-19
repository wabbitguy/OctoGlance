#pragma once
#include <HTTPClient.h>

// ── ntfy Notification Support ─────────────────────────────
// Configured at runtime via the OctoGlance web UI
// Variables defined in Settings.h:
//   ntfyEnabled, ntfyServer, ntfyTopic, ntfyToken, ntfyStallMin

static uint32_t ntfyLastProgressTime = 0;
static float    ntfyLastProgress     = 0.0f;
static bool     ntfyStallFired       = false;
static bool     ntfyDoneFired        = false;

// Toolhead position baseline — used alongside progress% so a plateau in
// display_status.progress (which is byte-position based when no M73 is
// present, and not time-linear) doesn't get misread as a real stall.
static bool  ntfyHavePos = false;
static float ntfyLastX = 0.0f, ntfyLastY = 0.0f, ntfyLastZ = 0.0f, ntfyLastE = 0.0f;

void ntfySend(const char* message, const char* title, const char* priority) {
  if (!ntfyEnabled) return;
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = ntfyServer + "/" + ntfyTopic;
  http.begin(url);
  http.addHeader("Title",        title);
  http.addHeader("Priority",     priority);
  http.addHeader("Content-Type", "text/plain");
  if (ntfyToken != "") {
    http.addHeader("Authorization", "Bearer " + ntfyToken);
  }
  http.POST(message);
  http.end();
}

// currentProgress: display_status.progress (0.0–1.0)
// x,y,z,e: toolhead.position — real motion, sampled every poll.
// A stall is only declared when BOTH progress% and toolhead position have
// been flat for ntfyStallMin minutes. Progress alone plateaus on long/complex
// prints even while the printer is genuinely moving, which caused false
// "no progress" alerts on long prints; toolhead position catches real motion
// that the file-position-based progress metric misses.
void ntfyCheckStall(float currentProgress, float x, float y, float z, float e) {
  bool progressMoved = currentProgress > ntfyLastProgress + 0.001f;
  bool positionMoved = !ntfyHavePos ||
                        fabs(x - ntfyLastX) > 0.005f ||
                        fabs(y - ntfyLastY) > 0.005f ||
                        fabs(z - ntfyLastZ) > 0.005f ||
                        fabs(e - ntfyLastE) > 0.005f;

  if (progressMoved || positionMoved) {
    if (progressMoved) ntfyLastProgress = currentProgress;
    ntfyLastX = x; ntfyLastY = y; ntfyLastZ = z; ntfyLastE = e;
    ntfyHavePos           = true;
    ntfyLastProgressTime  = millis();
    ntfyStallFired        = false;
    return;
  }

  if (!ntfyStallFired && ntfyLastProgressTime > 0) {
    uint32_t stallMs = (uint32_t)ntfyStallMin * 60UL * 1000UL;
    if ((millis() - ntfyLastProgressTime) >= stallMs) {
      ntfySend("No print progress or toolhead motion — filament change, runout, or jam?",
               "Printer Needs Attention", "high");
      ntfyStallFired = true;
    }
  }
}

void ntfyPrintComplete(const String& rawPath, float totalSecs) {
  if (ntfyDoneFired) return;

  // Strip path and extension for clean display name
  String filename = rawPath;
  int slashIdx = filename.lastIndexOf('/');
  if (slashIdx >= 0) filename = filename.substring(slashIdx + 1);
  int dotIdx = filename.lastIndexOf('.');
  if (dotIdx > 0) filename = filename.substring(0, dotIdx);

  int hrs  = (int)totalSecs / 3600;
  int mins = ((int)totalSecs % 3600) / 60;
  char msg[80];
  if (hrs > 0)
    snprintf(msg, sizeof(msg), "%s finished in %dh %02dm", filename.c_str(), hrs, mins);
  else
    snprintf(msg, sizeof(msg), "%s finished in %dm", filename.c_str(), mins);
  ntfySend(msg, "Print Complete", "default");
  ntfyDoneFired = true;
}

void ntfyResetForNewPrint() {
  ntfyLastProgressTime = millis();
  ntfyLastProgress     = 0.0f;
  ntfyStallFired        = false;
  ntfyDoneFired         = false;
  ntfyHavePos           = false;
  ntfyLastX = ntfyLastY = ntfyLastZ = ntfyLastE = 0.0f;
}