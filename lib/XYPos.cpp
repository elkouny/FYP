#include "XYpos.h"

Index operator-(Index a, Index b)
{
    int result = int(a) - int(b);
    assert(("subtraction out of bounds", result >= 1 && result <= 8));
    return static_cast<Index>(result);
}
Index operator-(Index a, int b)
{
    int result = int(a) - b;
    assert(("subtraction out of bounds", result >= 1 && result <= 8));
    return static_cast<Index>(result);
}

Index operator+(Index a, Index b)
{
    int result = int(a) + int(b);
    assert(("addition out of bounds", result >= 1 && result <= 8));
    return static_cast<Index>(result);
}
Index operator+(Index a, int b)
{
    int result = int(a) + b;
    assert(("addition out of bounds", result >= 1 && result <= 8));
    return static_cast<Index>(result);
}

class XYPos
{
public:
    Index x;
    int y;
    XYPos(int x, int y)
    {
        assert(x <= 8 && x >= 0);
        assert(y <= 8 && y >= 0);
        this->x = static_cast<Index>(x);
        this->y = y;
    }
    XYPos(Index x, int y)
    {
        assert(int(x) <= 8 && int(x) >= 0);
        assert(y <= 8 && y >= 0);
        this->x = x;
        this->y = y;
    }

    XYPos operator+(const XYPos &other)
    {
        return XYPos(this->x + other.x, this->y + other.y);
    }

    XYPos operator-(const XYPos &other)
    {
        return XYPos(this->x - other.x, this->y - other.y);
    }
    
    XYPos operator*(const int & other){
        return XYPos(this->x * other , this->y*other)
    }
    
    //Used for hashing
    bool operator==(const XYPos & other) {
        return this->x == other.x && this->y == other.y
    }
    
    //used for printing
    friend std::ostream& operator<<(std::ostream & os , const XYPos &xy ){
        os << "XYPos object object : x = " << xy.x << ", y = " <<xy.y; 
        return os;
    }
    ~XYPos();
};

template<>
namespace std {
    struct hash<XYPos>{
        std::size_t operator() (const XYPos& xy) {
            std::size_t h1 = std::hash<int>()(xy.x);
            std::size_t h2 = std::hash<int>()(xy.y);
            return h1 ^ (h2 << 1);
        }
    }
}
