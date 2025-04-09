import 'dart:convert';
import 'package:http/http.dart' as http;

class LichessService {
  final String token;

  LichessService(this.token);

  Future<void> createGame(String timeControl) async {
    final response = await http.post(
      Uri.parse('https://lichess.org/api/challenge/ai'),
      headers: {
        'Authorization': 'Bearer $token',
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: {'clock.limit': '300', 'level': '5'},
    );

    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      print('Game started: ${data['url']}');
    } else {
      print('Failed to create game: ${response.body}');
    }
  }
}
