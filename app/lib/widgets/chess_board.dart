import 'dart:async';
import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:chess/chess.dart' as chess;
import 'package:http/http.dart' as http;
import '../services/ble_manager.dart';
import 'package:collection/collection.dart';

class ChessBoard extends StatefulWidget {
  final BLEManager bleManager;
  final String whitePlayerName;
  final String blackPlayerName;
  final int whitePlayerRating;
  final int blackPlayerRating;
  final bool? playAgainstBot;
  final String? userColor;
  final String? lichessGameId;
  final String? lichessToken;

  const ChessBoard({
    required this.bleManager,
    required this.whitePlayerName,
    required this.blackPlayerName,
    required this.whitePlayerRating,
    required this.blackPlayerRating,
    super.key,
    this.lichessGameId,
    this.lichessToken,
    this.playAgainstBot,
    this.userColor,
  });

  @override
  State<ChessBoard> createState() => _ChessBoardState();
}

class _ChessBoardState extends State<ChessBoard> {
  late chess.Chess _game;
  String? selectedSquare;
  List<String> validMoves = [];
  List<String> capturedWhite = [];
  List<String> capturedBlack = [];
  String latestBleMessage = "";

  bool pauseBle = false;
  String lastCommand = "";
  late StreamSubscription<String> _statusSub;

  @override
  void initState() {
    super.initState();
    _game = chess.Chess();

    _statusSub = widget.bleManager.statusStream.listen((msg) {
      if (_statusSub.isPaused) return;
      if (pauseBle) return;
      if (msg.startsWith("hover:")) {
        final square = msg.split(":")[1];
        _highlightMoves(square);
      } else if (msg.startsWith("move:")) {
        final move = msg.split(":")[1];
        _makeMove(move.substring(0, 2), move.substring(2, 4));
      } else if (msg.startsWith("capture:")) {
        final move = msg.split(":")[1];
        _captureMove(move.substring(0, 2), move.substring(2, 4));
      } else if (msg.startsWith("clear")) {
        _clearHighlight();
      } else if (msg == "Disconnected") {
        _handleDisconnect();
      }
      latestBleMessage = msg;
    });
  }

  @override
  void dispose() {
    _statusSub.cancel();
    super.dispose();
  }

  void _handleDisconnect() {
    // Show an alert when disconnected
    _showAlert("❌ Bluetooth Disconnected");
    // Navigate back to the home screen
    _navigateToHome();
  }

  void _navigateToHome() async {
    await widget.bleManager.writeCharacteristic("game_ended");
    Navigator.pop(context); // Go back to the home screen
  }

  void _highlightMoves(String square) {
    final piece = _game.get(square);
    if (piece != null && piece.color == _game.turn) {
      setState(() {
        selectedSquare = square;
        validMoves =
            _game
                .generate_moves()
                .where((m) => m.fromAlgebraic == square)
                .map((m) => m.toAlgebraic)
                .toList();
      });
      String command = "light_on:${square + validMoves.join("")}";
      if (lastCommand != command) {
        widget.bleManager.writeCharacteristic(command);
        lastCommand = command;
      }
    } else {
      _clearHighlight();
    }
  }

  Future<void> _captureMove(String from, String to) async {
    final captured = _game.get(to);
    final success = _game.move({'from': from, 'to': to});

    if (success && captured != null) {
      if (captured.color == chess.Color.WHITE) {
        capturedWhite.add(captured.type.name);
      } else {
        capturedBlack.add(captured.type.name);
      }
      widget.bleManager.writeCharacteristic("capture_ack:$from$to");
      _clearHighlight();
      _checkGameStatus();
      if (widget.playAgainstBot == true) {
        final uci = from + to;
        await _sendMoveToLichess(uci);
        await _listenForBotMove();
      }
    } else {
      _showAlert("Invalid capture $from → $to");
    }
  }

