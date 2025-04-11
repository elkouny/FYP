import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import '/screens/game_screen.dart';

void main() => runApp(const ChessBLEApp());

class ChessBLEApp extends StatelessWidget {
  const ChessBLEApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Smart Chess Board',
      theme: ThemeData(primarySwatch: Colors.blue),
      home: const ChessBoardBLEPage(),
    );
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
  StreamSubscription? scanSub;
  StreamSubscription? connectionSub;
  StreamSubscription? charSub;
  bool showStartButton = false;
  String statusMessage = "🔍 Scanning for SmartChessBoard...";

  final String SERVICE_UUID = "180c";
  final String CHARACTERISTIC_UUID = "2a56";

  @override
  void initState() {
    super.initState();
    _requestPermissionsAndStartScan();
  }

  Future<void> _requestPermissionsAndStartScan() async {
    final permissions = await Future.wait([
      Permission.bluetooth.request(),
      Permission.bluetoothScan.request(),
      Permission.bluetoothConnect.request(),
      Permission.locationWhenInUse.request(),
    ]);

    if (permissions.every((status) => status.isGranted)) {
      startScan();
    } else {
      if (!mounted) return;
      setState(() => statusMessage = "❌ Permissions denied");
    }
  }

  Future<void> startScan() async {
    await FlutterBluePlus.setLogLevel(LogLevel.verbose);
    if (!mounted) return;
    setState(() => statusMessage = "🔍 Scanning for board...");
    scanSub?.cancel();

    scanSub = FlutterBluePlus.scanResults.listen(
      (results) async {
        for (var r in results) {
          if (r.device.platformName == "SmartChessBoard") {
            FlutterBluePlus.stopScan();
            await connectToDevice(r.device);
            return;
          }
        }
      },
      onError: (e) {
        if (!mounted) return;
        setState(() => statusMessage = "❌ Scan error: $e");
      },
    );

    FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 15),
      androidUsesFineLocation: false,
    );

    Timer(const Duration(seconds: 15), () {
      if (!mounted || device != null) return;
      setState(() => statusMessage = "❌ Board not found. Retrying...");
      startScan();
    });
  }

  Future<void> connectToDevice(BluetoothDevice d) async {
    device = d;
    try {
      if (!mounted) return;
      setState(() => statusMessage = "🔗 Connecting to board...");
      await device!.connect(autoConnect: false);
      await Future.delayed(const Duration(milliseconds: 500));
      await device!.requestMtu(23);

      if (!mounted) return;
      setState(() => statusMessage = "✅ Connected! Discovering services...");

      connectionSub = device!.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          if (!mounted) return;
          setState(() {
            statusMessage = "❌ Disconnected from board";
            showStartButton = false;
          });
          startScan();
        }
      });

      final services = await device!.discoverServices();
      for (var s in services) {
        if (s.uuid.toString().toLowerCase() == SERVICE_UUID) {
          for (var c in s.characteristics) {
            if (c.uuid.toString().toLowerCase() == CHARACTERISTIC_UUID) {
              gameStatusChar = c;

              if (c.properties.notify) {
                await c.setNotifyValue(true);
                charSub = c.onValueReceived.listen((value) {
                  final msg = utf8.decode(value).trim();
                  debugPrint("Received BLE notification: $msg");
                  if (!mounted) return;
                  _handleBLEMessage(msg);
                });
              }

              if (c.properties.read) {
                try {
                  final value = await c.read();
                  final msg = utf8.decode(value).trim();
                  if (!mounted) return;
                  setState(() => statusMessage = _parseStatus(msg));
                } catch (e) {
                  debugPrint("Initial read failed: $e");
                }
              }

              return;
            }
          }
        }
      }

      if (!mounted) return;
      setState(() => statusMessage = "⚠️ Service/Characteristic not found");
    } catch (e) {
      if (!mounted) return;
      setState(() => statusMessage = "❌ Connection failed: $e");
      startScan();
    }
  }

  String _parseStatus(String data) {
    final d = data.trim().toLowerCase();
    switch (d) {
      case "waiting":
        return "🧩 Waiting for pieces...";
      case "connected":
        return "📶 Connected. Waiting for board setup...";
      case "ready_to_start":
        showStartButton = true;
        return "✅ All pieces placed. Tap to start!";
      default:
        return "ℹ️ Status: $d";
    }
  }

  void _handleBLEMessage(String data) {
    if (data.startsWith("hover:")) {
      // You could pass this to the board later
      final square = data.split(":")[1];
      debugPrint("Piece hovered at $square");
    }

    if (!mounted) return;
    setState(() {
      statusMessage = _parseStatus(data);
    });
  }

  Future<void> _startGame() async {
    if (gameStatusChar == null || !gameStatusChar!.properties.write) return;

    try {
      if (!mounted) return;
      setState(() => statusMessage = "⏳ Starting game...");

      await gameStatusChar!.write(
        utf8.encode("start_confirmed"),
        withoutResponse: gameStatusChar!.properties.writeWithoutResponse,
      );

      if (!mounted) return;
      setState(() {
        showStartButton = false;
        statusMessage = "🚀 Game started!";
      });

      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (_) => const GameScreen()),
      );
    } catch (e) {
      if (!mounted) return;
      setState(() => statusMessage = "❌ Failed to start game: $e");
    }
  }

  @override
  void dispose() {
    scanSub?.cancel();
    connectionSub?.cancel();
    charSub?.cancel();
    device?.disconnect();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final showRetry =
        statusMessage.contains("Disconnected") ||
        statusMessage.contains("failed") ||
        statusMessage.contains("not found");

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
              if (showStartButton)
                ElevatedButton(
                  onPressed: _startGame,
                  child: const Text("▶️ Start Game"),
                ),
              if (showRetry)
                ElevatedButton(
                  onPressed: startScan,
                  child: const Text("🔁 Retry Connection"),
                ),
            ],
          ),
        ),
      ),
    );
  }
}
