#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>
#define EEPROM_SIZE 64

// EEPROM関連
float savedLat = 0.0;
float savedLng = 0.0;

void saveCoordinates(float lat, float lng) {
  EEPROM.put(0, lat);
  EEPROM.put(sizeof(float), lng);
  EEPROM.commit();
}

void loadCoordinates() {
  EEPROM.get(0, savedLat);
  EEPROM.get(sizeof(float), savedLng);
}


// BLE座標受信部分
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String receivedStr = pCharacteristic->getValue();

    if (receivedStr.length() > 0) {
      int commaIndex = receivedStr.indexOf(',');
      if (commaIndex > 0) {
        String latStr = receivedStr.substring(0, commaIndex);
        String lngStr = receivedStr.substring(commaIndex + 1);

        float lat = latStr.toFloat();
        float lng = lngStr.toFloat();

        Serial.printf("受信した座標: %f, %f\n", lat, lng);

        savedLat = lat;
        savedLng = lng;
        saveCoordinates(lat, lng);  // ← EEPROMに保存！
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);  // EEPROMの初期化
  loadCoordinates(); // 電源ON字に保存済み座標を読み込み

  BLEDevice::init("ESP32"); // EPP32としてBLEを初期化
  BLEServer *pServer = BLEDevice::createServer(); // 

  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE 座標受信サービスを開始しました");
  Serial.println("保存済みの座標を読み込みました:");
  Serial.printf("緯度: %f\n", savedLat);
  Serial.printf("経度: %f\n", savedLng);
}

void loop() {
  // 何もしない
}
