#include <cassert>
#include <iostream>
#include <string>

enum class Index
{
    a = 1,
    b = 2,
    c = 3,
    d = 4,
    e = 5,
    f = 6,
    g = 7,
    h = 8,
};
Index operator-(Index a, Index b)
{
    int result = int(a) - int(b);
    assert(("subraction out of bounds", result >= 1 && result <= 8));
    return static_cast<Index>(result);
}
Index operator-(Index a, int b)
{
    int result = int(a) - b;
    assert(("subraction out of bounds", result >= 1 && result <= 8));
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

    std::string toString(){
        return;
    }

    ~XYPos();
};