  void _makeMove(String from, String to) async {
    const int bitsEpCapture = 8;
    const int bitsKsideCastle = 32;
    const int bitsQsideCastle = 64;
    const int bitsPromotion = 16;

    final piece = _game.get(from);
    String? promo;

    if (piece != null && piece.color != _game.turn) {
      _showAlert("⛔ It's not ${piece.color.name}'s turn");
      return;
    }
    final moveObj = _game.generate_moves().firstWhereOrNull(
      (m) => m.fromAlgebraic == from && m.toAlgebraic == to,
    );
    if (moveObj == null) {
      _showAlert("⛔ Invalid move from $from to $to");
      return;
    }
    if (moveObj.flags & bitsPromotion != 0) {
      widget.bleManager.writeCharacteristic("promotion!!!");
      pauseBle = true;
      promo = await _showPromotionDialog();
      if (promo == null) {
        _clearHighlight();
      }
    }
    final success = _game.move({'from': from, 'to': to, 'promotion': promo});
    pauseBle = false;
    if (success) {
      if (moveObj.flags & bitsEpCapture != 0) {
        widget.bleManager.writeCharacteristic("en passant!!!");
        final capture = to[0] + from[1];
        final captured = _game.get(to);
        if (captured != null) {
          if (captured.color == chess.Color.WHITE) {
            capturedWhite.add(captured.type.name);
          } else {
            capturedBlack.add(captured.type.name);
          }
          widget.bleManager.writeCharacteristic("capture_ack:$from$capture");
          widget.bleManager.writeCharacteristic("move_ack:$capture$to");
          _clearHighlight();
        }
      } else if (moveObj.flags & bitsKsideCastle != 0) {
        widget.bleManager.writeCharacteristic("kingside castle!!!");
        final rookFrom = 'h${to[1]}';
        final rookTo = 'f${to[1]}';
        widget.bleManager.writeCharacteristic("move_ack:$from$to");
        widget.bleManager.writeCharacteristic("move_ack:$rookFrom$rookTo");
        _clearHighlight();
      } else if (moveObj.flags & bitsQsideCastle != 0) {
        widget.bleManager.writeCharacteristic("queenside castle!!!");
        final rookFrom = 'a${to[1]}';
        final rookTo = 'd${to[1]}';
        widget.bleManager.writeCharacteristic("move_ack:$from$to");
        widget.bleManager.writeCharacteristic("move_ack:$rookFrom$rookTo");
        _clearHighlight();
      } else {
        widget.bleManager.writeCharacteristic("move_ack:$from$to");
        _clearHighlight();
      }
      _checkGameStatus();
      if (widget.playAgainstBot == true) {
        final uci = from + to + (promo ?? "");
        await _sendMoveToLichess(uci);
        await _listenForBotMove();
      }
    } else {
      _showAlert("❌ Invalid move from $from to $to");
    }
  }

  Future<void> _sendMoveToLichess(String uci) async {
    if (widget.lichessGameId == null || widget.lichessToken == null) return;

    final response = await http.post(
      Uri.parse(
        "https://lichess.org/api/board/game/${widget.lichessGameId}/move/$uci",
      ),
      headers: {'Authorization': 'Bearer ${widget.lichessToken}'},
    );

    if (response.statusCode != 200) {
      _showAlert("❌ Lichess move failed: ${response.body}");
    }
  }

  Future<void> _listenForBotMove() async {
    final gameId = widget.lichessGameId;
    final token = widget.lichessToken;
    final userColor = widget.userColor;

    if (gameId == null || token == null || userColor == null) return;

    final request = http.Request(
      'GET',
      Uri.parse('https://lichess.org/api/board/game/stream/$gameId'),
    );
    request.headers['Authorization'] = 'Bearer $token';
    final response = await request.send();
    final lines = response.stream
        .transform(utf8.decoder)
        .transform(const LineSplitter());

    await for (final line in lines) {
      if (line.trim().isEmpty) continue;

      final event = jsonDecode(line);
      if (event['type'] == 'gameState') {
        final moveStr = event['moves'] as String;
        final moves = moveStr.split(" ");

        // Determine bot's latest move
        final myMoves = _game.history.map((m) => m.move.toAlgebraic).toList();
        if (moves.length > myMoves.length) {
          final latest = moves.last;
          final from = latest.substring(0, 2);
          final to = latest.substring(2, 4);
          // Optional promotion handling
          final promo = latest.length > 4 ? latest[4] : null;

          setState(() {
            _game.move({
              'from': from,
              'to': to,
              if (promo != null) 'promotion': promo,
            });
          });
          _highlightMoves(from);

          _checkGameStatus();
          break;
        }
      }
    }
  }

  void _checkGameStatus() {
    // Checkmate?
    if (_game.in_checkmate) {
      final winner = _game.turn == chess.Color.WHITE ? 'Black' : 'White';
      _showEndingAlert("🎉 Checkmate! $winner wins.");
    }

    // Stalemate?
    if (_game.in_stalemate) {
      _showEndingAlert("🤝 Stalemate — draw.");
    }

    // Draw by insufficient material?
    if (_game.insufficient_material) {
      _showEndingAlert("🤝 Draw by insufficient material.");
    }

    // Draw by threefold repetition?
    if (_game.in_threefold_repetition) {
      _showEndingAlert("🤝 Draw by threefold repetition.");
    }

    // Any other draw?
    if (_game.in_draw) {
      _showEndingAlert("🤝 Draw.");
    }
  }

