#include "XYpos.h"

Index operator-(Index a, Index b)
{
    int result = int(a) - int(b);
    return static_cast<Index>(result);
}
Index operator-(Index a, int b)
{
    int result = int(a) - b;
    return static_cast<Index>(result);
}

Index operator+(Index a, Index b)
{
    int result = int(a) + int(b);
    return static_cast<Index>(result);
}
Index operator+(Index a, int b)
{
    int result = int(a) + b;
    return static_cast<Index>(result);
}

class XYPos
{
public:
    Index x;
    int y;
    XYPos(int x, int y)
    {
        this->x = static_cast<Index>(x);
        this->y = y;
    }

    XYPos()
    {
        this->x = Index::a;
        this->y = 0;
    }

    XYPos(Index x, int y)
    {
        this->x = x;
        this->y = y;
    }

    XYPos(std::array<int, 2> xy)
    {
        this->x = static_cast<Index>(xy[0]);
        this->y = xy[1];
    }

    XYPos operator+(XYPos &other)
    {
        return XYPos(int(this->x) + int(other.x), this->y + other.y);
    }

    XYPos operator-(XYPos &other)
    {
        return XYPos(int(this->x) - int(other.x), this->y - other.y);
    }

    XYPos operator*(int &other)
    {
        return XYPos(int(this->x) * other, this->y * other);
    }

    friend XYPos operator*(int &lhs, XYPos &rhs)
    {
        return XYPos(lhs * int(rhs.x), lhs * rhs.y);
    }

    // Used for hashing
    bool operator==(XYPos &other)
    {
        return this->x == other.x && this->y == other.y;
    }

    // used for printing
    friend std::ostream &operator<<(std::ostream &os, XYPos &xy)
    {
        os << "XYPos object object : x = " << int(xy.x) << ", y = " << xy.y;
        return os;
    }
    ~XYPos();
};

namespace std
{
    template <>
    struct hash<XYPos>
    {
        std::size_t operator()(const XYPos &xy)
        {
            std::size_t h1 = std::hash<int>()(int(xy.x));
            std::size_t h2 = std::hash<int>()(xy.y);
            return h1 ^ (h2 << 1);
        }
    }
}
