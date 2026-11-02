#pragma once
#include <HTTPClient.h>

// ── ntfy Notification Support ─────────────────────────────
// Set up on the web page, variables are in Settings.h

static uint32_t ntfyLastProgressTime = 0;
static float    ntfyLastProgress     = 0.0f;
static bool     ntfyStallFired       = false;
static bool     ntfyDoneFired        = false;

static bool  ntfyHavePos = false;// last toolhead position
static float ntfyLastX = 0.0f, ntfyLastY = 0.0f, ntfyLastZ = 0.0f, ntfyLastE = 0.0f;

// ntfySend() queues, netTask posts with ntfyPost()
struct NtfyMsg {
  char url[128];
  char token[48];
  char title[32];
  char priority[10];
  char message[128];
};
QueueHandle_t ntfyQueue = nullptr;

void ntfySend(const char* message, const char* title, const char* priority) {
  if (!ntfyEnabled || !ntfyQueue) return;
  NtfyMsg m;
  snprintf(m.url, sizeof(m.url), "%s/%s", ntfyServer.c_str(), ntfyTopic.c_str());
  strlcpy(m.token,    ntfyToken.c_str(), sizeof(m.token));
  strlcpy(m.title,    title,             sizeof(m.title));
  strlcpy(m.priority, priority,          sizeof(m.priority));
  strlcpy(m.message,  message,           sizeof(m.message));
  xQueueSend(ntfyQueue, &m, 0);
}

void ntfyPost(const NtfyMsg& m) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(m.url);
  http.addHeader("Title",        m.title);
  http.addHeader("Priority",     m.priority);
  http.addHeader("Content-Type", "text/plain");
  if (m.token[0] != '\0') {
    http.addHeader("Authorization", "Bearer " + String(m.token));
  }
  http.POST(m.message);
  http.end();
}

void ntfyCheckStall(float currentProgress, float x, float y, float z, float e) {// stall = no progress or motion for ntfyStallMin
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

  String filename = rawPath;// strip path and extension
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