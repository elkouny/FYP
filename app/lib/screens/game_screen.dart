import 'package:flutter/material.dart';
import '../widgets/chess_board.dart';
import '../widgets/timer_widget.dart';

class GameScreen extends StatelessWidget {
  const GameScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Chess Game')),
      body: const Column(
        children: [
          TimerWidget(),
          Expanded(child: ChessBoard()),
        ],
      ),
    );
  }
}
