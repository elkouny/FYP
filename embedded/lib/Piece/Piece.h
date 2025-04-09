#ifndef PIECE_H
#define PIECE_H

#include <XYPos.h>
#include <array>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

enum Color {
    Black,
    White
};



class Piece {
public:
    Color color;
    Index index;
    bool moved;
    std::string name;
    Piece() = default;
    Piece(Color _color, Index _index);

    virtual ~Piece() = default;

    virtual std::vector<std::array<int, 2>> movements();

    virtual bool slidingPiece();

    bool hasMoved() const;

    friend std::ostream &operator<<(std::ostream &os, const Piece &piece);

    bool operator==(const Piece &p) const;
};

class Pawn : public Piece {
public:
    bool movedTwice;

    Pawn(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;
};

class Knight : public Piece {
public:
    Knight(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;
};

class Castle : public Piece {
public:
    Castle(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;

    bool slidingPiece() override;
};

class Bishop : public Piece {
public:
    Bishop(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;

    bool slidingPiece() override;
};

class Queen : public Piece {
public:
    Queen(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;

    bool slidingPiece() override;
};

class King : public Piece {
public:
    King(Color _color, Index _index);

    std::vector<std::array<int, 2>> movements() override;
};

namespace std {
template <>
struct hash<Piece> {
    std::size_t operator()(const Piece &p) const;
};
} // namespace std

#endif