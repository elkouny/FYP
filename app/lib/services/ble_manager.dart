import 'dart:async';
import 'dart:convert';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BLEManager {
  // Singleton instance.
  static final BLEManager _instance = BLEManager._internal();
  factory BLEManager() => _instance;
  BLEManager._internal();

  BluetoothDevice? device;
  BluetoothCharacteristic? gameStatusChar;

  // UUIDs (in lowercase for comparison).
  final String serviceUUID = "180c";
  final String characteristicUUID = "2a56";

  // StreamController for broadcasting BLE status and messages.
  final StreamController<String> _statusController =
      StreamController.broadcast();
  Stream<String> get statusStream => _statusController.stream;

  StreamSubscription? _scanSub;
  StreamSubscription? _connectionSub;
  StreamSubscription? _charSub;

  /// Helper to safely add messages if the stream is still open.
  void _addStatus(String status) {
    if (!_statusController.isClosed) {
      _statusController.add(status);
    }
  }

  /// Start scanning for the board and establish a connection.
  Future<void> startScanAndConnect() async {
    // Cancel any previous scan.
    _scanSub?.cancel();

    // Start scanning for BLE devices.
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 60));
    _scanSub = FlutterBluePlus.scanResults.listen((results) async {
      for (var result in results) {
        if (result.device.platformName == "SmartChessBoard") {
          FlutterBluePlus.stopScan();
          device = result.device;
          try {
            await device!.connect(autoConnect: false);
            await device!.requestMtu(23);

            // Listen for connection state changes.
            _connectionSub = device!.connectionState.listen((state) {
              if (state == BluetoothConnectionState.disconnected) {
                _addStatus("Disconnected");
                // Optionally trigger a new scan after disconnect.
                startScanAndConnect();
              }
            });

            // Discover services and characteristics.
            final services = await device!.discoverServices();
            for (var service in services) {
              if (service.uuid.toString().toLowerCase() == serviceUUID) {
                for (var characteristic in service.characteristics) {
                  if (characteristic.uuid.toString().toLowerCase() ==
                      characteristicUUID) {
                    gameStatusChar = characteristic;
                    // Enable notifications if supported.
                    if (characteristic.properties.notify) {
                      await characteristic.setNotifyValue(true);
                      _charSub = characteristic.onValueReceived.listen((value) {
                        final msg = utf8.decode(value).trim();
                        _addStatus(msg);
                      });
                    }
                  }
                }
              }
            }
            _addStatus("Connected");
          } catch (e) {
            _addStatus("Connection failed: $e");
            startScanAndConnect();
          }
          break;
        }
      }
    });
  }

  /// Write a message to the gameStatus characteristic.
  Future<void> writeCharacteristic(String message) async {
    if (gameStatusChar != null && gameStatusChar!.properties.write) {
      await gameStatusChar!.write(
        utf8.encode(message),
        withoutResponse: gameStatusChar!.properties.writeWithoutResponse,
      );
    }
  }

  /// Dispose resources. (Call this only when the app is terminating)
  void dispose() {
    _scanSub?.cancel();
    _connectionSub?.cancel();
    _charSub?.cancel();
    device?.disconnect();
    _statusController.close();
  }
}
