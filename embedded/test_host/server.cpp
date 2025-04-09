#include "Board.h"
#include "httplib.h"
#include "json.hpp"
using json = nlohmann::json;

int main() {
    httplib::Server svr;
    Board board;
    std::unordered_map<std::string, char> pieceMap = {
        {"Pawn", 'P'},
        {"Knight", 'N'},
        {"Bishop", 'B'},
        {"Castle", 'R'},
        {"Queen", 'Q'},
        {"King", 'K'}};

    // Allow CORS for all requests
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                             {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                             {"Access-Control-Allow-Headers", "Content-Type"}});

    svr.Get("/valid_moves", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("x") || !req.has_param("y")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing x or y\"}", "application/json");
            return;
        }
        int x = std::stoi(req.get_param_value("x"));
        int y = std::stoi(req.get_param_value("y"));
        auto piece = board.getPiece(XYPos(x, y));
        json response = json::array();
        if (piece.has_value()) {
            auto moves = board.getValidMoves(piece.value());
            for (const auto &move : moves) {
                response.push_back({{"x", static_cast<int>(move.x)}, {"y", move.y}});
            }
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/board_state", [&](const httplib::Request &, httplib::Response &res) {
        json boardArr = json::array();
        for (int y = 8; y >= 1; --y) {
            json row = json::array();
            for (int x = 1; x <= 8; ++x) {
                auto pieceOpt = board.getPiece(XYPos(x, y));
                if (pieceOpt.has_value()) {
                    const auto &p = pieceOpt.value();
                    char c = pieceMap[p->name];
                    row.push_back(std::string(1, p->color == Color::White ? std::toupper(c) : std::tolower(c)));
                } else {
                    row.push_back("");
                }
            }
            boardArr.push_back(row);
        }
        res.set_content(boardArr.dump(), "application/json");
    });

    svr.Get("/move_piece", [&](const httplib::Request &req, httplib::Response &res) {
        json boardArr = json::array();
        int fromX = std::stoi(req.get_param_value("fromX"));
        int fromY = std::stoi(req.get_param_value("fromY"));
        const int toX = std::stoi(req.get_param_value("toX"));
        const int toY = std::stoi(req.get_param_value("toY"));
        XYPos from(fromX, fromY);
        XYPos to(toX, toY);
        auto pieceOpt = board.getPiece(from);
        if (!pieceOpt.has_value()) {
            res.status = 404;
            res.set_content("{\"error\": \"No piece at source position\"}", "application/json");
            return;
        }
        auto piece = pieceOpt.value();
        board.movePiece(piece, to);
        res.set_content("{\"success\": true}", "application/json");
    });

    std::cout << "Server running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}
