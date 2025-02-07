//
// Created by Ahmed Elkouny on 28/01/2025.
//

#include <unordered_map>
#include <set>
#include <Piece.h>
#include <XYPos.h>
#include <algorithm>
#include <Constants.h>
#include <optional>

using PieceFactory = std::function<Piece(Color, Index)>;
std::vector<PieceFactory> piecesInOrder = {
    [](Color c, Index i)
    { return Castle(c, i); },
    [](Color c, Index i)
    { return Knight(c, i); },
    [](Color c, Index i)
    { return Bishop(c, i); },
    [](Color c, Index i)
    { return Queen(c, i); },
    [](Color c, Index i)
    { return King(c, i); },
    [](Color c, Index i)
    { return Bishop(c, i); },
    [](Color c, Index i)
    { return Knight(c, i); },
    [](Color c, Index i)
    { return Castle(c, i); }};

class Board
{
private:
    std::unordered_map<Piece, XYPos> pieceToCoordinate = {};
    std::unordered_map<XYPos, Piece> coordinateToPiece = {};
    King whiteKing = King(Color::White, Index::e);
    King blackKing = King(Color::Black, Index::e);

    void updatePiece(Piece &piece, XYPos &newPosition)
    {
        XYPos originalPosition = this->pieceToCoordinate[piece];
        if (this->coordinateToPiece.count(newPosition))
        {
            // capturing piece
            Piece pieceAtNewPosition = this->coordinateToPiece[newPosition];
            pieceToCoordinate.erase(pieceAtNewPosition);
        }
        coordinateToPiece.erase(originalPosition);
        addToBoard(piece, newPosition);
    }

    void addToBoard(Piece &p, XYPos &xyPos)
    {
        this->pieceToCoordinate[p] = xyPos;
        this->coordinateToPiece[xyPos] = p;
    }

    std::optional<Piece> getPiece(const XYPos &xyPos) const
    {
        if (this->coordinateToPiece.count(xyPos))
        {
            return this->coordinateToPiece.at(xyPos);
        }
        else
        {
            return std::nullopt; // No piece at this position
        }
    }

    XYPos getKingPosition(Color &color)
    {
        if (color == Color::Black)
        {
            return this->pieceToCoordinate[blackKing];
        }
        else
        {
            return this->pieceToCoordinate[whiteKing];
        }
    }

    bool isValidPosition(XYPos &xyPos)
    {
        return (std::min(int(xyPos.x), xyPos.y) >= 1 && std::max(int(xyPos.x), xyPos.y) <= 8);
    }

public:
    Board()
    {
        const std::vector<int> ranks = {1, 2, 7, 8};
        for (int x = MIN_FILE; x <= MAX_FILE; ++x)
        {
            for (int y : ranks)
            {
                auto xyPos = XYPos(x, y);
                if (y == 1)
                {
                    // Rank 1 of the board
                    auto piece = piecesInOrder[x - 1](Color::White, static_cast<Index>(x));
                    addToBoard(piece, xyPos);
                }
                else if (y == 2)
                {
                    auto piece = Pawn(Color::White, static_cast<Index>(x));
                    addToBoard(piece, xyPos);
                }
                else if (y == 7)
                {
                    auto piece = Pawn(Color::Black, static_cast<Index>(x));
                    addToBoard(piece, xyPos);
                }
                else if (y == 8)
                {
                    auto piece = piecesInOrder[x - 1](Color::Black, static_cast<Index>(x));
                    addToBoard(piece, xyPos);
                }
            }
        }
    }
    std::set<XYPos> slidingMoves(XYPos &currentPosition, XYPos &moveVector)
    {
        Piece piece = this->coordinateToPiece[currentPosition];
        std::set<XYPos> moves;
        for (int k = MIN_RANK; k < MAX_RANK; ++k)
        {
            XYPos potentialPosition = currentPosition + (k * moveVector);
            if (!isValidPosition(potentialPosition))
                return moves; // out of bounds
            if (this->coordinateToPiece.count(potentialPosition) == 0)
            {
                moves.insert(potentialPosition); // blank square, valide and move
            }
            else if (this->coordinateToPiece[potentialPosition].color != piece.color)
            {
                moves.insert(potentialPosition); // capture piece and stop
                return moves;
            }
            else
            {
                return moves; // same color
            }
        }
        return moves;
    }