  Future<String?> _showPromotionDialog() {
    // determine color so we show white or black pieces
    final isWhite = _game.turn == chess.Color.WHITE;
    final colorPrefix = isWhite ? 'w' : 'b';

    return showDialog<String>(
      context: context,
      barrierDismissible: false, // force a choice
      builder:
          (ctx) => AlertDialog(
            title: const Text("Choose promotion"),
            content: Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                _promoTile(ctx, '${colorPrefix}q', 'q'),
                _promoTile(ctx, '${colorPrefix}r', 'r'),
                _promoTile(ctx, '${colorPrefix}b', 'b'),
                _promoTile(ctx, '${colorPrefix}n', 'n'),
              ],
            ),
          ),
    );
  }

  Widget _promoTile(BuildContext ctx, String assetName, String promoPiece) {
    return GestureDetector(
      onTap: () => Navigator.of(ctx).pop(promoPiece),
      child: Image.asset('assets/pieces/$assetName.png', width: 48, height: 48),
    );
  }

  void _clearHighlight() {
    setState(() {
      selectedSquare = null;
      validMoves = [];
    });
    String command = "light_off";
    if (lastCommand != command) {
      widget.bleManager.writeCharacteristic(command);
      lastCommand = command;
    }
  }

  void _showAlert(String msg) {
    pauseBle = true;
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text("Alert"),
          content: Text(msg),
          actions: [
            TextButton(
              child: const Text("OK"),
              onPressed: () {
                Navigator.of(context).pop(); // Close the dialog
                pauseBle = false;
              },
            ),
          ],
        );
      },
    );
  }

  void _showEndingAlert(String msg) {
    pauseBle = true;
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text("Alert"),
          content: Text(msg),
          actions: [
            TextButton(
              child: const Text("OK"),
              onPressed: () {
                Navigator.of(context).pop(); // Close the dialog
                pauseBle = false;
                _navigateToHome();
              },
            ),
          ],
        );
      },
    );
  }

  Widget _buildPiece(chess.Piece piece) {
    final color = piece.color == chess.Color.WHITE ? 'w' : 'b';
    final type = piece.type.toLowerCase(); // p, r, n...
    final path = 'assets/pieces/$color$type.png';

    return Image.asset(path, width: 40, height: 40, fit: BoxFit.contain);
  }

  Widget _buildMoveLog() {
    final moves = _game.san_moves();
    return Container(
      height: 32,
      color: Colors.grey[850],
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: ListView.builder(
        scrollDirection: Axis.horizontal,
        reverse: true, // Scroll to the end by default
        itemCount: moves.length,
        itemBuilder: (ctx, i) {
          final moveIndex = moves.length - 1 - i; // Reverse index
          return Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Text(
              moves[moveIndex]!,
              style: const TextStyle(color: Colors.white, fontSize: 14),
            ),
          );
        },
      ),
    );
  }

  Widget _buildPlayerDisplay(String playerName, String playerRating) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.start,
        children: [
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                playerName,
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 18,
                  fontWeight: FontWeight.w500,
                ),
              ),
              Text(
                "($playerRating)",
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 14,
                  fontWeight: FontWeight.normal,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildInfoRow(String player, List<String> pieces, bool isActive) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween, // Space between
        children: [
          _buildCapturedPieces(pieces, player),
          _buildTimer(player, "05:00", isActive),
        ],
      ),
    );
  }

  Widget _buildCapturedPieces(List<String> pieces, String player) {
    final points = pieces.fold(0, (sum, p) {
      switch (p.toLowerCase()) {
        case 'p':
          return sum + 1;
        case 'n':
          return sum + 3;
        case 'b':
          return sum + 3;
        case 'r':
          return sum + 5;
        case 'q':
          return sum + 9;
        default:
          return sum;
      }
    });

    return Row(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        ...pieces.map((p) {
          final color = player == "Black" ? 'w' : 'b';
          return Padding(
            padding: const EdgeInsets.symmetric(horizontal: 2),
            child: Image.asset(
              'assets/pieces/$color${p.toLowerCase()}.png',
              width: 28,
              height: 28,
            ),
          );
        }),
        const SizedBox(width: 6),
        Text("($points pts)", style: const TextStyle(color: Colors.white)),
      ],
    );
  }

  Widget _buildTimer(String player, String time, bool isActive) {
    final bgColor =
        isActive
            ? (player == "White" ? Colors.white : Colors.black)
            : (player == "White" ? Colors.grey[300] : Colors.grey[900]);
    final textColor =
        isActive
            ? (player == "White" ? Colors.black : Colors.white)
            : (player == "White" ? Colors.black54 : Colors.grey);
    final fontWeight = isActive ? FontWeight.bold : FontWeight.normal;
    final iconColor = player == "White" ? Colors.black : Colors.white;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Container(
        decoration: BoxDecoration(
          color: bgColor,
          borderRadius: BorderRadius.circular(4),
          border: Border.all(color: Colors.grey[700]!, width: 1),
        ),
        padding: const EdgeInsets.all(8.0),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (isActive)
              Icon(
                Icons.timer,
                color: iconColor,
                size: 16,
              ), // Timer icon when active
            const SizedBox(width: 4),
            Text(
              time,
              style: TextStyle(
                color: textColor,
                fontSize: 16,
                fontWeight: fontWeight,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBoard() {
    return Column(
      children: [
        // 1: the board + ranks
        AspectRatio(
          aspectRatio: 1.0,
          child: Row(
            children: [
              // RANK LABELS: each Expanded takes 1/8 of the height
              Column(
                children: List.generate(8, (i) {
                  final rank = (8 - i).toString();
                  return Expanded(
                    child: Center(
                      child: Text(
                        rank,
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 12,
                        ),
                      ),
                    ),
                  );
                }),
              ),

              // THE BOARD ITSELF
              Expanded(
                child: GridView.builder(
                  itemCount: 64,
                  physics: const NeverScrollableScrollPhysics(),
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 8,
                  ),
                  itemBuilder: (context, index) => _buildSquare(context, index),
                ),
              ),
            ],
          ),
        ),

        // 2: the file labels row
        Row(
          children: [
            // Spacer matching the width of the rank‑column.

            // Now 8 Expanded children, each under its square
            Expanded(
              child: Row(
                children: List.generate(8, (i) {
                  final file = String.fromCharCode('a'.codeUnitAt(0) + i);
                  return Expanded(
                    child: Center(
                      child: Text(
                        file,
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 12,
                        ),
                      ),
                    ),
                  );
                }),
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _buildSquare(BuildContext context, int index) {
    final row = 7 - index ~/ 8;
    final col = index % 8;
    final square =
        String.fromCharCode('a'.codeUnitAt(0) + col) + (row + 1).toString();

    final piece = _game.get(square);
    final isLight = (row + col) % 2 == 0;
    final isSelected = selectedSquare == square;
    final isValid = validMoves.contains(square);
    final baseColor =
        isLight ? const Color(0xFFEEEED2) : const Color(0xFF769656);
    final squareColor = isSelected ? Colors.yellow : baseColor;

    return GestureDetector(
      onTap: () => _highlightMoves(square),
      child: Container(
        color: squareColor,
        child: Stack(
          children: [
            if (isValid && piece == null)
              Center(
                child: Container(
                  width: 12,
                  height: 12,
                  decoration: BoxDecoration(
                    color: Colors.grey[700]!,
                    shape: BoxShape.circle,
                  ),
                ),
              ),
            if (isValid && piece != null)
              Container(color: Colors.redAccent), // Capture highlight

            Center(child: piece == null ? null : _buildPiece(piece)),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final bool whiteToMove = _game.turn == chess.Color.WHITE;

    return Scaffold(
      backgroundColor: Colors.grey[800], // Dark gray background
      appBar: AppBar(
        title: const Text('Smart Chess Board'),
        backgroundColor: Colors.brown[700],
      ),
      body: Column(
        children: [
          _buildMoveLog(),
          _buildPlayerDisplay(
            widget.blackPlayerName,
            widget.blackPlayerRating.toString(),
          ),
          const SizedBox(height: 6),
          _buildInfoRow("Black", capturedWhite, !whiteToMove), // Combined row
          const SizedBox(height: 4),
          _buildBoard(),
          _buildInfoRow("White", capturedBlack, whiteToMove), // Combined row
          const SizedBox(height: 6),
          _buildPlayerDisplay(
            widget.whitePlayerName,
            widget.whitePlayerRating.toString(),
          ),
          const SizedBox(height: 10),
          // Quit button
          ElevatedButton.icon(
            onPressed: _navigateToHome,
            icon: const Icon(Icons.exit_to_app),
            label: const Text('Quit'),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.red,
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              textStyle: const TextStyle(fontSize: 16),
            ),
          ),
          const SizedBox(height: 10),
        ],
      ),
    );
  }
}
