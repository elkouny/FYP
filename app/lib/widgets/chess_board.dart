import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:chess/chess.dart' as chess;
import 'dart:async';


class ChessBoard extends StatefulWidget {
  const ChessBoard({super.key});

  @override
  State<ChessBoard> createState() => _ChessBoardState();
}

class _ChessBoardState extends State<ChessBoard> {
  late chess.Chess _game;
  String? selectedSquare;
  List<int> validMoves = [];

  BluetoothDevice? device;
  BluetoothCharacteristic? gameStatusChar;
  StreamSubscription? bleSub;

  final String SERVICE_UUID = "180c";
  final String CHARACTERISTIC_UUID = "2a56";

  @override
  void initState() {
    super.initState();
    _game = chess.Chess();
    _startBLE();
  }

  void _startBLE() async {
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

    FlutterBluePlus.scanResults.listen((results) async {
      for (var r in results) {
        if (r.device.platformName == "SmartChessBoard") {
          FlutterBluePlus.stopScan();
          device = r.device;
          await device!.connect(autoConnect: false);
          await device!.requestMtu(23);

          final services = await device!.discoverServices();
          for (var s in services) {
            if (s.uuid.toString().toLowerCase() == SERVICE_UUID) {
              for (var c in s.characteristics) {
                if (c.uuid.toString().toLowerCase() == CHARACTERISTIC_UUID) {
                  gameStatusChar = c;

                  if (c.properties.notify) {
                    await c.setNotifyValue(true);
                    bleSub = c.onValueReceived.listen((value) {
                      final msg = utf8.decode(value).trim();
                      if (msg.startsWith("hover:")) {
                        final square = msg.split(":")[1];
                        _highlightMoves(square);
                      } else if (msg.startsWith("move:")) {
                        final move = msg.split(":")[1];
                        final from = move.substring(0, 2);
                        final to = move.substring(2, 4);
                        _makeMove(from, to);
                      }
                    });
                  }
                }
              }
            }
          }

          break;
        }
      }
    });
  }

  void _highlightMoves(String square) {
    final piece = _game.get(square);
    if (piece != null && piece.color == _game.turn) {
      setState(() {
        selectedSquare = square;
        validMoves =
            _game
                .generate_moves()
                .where((m) => m.from == square)
                .map((m) => m.to)
                .toList();
      });
    } else {
      _clearHighlight();
    }
  }

  void _makeMove(String from, String to) {
    setState(() {
      _game.move({'from': from, 'to': to});
      _clearHighlight();
    });
  }

  void _clearHighlight() {
    selectedSquare = null;
    validMoves = [];
  }

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      itemCount: 64,
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 8,
      ),
      itemBuilder: (context, index) {
        int row = index ~/ 8;
        int col = index % 8;
        String square =
            String.fromCharCode('a'.codeUnitAt(0) + col) + (8 - row).toString();
        var piece = _game.get(square);
        bool isSelected = square == selectedSquare;
        bool isValidMove = validMoves.contains(square);

        return GestureDetector(
          onTap: () => _highlightMoves(square),
          child: Container(
            decoration: BoxDecoration(
              color:
                  isSelected
                      ? Colors.green
                      : isValidMove
                      ? Colors.greenAccent
                      : (row + col).isEven
                      ? Colors.brown[300]
                      : Colors.white,
              border: Border.all(color: Colors.black12),
            ),
            child: Center(
              child: Text(
                piece?.type.toUpperCase() ?? '',
                style: TextStyle(
                  fontSize: 24,
                  color:
                      piece?.color == chess.Color.WHITE
                          ? Colors.black
                          : Colors.brown[900],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  @override
  void dispose() {
    bleSub?.cancel();
    device?.disconnect();
    super.dispose();
  }
}
