import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';
import 'package:location/location.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';


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
  LatLng? savedLocation;
  final Location location = Location();
  Set<Marker> markers = {};

  @override
  void initState() {
    super.initState();
    _requestPermissions();
    _loadSavedLocation();
  }

  Future<void> _requestPermissions() async {
    await Permission.location.request();
    await Permission.locationWhenInUse.request();
  }

  void _onMapCreated(GoogleMapController controller) {
    mapController = controller;

    if (savedLocation != null) {
      mapController!.animateCamera(
        CameraUpdate.newLatLng(savedLocation!),
      );
    }
  }

  void _onTap(LatLng latLng) {
    setState(() {
      tappedLocation = latLng;
      markers = {
        Marker(
          markerId: const MarkerId('selected_location'),
          position: latLng,
          infoWindow: const InfoWindow(title: '選択した場所'),
        ),
      };
    });
  }

  void _onSetLocation() {
    if (tappedLocation != null) {
      setState(() {
        savedLocation = tappedLocation;
      });
      _saveLocationToPrefs(tappedLocation!);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('座標を設定しました')),
      );
    }
  }

  Future<void> _moveToCurrentLocation() async {
    try {
      final userLocation = await location.getLocation();
      if (mapController != null) {
        mapController!.animateCamera(
          CameraUpdate.newLatLng(
            LatLng(userLocation.latitude!, userLocation.longitude!),
          ),
        );
      }
    } catch (e) {
      print('現在位置取得に失敗しました: $e');
    }
  }

  Future<void> _moveToSavedLocation() async {
    if (savedLocation != null && mapController != null) {
      mapController!.animateCamera(
        CameraUpdate.newLatLng(savedLocation!),
      );
    }
  }

  Future<void> _saveLocationToPrefs(LatLng location) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble('saved_latitude', location.latitude);
    await prefs.setDouble('saved_longitude', location.longitude);
  }

  Future<void> _loadSavedLocation() async {
    final prefs = await SharedPreferences.getInstance();
    final lat = prefs.getDouble('saved_latitude');
    final lng = prefs.getDouble('saved_longitude');

    if (lat != null && lng != null) {
      final location = LatLng(lat, lng);
      setState(() {
        savedLocation = location;
        markers = {
          Marker(
            markerId: const MarkerId('saved_location'),
            position: location,
            infoWindow: const InfoWindow(title: '設定済みの場所'),
          ),
        };
      });
    }
  }


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('GPSロッカー設定アプリ'),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8.0),
            child: ElevatedButton(
              onPressed: _onSetLocation,
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.orange,
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                textStyle: const TextStyle(fontWeight: FontWeight.bold),
              ),
              child: const Text('座標設定'),
            ),
          ),
        ],
      ),
      body: Stack(
        children: [
          Column(
            children: [
              Expanded(
                child: GoogleMap(
                  onMapCreated: _onMapCreated,
                  initialCameraPosition: const CameraPosition(
                    target: LatLng(35.681236, 139.767125),
                    zoom: 14,
                  ),
                  onTap: _onTap,
                  myLocationEnabled: true,
                  markers: markers,
                ),
              ),
              if (tappedLocation != null)
                Padding(
                  padding: const EdgeInsets.all(8.0),
                  child: Text(
                    '選択中の位置：\n緯度: ${tappedLocation!.latitude.toStringAsFixed(6)} \n経度: ${tappedLocation!.longitude.toStringAsFixed(6)}',
                    textAlign: TextAlign.center,
                  ),
                ),
              if (savedLocation != null)
                Padding(
                  padding: const EdgeInsets.all(8.0),
                  child: Text(
                    '✅ 設定済みの位置：\n緯度: ${savedLocation!.latitude.toStringAsFixed(6)} \n経度: ${savedLocation!.longitude.toStringAsFixed(6)}',
                    style: const TextStyle(fontWeight: FontWeight.bold),
                    textAlign: TextAlign.center,
                  ),
                ),
                if (savedLocation != null)
                  Positioned(
                    bottom: 150,
                    right: 16,
                    child: FloatingActionButton.extended(
                      onPressed: _moveToSavedLocation,
                      icon: const Icon(Icons.place),
                      label: const Text('設定位置へ'),
                    ),
                  ),
            ],
          ),
        ],
      ),
    );
  }
}