    std::set<XYPos> pseudoMoves(Piece &piece)
    {
        std::set<XYPos> moves;
        XYPos moveVector;
        XYPos potentialPosition;
        std::optional<Piece> pieceAtPotentialPosition;
        std::set<XYPos> slidingMovesSet;
        XYPos currentPosition = this->pieceToCoordinate[piece];
        if (piece.slidingPiece())
        {
            for (auto move : piece.movements())
            {
                moveVector = XYPos(move);
                slidingMovesSet = this->slidingMoves(currentPosition, moveVector);
                moves.insert(slidingMovesSet.begin(), slidingMovesSet.end());
                slidingMovesSet = this->slidingMoves(currentPosition, moveVector * -1);
                moves.insert(slidingMovesSet.begin(), slidingMovesSet.end());
            }
        }
        else
        {
            for (std::array<int, 2> move : piece.movements())
            {
                moveVector = XYPos(move);
                potentialPosition = currentPosition + moveVector;
                pieceAtPotentialPosition = getPiece(potentialPosition);
                if (!isValidPosition(potentialPosition))
                    continue;
                if (typeid(piece) == typeid(Pawn) && piece.color == Color::White && ((move == std::array<int, 2>({1, 1}) || move == std::array<int, 2>({-1, 1}))))
                {
                    // Pawn killing En-Passant or normal
                    XYPos enPassant = XYPos(potentialPosition.x, potentialPosition.y - 1);
                    std::optional<Piece> pieceAtEnPassant = getPiece(enPassant);
                    // Normal killing case
                    if (pieceAtPotentialPosition.has_value() && pieceAtPotentialPosition->color != piece.color)
                    {
                        moves.insert(potentialPosition);
                    }
                    else if (!pieceAtPotentialPosition.has_value() && pieceAtEnPassant.has_value() && typeid(*pieceAtEnPassant) == typeid(Pawn))
                    {
                        // En-Passant case
                        Pawn *pawnAtEnPassant = dynamic_cast<Pawn *>(&pieceAtEnPassant.value());
                        if (pawnAtEnPassant->movedTwice && pawnAtEnPassant->color != piece.color)
                            moves.insert(potentialPosition);
                    }
                }
                else if (typeid(piece) == typeid(Pawn) && piece.color == Color::Black && ((move == std::array<int, 2>({-1, -1}) || move == std::array<int, 2>({1, -1}))))
                {
                    // Pawn killing En-Passant or normal
                    XYPos enPassant = XYPos(potentialPosition.x, potentialPosition.y + 1);
                    std::optional<Piece> pieceAtEnPassant = getPiece(enPassant);
                    // Normal killing case
                    if (pieceAtPotentialPosition.has_value() && pieceAtPotentialPosition->color != piece.color)
                    {
                        moves.insert(potentialPosition);
                    }
                    else if (!pieceAtPotentialPosition.has_value() && pieceAtEnPassant.has_value() && typeid(*pieceAtEnPassant) == typeid(Pawn))
                    {
                        // En-Passant case
                        Pawn *pawnAtEnPassant = dynamic_cast<Pawn *>(&pieceAtEnPassant.value());
                        if (pawnAtEnPassant->movedTwice && pawnAtEnPassant->color != piece.color)
                            moves.insert(potentialPosition);
                    }
                }
                else if (typeid(piece) == typeid(King) && (move == std::array<int, 2>({-2, 0}) || move == std::array<int, 2>({2, 0})))
                {
                    int rank = piece.color == Color::White ? MIN_RANK : MAX_RANK;
                    if (move[0] == -2)
                    {
                        // Far castle
                        if (!getPiece(XYPos(Index::d, rank)).has_value() && !getPiece(XYPos(Index::c, rank)).has_value() && !getPiece(XYPos(Index::b, rank)).has_value() && !getPiece(XYPos(Index::a, rank)).has_value() &&
                            typeid(getPiece(XYPos(Index::a, rank))) == typeid(Castle) && !getPiece(XYPos(Index::a, rank))->hasMoved())
                        {
                            moves.insert(potentialPosition);
                        }
                        else if (!getPiece(XYPos(Index::f, rank)).has_value() && !getPiece(XYPos(Index::g, rank)).has_value() && getPiece(XYPos(Index::h, rank)).has_value() &&
                                 typeid(getPiece(XYPos(Index::h, rank))) == typeid(Castle) && !getPiece(XYPos(Index::h, rank))->hasMoved())
                        {
                            moves.insert(potentialPosition);
                        }
                    }
                }
                // if blank or opposite color
                else if (!pieceAtPotentialPosition.has_value() || pieceAtPotentialPosition->color != piece.color)
                {
                    moves.insert(potentialPosition);
                }
            }
        }
        return moves;
    }

    bool isCheck(Color color)
    {
        std::vector<Piece> opponents;
        for (auto pair : this->pieceToCoordinate)
        {
            if (pair.first.color != color)
            {
                opponents.push_back(pair.first);
            }
        }
        XYPos kingPositon = getKingPosition(color);
        for (auto opponent : opponents)
        {
            if (pseudoMoves(opponent).count(kingPositon))
            {
                return true;
            }
        }
        return false;
    }

    bool isKingExposed(Piece piece, XYPos potentialPosition)
    {
        std::optional<Piece> pieceAtPotentailPosition = getPiece(potentialPosition);
        XYPos originalPoition = this->pieceToCoordinate[piece];
        updatePiece(piece, potentialPosition);
        bool kingInCheck = isCheck(piece.color);
        updatePiece(piece, originalPoition);
        if (pieceAtPotentailPosition.has_value())
            addToBoard(*pieceAtPotentailPosition, potentialPosition);
        return kingInCheck;
    }

    std::set<XYPos> getValidMoves(Piece piece)
    {
        std::set<XYPos> moves = pseudoMoves(piece);
        std::set<XYPos> validMoves;
        for (auto move : moves)
        {
            if (!isKingExposed(piece, move))
                validMoves.insert(move);
        }
        return validMoves;
    }

    void movePiece(Piece &piece, XYPos &finalPosition)
    {
        std::set<XYPos> validMoves = getValidMoves(piece);
        XYPos initialPosition = this->pieceToCoordinate[piece];
        XYPos move = finalPosition - initialPosition;

        if (validMoves.count(finalPosition))
        {
            piece.moved = true;
            if (typeid(piece) == typeid(Pawn))
            {
                Pawn &pawn = static_cast<Pawn &>(piece);
                if (std::abs(move.y) == 2)
                {
                    pawn.movedTwice = true;
                }
            }
            updatePiece(piece,finalPosition);
        }
    }
};