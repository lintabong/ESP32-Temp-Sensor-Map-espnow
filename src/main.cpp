
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>

#include "dashboard_html.h"

// void handleRoot(AsyncWebServerRequest* req) {
//     req->send_P(200, "text/html", DASHBOARD_HTML);
// }

#define WIFI_SSID        ""
#define WIFI_PASSWORD    ""
#define WIFI_CHANNEL     1

// SD card SPI chip-select pin
#define SD_CS_PIN        5

// Log file on SD card
#define LOG_FILE         "/sensor_log.csv"

// Maximum in-memory records shown on dashboard
#define MAX_RECORDS      200

// NTP
#define NTP_SERVER       "pool.ntp.org"
#define GMT_OFFSET_SEC   0         // change for your timezone, e.g. 25200 for UTC+7
#define DAYLIGHT_OFFSET  0

typedef struct __attribute__((packed)) {
    uint64_t chip_id;
    uint32_t uptime_ms;
    float    temperature;
    float    humidity;
} sensor_packet_t;

struct SensorRecord {
    char     timestamp[32];
    uint64_t chip_id;
    uint32_t uptime_ms;
    float    temperature;
    float    humidity;
    uint8_t  sender_mac[6];
};

SensorRecord records[MAX_RECORDS];
int          recordCount   = 0;
int          totalReceived = 0;
bool         sdAvailable   = false;
bool         ntpSynced     = false;

SemaphoreHandle_t dataMutex;
AsyncWebServer    server(80);

String macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void getTimestamp(char* buf, size_t len) {
    if (ntpSynced) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(buf, len, "uptime_%lums", millis());
    }
}

// ─────────────────────────────────────────────
//  SD CARD
// ─────────────────────────────────────────────
bool initSD() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[SD] Mount failed or no card");
        return false;
    }
    Serial.println("[SD] Mounted OK");

    // Write CSV header if file does not exist
    if (!SD.exists(LOG_FILE)) {
        File f = SD.open(LOG_FILE, FILE_WRITE);
        if (f) {
            f.println("timestamp,chip_id,uptime_ms,temperature_c,humidity_pct,sender_mac");
            f.close();
            Serial.println("[SD] Created log file with header");
        } else {
            Serial.println("[SD] Cannot create log file");
            return false;
        }
    }
    return true;
}

void appendToSD(const SensorRecord& r) {
    if (!sdAvailable) return;
    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (!f) {
        Serial.println("[SD] Cannot open log file for append");
        return;
    }
    char line[128];
    snprintf(line, sizeof(line), "%s,%llu,%lu,%.2f,%.2f,%s",
             r.timestamp,
             r.chip_id,
             r.uptime_ms,
             r.temperature,
             r.humidity,
             macToString(r.sender_mac).c_str());
    f.println(line);
    f.close();
}

// ─────────────────────────────────────────────
//  ESP-NOW CALLBACK
// ─────────────────────────────────────────────
void onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
    if (len != sizeof(sensor_packet_t)) {
        Serial.printf("[ESP-NOW] Unexpected packet size %d (expected %d)\n",
                      len, sizeof(sensor_packet_t));
        return;
    }

    sensor_packet_t pkt;
    memcpy(&pkt, data, sizeof(pkt));

    SensorRecord r;
    getTimestamp(r.timestamp, sizeof(r.timestamp));
    r.chip_id     = pkt.chip_id;
    r.uptime_ms   = pkt.uptime_ms;
    r.temperature = pkt.temperature;
    r.humidity    = pkt.humidity;
    memcpy(r.sender_mac, mac, 6);

    // Append to SD before taking mutex (SD ops can be slow)
    appendToSD(r);

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        totalReceived++;
        // Ring-buffer: shift when full
        if (recordCount < MAX_RECORDS) {
            records[recordCount++] = r;
        } else {
            memmove(&records[0], &records[1], sizeof(SensorRecord) * (MAX_RECORDS - 1));
            records[MAX_RECORDS - 1] = r;
        }
        xSemaphoreGive(dataMutex);
    }

    Serial.printf("[ESP-NOW] RX from %s | T=%.2f°C H=%.2f%% ID=%llu\n",
                  macToString(mac).c_str(),
                  pkt.temperature, pkt.humidity, pkt.chip_id);
}

// ─────────────────────────────────────────────
//  WEB HANDLERS
// ─────────────────────────────────────────────

