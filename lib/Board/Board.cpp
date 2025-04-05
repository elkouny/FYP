//
// Created by Ahmed Elkouny on 28/01/2025.
//

#include "Board.h"
#include <Constants.h>
#include <Piece.h>
#include <XYPos.h>
#include <algorithm>
#include <optional>
#include <set>
#include <unordered_map>

using PieceFactory = std::function<std::shared_ptr<Piece>(Color, Index)>;
std::vector<PieceFactory> piecesInOrder = {
    [](Color c, Index i) { return std::make_shared<Castle>(c, i); },
    [](Color c, Index i) { return std::make_shared<Knight>(c, i); },
    [](Color c, Index i) { return std::make_shared<Bishop>(c, i); },
    [](Color c, Index i) { return std::make_shared<Queen>(c, i); },
    [](Color c, Index i) { return std::make_shared<King>(c, i); },
    [](Color c, Index i) { return std::make_shared<Bishop>(c, i); },
    [](Color c, Index i) { return std::make_shared<Knight>(c, i); },
    [](Color c, Index i) { return std::make_shared<Castle>(c, i); }};

Board::Board() {
    const std::vector<int> ranks = {1, 2, 7, 8};
    for (int x = MIN_FILE; x <= MAX_FILE; ++x) {
        for (int y : ranks) {
            auto xyPos = XYPos(x, y);
            Index file = static_cast<Index>(x);
            std::shared_ptr<Piece> piece;
            if (y == 1)
                piece = piecesInOrder[x - 1](Color::White, file);
            else if (y == 2)
                piece = std::make_shared<Pawn>(Color::White, file);
            else if (y == 7)
                piece = std::make_shared<Pawn>(Color::Black, file);
            else if (y == 8)
                piece = piecesInOrder[x - 1](Color::Black, file);
            if (piece) addToBoard(piece, xyPos);
            if (piece && piece->name == "King") {
                if (piece->color == Color::White)
                    whiteKing = std::dynamic_pointer_cast<King>(piece);
                else
                    blackKing = std::dynamic_pointer_cast<King>(piece);
            }
        }
    }
}

void Board::addToBoard(std::shared_ptr<Piece> p, XYPos &xyPos) {
    pieceToCoordinate[p] = xyPos;
    coordinateToPiece[xyPos] = p;
}

void Board::updatePiece(std::shared_ptr<Piece> piece, XYPos &newPosition) {
    XYPos originalPosition = pieceToCoordinate[piece];
    if (coordinateToPiece.count(newPosition)) {
        auto captured = coordinateToPiece[newPosition];
        pieceToCoordinate.erase(captured);
    }
    coordinateToPiece.erase(originalPosition);
    addToBoard(piece, newPosition);
}

XYPos Board::getKingPosition(Color color) {
    return (color == Color::Black) ? pieceToCoordinate[blackKing] : pieceToCoordinate[whiteKing];
}

bool Board::isValidPosition(XYPos &xyPos) {
    return (std::min(int(xyPos.x), xyPos.y) >= MIN_FILE && std::max(int(xyPos.x), xyPos.y) <= MAX_FILE);
}

std::optional<std::shared_ptr<Piece>> Board::getPiece(const XYPos &xyPos) const {
    if (coordinateToPiece.count(xyPos)) return coordinateToPiece.at(xyPos);
    return std::nullopt;
}

std::set<XYPos> Board::slidingMoves(XYPos &currentPosition, const XYPos &moveVector) {
    auto piece = coordinateToPiece[currentPosition];
    std::set<XYPos> moves;
    for (int k = MIN_RANK; k < MAX_RANK; ++k) {
        XYPos potential = currentPosition + (k * moveVector);
        if (!isValidPosition(potential)) return moves;
        if (coordinateToPiece.count(potential) == 0)
            moves.insert(potential);
        else if (coordinateToPiece[potential]->color != piece->color) {
            moves.insert(potential);
            return moves;
        } else
            return moves;
    }
    return moves;
}

