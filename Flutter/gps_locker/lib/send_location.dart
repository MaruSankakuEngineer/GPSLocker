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

  final serviceUuid = Guid('6E400001-B5A3-F393-E0A9-E50E24DCCA9E');
  final characteristicUuid = Guid('6E400002-B5A3-F393-E0A9-E50E24DCCA9E');

  @override
  void initState() {
    super.initState();
    _connectAndSend();
  }

  Future<void> _connectAndSend() async {
    try {
      FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));

      BluetoothDevice? device;
      await for (final results in FlutterBluePlus.scanResults) {
        for (final r in results) {
          if (r.device.name == 'ESP32') {
            device = r.device;
            break;
          }
        }
        if (device != null) break;
      }

      FlutterBluePlus.stopScan();
      if (device == null) {
        setState(() => status = 'ESP32が見つかりませんでした');
        return;
      }

      await device.connect(autoConnect: false);
      final services = await device.discoverServices();

      final service = services.firstWhere(
        (s) => s.uuid.toString().toLowerCase() == serviceUuid.toString().toLowerCase(),
        orElse: () => throw Exception('サービスUUIDが見つかりませんでした'),
      );

      final characteristic = service.characteristics.firstWhere(
        (c) => c.uuid.toString().toLowerCase() == characteristicUuid.toString().toLowerCase(),
        orElse: () => throw Exception('キャラクタリスティックが見つかりませんでした'),
      );

      final payload =
          '${widget.location.latitude.toStringAsFixed(6)},${widget.location.longitude.toStringAsFixed(6)}';

      await characteristic.write(payload.codeUnits);
      await device.disconnect();

      setState(() => status = '座標送信完了！');
    } catch (e) {
      setState(() => status = '通信中にエラーが発生しました');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('座標送信画面')),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Text(
            status,
            style: const TextStyle(fontSize: 16),
            textAlign: TextAlign.center,
          ),
        ),
      ),
    );
  }
}
