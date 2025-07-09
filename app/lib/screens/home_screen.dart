// Flutter/Dart Imports
import 'package:flutter/material.dart';

// External Package Imports
import 'package:flutter_appauth/flutter_appauth.dart';
import 'package:shared_preferences/shared_preferences.dart';

// Project-specific Imports
import 'package:smart_chess_app/services/lichess_service.dart';
import 'package:smart_chess_app/widgets/BotOptionsWidget.dart';
import '../services/ble_manager.dart';
import '../widgets/chess_board.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({Key? key}) : super(key: key);
  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  //============================================================================
  // Properties & Constants
  //============================================================================

  final BLEManager _bleManager = BLEManager();
  final FlutterAppAuth _appAuth = const FlutterAppAuth();

  // State variables
  bool _isLoggedIn = false;
  String _statusMessage = "🔍 Scanning for SmartChessBoard...";
  bool _readyToStart = false;
  Duration _baseTime = const Duration(minutes: 10);
  Duration _increment = Duration.zero;

  // Lichess OAuth constants
  static const _lichessClientId = 'YOUR_CLIENT_ID';
  static const _redirectUrl = 'com.yourapp://oauthredirect';
  static const _authorizationEndpoint = 'https://lichess.org/oauth';
  static const _tokenEndpoint = 'https://lichess.org/api/token';
  static const _scopes = <String>['board:play'];

  //============================================================================
  // Lifecycle Methods
  //============================================================================

  @override
  void initState() {
    super.initState();
    _checkLogin();
    _setupBle();
  }

  @override
  void dispose() {
    super.dispose();
  }

  //============================================================================
  // Build Method
  //============================================================================

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

    return Scaffold(
      backgroundColor: const Color(0xFF1E1E1E),
      body: SafeArea(
        child: Column(
          children: [
            const SizedBox(height: 20),
            const Text(
              "Play",
              style: TextStyle(
                fontSize: 32,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
            const SizedBox(height: 40),
            ElevatedButton.icon(
              onPressed: () => _showTimeSelector(context),
              icon: const Icon(Icons.timer),
              label: Text(
                _increment.inSeconds > 0
                    ? '${_baseTime.inMinutes} | ${_increment.inSeconds}'
                    : '${_baseTime.inMinutes} min',
              ),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.grey[800],
                padding: const EdgeInsets.symmetric(
                  horizontal: 24,
                  vertical: 12,
                ),
                foregroundColor: Colors.white,
              ),
            ),
            const SizedBox(height: 10),
            _buildMenuButton("Start an online game", Icons.play_arrow, () {
              // TODO: add navigation logic
            }),
            _buildMenuButton("Play a Friend online", Icons.group, () {
              // TODO: add navigation logic
            }),
            _buildMenuButton("Play a Bot", Icons.smart_toy, () {
              _showBotOptions(context);
            }),
            _buildMenuButton(
              "Play Locally on the Board",
              Icons.sports_esports,
              _startLocalGame,
            ),
            const Spacer(),
            Padding(
              padding: const EdgeInsets.only(bottom: 20.0),
              child: Text(
                _statusMessage,
                style: const TextStyle(color: Colors.grey, fontSize: 18),
                textAlign: TextAlign.center,
              ),
            ),
          ],
        ),
      ),
    );
  }

  //============================================================================
  // Widget Builders & UI Handlers
  //============================================================================

  Widget _buildMenuButton(String label, IconData icon, VoidCallback onPressed) {
    final isEnabled = _readyToStart;
    final color = isEnabled ? Colors.green : Colors.grey[850];

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0, horizontal: 16),
      child: InkWell(
        onTap: isEnabled ? onPressed : null, // Disable tap if not connected
        child: Container(
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Row(
            children: [
              Icon(icon, color: Colors.white),
              const SizedBox(width: 16),
              Text(
                label,
                style: const TextStyle(color: Colors.white, fontSize: 18),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildTimeRow(String label, List<List<int>> times) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.only(left: 8, top: 12, bottom: 8),
          child: Text(
            label,
            style: const TextStyle(
              color: Colors.white70,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
        Wrap(
          spacing: 8,
          children:
              times.map((t) {
                final isSelected =
                    _baseTime.inMinutes == t[0] && _increment.inSeconds == t[1];
                final display = t[1] > 0 ? "${t[0]} | ${t[1]}" : "${t[0]} min";
                return GestureDetector(
                  onTap: () {
                    setState(() {
                      _baseTime = Duration(minutes: t[0]);
                      _increment = Duration(seconds: t[1]);
                    });
                    Navigator.pop(context);
                  },
                  child: Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 10,
                    ),
                    decoration: BoxDecoration(
                      color: isSelected ? Colors.green : Colors.grey[800],
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Text(
                      display,
                      style: TextStyle(
                        color: isSelected ? Colors.black : Colors.white,
                      ),
                    ),
                  ),
                );
              }).toList(),
        ),
      ],
    );
  }

  void _showTimeSelector(BuildContext context) {
    showModalBottomSheet(
      context: context,
      backgroundColor: const Color(0xFF2C2C2C),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(16)),
      ),
      builder:
          (_) => Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Text(
                  "Choose Time",
                  style: TextStyle(color: Colors.white, fontSize: 18),
                ),
                const SizedBox(height: 16),
                _buildTimeRow("Blitz", [
                  [3, 0],
                  [3, 2],
                  [5, 0],
                ]),
                _buildTimeRow("Rapid", [
                  [10, 0],
                  [10, 5],
                  [15, 10],
                ]),
                _buildTimeRow("Classical", [
                  [30, 0],
                  [30, 20],
                ]),
              ],
            ),
          ),
    );
  }

  Future<void> _showBotOptions(BuildContext context) async {
    final settings = await showModalBottomSheet<BotSettings>(
      context: context,
      isScrollControlled: true,
      backgroundColor: const Color(0xFF2C2C2C),
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(16)),
      ),
      builder: (_) => const BotOptionsSheet(),
    );
    print('Bot settings: $settings');
    if (settings == null) return;
    _startBotGame(settings);
  }

  //============================================================================
  // Game Start Logic
  //============================================================================

  Future<void> _startLocalGame() async {
    final prefs = await SharedPreferences.getInstance();
    final token = prefs.getString('lichess_token');

    if (_bleManager.gameStatusChar == null) return;
    setState(() {
      _statusMessage = '⏳ Starting game...';
      _readyToStart = false;
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
                baseTime: _baseTime,
                increment: _increment,
                whitePlayerName: 'Ismail',
                blackPlayerName: 'Kouny',
                whitePlayerRating: 500,
                blackPlayerRating: 1200,
                lichessToken: token,
              ),
        ),
      );
    }
  }

  Future<void> _startBotGame(BotSettings settings) async {
    final prefs = await SharedPreferences.getInstance();
    final token = prefs.getString('lichess_token');
    print('Lichess token: $token');
    if (token == null) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text("Login required")));
      return;
    }
    print(
      'Starting bot game with settings: ${settings.color}, ${settings.difficulty}',
    );
    final lichess = LichessService();
    final challengeId = await lichess.challengeBot(
      token: token,
      level: settings.difficulty,
      color: settings.color,
      baseTime: _baseTime,
      increment: _increment,
    );
    print('Challenge ID: $challengeId');

    // Wait for gameStart event
    final stream = lichess.streamLichessEvents(token);

    final userName = await lichess.getUsername(token);
    print(userName);
    print('Listening for gameStart event...');

    await for (final event in stream) {
      print('Received event: $event');
      if (event['type'] == 'gameStart') {
        if (_bleManager.gameStatusChar == null) return;
        setState(() {
          _statusMessage = '⏳ Starting game...';
          _readyToStart = false;
        });
        await _bleManager.writeCharacteristic('start_confirmed');
        setState(() => _statusMessage = '🚀 Game started!');
        String opponentUsername = event['game']['opponent']['username'];
        int opponentRating =
            settings.difficulty * 150; // Placeholder for opponent rating
        String userColor = event['game']['color'];
        print('getting user rating...');
        final category = determineCategory(_baseTime, _increment);
        final userRating = await lichess.getRatingForCategory(
          userName,
          category,
        );
        print('User rating: $userRating');
        final gameId = event['game']['gameId'];
        if (mounted) {
          Navigator.push(
            context,
            MaterialPageRoute(
              builder:
                  (_) => ChessBoard(
                    key: UniqueKey(),
                    bleManager: _bleManager,
                    lichessGameId: gameId,
                    userColor: userColor,
                    lichessToken: token,
                    baseTime: _baseTime,
                    increment: _increment,
                    whitePlayerName:
                        userColor == 'black' ? opponentUsername : userName,
                    blackPlayerName:
                        userColor == 'black' ? userName : opponentUsername,
                    whitePlayerRating:
                        userColor == 'black' ? opponentRating : userRating,
                    blackPlayerRating:
                        userColor == 'black' ? userRating : opponentRating,
                  ),
            ),
          );
        }
        break;
      }
    }
  }

  //============================================================================
  // Service & Initialization Logic
  //============================================================================

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
          parsed = '✅ All pieces placed';
          setState(() => _readyToStart = true);
          break;
        default:
          parsed = 'ℹ️ Status: $d';
      }
      if (mounted) {
        setState(() => _statusMessage = parsed);
      }
    });
  }

  //============================================================================
  // Utility Functions
  //============================================================================

  String determineCategory(Duration base, Duration increment) {
    final adjustedSeconds = base.inSeconds + 40 * increment.inSeconds;

    if (adjustedSeconds <= 15) return 'ultraBullet';
    if (adjustedSeconds < 180) return 'bullet';
    if (adjustedSeconds < 480) return 'blitz';
    if (adjustedSeconds < 1500) return 'rapid';
    return 'classical';
  }
}
