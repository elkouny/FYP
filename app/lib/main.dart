import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import './services/ble_manager.dart';
import './widgets/chess_board.dart';

void main() {
  runApp(const ChessBLEApp());
}

class ChessBLEApp extends StatelessWidget {
  const ChessBLEApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Chess Board',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(primarySwatch: Colors.blue),
      home: const ChessBoardBLEPage(),
    );
  }
}

class ChessBoardBLEPage extends StatefulWidget {
  const ChessBoardBLEPage({Key? key}) : super(key: key);

  @override
  State<ChessBoardBLEPage> createState() => _ChessBoardBLEPageState();
}

class _ChessBoardBLEPageState extends State<ChessBoardBLEPage> {
  final BLEManager bleManager = BLEManager();
  String statusMessage = "🔍 Scanning for SmartChessBoard...";
  bool showStartButton = false;

  @override
  void initState() {
    super.initState();
    _requestPermissionsAndStartScan();

    // Listen to BLEManager's status stream
    bleManager.statusStream.listen((msg) {
      if (!mounted) return;
      setState(() {
        statusMessage = _parseStatus(msg);
      });
    });
  }

  String _parseStatus(String data) {
    final d = data.trim().toLowerCase();

    switch (d) {
      case "waiting":
        return "🧩 Waiting for pieces...";
      case "connected":
        return "📶 Connected. Waiting for board setup...";
      case "ready_to_start":
        setState(() => showStartButton = true);
        return "✅ All pieces placed. Tap to start!";
      default:
        return "ℹ️ Status: $d";
    }
  }

  Future<void> _requestPermissionsAndStartScan() async {
    final permissions = await Future.wait([
      Permission.bluetooth.request(),
      Permission.bluetoothScan.request(),
      Permission.bluetoothConnect.request(),
      Permission.locationWhenInUse.request(),
    ]);

    if (permissions.every((status) => status.isGranted)) {
      bleManager.startScanAndConnect();
    } else {
      setState(() => statusMessage = "❌ Permissions denied");
    }
  }

  Future<void> _startGame() async {
    if (bleManager.gameStatusChar == null) return;

    setState(() {
      statusMessage = "⏳ Starting game...";
      showStartButton = false;
    });

    try {
      await bleManager.writeCharacteristic("start_confirmed");

      setState(() {
        statusMessage = "🚀 Game started!";
      });

      Navigator.push(
        context,
        MaterialPageRoute(
          builder:
              (_) => ChessBoard(
                key: UniqueKey(),
                bleManager: bleManager,
                whitePlayerName: "Ismail", // Default name
                blackPlayerName: "Kouny", // Default name
                whitePlayerRating: 500, // Default rating
                blackPlayerRating: 1200, // Default rating
              ),
        ),
      );
    } catch (e) {
      setState(() => statusMessage = "❌ Failed to start game: $e");
    }
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
                ElevatedButton.icon(
                  onPressed: _startGame,
                  icon: const Icon(Icons.play_arrow),
                  label: const Text("Start Game"),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.green,
                    padding: const EdgeInsets.symmetric(
                      horizontal: 24,
                      vertical: 12,
                    ),
                  ),
                ),

              if (showRetry)
                ElevatedButton.icon(
                  onPressed: () async {
                    await bleManager.startScanAndConnect();
                  },
                  icon: const Icon(Icons.refresh),
                  label: const Text("Retry Connection"),
                ),
            ],
          ),
        ),
      ),
    );
  }

  @override
  void dispose() {
    super.dispose();
  }
}
