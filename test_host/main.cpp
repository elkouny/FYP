#include "../lib/Board/Board.h"
#include <gtest/gtest.h>

// Helper: clone a piece by name
std::shared_ptr<Piece> clonePiece(const std::shared_ptr<Piece> &piece) {
    if (piece->name == "Pawn") {
        auto p = std::make_shared<Pawn>(*std::dynamic_pointer_cast<Pawn>(piece));
        return p;
    } else if (piece->name == "Knight") {
        return std::make_shared<Knight>(*std::dynamic_pointer_cast<Knight>(piece));
    } else if (piece->name == "Bishop") {
        return std::make_shared<Bishop>(*std::dynamic_pointer_cast<Bishop>(piece));
    } else if (piece->name == "Castle") {
        return std::make_shared<Castle>(*std::dynamic_pointer_cast<Castle>(piece));
    } else if (piece->name == "Queen") {
        return std::make_shared<Queen>(*std::dynamic_pointer_cast<Queen>(piece));
    } else if (piece->name == "King") {
        return std::make_shared<King>(*std::dynamic_pointer_cast<King>(piece));
    }
    return nullptr;
}

Board deepCopyBoard(const Board &original) {
    Board copy;
    copy.pieceToCoordinate.clear();
    copy.coordinateToPiece.clear();

    for (const auto &[origPiece, pos] : original.pieceToCoordinate) {
        auto newPiece = clonePiece(origPiece);
        copy.pieceToCoordinate[newPiece] = pos;
        copy.coordinateToPiece[pos] = newPiece;

        if (newPiece->name == "King") {
            if (newPiece->color == Color::White)
                copy.whiteKing = std::dynamic_pointer_cast<King>(newPiece);
            else
                copy.blackKing = std::dynamic_pointer_cast<King>(newPiece);
        }
    }

    return copy;
}

int moveGenerationTest(Board &board, int depth, const Color &color) {
    if (depth == 0) return 1;

    int total = 0;
    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        if (piece->color == color) {
            auto moves = board.getValidMoves(piece);
            for (const auto &move : moves) {
                Board newBoard = deepCopyBoard(board);
                newBoard.movePiece(piece, const_cast<XYPos &>(move));
                total += moveGenerationTest(newBoard, depth - 1, color);
            }
        }
    }
    return total;
}

TEST(BoardTest, BoardInitializedCorrectly) {
    Board board;
    EXPECT_EQ(board.pieceToCoordinate.size(), board.coordinateToPiece.size());
    EXPECT_EQ(board.pieceToCoordinate.size(), 32);
}

TEST(BoardTest, StartingPositionHas20MovesWhite) {
    Board board;
    int moveCount = 0;

    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        if (piece->color == Color::White) {
            auto moves = board.getValidMoves(piece);
            moveCount += moves.size();
        }
    }

    EXPECT_EQ(moveCount, 20);
}

TEST(BoardTest, depth1) {
    Board board;
    EXPECT_EQ(moveGenerationTest(board, 1, Color::White), 20);
}

TEST(BoardTest, KingsStartInCorrectPositions) {
    Board board;
    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        if (piece->name == "King") {
            if (piece->color == Color::White) {
                EXPECT_EQ(pos, XYPos(Index::e, 1));
            } else {
                EXPECT_EQ(pos, XYPos(Index::e, 8));
            }
        }
    }
}

TEST(BoardTest, KnightsHaveTwoMovesAtStart) {
    Board board;
    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        auto moves = board.getValidMoves(piece);
        if (piece->name == "Knight") {
            EXPECT_EQ(moves.size(), 2);
        }
    }
}

TEST(BoardTest, Depth2_TotalNodeCount) {
    Board board;
    int result = moveGenerationTest(board, 2, Color::White);
    EXPECT_EQ(result, 400);
}

TEST(BoardTest, Depth3_TotalNodeCount) {
    Board board;
    int result = moveGenerationTest(board, 3, Color::White);
    EXPECT_EQ(result, 8000);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
