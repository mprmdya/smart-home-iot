/**
 * Smart Home Automation System - ESP32 Firmware
 * 
 * Target Board: ESP32 Dev Module
 * Features:
 *  - WiFi Connection & MQTT Client (PubSubClient)
 *  - Temperature & Humidity Monitoring (DHT22)
 *  - Dual Relay Control (Lampu Utama & Kipas/AC)
 *  - Automatic & Manual Modes
 * 
 * Author: mprmdya
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ========== KONFIGURASI WIFI & MQTT ==========
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASS     = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER   = "broker.emqx.io"; // Atau IP Server Node.js lokal
const int   MQTT_PORT     = 1883;
const char* CLIENT_ID     = "ESP32_SmartHome_Gateway";

// ========== PINOUT DEFINITIONS ==========
#define PIN_DHT           4    // GPIO 4 -> DHT22 Data Pin
#define DHTTYPE           DHT22
#define PIN_RELAY_LIGHT   18   // GPIO 18 -> Relay 1 (Lampu)
#define PIN_RELAY_FAN     19   // GPIO 19 -> Relay 2 (Kipas/AC)
#define PIN_LED_STATUS    2    // GPIO 2 -> Built-in LED Status

// ========== MQTT TOPICS ==========
const char* TOPIC_CONTROL_LIGHT = "smarthome/control/light";
const char* TOPIC_CONTROL_FAN   = "smarthome/control/fan";
const char* TOPIC_CONTROL_AUTO  = "smarthome/control/auto";
const char* TOPIC_STATE_SENSOR  = "smarthome/state/telemetry";
const char* TOPIC_STATE_STATUS  = "smarthome/state/devices";

// ========== OBJEK & VARIABEL GLOBAL ==========
WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT dht(PIN_DHT, DHTTYPE);

bool lightState = false;
bool fanState   = false;
bool autoFanMode = true;
float autoTempThreshold = 29.0; // Suhu (°C) untuk menyalakan kipas otomatis

unsigned long lastSensorPublish = 0;
const unsigned long SENSOR_INTERVAL = 5000; // Kirim telemetri setiap 5 detik

// ========== PROTOTIPE FUNGSI ==========
void setupWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishDeviceStatus();
void readAndPublishSensors();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== SMART HOME AUTOMATION ESP32 ===");

  // Inisialisasi Pin GPIO
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Relay Active-LOW (umum pada modul relay Arduino/ESP32)
  digitalWrite(PIN_RELAY_LIGHT, HIGH);
  digitalWrite(PIN_RELAY_FAN, HIGH);
  digitalWrite(PIN_LED_STATUS, LOW);

  // Inisialisasi Sensor DHT
  dht.begin();

  // Koneksi Jaringan
  setupWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorPublish >= SENSOR_INTERVAL) {
    lastSensorPublish = currentMillis;
    readAndPublishSensors();
  }
}

void setupWiFi() {
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
  }

  digitalWrite(PIN_LED_STATUS, HIGH);
  Serial.println("\nWiFi Berhasil Terhubung!");
  Serial.print("IP Address ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Mencoba koneksi MQTT ke broker...");
    if (mqttClient.connect(CLIENT_ID)) {
      Serial.println(" Terhubung!");
      // Subscribe ke topik kontrol
      mqttClient.subscribe(TOPIC_CONTROL_LIGHT);
      mqttClient.subscribe(TOPIC_CONTROL_FAN);
      mqttClient.subscribe(TOPIC_CONTROL_AUTO);
      
      // Kirim status awal
      publishDeviceStatus();
    } else {
      Serial.print(" Gagal, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Coba lagi dalam 4 detik...");
      delay(4000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  message.toUpperCase();

  Serial.printf("[MQTT RX] Topik: %s | Pesan: %s\n", topic, message.c_str());

  if (String(topic) == TOPIC_CONTROL_LIGHT) {
    if (message == "ON" || message == "1") {
      lightState = true;
      digitalWrite(PIN_RELAY_LIGHT, LOW); // Active Low
    } else if (message == "OFF" || message == "0") {
      lightState = false;
      digitalWrite(PIN_RELAY_LIGHT, HIGH);
    }
    publishDeviceStatus();
  } 
  else if (String(topic) == TOPIC_CONTROL_FAN) {
    autoFanMode = false; // Matikan mode auto jika dikontrol manual
    if (message == "ON" || message == "1") {
      fanState = true;
      digitalWrite(PIN_RELAY_FAN, LOW);
    } else if (message == "OFF" || message == "0") {
      fanState = false;
      digitalWrite(PIN_RELAY_FAN, HIGH);
    }
    publishDeviceStatus();
  }
  else if (String(topic) == TOPIC_CONTROL_AUTO) {
    if (message == "ON" || message == "1" || message == "TRUE") {
      autoFanMode = true;
    } else {
      autoFanMode = false;
    }
    publishDeviceStatus();
  }
}

void readAndPublishSensors() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("[Sensor Error] Gagal membaca data dari DHT22!");
    return;
  }

  // Logika Otomatis Kipas
  if (autoFanMode) {
    if (temp >= autoTempThreshold && !fanState) {
      fanState = true;
      digitalWrite(PIN_RELAY_FAN, LOW);
      publishDeviceStatus();
    } else if (temp < (autoTempThreshold - 1.0) && fanState) {
      fanState = false;
      digitalWrite(PIN_RELAY_FAN, HIGH);
      publishDeviceStatus();
    }
  }

  // Buat Payload JSON
  StaticJsonDocument<200> doc;
  doc["temperature"] = round(temp * 10) / 10.0;
  doc["humidity"]    = round(hum * 10) / 10.0;
  doc["autoFan"]     = autoFanMode;
  doc["uptimeSec"]   = millis() / 1000;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  mqttClient.publish(TOPIC_STATE_SENSOR, jsonBuffer);
  Serial.printf("[Telemetri] %s\n", jsonBuffer);
}

void publishDeviceStatus() {
  StaticJsonDocument<200> doc;
  doc["light"] = lightState ? "ON" : "OFF";
  doc["fan"]   = fanState ? "ON" : "OFF";
  doc["auto"]  = autoFanMode ? "ON" : "OFF";

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  mqttClient.publish(TOPIC_STATE_STATUS, jsonBuffer, true);
}
