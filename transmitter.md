#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "DHT.h"

#define DHT_PIN 4
#define DHT_TYPE DHT22

#define SEND_INTERVAL_MS 5000
#define WIFI_CHANNEL 1

DHT dht(DHT_PIN, DHT_TYPE);

// Receiver MAC (broadcast by default)
uint8_t receiverAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

typedef struct __attribute__((packed)) {
    uint64_t chip_id;
    uint32_t uptime_ms;
    float temperature;
    float humidity;
} sensor_packet_t;

sensor_packet_t packet;

esp_now_peer_info_t peerInfo;

uint64_t getChipId()
{
    uint64_t chipid = ESP.getEfuseMac();
    return chipid;
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

bool initEspNow()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return false;
    }

    esp_now_register_send_cb(onDataSent);

    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = WIFI_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Peer add failed (continuing anyway)");
    }

    return true;
}

void readSensor()
{
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("DHT read error");
        return;
    }

    packet.temperature = t;
    packet.humidity = h;
}

void sendPacket()
{
    packet.uptime_ms = millis();

    esp_err_t result = esp_now_send(
        receiverAddress,
        (uint8_t *)&packet,
        sizeof(packet)
    );

    if (result != ESP_OK) {
        Serial.println("Send attempt failed (retry next cycle)");
    }
}

void printStatus()
{
    Serial.println("----- SENSOR STATUS -----");

    Serial.print("Chip ID: ");
    Serial.println((uint32_t)(packet.chip_id >> 32), HEX);
    Serial.println((uint32_t)packet.chip_id, HEX);

    Serial.print("Temp: ");
    Serial.println(packet.temperature);

    Serial.print("Humidity: ");
    Serial.println(packet.humidity);

    Serial.print("Uptime(ms): ");
    Serial.println(packet.uptime_ms);

    Serial.println("-------------------------");
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    packet.chip_id = getChipId();

    Serial.println("ESP32 SENSOR NODE START");
    Serial.print("Chip ID: ");
    Serial.println(packet.chip_id);

    dht.begin();

    if (!initEspNow()) {
        Serial.println("ESP-NOW not ready, will still attempt sends");
    }
}

void loop()
{
    readSensor();

    sendPacket();

    printStatus();

    delay(SEND_INTERVAL_MS);
}