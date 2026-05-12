#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include "index.h"

// ===== WIFI CONFIG =====
const char* ssid = "Default";
const char* password = "default234";

// ===== CONFIG =====
#define SD_CS 5
const char* LOG_FILE = "/log3.txt";

// ===== OBJECTS =====
RTC_DS3231 rtc;
bool rtc_ok = false;

WebServer server(80);
HardwareSerial NodeSerial(2); // UART2

// ===== WIFI HANDLER =====
unsigned long lastWifiCheck = 0;
bool wifiConnected = false;

String getTimestamp() {
  if (!rtc_ok) return "[NO RTC]";

  DateTime now = rtc.now();
  char ts[25];
  sprintf(ts, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(ts);
}

void connectWiFi() {
  WiFi.setHostname("esp32-logger");
  WiFi.begin(ssid, password);
}

void handleWiFi() {
  if (millis() - lastWifiCheck < 5000) return;
  lastWifiCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi reconnecting...");
    WiFi.disconnect();
    connectWiFi();
    wifiConnected = false;

  } else {
    if (!wifiConnected) {
      wifiConnected = true;

      Serial.print("\nWiFi OK  ");
      Serial.println(WiFi.localIP());
    }
  }

  NodeSerial.print("IP:");
  NodeSerial.println(WiFi.localIP());

  Serial.print("[");
  Serial.print(getTimestamp());
  Serial.print("] IP: ");
  Serial.println(WiFi.localIP());
}

// ===== WEB HANDLERS =====
void handleRoot() {
  File file = SD.open(LOG_FILE);
  if (!file) {
    server.send(500, "text/plain", "Failed to open log file");
    return;
  }

  const int MAX_TAIL_BYTES = 1024;
  String logData = "\"";

  long fileSize = file.size();
  long startPos = max(0L, fileSize - MAX_TAIL_BYTES);
  file.seek(startPos);

  if (startPos > 0) {
    while (file.available()) {
      char c = file.read();
      if (c == '\n') break;
    }
  }

  while (file.available()) {
    char c = file.read();
    if (c == '"')       logData += "\\\"";
    else if (c == '\\') logData += "\\\\";
    else if (c == '\r') continue;
    else if (c == '\n') logData += "\\n";
    else                logData += c;
  }
  logData += "\"";
  file.close();

  String html = INDEX_TOP_HTML;

  html += logData;

  html += INDEX_BOT_HTML;

  server.send(200, "text/html", html);
}

void handleDownload() {
  File file = SD.open(LOG_FILE);
  server.streamFile(file, "application/octet-stream");
  file.close();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);

  if (rtc.begin()) {
    rtc_ok = true;
    Serial.println("RTC OK");
  } else {
    Serial.println("RTC FAIL");
  }

  // rtc.adjust(DateTime(2026, 5, 1, 2, 55, 0));

  NodeSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("UART2 READY");

  SPI.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD FAIL");
    while (1);
  }
  Serial.println("SD OK");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  connectWiFi();
  Serial.println("WiFi starting...");

  // ===== WEB SERVER =====
  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  handleWiFi();

  static String incoming = "";

  // ===== READ UART DATA =====
  while (NodeSerial.available()) {
    char c = NodeSerial.read();

    if (c == '\n') {
      incoming.trim();

      if (incoming.length() > 0 && rtc_ok) {
        DateTime now = rtc.now();

        char timestamp[30];
        sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());

        String finalLine = String(timestamp) + "," + incoming;

        Serial.println(finalLine);

        // ===== SAVE TO SD =====
        File file = SD.open(LOG_FILE, FILE_APPEND);
        if (file) {
          file.println(finalLine);
          file.close();
          Serial.println("Saved");
        } else {
          Serial.println("Write failed");
        }
      }

      incoming = "";
    } else {
      incoming += c;
    }
  }
}
