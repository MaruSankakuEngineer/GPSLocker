#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define EEPROM_SIZE 64
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// 保存された座標
float savedLat = 0.0;
float savedLng = 0.0;

// GPS関連
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);  // UART2（GPIO16=RX, GPIO17=TX）

// EEPROMに保存
void saveCoordinates(float lat, float lng) {
  EEPROM.put(0, lat);
  EEPROM.put(sizeof(float), lng);
  EEPROM.commit();
}

// EEPROMから読み込み
void loadCoordinates() {
  EEPROM.get(0, savedLat);
  EEPROM.get(sizeof(float), savedLng);
}

// BLE受信処理クラス
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String receivedStr = String(pCharacteristic->getValue().c_str());

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
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);  // GPS: TX→GPIO16, RX→GPIO17

  EEPROM.begin(EEPROM_SIZE);
  loadCoordinates();

  Serial.println("保存済みの座標を読み込みました:");
  Serial.printf("緯度: %f\n", savedLat);
  Serial.printf("経度: %f\n", savedLng);

  // BLE初期化
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

// Haversine距離計算
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
  // GPSからのデータをエンコード
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  // GPS位置が更新されたら距離判定
  if (gps.location.isUpdated()) {
    float currentLat = gps.location.lat();
    float currentLng = gps.location.lng();

    Serial.println("現在位置を取得:");
    Serial.printf("緯度: %f\n", currentLat);
    Serial.printf("経度: %f\n", currentLng);

    double dist = distanceMeters(currentLat, currentLng, savedLat, savedLng);
    Serial.printf("保存された座標との距離: %.2f m\n", dist);

    if (dist < 100.0) {
      Serial.println("✅ 近接一致（20m以内）: ロック解除可能");
    } else {
      Serial.println("❌ 距離が離れています: ロック解除不可");
    }

    delay(5000);  // 5秒おきに比較
  }
}
