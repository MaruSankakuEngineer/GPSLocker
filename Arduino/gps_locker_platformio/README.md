# GPS Locker ESP32

GPSとBLEを使用した位置情報ベースのロックシステム

## 概要

このプロジェクトは、ESP32-WROVERを使用して、特定の位置情報に基づいてロック解除を行うシステムです。
BLEを通じて保存位置を設定し、現在のGPS位置が保存位置から100m以内の場合にロック解除信号を出力します。

## ハードウェア要件

- ESP32-WROVER開発ボード
- GPSモジュール（UART接続）
  - TX → GPIO16（RX2）
  - RX → GPIO17（TX2）

## ソフトウェア要件

- PlatformIO
- Arduino Framework for ESP32
- 必要なライブラリ（platformio.iniに記載）
  - TinyGPS++
  - ESP32 BLE Arduino

## セットアップ方法

1. PlatformIOをインストール
2. プロジェクトをクローン
3. `platformio.ini`の設定を確認
4. ビルドとアップロード

## 使用方法

1. ESP32をGPSモジュールと接続
2. プログラムをアップロード
3. BLEクライアントから接続（デバイス名：ESP32-WROVER）
4. 基準位置をBLEで送信（フォーマット：`latitude,longitude`）
5. シリアルモニタで動作確認（115200bps）

## ライセンス

MIT License 