import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';

class SendLocationPage extends StatefulWidget {
  final LatLng location;

  const SendLocationPage({super.key, required this.location});

  @override
  State<SendLocationPage> createState() => _SendLocationPageState();
}

class _SendLocationPageState extends State<SendLocationPage> {
  String status = 'BLE接続中...';
  bool isProcessing = true;

  final serviceUuid = Guid('6E400001-B5A3-F393-E0A9-E50E24DCCA9E');
  final characteristicUuid = Guid('6E400002-B5A3-F393-E0A9-E50E24DCCA9E');

  @override
  void initState() {
    super.initState();
    _connectAndSend();
  }

  Future<void> _connectAndSend() async {
    try {
      setState(() {
        status = 'ESP32を探しています...';
        isProcessing = true;
      });

      // スキャン開始
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

      BluetoothDevice? device;
      await for (final results in FlutterBluePlus.scanResults) {
        for (final r in results) {
          debugPrint('デバイス発見: ${r.device.platformName} (${r.device.id})');
          // ESP32-WROVERまたはESP32という名前のデバイスを探す
          if (r.device.platformName?.contains('ESP32') ?? false) {
            device = r.device;
            break;
          }
        }
        if (device != null) break;
      }

      await FlutterBluePlus.stopScan();

      if (device == null) {
        setState(() {
          status = 'ESP32が見つかりませんでした。\n近くにデバイスがあることを確認してください。';
          isProcessing = false;
        });
        return;
      }

      setState(() => status = '${device?.platformName ?? "ESP32"}に接続中...');

      // 接続とサービス検索
      await device.connect(autoConnect: false);
      setState(() => status = 'サービスを検索中...');

      final services = await device.discoverServices();
      final service = services.firstWhere(
        (s) =>
            s.uuid.toString().toLowerCase() ==
            serviceUuid.toString().toLowerCase(),
        orElse: () => throw Exception('サービスUUIDが見つかりませんでした'),
      );

      final characteristic = service.characteristics.firstWhere(
        (c) =>
            c.uuid.toString().toLowerCase() ==
            characteristicUuid.toString().toLowerCase(),
        orElse: () => throw Exception('キャラクタリスティックが見つかりませんでした'),
      );

      setState(() => status = '座標データを送信中...');

      // 座標データの送信
      final payload =
          '${widget.location.latitude.toStringAsFixed(6)},${widget.location.longitude.toStringAsFixed(6)}';
      debugPrint('送信データ: $payload');

      await characteristic.write(payload.codeUnits, withoutResponse: false);
      await Future.delayed(const Duration(seconds: 1)); // 送信完了を待つ
      await device.disconnect();

      setState(() {
        status =
            '座標送信完了！\n\n緯度: ${widget.location.latitude}\n経度: ${widget.location.longitude}';
        isProcessing = false;
      });
    } catch (e) {
      debugPrint('エラー発生: $e');
      setState(() {
        status = '通信中にエラーが発生しました。\n\n$e';
        isProcessing = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('座標送信画面')),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              if (isProcessing)
                const CircularProgressIndicator(
                  valueColor: AlwaysStoppedAnimation<Color>(Colors.blue),
                ),
              const SizedBox(height: 24),
              Text(
                status,
                style: const TextStyle(fontSize: 16),
                textAlign: TextAlign.center,
              ),
            ],
          ),
        ),
      ),
    );
  }
}
