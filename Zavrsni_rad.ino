#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SDA_PIN 2
#define SCL_PIN 1
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
TinyGPSPlus gps;

// BLE karakteristike
BLECharacteristic *tempChar;
BLECharacteristic *gpsChar;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // === BME280 inicijalizacija ===
  if (!bme.begin(0x76)) {
    Serial.println("❌ BME280 not found!");
  } else {
    Serial.println("✅ BME280 ready.");
  }

  // === GPS inicijalizacija ===
  Serial1.begin(9600, SERIAL_8N1, 8, 9); // RX=9, TX=8
  Serial.println("✅ GPS ready.");

  // === BLE inicijalizacija ===
  BLEDevice::init("Zavrsni rad");
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService("12345678-1234-5678-1234-56789abcdef0");

  // Temperatura / pritisak / vlaga
  tempChar = service->createCharacteristic(
      "abcd0001-1234-5678-1234-56789abcdef0",
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  tempChar->addDescriptor(new BLE2902());

  // GPS karakteristika
  gpsChar = service->createCharacteristic(
      "abcd0002-1234-5678-1234-56789abcdef0",
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  gpsChar->addDescriptor(new BLE2902());

  service->start();
  server->getAdvertising()->start();

  Serial.println("✅ BLE ready. Connect from your phone (nRF Connect etc.)");
}

void loop() {
  // === GPS dekodiranje podataka ===
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // === BME280 čitanje ===
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;

  // === BLE slanje BME podataka ===
  String bmeJson = "{\"t\":" + String(t, 2) +
                   ",\"h\":" + String(h, 2) +
                   ",\"p\":" + String(p, 2) + "}";
  tempChar->setValue(bmeJson.c_str());
  tempChar->notify();
  Serial.println("🌡️ Sent BLE BME data: " + bmeJson);

  // === BLE slanje GPS podataka ===
  if (gps.location.isUpdated()) {
    String gpsJson = "{\"lat\":" + String(gps.location.lat(), 6) +
                     ",\"lng\":" + String(gps.location.lng(), 6) + "}";
    gpsChar->setValue(gpsJson.c_str());
    gpsChar->notify();
    Serial.println("📡 Sent BLE GPS data: " + gpsJson);
  }

  delay(5000);
}
