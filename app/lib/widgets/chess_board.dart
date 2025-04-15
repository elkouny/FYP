import 'package:flutter/material.dart';
import 'package:chess/chess.dart' as chess;
import '../services/ble_manager.dart';

class ChessBoard extends StatefulWidget {
  final BLEManager bleManager;
  const ChessBoard({required this.bleManager, super.key});

  @override
  State<ChessBoard> createState() => _ChessBoardState();
}

class _ChessBoardState extends State<ChessBoard> {
  late chess.Chess _game;
  String? selectedSquare;
  List<String> validMoves = [];
  List<String> capturedWhite = [];
  List<String> capturedBlack = [];

  @override
  void initState() {
    super.initState();
    _game = chess.Chess();

    widget.bleManager.statusStream.listen((msg) {
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
                .where((m) => m.fromAlgebraic == square)
                .map((m) => m.toAlgebraic)
                .toList();
      });
    } else {
      _clearHighlight();
    }
  }

  void _captureMove(String from, String to) {
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
    } else {
      _showAlert("Invalid capture $from → $to");
    }
  }

  void _makeMove(String from, String to) {
    const int BITS_EP_CAPTURE = 8;

    final piece = _game.get(from);

    if (piece == null || piece.color != _game.turn) {
      widget.bleManager.writeCharacteristic("Invalid move");
      _showAlert("⛔ It's not ${piece?.color.name}'s turn");
      return;
    }
    chess.Move moveObj = _game.generate_moves().firstWhere(
      (m) => m.fromAlgebraic == from && m.toAlgebraic == to,
    );
    final success = _game.move({'from': from, 'to': to});
    if (success) {
      if (moveObj.flags & BITS_EP_CAPTURE != 0) {
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
      } else {
        widget.bleManager.writeCharacteristic("move_ack:$from$to");
        _clearHighlight();
      }
    } else {
      _showAlert("❌ Invalid move from $from to $to");
    }
  }

  void _clearHighlight() {
    setState(() {
      selectedSquare = null;
      validMoves = [];
    });
  }

  void _showAlert(String msg) {
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(SnackBar(content: Text(msg), backgroundColor: Colors.red));
  }

  Widget _buildPiece(chess.Piece piece) {
    final color = piece.color == chess.Color.WHITE ? 'w' : 'b';
    final type = piece.type.toLowerCase(); // p, r, n...
    final path = 'assets/pieces/$color$type.png';

    return Image.asset(path, width: 40, height: 40, fit: BoxFit.contain);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.green[900],
      appBar: AppBar(
        title: const Text('Smart Chess Board'),
        backgroundColor: Colors.brown[700],
      ),
      body: Column(
        children: [
          const SizedBox(height: 6),
          _buildCapturedRow(capturedWhite, "Black"),
          const SizedBox(height: 4),
          _buildTimer("Black", "05:00"),
          _buildBoard(),
          _buildTimer("White", "05:00"),
          _buildCapturedRow(capturedBlack, "White"),
          const SizedBox(height: 6),
          Text(
            _game.turn == chess.Color.WHITE ? "White to move" : "Black to move",
            style: const TextStyle(color: Colors.white, fontSize: 18),
          ),
          const SizedBox(height: 10),
        ],
      ),
    );
  }

  Widget _buildCapturedRow(List<String> pieces, String player) {
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
      mainAxisAlignment: MainAxisAlignment.center,
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

  Widget _buildTimer(String player, String time) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Text(
        "$player: $time",
        style: const TextStyle(color: Colors.white, fontSize: 16),
      ),
    );
  }

  Widget _buildBoard() {
    return AspectRatio(
      aspectRatio: 1.0,
      child: GridView.builder(
        itemCount: 64,
        physics: const NeverScrollableScrollPhysics(),
        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
          crossAxisCount: 8,
        ),
        itemBuilder: (context, index) {
          final row = 7 - index ~/ 8;
          final col = index % 8;
          final square =
              String.fromCharCode('a'.codeUnitAt(0) + col) +
              (row + 1).toString();

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
        },
      ),
    );
  }
}
