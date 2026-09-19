#pragma once
#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

const char* ntpServerName = "pool.ntp.org";
IPAddress timeServerIP;

time_t utc = 0;
bool timeValid = false;

unsigned int localPort = 2390;
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

WiFiUDP udp;

uint32_t nextSendTime = 0;
uint32_t no_packet_count = 0;

inline time_t toLocal(time_t utcTime) {
  return utcTime + tzOffset;
}

int32_t fetchTZOffsetByIP(int32_t currentOffset) {
  if (WiFi.status() != WL_CONNECTED) return currentOffset;

  HTTPClient http;
  http.begin("http://ip-api.com/json/?fields=offset");
  http.setTimeout(5000);
  int code = http.GET();

  if (code != 200) {
    Serial.println("TZ fetch failed, HTTP " + String(code));
    http.end();
    return currentOffset;
  }

  String payload = http.getString();
  http.end();

  int idx = payload.indexOf("\"offset\":");
  if (idx < 0) {
    Serial.println("TZ parse failed — key not found");
    return currentOffset;
  }

  int start = idx + 9;
  int end = payload.indexOf('}', start);
  int32_t offset = payload.substring(start, end).toInt();

  Serial.printf("TZ offset fetched via IP: %d seconds (%.1f hrs)\n", offset, (float)offset / 3600.0f);
  return offset;
}

void sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void decodeNTP() {
  timeValid = false;
  uint32_t waitTime = millis() + 500;

  while (millis() < waitTime && !timeValid) {
    yield();
    if (udp.parsePacket()) {
      udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      utc = secsSince1900 - 2208988800UL;
      setTime(utc);
      timeValid = true;
    }
  }

  if (timeValid) no_packet_count = 0;
  else {
    Serial.println("No NTP reply");
    no_packet_count++;
  }
}

void syncTime() {
  if (nextSendTime < millis()) {
    WiFi.hostByName(ntpServerName, timeServerIP);
    nextSendTime = millis() + 5000;
    sendNTPpacket(timeServerIP);
    decodeNTP();
  }
}

// Called once a day at 2am local — re-fetches the offset to catch any DST change.
bool checkTimezoneOffsets() {
  int32_t newOffset = fetchTZOffsetByIP(tzOffset);
  if (newOffset != tzOffset) {
    tzOffset = newOffset;
    Serial.printf("TZ offset updated: %d\n", tzOffset);
    return true;
  }
  Serial.println("TZ offset unchanged.");
  return false;
}