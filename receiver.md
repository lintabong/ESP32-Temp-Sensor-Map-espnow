#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

#define MAX_NODES 5
#define POT_TOP_PIN 34
#define POT_BOT_PIN 35
#define BUZZER_PIN 18
#define HAZARD_LED_PIN 25

#define NODE_TIMEOUT 20000     // ms
#define SEND_INTERVAL 60000    // UART send interval (ms)

HardwareSerial NodeSerial(2);
LiquidCrystal_I2C lcd(0x27, 20, 4);

typedef struct {
  uint8_t node_id;
  float temp;
  float hum;
  uint32_t uptime;
  int8_t rssi;
  bool valid;
} SensorData;

SensorData nodes[MAX_NODES];
bool nodeActive[MAX_NODES] = {false};
unsigned long lastSeen[MAX_NODES] = {0};

float meanTemperature = 0;
int potTopTemperature = 0;
int potBotTemperature = 0;

String uartIncoming = "";
String receivedIP = "";

String scrollText = "";
int scrollIndex = 0;
unsigned long lastScroll = 0;

void buildCSV(char *out) {
  float t[MAX_NODES] = {0};
  float h[MAX_NODES] = {0};

  for (int i = 0; i < MAX_NODES; i++) {
    if (nodeActive[i] && nodes[i].valid && (millis() - lastSeen[i] < NODE_TIMEOUT)) {
      t[i] = nodes[i].temp;
      h[i] = nodes[i].hum;
    }
  }

  sprintf(out,
          "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f",
          t[0], h[0],
          t[1], h[1],
          t[2], h[2],
          t[3], h[3],
          t[4], h[4]
  );
} 

void printCSV() {
  char line[120];
  buildCSV(line);
  Serial.println(line);
}

// ===== RECEIVE CALLBACK =====
void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  SensorData incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));

  int idx = incoming.node_id - 1;

  if (idx >= 0 && idx < MAX_NODES) {
    nodes[idx] = incoming;
    nodeActive[idx] = true;
    lastSeen[idx] = millis();

    Serial.print("RX Node ");
    Serial.print(incoming.node_id);
    Serial.print(" | Temp: ");
    Serial.print(incoming.temp);
    Serial.print(" | Hum: ");
    Serial.println(incoming.hum);
  } else {
    Serial.println("Invalid node_id");
    return;
  }

  // ===== MEAN TEMP =====
  float sum = 0;
  int count = 0;

  for (int i = 0; i < MAX_NODES; i++) {
    if (nodeActive[i] && nodes[i].valid && (millis() - lastSeen[i] < NODE_TIMEOUT)) {
      sum += nodes[i].temp;
      count++;
    }
  }

  meanTemperature = (count > 0) ? (sum / count) : 0;

  // ===== SCROLL TEXT =====
  scrollText = "";
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodeActive[i] && nodes[i].valid) {
      scrollText += "N";
      scrollText += String(i + 1);
      scrollText += ":";
      scrollText += String(nodes[i].temp, 1);
      scrollText += (char)223;
      scrollText += "C ";
      scrollText += String(nodes[i].hum, 1);
      scrollText += "%   ";
    }
  }

  printCSV();
}

float readStableTemp(int pin) {
  int samples = 10;
  float sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
  }

  float avg = sum / samples;
  return (50.0 - (avg / 4095.0) * 50.0);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  NodeSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("UART2 ready");

  lcd.begin();
  lcd.backlight();
  lcd.print("Starting...");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HAZARD_LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(HAZARD_LED_PIN, LOW);

  WiFi.mode(WIFI_STA);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    lcd.clear();
    lcd.print("ESP-NOW FAIL");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  lcd.clear();
}

void loop() {
  // potTopTemperature = floor(50.0 - (analogRead(POT_TOP_PIN) / 4095.0) * 50.0);
  // potBotTemperature = floor(50.0 - (analogRead(POT_BOT_PIN) / 4095.0) * 50.0);

  while (NodeSerial.available()) {
    char c = NodeSerial.read();

    if (c == '\n') {
      uartIncoming.trim();

      if (uartIncoming.startsWith("IP:")) {
        receivedIP = uartIncoming.substring(3);

        Serial.print("[UART RX] IP received: ");
        Serial.println(receivedIP);
      }

      uartIncoming = "";
    } else {
      uartIncoming += c;
    }
  }

  potTopTemperature = floor(readStableTemp(POT_TOP_PIN));
  potBotTemperature = floor(readStableTemp(POT_BOT_PIN));

  // ===== count active nodes =====
  int activeCount = 0;
  for (int i = 0; i < MAX_NODES; i++) {
    if (nodeActive[i] && (millis() - lastSeen[i] < NODE_TIMEOUT)) {
      activeCount++;
    }
  }

  // ===== BUZZER =====
  static unsigned long lastBeep = 0;
  static bool buzzerState = false;

  if ((potBotTemperature > meanTemperature) || (meanTemperature > potTopTemperature)) {
    if (millis() - lastBeep > 200) {
      lastBeep = millis();
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
      digitalWrite(HAZARD_LED_PIN, HIGH);
    } 
  } else { 
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(HAZARD_LED_PIN, LOW);
  }

  lcd.setCursor(0, 0);
  // lcd.print(activeCount);
  // lcd.print(" ");

  lcd.print(receivedIP);
  lcd.print("  n:");
  lcd.print(activeCount);

  lcd.setCursor(0, 1);
  lcd.print("top: ");
  lcd.print(potTopTemperature);
  lcd.print((char)223);
  lcd.print("C bot: ");
  lcd.print(potBotTemperature);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 2);
  lcd.print("Mean: ");
  lcd.print(meanTemperature, 1);
  lcd.print((char)223);
  lcd.print("C   ");

  if (millis() - lastScroll > 300) {
    lastScroll = millis();

    if (scrollText.length() > 0) {
      String padded = "                    " + scrollText + "                    ";
      String view = padded.substring(scrollIndex, scrollIndex + 20);

      lcd.setCursor(0, 3);
      lcd.print(view);

      scrollIndex++;
      if (scrollIndex > padded.length() - 20) {
        scrollIndex = 0;
      }
    }
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    printCSV();
  }

  static unsigned long lastSend = 0;
  if (millis() - lastSend > SEND_INTERVAL) {
    lastSend = millis();\

    char line[120];
    buildCSV(line);

    NodeSerial.println(line);

    Serial.print("[UART SEND] ");
    Serial.println(line);
  }
}
