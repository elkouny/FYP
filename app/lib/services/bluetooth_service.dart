import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BluetoothService {
  Future<void> connectToESP32() async {
    final devices = await FlutterBluePlus.connectedDevices;
    for (var device in devices) {
      if (device.platformName == 'ESP32-Chess') {
        await device.connect();
        // Subscribe to notifications
      }
    }
  }

  void sendMove(String from, String to) {
    // Send move to ESP32 over BLE
  }

  void handleIncomingEvent(String data) {
    // e.g., {"event": "pickup", "square": "e2"}
  }
}
