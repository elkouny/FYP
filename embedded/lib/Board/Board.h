#ifndef BOARD_H
#define BOARD_H

#include <Constants.h>
#include <Piece.h>
#include <XYPos.h>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <memory> 
#include <unordered_set>

class Board {
public:
    Board();
    std::shared_ptr<King> whiteKing;
    std::shared_ptr<King> blackKing;
    XYPos getKingPosition(Color color);
    bool isValidPosition(XYPos &xyPos);
    void addToBoard(std::shared_ptr<Piece> p, XYPos &xyPos);
    std::unordered_map<std::shared_ptr<Piece>, XYPos> pieceToCoordinate = {};
    std::unordered_map<XYPos, std::shared_ptr<Piece>> coordinateToPiece = {};

    std::unordered_set<XYPos> slidingMoves(XYPos &currentPosition, const XYPos &moveVector);

    std::unordered_set<XYPos> pseudoMoves(std::shared_ptr<Piece> piece);
    void updatePiece(std::shared_ptr<Piece> piece, XYPos &newPosition);
    bool isCheck(Color color);
    bool isKingExposed(std::shared_ptr<Piece> piece, XYPos &potential);

    std::unordered_set<XYPos> getValidMoves(std::shared_ptr<Piece> piece);
    void movePiece(std::shared_ptr<Piece> piece, XYPos &finalPosition);
    std::optional<std::shared_ptr<Piece>> getPiece(const XYPos &xyPos) const;
};

#endif