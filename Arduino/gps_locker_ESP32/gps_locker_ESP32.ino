#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
  String receivedStr = pCharacteristic->getValue();

  if (receivedStr.length() > 0) {
    Serial.println("受信した座標文字列: " + receivedStr);

    int commaIndex = receivedStr.indexOf(',');
    if (commaIndex > 0) {
      String latStr = receivedStr.substring(0, commaIndex);
      String lngStr = receivedStr.substring(commaIndex + 1);

      float latitude = latStr.toFloat();
      float longitude = lngStr.toFloat();

      Serial.printf("緯度: %f\n", latitude);
      Serial.printf("経度: %f\n", longitude);
    }
  }
}


};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("ESP32");
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

  Serial.println("BLE 座標受信サービスを開始しました");
}

void loop() {
  // 何もしない
}
