import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

void main() => runApp(const ChessBLEApp());

class ChessBLEApp extends StatelessWidget {
  const ChessBLEApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(home: ChessBoardBLEPage());
  }
}

class ChessBoardBLEPage extends StatefulWidget {
  const ChessBoardBLEPage({super.key});

  @override
  State<ChessBoardBLEPage> createState() => _ChessBoardBLEPageState();
}

class _ChessBoardBLEPageState extends State<ChessBoardBLEPage> {
  BluetoothDevice? device;
  BluetoothCharacteristic? gameStatusChar;
  String statusMessage = "🔍 Scanning for SmartChessBoard...";
  StreamSubscription? scanSub;
  StreamSubscription? connectionSub;

  @override
  void initState() {
    super.initState();
    startScan();
  }

  Future<void> startScan() async {
    await Permission.bluetooth.request();
    await Permission.location.request();

    setState(() => statusMessage = "🔍 Scanning...");
    scanSub?.cancel();

    scanSub = FlutterBluePlus.scanResults.listen((results) async {
      for (var r in results) {
        if (r.device.platformName == "SmartChessBoard") {
          FlutterBluePlus.stopScan();
          scanSub?.cancel();
          setState(() => statusMessage = "🔗 Connecting to board...");
          connectToDevice(r.device);
          return;
        }
      }
    });

    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));
  }

  Future<void> connectToDevice(BluetoothDevice d) async {
    device = d;
    try {
      await device!.connect(autoConnect: false);
      setState(() => statusMessage = "✅ Connected!");

      connectionSub = device!.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          setState(() => statusMessage = "❌ Disconnected from board");
        }
      });

      List<BluetoothService> services = await device!.discoverServices();
      for (var service in services) {
        for (var c in service.characteristics) {
          if (c.uuid.toString().toLowerCase().contains("2a56")) {
            gameStatusChar = c;

            await c.setNotifyValue(true);
            c.onValueReceived.listen((value) {
              final data = String.fromCharCodes(value);
              setState(() => statusMessage = parseStatus(data));
            });

            return;
          }
        }
      }

      setState(() => statusMessage = "⚠️ Couldn't find game status characteristic.");
    } catch (e) {
      setState(() => statusMessage = "❌ Connection failed: $e");
    }
  }

  String parseStatus(String data) {
    switch (data.trim()) {
      case "waiting":
        return "🧩 Waiting for pieces...";
      case "connected":
        return "📶 Connected. Waiting for board setup...";
      case "ready_to_start":
        return "✅ All pieces placed. Tap to start!";
      default:
        return "ℹ️ Status: $data";
    }
  }

  @override
  void dispose() {
    scanSub?.cancel();
    connectionSub?.cancel();
    device?.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final showRetry = statusMessage.contains("Disconnected") ||
        statusMessage.contains("Connection failed") ||
        statusMessage.contains("Couldn't find");

    return Scaffold(
      appBar: AppBar(title: const Text("Smart Chess Board")),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                statusMessage,
                textAlign: TextAlign.center,
                style: const TextStyle(fontSize: 20),
              ),
              const SizedBox(height: 20),
              if (showRetry)
                ElevatedButton(
                  onPressed: () {
                    setState(() {
                      statusMessage = "🔁 Retrying scan...";
                      device = null;
                      gameStatusChar = null;
                    });
                    startScan();
                  },
                  child: const Text("🔁 Retry"),
                ),
            ],
          ),
        ),
      ),
    );
  }
}
