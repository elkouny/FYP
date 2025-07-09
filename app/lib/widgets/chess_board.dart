// Dart imports
import 'dart:async';
import 'dart:convert';
import 'dart:math';

// Package imports
import 'package:chess/chess.dart' as chess;
import 'package:collection/collection.dart';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:smart_chess_app/services/lichess_service.dart';

// Project imports
import '../services/ble_manager.dart';

class ChessBoard extends StatefulWidget {
  final BLEManager bleManager;
  final String whitePlayerName;
  final String blackPlayerName;
  final int whitePlayerRating;
  final int blackPlayerRating;
  final Duration baseTime;
  final Duration increment;
  final String? userColor;
  final String? lichessGameId;
  final String? lichessToken;

  const ChessBoard({
    required this.bleManager,
    required this.whitePlayerName,
    required this.blackPlayerName,
    required this.whitePlayerRating,
    required this.blackPlayerRating,
    required this.baseTime,
    required this.increment,
    super.key,
    this.lichessGameId,
    this.lichessToken,
    this.userColor,
  });

  @override
  State<ChessBoard> createState() => _ChessBoardState();
}

class _ChessBoardState extends State<ChessBoard> {
  //============================================================================
  // Properties
  //============================================================================

  late chess.Chess _game;
  String? selectedSquare;
  List<String> validMoves = [];
  List<String> capturedWhite = [];
  List<String> capturedBlack = [];
  String latestBleMessage = "";
  bool waitingForLichessMove = false;
  String expectedLichessMove = "";
  int sameCommandCount = 0;
  LichessService lichessService = LichessService();
  late Duration whiteTime = widget.baseTime;
  late Duration blackTime = widget.baseTime;
  late Duration increment = widget.increment;
  bool isWhiteTurn = true;
  chess.Color _lastTurn = chess.Color.WHITE;
  Timer? _timer;
  bool pauseBle = false;
  String lastCommand = "";
  late StreamSubscription<String> _statusSub;
  bool switchTimer = false;

  //============================================================================
  // Lifecycle Methods
  //============================================================================

