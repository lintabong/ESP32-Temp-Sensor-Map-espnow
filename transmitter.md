#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define NODE_ID 5
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
// F4:2D:C9:8A:64:28
uint8_t receiverMAC[] = {0xF4, 0x2D, 0xC9, 0x8A, 0x64, 0x28};

typedef struct {
  uint8_t node_id;
  float temp;
  float hum;
  uint32_t uptime;
  int8_t rssi;
  bool valid;
} SensorData;

SensorData data;

unsigned long lastRead = 0;
const unsigned long interval = 2000;

// send callback (debug)
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// DHT read
bool readDHT(float &t, float &h) {
  for (int i = 0; i < 3; i++) {
    h = dht.readHumidity();
    t = dht.readTemperature();

    if (!isnan(t) && !isnan(h)) {
      return true;
    }
    delay(100);
  }
  return false;
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA); // required for ESP-NOW

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return;
  }

  esp_now_register_send_cb(onSent);

  // 🔗 register receiver
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
      return;
    }
  }

  dht.begin();

  // print sender MAC
  Serial.print("Sender MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Node ID: ");
  Serial.println(NODE_ID);
}

void loop() {
  if (millis() - lastRead >= interval) {
    lastRead = millis();

    data.node_id = NODE_ID;
    data.uptime = millis();
    data.rssi = 0;

    float t, h;
    data.valid = readDHT(t, h);

    if (data.valid) {
      data.temp = t;
      data.hum = h;
      Serial.printf("Temp: %.2f | Hum: %.2f\n", t, h);
    } else {
      data.temp = 0;
      data.hum = 0;
      Serial.println("DHT FAIL");
    }

    esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

    if (result != ESP_OK) {
      Serial.println("Send error");
    }
  }
}
