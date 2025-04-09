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

int moveGenerationTest(Board board, int depth, const Color &color) {
    if (depth == 0) return 1;

    Color nextColor = (color == Color::White) ? Color::Black : Color::White;
    int total = 0;

    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        if (piece->color != color) continue;

        auto moves = board.getValidMoves(piece);
        for (const auto &move : moves) {
            // Create a new board for this move
            Board newBoard = deepCopyBoard(board);
            
            // Find the corresponding piece in the new board
            std::shared_ptr<Piece> newPiece = nullptr;
            for (const auto &[p, position] : newBoard.pieceToCoordinate) {
                if (position == pos && p->name == piece->name && p->color == piece->color) {
                    newPiece = p;
                    break;
                }
            }
            
            if (newPiece) {
                // Use the piece from the copied board
                XYPos newMove(move); // Create a copy of the move position
                newBoard.movePiece(newPiece, newMove);
                total += moveGenerationTest(newBoard, depth - 1, nextColor);
            }
        }
    }

    return total;
}

void perftBreakdown(Board &board, int depth, Color color) {
    int total = 0;
    Color nextColor = (color == Color::White) ? Color::Black : Color::White;
    
    for (const auto &[piece, pos] : board.pieceToCoordinate) {
        if (piece->color != color) continue;

        auto moves = board.getValidMoves(piece);
        for (const auto &move : moves) {
            // Create a new board for this move
            Board newBoard = deepCopyBoard(board);
            
            // Find the corresponding piece in the new board
            std::shared_ptr<Piece> newPiece = nullptr;
            for (const auto &[p, position] : newBoard.pieceToCoordinate) {
                if (position == pos && p->name == piece->name && p->color == piece->color) {
                    newPiece = p;
                    break;
                }
            }
            
            if (newPiece) {
                // Use the piece from the copied board
                XYPos newMove(move); // Create a copy of the move position
                newBoard.movePiece(newPiece, newMove);
                
                int subTotal = moveGenerationTest(newBoard, depth - 1, nextColor);
                std::cout << *piece << " " << pos << " → " << move << ": " << subTotal << std::endl;
                total += subTotal;
            }
        }
    }

    std::cout << "Total nodes at depth " << depth << ": " << total << std::endl;
}

TEST(BoardTest, DeepCopyPreservesStateAndIsIndependent) {
    Board original;
    Board copy = deepCopyBoard(original);

    // 1. Same number of pieces
    EXPECT_EQ(original.pieceToCoordinate.size(), copy.pieceToCoordinate.size());

    // 2. All pieces are in same positions
    for (const auto &[piece, pos] : original.pieceToCoordinate) {
        bool found = false;
        for (const auto &[cpiece, cpos] : copy.pieceToCoordinate) {
            if (pos == cpos && piece->name == cpiece->name && piece->color == cpiece->color) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    // 3. White and black king pointers are not null
    EXPECT_NE(copy.whiteKing, nullptr);
    EXPECT_NE(copy.blackKing, nullptr);
    EXPECT_EQ(copy.whiteKing->name, "King");
    EXPECT_EQ(copy.whiteKing->color, Color::White);
    EXPECT_EQ(copy.blackKing->name, "King");
    EXPECT_EQ(copy.blackKing->color, Color::Black);

    // 4. Changing copy should not affect original
    auto copyPiece = copy.getPiece(XYPos(Index::e, 2)); // a white pawn
    ASSERT_TRUE(copyPiece.has_value());
    XYPos newPos = XYPos(Index::e, 4);
    copy.movePiece(copyPiece.value(), newPos);

    // The original should still have the pawn at the old spot
    auto originalPiece = original.getPiece(XYPos(Index::e, 2));
    EXPECT_TRUE(originalPiece.has_value());
    EXPECT_EQ(originalPiece.value()->name, "Pawn");

    auto movedPieceInOriginal = original.getPiece(XYPos(Index::e, 4));
    EXPECT_FALSE(movedPieceInOriginal.has_value());
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

TEST(BoardTest, depth2) {
    Board board;
    EXPECT_EQ(moveGenerationTest(board, 2, Color::White), 400);
}

TEST(BoardTest, depth3) {
    Board board;
    EXPECT_EQ(moveGenerationTest(board, 3, Color::White), 8902);
}

TEST(BoardTest, depth4) {
    Board board;
    EXPECT_EQ(moveGenerationTest(board, 4, Color::White), 197281);
}
TEST(BoardTest, depth5) {
    Board board;
    EXPECT_EQ(moveGenerationTest(board, 5, Color::White), 4865609);
}

// TEST(BoardTest, perf) {
//     Board board;
//     perftBreakdown(board,4,Color::White);
// }



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


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}