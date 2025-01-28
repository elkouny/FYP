#ifndef PIECE_H
#define PIECE_H

#include "XYpos.h"
#include <iostream>
#include <vector>
#include <string>
#include <functional> // For std::hash

enum Color {
    Black,
    White,
    Blank
};

class Piece {
public:
    Color color;
    Index index;
    bool moved;
    std::string name;

    Piece(Color _color, Index _index);

    virtual ~Piece() = default;

    virtual std::vector<int> movements() = 0;

    virtual bool strongPiece();

    bool hasMoved();

    friend std::ostream &operator<<(std::ostream &os, const Piece &piece);

    bool operator==(const Piece &p);
};

class Pawn : public Piece {
public:
    bool movedTwice;

    Pawn(Color _color, Index _index);

    std::vector<int> movements() override;
};

class Knight : public Piece {
public:
    Knight(Color _color, Index _index);

    std::vector<int> movements() override;
};

class Castle : public Piece {
public:
    Castle(Color _color, Index _index);

    std::vector<int> movements() override;

    bool strongPiece() override;
};

class Bishop : public Piece {
public:
    Bishop(Color _color, Index _index);

    std::vector<int> movements() override;

    bool strongPiece() override;
};

class Queen : public Piece {
public:
    Queen(Color _color, Index _index);

    std::vector<int> movements() override;

    bool strongPiece() override;
};

class King : public Piece {
public:
    King(Color _color, Index _index);

    std::vector<int> movements() override;
};

namespace std {
    template<>
    struct hash<Piece> {
        std::size_t operator()(const Piece &p) const;
    };
}

#endif // PIECE_H