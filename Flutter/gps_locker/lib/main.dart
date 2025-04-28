import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:location/location.dart';
import 'package:permission_handler/permission_handler.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: MapSample(),
    );
  }
}

class MapSample extends StatefulWidget {
  @override
  State<MapSample> createState() => MapSampleState();
}

class MapSampleState extends State<MapSample> {
  GoogleMapController? mapController;
  LatLng? tappedLocation;

  @override
  void initState() {
    super.initState();
    _requestPermissions();
  }

  Future<void> _requestPermissions() async {
    await Permission.location.request();
    await Permission.locationWhenInUse.request();
  }

  void _onMapCreated(GoogleMapController controller) {
    mapController = controller;
  }

  void _onTap(LatLng latLng) {
    setState(() {
      tappedLocation = latLng;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('GPSロッカー設定アプリ'),
      ),
      body: Column(
        children: [
          Expanded(
            child: GoogleMap(
              onMapCreated: _onMapCreated,
              initialCameraPosition: const CameraPosition(
                target: LatLng(35.681236, 139.767125), // 東京駅周辺
                zoom: 14,
              ),
              onTap: _onTap,
              myLocationEnabled: true,
            ),
          ),
          if (tappedLocation != null)
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Text(
                '選択した位置：\n緯度: ${tappedLocation!.latitude}, 経度: ${tappedLocation!.longitude}',
                textAlign: TextAlign.center,
              ),
            ),
        ],
      ),
    );
  }
}
