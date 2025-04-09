import 'package:flutter/material.dart';
import 'package:chess/chess.dart' as chess;

class ChessBoard extends StatefulWidget {
  const ChessBoard({super.key});

  @override
  State<ChessBoard> createState() => _ChessBoardState();
}

class _ChessBoardState extends State<ChessBoard> {
  late chess.Chess _game;

  @override
  void initState() {
    super.initState();
    _game = chess.Chess();
  }

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      itemCount: 64,
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(crossAxisCount: 8),
      itemBuilder: (context, index) {
        int row = index ~/ 8;
        int col = index % 8;
        String square = String.fromCharCode('a'.codeUnitAt(0) + col) + (8 - row).toString();
        var piece = _game.get(square);
        return GestureDetector(
          onTap: () {
            // TODO: handle piece pick/place
          },
          child: Container(
            decoration: BoxDecoration(
              color: (row + col).isEven ? Colors.brown[300] : Colors.white,
            ),
            child: Center(child: Text(piece?.type.toUpperCase() ?? '')),
          ),
        );
      },
    );
  }
}
