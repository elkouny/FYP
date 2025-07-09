import 'dart:convert';
import 'dart:async';
import 'package:dio/dio.dart';
import 'package:http/http.dart' as http;

class LichessService {
  /// Sends a challenge to the Lichess AI
  Future<String> challengeBot({
    required String token,
    required int level,
    required String color,
    required Duration baseTime,
    required Duration increment,
  }) async {
    print(
      'Challenging bot with level: $level, color: $color, baseTime: $baseTime, increment: $increment',
    );
    final response = await http.post(
      Uri.parse('https://lichess.org/api/challenge/ai'),
      headers: {'Authorization': 'Bearer $token'},
      body: {
        'level': level.toString(),
        'color': color,
        'clock.limit': baseTime.inSeconds.toString(),
        'clock.increment': increment.inSeconds.toString(),
      },
    );
    print('Challenge response: ${response.statusCode} - ${response.body}');

    if (response.statusCode == 201) {
      final data = jsonDecode(response.body);
      return data['id']; // You'll listen for gameStart separately
    } else {
      throw Exception('Failed to challenge bot: ${response.body}');
    }
  }

  Future<String> getUsername(String token) async {
    final response = await http.get(
      Uri.parse('https://lichess.org/api/account'),
      headers: {'Authorization': 'Bearer $token'},
    );
    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      return data['username'] as String;
    } else {
      throw Exception('Failed to fetch username: ${response.body}');
    }
  }

  Future<int> getRatingForCategory(String username, String category) async {
    final response = await http.get(
      Uri.parse('https://lichess.org/api/user/$username'),
    );

    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      return data['perfs']?[category]?['rating'];
    }

    return 0;
  }

  Future<bool> resignGame(String gameId, String token) async {
    final response = await http.post(
      Uri.parse('https://lichess.org/api/board/game/$gameId/resign'),
      headers: {'Authorization': 'Bearer $token'},
    );
    if (response.statusCode == 200) {
      return true; // Resignation successful
    } else {
      throw false;
    }
  }

  Future<bool> uploadGame(String pgn, String token) async {
    final response = await http.post(
      Uri.parse('https://lichess.org/api/import'),
      headers: {
        'Authorization': 'Bearer $token',
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: {'pgn': pgn},
    );

    if (response.statusCode == 200) {
      print("✅ Game uploaded successfully.");
      return true;
    } else {
      print("❌ Failed to upload game. Status: ${response.statusCode}");
      print("Response: ${response.body}");
      return false;
    }
  }

  /// Streams incoming events from the user's Lichess account (gameStart, challenge, etc.)
  Stream<Map<String, dynamic>> streamLichessEvents(String token) async* {
    final request = http.Request(
      'GET',
      Uri.parse('https://lichess.org/api/stream/event'),
    );
    request.headers['Authorization'] = 'Bearer $token';
    final response = await request.send();

    final lines = response.stream
        .transform(utf8.decoder)
        .transform(const LineSplitter());

    await for (final line in lines) {
      if (line.trim().isEmpty) continue;
      final json = jsonDecode(line) as Map<String, dynamic>;
      yield json;
    }
  }
}
