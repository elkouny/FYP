#ifndef XY_POS_H
#define XY_POS_H

#include <iostream>
#include <string>
#include <vector>
#include <array>

// Enum class to represent index values
enum class Index
{
    a = 1, b, c, d, e, f, g, h
};

// Operator overloading for Index
Index operator-(Index a, Index b);
Index operator-(Index a, int b);
Index operator+(Index a, Index b);
Index operator+(Index a, int b);

// Class to represent XY position
class XYPos
{
public:
    Index x;
    int y;

    // Constructors
    XYPos();
    XYPos(int x, int y);
    XYPos(Index x, int y);
    XYPos(std::array<int, 2> &xy);

    // Operator overloads
    XYPos operator+(const XYPos &other) const;
    XYPos operator-(const XYPos &other) const;
    XYPos operator*(int other) const;
    friend XYPos operator*(int lhs, const XYPos &rhs);
    bool operator==(const XYPos &other) const;

    // Printing support
    friend std::ostream &operator<<(std::ostream &os, const XYPos &xy);

    // Destructor
    ~XYPos();
};

// Specialization of std::hash for XYPos
namespace std
{
    template <>
    struct hash<XYPos>
    {
        std::size_t operator()(const XYPos &xy) const;
    };
}

#endif // XY_POS_H