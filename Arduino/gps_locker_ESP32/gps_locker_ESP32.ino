#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "esp_bt_main.h"

#define EEPROM_SIZE 64
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

float savedLat = 0.0;
float savedLng = 0.0;

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);  // UART2（GPIO16=RX2, GPIO17=TX2）

void saveCoordinates(float lat, float lng) {
  EEPROM.put(0, lat);
  EEPROM.put(sizeof(float), lng);
  EEPROM.commit();
}

void loadCoordinates() {
  EEPROM.get(0, savedLat);
  EEPROM.get(sizeof(float), savedLng);
  if (isnan(savedLat) || isnan(savedLng) || savedLat < -90 || savedLat > 90 || savedLng < -180 || savedLng > 180) {
    Serial.println("⚠️ EEPROMデータが無効です。初期化します。");
    savedLat = 0.0;
    savedLng = 0.0;
    saveCoordinates(savedLat, savedLng);
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
     String receivedStr = pCharacteristic->getValue();

    int commaIndex = receivedStr.indexOf(',');
    if (commaIndex > 0) {
      float lat = receivedStr.substring(0, commaIndex).toFloat();
      float lng = receivedStr.substring(commaIndex + 1).toFloat();
      Serial.printf("受信した座標: %f, %f\n", lat, lng);
      savedLat = lat;
      savedLng = lng;
      saveCoordinates(lat, lng);
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("== SETUP START ==");

  // Classic BTメモリ解放（BLE専用のため）
  if (esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT) == ESP_OK) {
    Serial.println("✅ Classic BTメモリ解放成功");
  }

  EEPROM.begin(EEPROM_SIZE);
  loadCoordinates();

  Serial.printf("保存座標: 緯度: %f 経度: %f\n", savedLat, savedLng);

  // BLE初期化
  BLEDevice::init("ESP32-WROVER");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  Serial.println("✅ BLE 座標受信サービス開始");

  // BLE初期化が完了してからGPSシリアル初期化（競合回避）
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("✅ GPS Serial開始完了");
}

double distanceMeters(float lat1, float lon1, float lat2, float lon2) {
  const double R = 6371000.0;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) *
             sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    float currentLat = gps.location.lat();
    float currentLng = gps.location.lng();

    Serial.printf("現在位置: 緯度=%f 経度=%f\n", currentLat, currentLng);
    double dist = distanceMeters(currentLat, currentLng, savedLat, savedLng);
    Serial.printf("距離: %.2f m\n", dist);

    if (dist < 100.0) {
      Serial.println("✅ 近接一致: ロック解除可");
    } else {
      Serial.println("❌ 距離不一致: ロック解除不可");
    }

    delay(5000);
  }
}
