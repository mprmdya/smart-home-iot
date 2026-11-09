# Smart Home Automation System (ESP32 + MQTT + Web)

Sistem otomasi rumah pintar berbasis mikrokontroler **ESP32** dengan protokol **MQTT** untuk kontrol relai (lampu, kipas) dan telemetri suhu & kelembapan real-time. Dilengkapi dengan **Web Dashboard** modern dan **Node.js WebSocket/REST Gateway**.

## 📑 Fitur Utama

- **Real-time Telemetri**: Pembacaan suhu dan kelembapan ruangan menggunakan sensor DHT22.
- **Relay Control Dual-Channel**: Kontrol on/off untuk pencahayaan dan kipas angin / AC pendingin.
- **Mode Otomatis Cerdas (Smart Automation)**: Kipas menyala otomatis jika suhu ruangan mencapai atau melebihi threshold (29°C) dan mati ketika dingin.
- **Komunikasi Ringan & Cepat (MQTT)**: Mendukung broker MQTT publik (EMQX/HiveMQ) maupun broker privat/lokal (Mosquitto).
- **Web Dashboard Terintegrasi**: Dashboard responsif dark-mode yang dapat langsung beroperasi di peramban via WebSocket WSS.
- **Node.js Gateway Bridge**: Opsi backend Express + WebSocket untuk integrasi REST API & penyimpanan database riwayat.

## 📐 Skema Rangkaian & Pinout ESP32

| Komponen | Pin ESP32 | Keterangan |
|---|---|---|
| Sensor DHT22 Data | `GPIO 4` | Resistor pull-up 10kΩ ke 3.3V |
| Modul Relay Channel 1 (Lampu) | `GPIO 18` | Active-LOW Relay |
| Modul Relay Channel 2 (Kipas/AC) | `GPIO 19` | Active-LOW Relay |
| Status LED Indicator | `GPIO 2` | On-board Blue LED |
| VCC Sensor & Relay | `VIN (5V) / 3.3V`| Sesuaikan tegangan modul |
| Ground | `GND` | Common Ground |

## 📡 Daftar Topik MQTT

| Arah | Topik | Payload | Deskripsi |
|---|---|---|---|
| ESP32 &rarr; Broker | `smarthome/state/telemetry` | `{"temperature":28.5,"humidity":65.0,"autoFan":true,"uptimeSec":120}` | Laporan sensor berkala (5s) |
| ESP32 &rarr; Broker | `smarthome/state/devices` | `{"light":"OFF","fan":"ON","auto":"ON"}` | Status aktual beban |
| Client &rarr; ESP32 | `smarthome/control/light` | `ON` / `OFF` | Perintah nyala/mati lampu |
| Client &rarr; ESP32 | `smarthome/control/fan` | `ON` / `OFF` | Perintah manual kipas |
| Client &rarr; ESP32 | `smarthome/control/auto` | `ON` / `OFF` | Mengaktifkan/mematikan mode auto kipas |

## 🚀 Panduan Menjalankan

### 1. Upload Firmware ESP32
1. Buka Arduino IDE, pastikan board ESP32 sudah terpasang.
2. Install library: `PubSubClient`, `DHT sensor library`, dan `ArduinoJson`.
3. Buka file [`firmware/esp32_smart_home.ino`](./firmware/esp32_smart_home.ino).
4. Sesuaikan `WIFI_SSID` dan `WIFI_PASS`.
5. Pilih board **ESP32 Dev Module**, tentukan COM Port, lalu klik **Upload**.

### 2. Membuka Web Dashboard
1. Cukup buka [`web-dashboard/index.html`](./web-dashboard/index.html) di browser Anda.
2. Dashboard langsung terhubung via WebSocket ke broker MQTT dan menampilkan telemetri waktu nyata.

### 3. (Opsional) Menjalankan Server Node.js
```bash
cd server
npm install
npm run dev
```

---
Dibuat dengan ❤️ oleh [mprmdya](https://github.com/mprmdya).
