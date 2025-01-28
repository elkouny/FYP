//
// Created by Ahmed Elkouny on 28/01/2025.
//

#include <unordered_map>
#include "Piece.h"
#include "XYpos.h"

class Board {
private:
    class Piece;

    class Castle;

    class Knight;

    class Bishop;

    class Queen;

    class King;

    class Pawn;

    using PieceFactory = std::function<std::unique_ptr<Piece>(Color, Index)>;

public:
    std::unordered_map<Piece, XYPos> pieceToCoordinate;
    std::unordered_map<XYPos, Piece> coordinateToCoordinate;
    std::vector<PieceFactory> piecesInOrder;

    Board() {

    }

};