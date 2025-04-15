#ifndef XY_POS_H
#define XY_POS_H

#include <Arduino.h>
#include <array>
#include <iostream>
#include <string>
#include <vector>

// Enum class to represent index values
enum class Index {
    a = 1,
    b,
    c,
    d,
    e,
    f,
    g,
    h
};

// Operator overloading for Index
Index operator-(Index a, Index b);
Index operator-(Index a, int b);
Index operator+(Index a, Index b);
Index operator+(Index a, int b);

// Class to represent XY position
class XYPos {
public:
    Index x;
    int y;

    // Constructors
    XYPos();
    XYPos(int x, int y);
    XYPos(Index x, int y);
    XYPos(std::array<int, 2> &xy);
    XYPos(const std::array<int, 2> &xy);
    XYPos(std::string pos) : x(static_cast<Index>(pos[0]) - 'a' + 1), y(pos[1] - '0') {}

    // Operator overloads
    XYPos operator+(const XYPos &other) const;
    XYPos operator-(const XYPos &other) const;
    XYPos operator*(int other) const;
    friend XYPos operator*(int lhs, const XYPos &rhs);
    bool operator==(const XYPos &other) const;

    String toString() const;

    bool operator<(const XYPos &other) const;
    bool operator>(const XYPos &other) const;

    // Printing support
    friend std::ostream &operator<<(std::ostream &os, const XYPos &xy);

    // Destructor
    ~XYPos() = default;
};

namespace std {
template <>
struct hash<XYPos> {
    std::size_t operator()(const XYPos &xy) const;
};
} // namespace std

#endif // XY_POS_H