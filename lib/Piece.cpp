#include "XYpos.h"

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
    Piece(Color _color, Index _index)
            : color(_color), index(_index), moved(false) {
        this-> name= __func__;
    }
    
    virtual ~Piece() = default;
    
    virtual std::vector<int> movements() = 0 {
        return {{0, 0}};
    }

    virtual bool strongPiece() {
        return false;
    }

    bool hasMoved() {
        return this->moved;
    }

    friend std::ostream &operator<<(std::ostream &os, const Piece &piece) {
        os << "Piece object : type = " << piece.name
           << ", color = " << (piece.color == Color::White ? "White" : "Black")
           << ", Index = " << int(piece.index)
           << ", moved = " << (piece.moved ? "Yes" : "No");
        return os;
    }

    bool operator==(const Piece &p) {
        return this->color == p.color && this->index == p.index && this->name == p.name;
    }
};

class Pawn : public Piece {
public:
    bool movedTwice;
    Pawn(Color _color , Index _index) : Piece(_color , _index) , movedTwice(false) {
        this->name =__func__ ;
    }
    std::vector<int> movements() override {
        if (this->color == Color::White){
            if (this->moved){
                return {{0,1},{1,1},{-1,1}};
            } else{
                return {{0,1},{1,1},{-1,1},{0,2}};
            }
        }else{
            if (this->moved){
                return {{0,-1},{-1,-1},{1,-1}};
            } else {
                return {{0,  -1},
                        {-1, -1},
                        {1,  -1},
                        {0,  -2}};
            }
        }
    }
};

class Knight : public Piece {
public:
    Knight(Color _color , Index _index) : Piece(_color , _index){
        this->name =__func__ ;
    }
    std::vector<int> movements() override{
        return {{1,2} ,{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    }
};

class Castle : public Piece {
public:
    Castle(Color _color , Index _index) : Piece(_color , _index){}
    std::vector<int> movements() override{
        return {{0,1},{1,0}};
    }
    bool strongPiece() override{
        return true;
    }
};

class Bishop : public Piece{
public:
    Bishop(Color _color , Index _index) : Piece(_color , _index){
        this->name = __func__ ;
    }
    std::vector<int> movements() override{
        return {{1,1},{1,-1}};
    }
    bool strongPiece() override {
        return true;
    }
};

class Queen : public Piece {
public:
    Queen(Color _color , Index _index) : Piece(_color , _index){
        this->name =__func__ ;
    }
    std::vector<int> movements() override{
        return {{-1,0},{-1,1},{0,1},{1,1}}
    }
    bool strongPiece() override {
        return true;
    }
};

class King: public Piece{
public:
    King(Color _color , Index _index) : Piece(_color , _index){
        this->name = __func__ ;
    }
    std::vector<int> movements() override{
        if(this->hasMoved()){
            return {{0,1},{1,1},{0,1},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
        } else{
            return {{0,1},{1,1},{0,1},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1},{-2,0},{2,0}};
        }
    }
};

template<>
namespace std {
    struct hash<Piece>{
        std::size_t operator() (const Piece& p) {
            std::size_t h1 = std::hash<std::string>()(p.name);
            std::size_t h2 = std::hash<int>()(p.index);
            std::size_t h3 = std::hash<int>()(p.color);
            return h1 ^ (h2 << 1) ^ (h3 <<2);
        }
    }
}