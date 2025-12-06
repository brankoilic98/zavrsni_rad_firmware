#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <MPU9250_asukiaaa.h>

#define SDA_PIN 2
#define SCL_PIN 1
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
TinyGPSPlus gps;
MPU9250_asukiaaa mpu;

// BLE karakteristike
BLECharacteristic *tempChar;
BLECharacteristic *gpsChar;
BLECharacteristic *imuChar;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // === BME280 inicijalizacija ===
  if (!bme.begin(0x76)) {
    Serial.println("❌ BME280 not found!");
  } else {
    Serial.println("✅ BME280 ready.");
  }

  // === MPU9250 inicijalizacija ===
  mpu.setWire(&Wire);
  mpu.beginAccel();
  mpu.beginGyro();
  Serial.println("✅ MPU9250 Acc+Gyro ready.");

  // === GPS inicijalizacija ===
  Serial1.begin(9600, SERIAL_8N1, 8, 9);  // RX=9, TX=8
  Serial.println("✅ GPS ready.");

  // === BLE inicijalizacija ===
  BLEDevice::init("Zavrsni rad");
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService("12345678-1234-5678-1234-56789abcdef0");

  tempChar = service->createCharacteristic(
    "abcd0001-1234-5678-1234-56789abcdef0",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  tempChar->addDescriptor(new BLE2902());

  gpsChar = service->createCharacteristic(
    "abcd0002-1234-5678-1234-56789abcdef0",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  gpsChar->addDescriptor(new BLE2902());

  // === Nova BLE karakteristika: IMU ===
  imuChar = service->createCharacteristic(
    "abcd0003-1234-5678-1234-56789abcdef0",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  imuChar->addDescriptor(new BLE2902());

  service->start();
  server->getAdvertising()->start();
  Serial.println("📡 BLE ready!");
}

void loop() {
  // === GPS dekodiranje ===
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // === BME280 ===
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F;

  String bmeJson = "{\"t\":" + String(t, 2) + ",\"h\":" + String(h, 2) + ",\"p\":" + String(p, 2) + "}";

  tempChar->setValue(bmeJson.c_str());
  tempChar->notify();

  Serial.println("🌡️ BLE BME: " + bmeJson);

  // === MPU9250 (Accel + Gyro) ===
  mpu.accelUpdate();
  mpu.gyroUpdate();

  String imuJson = "{\"ax\":" + String(mpu.accelX(), 3) + ",\"ay\":" + String(mpu.accelY(), 3) + ",\"az\":" + String(mpu.accelZ(), 3) + ",\"gx\":" + String(mpu.gyroX(), 3) + ",\"gy\":" + String(mpu.gyroY(), 3) + ",\"gz\":" + String(mpu.gyroZ(), 3) + "}";

  imuChar->setValue(imuJson.c_str());
  imuChar->notify();

  Serial.println("🌀 BLE IMU: " + imuJson);

  // === GPS samo kad ima update ===
  // === BLE slanje GPS podataka ===
  if (gps.location.isUpdated()) {

    double lat = gps.location.lat();
    double lng = gps.location.lng();
    double alt = gps.altitude.meters();
    double spd = gps.speed.mps();      // mp/S
    int sats = gps.satellites.value();  // broj satelita
    double hdop = gps.hdop.hdop();      // horizontal dilution

    String gpsJson = "{"
                     "\"lat\":"
                     + String(lat, 6) + ","
                                        "\"lng\":"
                     + String(lng, 6) + ","
                                        "\"alt\":"
                     + String(alt, 2) + ","
                                        "\"spd\":"
                     + String(spd, 2) + ","
                                        "\"sat\":"
                     + String(sats) + "}";

    gpsChar->setValue(gpsJson.c_str());
    gpsChar->notify();

    Serial.println("📡 Sent BLE GPS data: " + gpsJson);
  }

  delay(1000);
}
