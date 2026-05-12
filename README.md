# ESP32 Multi-Node Temperature Monitoring System


## Deskripsi
Project ini adalah sistem monitoring suhu berbasis beberapa ESP32 yang saling terhubung:

1. Node (Pengirim)
Membaca suhu & kelembaban dari sensor DHT22, lalu kirim via ESP-NOW.
2. Receiver (Penerima + LCD + Alarm)
Menerima data dari banyak node, menghitung rata-rata suhu, tampil di LCD, dan memberi alarm jika tidak normal.
3. Logger (WiFi + SD Card + Web Server)
Menerima data dari receiver via UART, menyimpan ke SD Card, dan menampilkan log via web.

## Arsitektur Sistem
[ESP32 Node] --ESP-NOW--> [ESP32 Receiver] --UART--> [ESP32 Logger]
                                                    |
                                                    +--> SD Card
                                                    +--> Web Server

## Kebutuhan Hardware
1. Node (Pengirim)
- ESP32
- Sensor DHT22
2. Receiver
- ESP32
- LCD I2C 20x4
- Buzzer
- LED
- Potensiometer (opsional)
3. Logger
- ESP32
- SD Card module
- RTC DS3231
- WiFi