// GET /api/data  → JSON array of records
void handleApiData(AsyncWebServerRequest* req) {
    String json = "[";

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        int start = max(0, recordCount - 50); // last 50 for API
        for (int i = start; i < recordCount; i++) {
            if (i > start) json += ",";
            const SensorRecord& r = records[i];
            char entry[256];
            snprintf(entry, sizeof(entry),
                     "{\"ts\":\"%s\",\"chip_id\":\"%llu\","
                     "\"uptime\":%lu,\"temp\":%.2f,\"hum\":%.2f,"
                     "\"mac\":\"%s\"}",
                     r.timestamp, r.chip_id, r.uptime_ms,
                     r.temperature, r.humidity,
                     macToString(r.sender_mac).c_str());
            json += entry;
        }
        xSemaphoreGive(dataMutex);
    }

    json += "]";
    req->send(200, "application/json", json);
}

// GET /api/status  → system status JSON
void handleApiStatus(AsyncWebServerRequest* req) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"total_received\":%d,\"in_memory\":%d,"
             "\"sd_available\":%s,\"ntp_synced\":%s,"
             "\"free_heap\":%lu,\"uptime_ms\":%lu}",
             totalReceived, recordCount,
             sdAvailable ? "true" : "false",
             ntpSynced   ? "true" : "false",
             (unsigned long)ESP.getFreeHeap(),
             (unsigned long)millis());
    req->send(200, "application/json", buf);
}

// GET /download  → stream CSV from SD card
void handleDownload(AsyncWebServerRequest* req) {
    if (!sdAvailable) {
        req->send(503, "text/plain", "SD card not available");
        return;
    }
    if (!SD.exists(LOG_FILE)) {
        req->send(404, "text/plain", "Log file not found");
        return;
    }
    req->send(SD, LOG_FILE, "text/csv",
              true /* download */);
}

// GET /clear  → clear SD log (admin action)
void handleClear(AsyncWebServerRequest* req) {
    if (sdAvailable && SD.exists(LOG_FILE)) {
        SD.remove(LOG_FILE);
        // Re-create with header
        File f = SD.open(LOG_FILE, FILE_WRITE);
        if (f) {
            f.println("timestamp,chip_id,uptime_ms,temperature_c,humidity_pct,sender_mac");
            f.close();
        }
    }
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        recordCount   = 0;
        totalReceived = 0;
        xSemaphoreGive(dataMutex);
    }
    req->send(200, "text/plain", "Cleared");
}

// GET /  → main dashboard HTML
void handleRoot(AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", DASHBOARD_HTML);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n====== ESP32 ESP-NOW RECEIVER + WEB SERVER ======");

    // Mutex for thread-safe ring buffer
    dataMutex = xSemaphoreCreateMutex();

    // ── SD Card ──────────────────────────────
    sdAvailable = initSD();

    // ── WiFi: AP+STA for ESP-NOW + web server ─
    // ESP-NOW requires STA mode on the same channel as the sender.
    // We start in AP+STA mode: AP for the web dashboard while STA is
    // used internally by ESP-NOW.
    WiFi.mode(WIFI_AP_STA);

    // Connect STA to your router for web access & NTP
    Serial.printf("[WiFi] Connecting to %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

        // NTP sync
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
        Serial.print("[NTP] Syncing");
        for (int i = 0; i < 10; i++) {
            time_t now = time(nullptr);
            if (now > 1000000000) {
                ntpSynced = true;
                Serial.println(" OK");
                break;
            }
            delay(500);
            Serial.print(".");
        }
        if (!ntpSynced) Serial.println(" timeout, using local uptime");
    } else {
        Serial.println("\n[WiFi] Could not connect to router — running AP only");
        WiFi.softAP("ESP32-Gateway", "12345678");
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    // ── ESP-NOW init ─────────────────────────
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed!");
    } else {
        esp_now_register_recv_cb(onDataReceived);
        Serial.println("[ESP-NOW] Ready — listening for packets");
    }

    // ── Web server routes ─────────────────────
    server.on("/",        HTTP_GET, handleRoot);
    server.on("/api/data",   HTTP_GET, handleApiData);
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/download",   HTTP_GET, handleDownload);
    server.on("/clear",      HTTP_GET, handleClear);

    // CORS for local dev tools
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.begin();
    Serial.println("[WEB] Server started on port 80");

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WEB] Dashboard → http://%s/\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("[WEB] Dashboard → http://%s/\n", WiFi.softAPIP().toString().c_str());
    }
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────
void loop() {
    // ESPAsyncWebServer runs on its own FreeRTOS task.
    // Main loop is free for watchdog / optional status prints.
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
        lastPrint = millis();
        Serial.printf("[STATUS] Packets: %d | Heap: %lu bytes | SD: %s\n",
                      totalReceived,
                      (unsigned long)ESP.getFreeHeap(),
                      sdAvailable ? "OK" : "N/A");
    }
    delay(100);
}
