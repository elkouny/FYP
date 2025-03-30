#include "Piece.h"
#include "XYpos.h"

// Constructor Implementation
Piece::Piece(Color _color, Index _index) : color(_color), index(_index), moved(false), name("Piece") {}

bool Piece::hasMoved() const
{
    return this->moved;
}

std::vector<std::array<int, 2>> Piece::movements()
{
    return {{0, 0}};
}

bool Piece::slidingPiece()
{
    return false;
}

bool Piece::operator==(const Piece &p) const
{
    return this->color == p.color && this->index == p.index && this->name == p.name;
}

std::ostream &operator<<(std::ostream &os, const Piece &piece)
{
    os << "Piece object : type = " << piece.name
       << ", color = " << (piece.color == Color::White ? "White" : "Black")
       << ", Index = " << int(piece.index)
       << ", moved = " << (piece.moved ? "Yes" : "No");
    return os;
}

// Pawn Implementation
Pawn::Pawn(Color _color, Index _index) : Piece(_color, _index), movedTwice(false)
{
    this->name = "Pawn";
}

std::vector<std::array<int, 2>> Pawn::movements()
{
    if (this->color == Color::White)
    {
        if (this->moved)
        {
            return {{0, 1}, {1, 1}, {-1, 1}};
        }
        else
        {
            return {{0, 1}, {1, 1}, {-1, 1}, {0, 2}};
        }
    }
    else
    {
        if (this->moved)
        {
            return {{0, -1}, {-1, -1}, {1, -1}};
        }
        else
        {
            return {{0, -1}, {-1, -1}, {1, -1}, {0, -2}};
        }
    }
}

// Knight Implementation
Knight::Knight(Color _color, Index _index) : Piece(_color, _index)
{
    this->name = "Knight";
}

std::vector<std::array<int, 2>> Knight::movements()
{
    return {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
}

// Castle Implementation
Castle::Castle(Color _color, Index _index) : Piece(_color, _index)
{
    this->name = "Castle";
}

std::vector<std::array<int, 2>> Castle::movements()
{
    return {{0, 1}, {1, 0}};
}

bool Castle::slidingPiece()
{
    return true;
}

// Bishop Implementation
Bishop::Bishop(Color _color, Index _index) : Piece(_color, _index)
{
    this->name = "Bishop";
}

std::vector<std::array<int, 2>> Bishop::movements()
{
    return {{1, 1}, {1, -1}};
}

bool Bishop::slidingPiece()
{
    return true;
}

// Queen Implementation
Queen::Queen(Color _color, Index _index) : Piece(_color, _index)
{
    this->name = "Queen";
}

std::vector<std::array<int, 2>> Queen::movements()
{
    return {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
}

bool Queen::slidingPiece()
{
    return true;
}

// King Implementation
King::King(Color _color, Index _index) : Piece(_color, _index)
{
    this->name = "King";
}

std::vector<std::array<int, 2>> King::movements()
{
    if (this->hasMoved())
    {
        return {{0, 1}, {1, 1}, {0, 1}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
    }
    else
    {
        return {{0, 1}, {1, 1}, {0, 1}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {-2, 0}, {2, 0}};
    }
}

// Hash Function Implementation
std::size_t std::hash<Piece>::operator()(const Piece &p) const
{
    std::size_t h1 = std::hash<std::string>()(p.name);
    std::size_t h2 = std::hash<int>()(int(p.index));
    std::size_t h3 = std::hash<int>()(p.color);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}