std::set<XYPos> Board::pseudoMoves(std::shared_ptr<Piece> piece) {
    std::set<XYPos> moves;
    XYPos current = pieceToCoordinate[piece];

    if (piece->slidingPiece()) {
        for (auto move : piece->movements()) {
            XYPos mv(move);
            auto set1 = slidingMoves(current, mv);
            auto set2 = slidingMoves(current, mv * -1);
            moves.insert(set1.begin(), set1.end());
            moves.insert(set2.begin(), set2.end());
        }
    } else {
        for (auto move : piece->movements()) {
            XYPos mv(move);
            XYPos potential = current + mv;
            if (!isValidPosition(potential)) continue; // out of bounds
            auto atPot = getPiece(potential);

            if (piece->name == "Pawn") {
                std::shared_ptr<Pawn> pawn = std::dynamic_pointer_cast<Pawn>(piece);
                bool isWhite = piece->color == Color::White;
                if ((isWhite && (move == std::array<int, 2>{1, 1} || move == std::array<int, 2>{-1, 1})) ||
                    (!isWhite && (move == std::array<int, 2>{1, -1} || move == std::array<int, 2>{-1, -1}))) {
                    XYPos enPassant = isWhite ? XYPos(potential.x, potential.y - 1) : XYPos(potential.x, potential.y + 1);
                    auto atEnPassant = getPiece(enPassant);
                    if ((atPot && atPot.value()->color != piece->color) ||
                        (!atPot && atEnPassant && atEnPassant.value()->name == "Pawn" &&
                         std::dynamic_pointer_cast<Pawn>(atEnPassant.value())->movedTwice &&
                         atEnPassant.value()->color != piece->color)) {
                        moves.insert(potential);
                    }
                    continue;
                }
            }
            if (piece->name == "King" && (move == std::array<int, 2>{-2, 0} || move == std::array<int, 2>{2, 0})) {
                int rank = piece->color == Color::White ? MIN_RANK : MAX_RANK;
                if (move[0] == -2) {
                    if (!getPiece(XYPos(Index::d, rank)) && !getPiece(XYPos(Index::c, rank)) &&
                        !getPiece(XYPos(Index::b, rank)) && getPiece(XYPos(Index::a, rank))) {
                        auto rook = getPiece(XYPos(Index::a, rank)).value();
                        if (rook->name == "Castle" && !rook->hasMoved()) moves.insert(potential);
                    }
                } else {
                    if (!getPiece(XYPos(Index::f, rank)) && !getPiece(XYPos(Index::g, rank)) &&
                        getPiece(XYPos(Index::h, rank))) {
                        auto rook = getPiece(XYPos(Index::h, rank)).value();
                        if (rook->name == "Castle" && !rook->hasMoved()) moves.insert(potential);
                    }
                }
                continue;
            }
            if (!atPot || atPot.value()->color != piece->color) moves.insert(potential);
        }
    }
    return moves;
}

bool Board::isCheck(Color color) {
    XYPos kingPos = getKingPosition(color);
    for (auto &[p, _] : pieceToCoordinate) {
        if (p->color != color && pseudoMoves(p).count(kingPos)) {
            return true;
        }
    }
    return false;
}

bool Board::isKingExposed(std::shared_ptr<Piece> piece, XYPos potential) {
    auto original = pieceToCoordinate[piece];
    auto prevPiece = getPiece(potential);
    updatePiece(piece, potential);
    bool exposed = isCheck(piece->color);
    updatePiece(piece, original);
    if (prevPiece) addToBoard(prevPiece.value(), potential);
    return exposed;
}

std::set<XYPos> Board::getValidMoves(std::shared_ptr<Piece> piece) {
    std::set<XYPos> result;
    for (auto move : pseudoMoves(piece)) {
        if (!isKingExposed(piece, move)) result.insert(move);
    }
    return result;
}

void Board::movePiece(std::shared_ptr<Piece> piece, XYPos &dest) {
    if (!getValidMoves(piece).count(dest)) return;
    XYPos origin = pieceToCoordinate[piece];
    XYPos delta = dest - origin;
    piece->moved = true;
    if (piece->name == "Pawn" && std::abs(delta.y) == 2) {
        std::dynamic_pointer_cast<Pawn>(piece)->movedTwice = true;
    }
    updatePiece(piece, dest);
}
