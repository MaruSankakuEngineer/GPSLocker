#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "esp_bt_main.h"

// デバッグモード制御
#define DEBUG_MODE  // コメントアウトするとデバッグ出力が無効になります

#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) Serial.printf(format, __VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
#endif

#define EEPROM_SIZE 64
#define SERVICE_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

float savedLat = 0.0;
float savedLng = 0.0;
unsigned long lastDebugTime = 0;  // デバッグ用のタイマー

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);  // UART1（GPIO21=RX, GPIO22=TX）を使用

void saveCoordinates(float lat, float lng) {
  EEPROM.put(0, lat);
  EEPROM.put(sizeof(float), lng);
  EEPROM.commit();
  DEBUG_PRINTF("座標を保存しました: 緯度=%f 経度=%f\n", lat, lng);
}

void loadCoordinates() {
  EEPROM.get(0, savedLat);
  EEPROM.get(sizeof(float), savedLng);
  if (isnan(savedLat) || isnan(savedLng) || savedLat < -90 || savedLat > 90 || savedLng < -180 || savedLng > 180) {
    DEBUG_PRINTLN("⚠️ EEPROMデータが無効です。初期化します。");
    savedLat = 0.0;
    savedLng = 0.0;
    saveCoordinates(savedLat, savedLng);
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    String receivedStr = String(value.c_str());
    
    DEBUG_PRINT("受信したデータ: ");
    DEBUG_PRINTLN(receivedStr);

    int commaIndex = receivedStr.indexOf(',');
    if (commaIndex > 0) {
      String latStr = receivedStr.substring(0, commaIndex);
      String lngStr = receivedStr.substring(commaIndex + 1);
      
      float lat = latStr.toFloat();
      float lng = lngStr.toFloat();
      
      DEBUG_PRINTF("パース結果: 緯度=%f 経度=%f\n", lat, lng);
      
      if (lat != 0.0 || lng != 0.0) {  // 簡易的な妥当性チェック
        savedLat = lat;
        savedLng = lng;
        saveCoordinates(lat, lng);
      } else {
        DEBUG_PRINTLN("⚠️ 無効な座標データです");
      }
    } else {
      DEBUG_PRINTLN("⚠️ 不正なデータ形式です");
    }
  }
};

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

void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("== SETUP START ==");

  // Classic BTメモリ解放（BLE専用のため）
  if (esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT) == ESP_OK) {
    DEBUG_PRINTLN("✅ Classic BTメモリ解放成功");
  }

  EEPROM.begin(EEPROM_SIZE);
  loadCoordinates();

  DEBUG_PRINTF("保存座標: 緯度: %f 経度: %f\n", savedLat, savedLng);

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
  DEBUG_PRINTLN("✅ BLE 座標受信サービス開始");

  // GPSシリアル初期化
  gpsSerial.begin(9600, SERIAL_8N1, 21, 22);  // RX=GPIO21, TX=GPIO22
  DEBUG_PRINTLN("✅ GPS Serial開始完了");
  DEBUG_PRINTLN("GPS接続ピン: RX=GPIO21, TX=GPIO22");
}

void loop() {
  // GPSデータの読み取りとデコード
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);
    #ifdef DEBUG_MODE
      Serial.print(c);  // 生のGPSデータを出力（デバッグモードの場合のみ）
    #endif
  }

  // 定期的なステータス出力（5秒ごと）
  if (millis() - lastDebugTime > 5000) {
    lastDebugTime = millis();
    #ifdef DEBUG_MODE
      DEBUG_PRINTF("GPS Stats: Chars=%lu, Sentences=%lu, Failed=%lu\n",
                   gps.charsProcessed(), gps.sentencesWithFix(), gps.failedChecksum());
      
      // 衛星情報の出力
      if (gps.satellites.isValid()) {
        DEBUG_PRINTF("衛星数: %d\n", gps.satellites.value());
      } else {
        DEBUG_PRINTLN("衛星数: 不明");
      }
    #endif
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    float currentLat = gps.location.lat();
    float currentLng = gps.location.lng();

    DEBUG_PRINTF("現在位置: 緯度=%f 経度=%f\n", currentLat, currentLng);
    double dist = distanceMeters(currentLat, currentLng, savedLat, savedLng);
    DEBUG_PRINTF("距離: %.2f m\n", dist);

    if (dist < 100.0) {
      DEBUG_PRINTLN("✅ 近接一致: ロック解除可");
    } else {
      DEBUG_PRINTLN("❌ 距離不一致: ロック解除不可");
    }
  }
} 