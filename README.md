

 * Features:
 *  - Receives sensor_packet_t from ESP-NOW senders
 *  - Web dashboard with live data (auto-refresh)
 *  - Saves data to SD card as CSV
 *  - REST API for JSON data
 *  - Download CSV from browser

 * Wiring (SD Card - SPI):
 *  SD MOSI -> GPIO 23
 *  SD MISO -> GPIO 19
 *  SD CLK  -> GPIO 18
 *  SD CS   -> GPIO 5
 * 
 * Dependencies (install via Library Manager):
 *  - AsyncTCP
 *  - ESPAsyncWebServer
 *  - ArduinoJson