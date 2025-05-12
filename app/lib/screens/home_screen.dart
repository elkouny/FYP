import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_appauth/flutter_appauth.dart';
import '../services/ble_manager.dart';
import '../widgets/chess_board.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);
  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final BLEManager _bleManager = BLEManager();
  final FlutterAppAuth _appAuth = const FlutterAppAuth();

  bool _isLoggedIn = false;
  String _statusMessage = "🔍 Scanning for SmartChessBoard...";
  bool _showStartButton = false;
  static const _lichessClientId = 'YOUR_CLIENT_ID';
  static const _redirectUrl = 'com.yourapp://oauthredirect';
  static const _authorizationEndpoint = 'https://lichess.org/oauth';
  static const _tokenEndpoint = 'https://lichess.org/api/token';
  static const _scopes = <String>['board:play'];

  @override
  void initState() {
    super.initState();
    _checkLogin();
    _setupBle();
  }

  Future<void> _checkLogin() async {
    final prefs = await SharedPreferences.getInstance();
    final token = prefs.getString('lichess_token');
    setState(() => _isLoggedIn = token != null);
  }

  Future<void> _doLogin() async {
    try {
      final result = await _appAuth.authorizeAndExchangeCode(
        AuthorizationTokenRequest(
          _lichessClientId,
          _redirectUrl,
          serviceConfiguration: AuthorizationServiceConfiguration(
            authorizationEndpoint: _authorizationEndpoint,
            tokenEndpoint: _tokenEndpoint,
          ),
          scopes: _scopes,
        ),
      );
      if (result?.accessToken != null) {
        final prefs = await SharedPreferences.getInstance();
        await prefs.setString('lichess_token', result!.accessToken!);
        setState(() => _isLoggedIn = true);
      }
    } catch (e) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Login failed: $e')));
    }
  }

  void _setupBle() {
    _bleManager.startScanAndConnect();
    _bleManager.statusStream.listen((msg) {
      final d = msg.trim().toLowerCase();
      String parsed;
      switch (d) {
        case 'waiting':
          parsed = '🧩 Waiting for pieces...';
          break;
        case 'connected':
          parsed = '📶 Connected. Waiting for board setup...';
          break;
        case 'ready_to_start':
          parsed = '✅ All pieces placed. Tap to start!';
          setState(() => _showStartButton = true);
          break;
        default:
          parsed = 'ℹ️ Status: $d';
      }
      if (mounted) {
        setState(() => _statusMessage = parsed);
      }
    });
  }

  Future<void> _startGame() async {
    if (_bleManager.gameStatusChar == null) return;
    setState(() {
      _statusMessage = '⏳ Starting game...';
      _showStartButton = false;
    });
    await _bleManager.writeCharacteristic('start_confirmed');
    setState(() => _statusMessage = '🚀 Game started!');
    if (mounted) {
      Navigator.push(
        context,
        MaterialPageRoute(
          builder:
              (_) => ChessBoard(
                key: UniqueKey(),
                bleManager: _bleManager,
                whitePlayerName: 'Ismail',
                blackPlayerName: 'Kouny',
                whitePlayerRating: 500,
                blackPlayerRating: 1200,
              ),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    // If not logged in yet, show only the login button
    if (!_isLoggedIn) {
      return Scaffold(
        appBar: AppBar(title: const Text('Welcome')),
        body: Center(
          child: ElevatedButton.icon(
            icon: const Icon(Icons.login),
            label: const Text('Login with Lichess'),
            onPressed: _doLogin,
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
            ),
          ),
        ),
      );
    }

    // Otherwise show BLE + board status + Start button
    final showRetry =
        _statusMessage.contains('Disconnected') ||
        _statusMessage.contains('failed') ||
        _statusMessage.contains('not found');

    return Scaffold(
      appBar: AppBar(title: const Text('Smart Chess Board')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              _statusMessage,
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 20),
            ),
            const SizedBox(height: 20),
            if (_showStartButton)
              ElevatedButton.icon(
                onPressed: _startGame,
                icon: const Icon(Icons.play_arrow),
                label: const Text('Start Game'),
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
                onPressed: _bleManager.startScanAndConnect,
                icon: const Icon(Icons.refresh),
                label: const Text('Retry Connection'),
              ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    super.dispose();
  }
}