  @override
  void initState() {
    super.initState();
    _game = chess.Chess();
    _startTimer();
    _monitorTurn();
    _statusSub = widget.bleManager.statusStream.listen((msg) {
      if (_statusSub.isPaused) return;
      if (pauseBle) return;
      if (waitingForLichessMove) {
        if (msg == "move:${expectedLichessMove.substring(0, 4)}" ||
            msg == "capture:${expectedLichessMove.substring(0, 4)}") {
          waitingForLichessMove = false;
          expectedLichessMove = "";
          _makeMove(
            expectedLichessMove.substring(0, 2),
            expectedLichessMove.substring(2, 4),
          );
        } else if (msg.startsWith("move:") || msg.startsWith("capture:")) {
          _showAlert("❌ Waiting for Lichess move, but got: $msg");
          _highlightMoves(
            expectedLichessMove.substring(0, 2),
            stopAt: expectedLichessMove.substring(2, 4),
          );
        }
      } else if (msg.startsWith("hover:")) {
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
    print("user color: ${widget.userColor}");
    if (widget.userColor == "black") {
      print("user black, waiting for lichess move");
      _onlineStart();
    }
  }

  @override
  void dispose() {
    _statusSub.cancel();
    super.dispose();
  }

  //============================================================================
  // Build Method
  //============================================================================

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
          _buildInfoRow(
            "Black",
            capturedWhite,
            !whiteToMove,
            blackTime,
          ), // Combined row
          const SizedBox(height: 4),
          _buildBoard(),
          _buildInfoRow(
            "White",
            capturedBlack,
            whiteToMove,
            whiteTime,
          ), // Combined row
          const SizedBox(height: 6),
          _buildPlayerDisplay(
            widget.whitePlayerName,
            widget.whitePlayerRating.toString(),
          ),
          const SizedBox(height: 10),
          // Quit button
          ElevatedButton.icon(
            onPressed: () async {
              if (widget.lichessGameId == null) {
                print("quitting local game");
                _navigateToHome();
                return;
              }
              bool quit;
              quit = await lichessService.resignGame(
                widget.lichessGameId!,
                widget.lichessToken!,
              );
              if (quit) {
                _navigateToHome();
              } else {
                _showAlert("❌ Failed to resign game");
              }
            },
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

  //============================================================================
  // Widget Builders
  //============================================================================

  Widget _buildBoard() {
    return Column(
      children: [
        // 1: the board + ranks
        AspectRatio(
          aspectRatio: 1.0,
          child: Row(
            children: [
              // RANK LABELS: each Expanded takes 1/8 of the height
              // Replace the RANK LABELS part with this:
              Column(
                children: List.generate(8, (i) {
                  final rank =
                      isBoardFlipped ? (i + 1).toString() : (8 - i).toString();
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
            // Replace the FILE LABELS part with this:
            Expanded(
              child: Row(
                children: List.generate(8, (i) {
                  final fileIndex = isBoardFlipped ? 7 - i : i;
                  final file = String.fromCharCode(
                    'a'.codeUnitAt(0) + fileIndex,
                  );
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
    final row = isBoardFlipped ? index ~/ 8 : 7 - index ~/ 8;
    final col = isBoardFlipped ? 7 - (index % 8) : index % 8;

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

  Widget _buildPiece(chess.Piece piece) {
    final color = piece.color == chess.Color.WHITE ? 'w' : 'b';
    final type = piece.type.toLowerCase();
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

  Widget _buildInfoRow(
    String player,
    List<String> pieces,
    bool isActive,
    Duration time,
  ) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween, // Space between
        children: [
          _buildCapturedPieces(pieces, player),
          _buildTimer(player, formatTime(time), isActive),
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

  Widget _promoTile(BuildContext ctx, String assetName, String promoPiece) {
    return GestureDetector(
      onTap: () => Navigator.of(ctx).pop(promoPiece),
      child: Image.asset('assets/pieces/$assetName.png', width: 48, height: 48),
    );
  }

  //============================================================================
  // Core Game Logic
  //============================================================================

  void _highlightMoves(String square, {String? stopAt}) {
    if (waitingForLichessMove) return;
    _clearHighlight();

    final piece = _game.get(square);
    if (piece != null && piece.color == _game.turn) {
      final allMoves =
          _game
              .generate_moves()
              .where((m) => m.fromAlgebraic == square)
              .map((m) => m.toAlgebraic)
              .toList();

      List<String> filteredMoves = [];

      if (stopAt != null) {
        // Handle knight separately: only highlight stopAt
        if (piece.type.toLowerCase() == 'n') {
          filteredMoves = [stopAt];
        } else {
          final dx = fileDiff(square, stopAt);
          final dy = rankDiff(square, stopAt);

          // Normalize direction
          final stepX = dx == 0 ? 0 : dx ~/ dx.abs();
          final stepY = dy == 0 ? 0 : dy ~/ dy.abs();

          String? current = square;

          while (true) {
            if (current == null || current == stopAt) break;
            current = nextSquare(current, stepX, stepY);
            if (allMoves.contains(current)) filteredMoves.add(current!);
          }
          if (allMoves.contains(stopAt)) filteredMoves.add(stopAt);
        }
      } else {
        filteredMoves = allMoves;
      }

      setState(() {
        selectedSquare = square;
        validMoves = filteredMoves;
      });

      final command = "light_on:${square + validMoves.join("")}";
      print("Highlighting moves for $square: $validMoves");

      widget.bleManager.writeCharacteristic(command);
    } else {
      _clearHighlight();
    }
  }

  void _clearHighlight() {
    if (waitingForLichessMove) return;
    setState(() {
      selectedSquare = null;
      validMoves = [];
    });
    String command = "light_off";
    if (command != widget.bleManager.lastMessage) {
      widget.bleManager.writeCharacteristic(command);
    }
  }

  Future<void> _captureMove(String from, String to) async {
    int userColorEnum = widget.userColor == "white" ? 0 : 1;
    const int bitsPromotion = 16;
    final moveObj = _game.generate_moves().firstWhereOrNull(
      (m) => m.fromAlgebraic == from && m.toAlgebraic == to,
    );
    String? promo;
    if (moveObj == null) {
      _showAlert("⛔ Invalid capture from $from to $to");
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

    final captured = _game.get(to);
    final success = _game.move({'from': from, 'to': to, 'promotion': promo});

    if (success && captured != null) {
      if (captured.color == chess.Color.WHITE) {
        capturedWhite.add(captured.type.name);
      } else {
        capturedBlack.add(captured.type.name);
      }
      widget.bleManager.writeCharacteristic("capture_ack:$from$to");
      _clearHighlight();
      if (widget.lichessGameId != null && _game.turn.index != userColorEnum) {
        final uci = from + to;
        await _sendMoveToLichess(uci);
        await _listenForLichessMove();
      }
      _checkGameStatus();
    } else {
      _showAlert("Invalid capture $from → $to");
    }
  }

  void _makeMove(String from, String to) async {
    int userColorEnum = widget.userColor == "white" ? 0 : 1;
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
      if (widget.lichessGameId != null && _game.turn.index != userColorEnum) {
        final uci = from + to + (promo ?? "");
        await _sendMoveToLichess(uci);
        await _listenForLichessMove();
      }
      _clearHighlight();
      _checkGameStatus();
    } else {
      _showAlert("❌ Invalid move from $from to $to");
    }
  }

  void _checkGameStatus() {
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

    if (whiteTime == Duration.zero || blackTime == Duration.zero) {
      final winner = whiteTime > blackTime ? 'White' : 'Black';
      _showEndingAlert("⏰ Time's up! $winner wins.");
    }
  }

  //============================================================================
  // Service Handlers (BLE, Lichess)
  //============================================================================

  Future<void> _handleDisconnect() async {
    // Show an alert when disconnected
    _showAlert("❌ Bluetooth Disconnected");
    bool resign;
    resign = await lichessService.resignGame(
      widget.lichessGameId!,
      widget.lichessToken!,
    );
    // Navigate back to the home screen
    if (resign) {
      _navigateToHome();
    } else {
      _showAlert("❌ Failed to resign game");
    }
  }

  void _navigateToHome() async {
    await widget.bleManager.writeCharacteristic("game_ended");
    if (widget.lichessGameId == null) {
      print("uploading local game to Lichess");
      // local game
      print("pgn: ${_game.pgn()}");
      bool upload = await lichessService.uploadGame(
        _game.pgn(),
        widget.lichessToken!,
      );
      if (!upload) {
        _showAlert("❌ Failed to upload game to Lichess");
      } else {
        print("Game uploaded successfully to Lichess");
      }
    }
    Navigator.pop(context); // Go back to the home screen
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

  Future<void> _listenForLichessMove() async {
    final gameId = widget.lichessGameId;
    final token = widget.lichessToken;
    final userColor = widget.userColor;
    String moveStr;
    List<String> moves = [];
    print("entered _listenForLichessMove");
    if (gameId == null || token == null || userColor == null) return;
    print("sending request to lichess for game $gameId with token $token");
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
      if (event['type'] == 'gameState' || event['type'] == 'gameFull') {
        if (event['type'] == 'gameState') {
          moveStr = event['moves'] as String;
          moves = moveStr.split(" ");
        } else {
          moveStr = event['state']['moves'] as String;
          moves = moveStr.split(" ");
        }

        print("Received moves: $moves");
        // Determine Lichess's latest move
        final myMoves = _game.history.map((m) => m.move.toAlgebraic).toList();
        print('My moves: $myMoves');
        if (moves.length > myMoves.length) {
          switchTimer = true;
          _startTimer();
          final latest = moves.last;
          final from = latest.substring(0, 2);
          final to = latest.substring(2, 4);
          //  promotion handling
          final promo = latest.length > 4 ? latest[4] : null;
          print("Lichess move: $from to $to with promo $promo");
          _highlightMoves(from, stopAt: to);
          setState(() {
            expectedLichessMove = from + to + (promo ?? "");
            waitingForLichessMove = true;
          });
          if (_isKnightMove(from, to)) {
            final jumpedOverSquares = _knightPath(from, to);
            widget.bleManager.writeCharacteristic("clear_piece:$jumpedOverSquares");
          }

          widget.bleManager.writeCharacteristic("move_cnc:$from$to");
          // Wait for the board to send the move via BLE
          _checkGameStatus();
          break;
        }
      }
    }
  }

  Future<void> _onlineStart() async {
    await _listenForLichessMove();
  }
  //============================================================================
  // Timer Management
  //============================================================================

  void _monitorTurn() {
    Timer.periodic(const Duration(milliseconds: 100), (timer) {
      if (_game.turn != _lastTurn) {
        switchTimer = false;
        _switchTurnWithIncrement(); // triggers logic when turn changes
        _lastTurn = _game.turn;
      }

      if (_game.in_check) {
        widget.bleManager.writeCharacteristic(
          "in_check:${chess.Chess.algebraic(_game.kings[_game.turn])}",
        );
        print("in_check:${chess.Chess.algebraic(_game.kings[_game.turn])}");
      }
    });
  }

  void _startTimer() {
    if (_game.history.length < 2) return;
    _timer?.cancel();
    _timer = Timer.periodic(const Duration(seconds: 1), (_) {
      setState(() {
        if (isWhiteTurn || switchTimer && widget.userColor == "white") {
          if (whiteTime > Duration.zero) {
            whiteTime -= const Duration(seconds: 1);
          }
        } else if (!isWhiteTurn || switchTimer && widget.userColor == "black") {
          if (blackTime > Duration.zero) {
            blackTime -= const Duration(seconds: 1);
          }
        }
      });
    });
  }

  void _switchTurnWithIncrement() {
    setState(() {
      // Apply increment to the player who just moved
      if (isWhiteTurn) {
        whiteTime += increment;
      } else {
        blackTime += increment;
      }

      // Flip turn
      isWhiteTurn = !isWhiteTurn;
    });

    _startTimer();
  }

  //============================================================================
  // UI Dialogs & Alerts
  //============================================================================

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

  //============================================================================
  // Utility Functions
  //============================================================================

  int fileDiff(String from, String to) => to.codeUnitAt(0) - from.codeUnitAt(0);

  int rankDiff(String from, String to) => int.parse(to[1]) - int.parse(from[1]);

  String? nextSquare(String square, int dx, int dy) {
    final file = square.codeUnitAt(0) + dx;
    final rank = int.parse(square[1]) + dy;

    if (file < 'a'.codeUnitAt(0) || file > 'h'.codeUnitAt(0)) return null;
    if (rank < 1 || rank > 8) return null;

    return "${String.fromCharCode(file)}$rank";
  }

  String formatTime(Duration d) {
    final m = d.inMinutes.remainder(60).toString().padLeft(2, '0');
    final s = d.inSeconds.remainder(60).toString().padLeft(2, '0');
    return "$m:$s";
  }

  bool get isBoardFlipped => widget.userColor == "black";
}

bool _isKnightMove(String from, String to) {
  final fx = from.codeUnitAt(0) - 'a'.codeUnitAt(0);
  final fy = int.parse(from[1]) - 1;
  final tx = to.codeUnitAt(0) - 'a'.codeUnitAt(0);
  final ty = int.parse(to[1]) - 1;
  final dx = (fx - tx).abs();
  final dy = (fy - ty).abs();
  return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

String _knightPath(String from, String to) {
  final fx = from.codeUnitAt(0);
  final fy = int.parse(from[1]);
  final tx = to.codeUnitAt(0);
  final ty = int.parse(to[1]);

  final dx = tx - fx;
  final dy = ty - fy;

  String path = "";

  if ((dx.abs() == 2 && dy.abs() == 1) || (dx.abs() == 1 && dy.abs() == 2)) {
    if (dx.abs() == 2) {
      // Move 1 square in X, then 1 square in Y
      final step1 = String.fromCharCode(fx + dx.sign) + fy.toString();
      final step2 = String.fromCharCode(fx + dx.sign) + (fy + dy).toString();
      path = step1 + step2;
    } else {
      // Move 1 square in Y, then 1 square in X
      final step1 = String.fromCharCode(fx) + (fy + dy.sign).toString();
      final step2 = String.fromCharCode(fx + dx) + (fy + dy.sign).toString();
      path = step1 + step2;
    }
  }

  return path;
}
