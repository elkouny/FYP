#include "XYPos.h"
Index operator-(Index a, Index b) {
    return static_cast<Index>(static_cast<int>(a) - static_cast<int>(b));
}
Index operator-(Index a, int b) {
    return static_cast<Index>(static_cast<int>(a) - b);
}
Index operator+(Index a, Index b) {
    return static_cast<Index>(static_cast<int>(a) + static_cast<int>(b));
}
Index operator+(Index a, int b) {
    return static_cast<Index>(static_cast<int>(a) + b);
}

XYPos::XYPos(int x, int y) : x(static_cast<Index>(x)), y(y) {}
XYPos::XYPos() : x(Index::a), y(0) {}
XYPos::XYPos(Index x, int y) : x(x), y(y) {}
XYPos::XYPos(std::array<int, 2> &xy) : x(static_cast<Index>(xy[0])), y(xy[1]) {}

XYPos XYPos::operator+(const XYPos &other) const {
    return XYPos(static_cast<int>(x) + static_cast<int>(other.x), y + other.y);
}
XYPos XYPos::operator-(const XYPos &other) const {
    return XYPos(static_cast<int>(x) - static_cast<int>(other.x), y - other.y);
}
XYPos XYPos::operator*(int other) const {
    return XYPos(static_cast<int>(x) * other, y * other);
}

XYPos operator*(int lhs, const XYPos &rhs) {
    return XYPos(lhs * static_cast<int>(rhs.x), lhs * rhs.y);
}

bool XYPos::operator>(const XYPos &other) const {
    return (x > other.x) || (x == other.x && y > other.y);
}

bool XYPos::operator<(const XYPos &other) const {
    return (x < other.x) || (x == other.x && y < other.y);
}

bool XYPos::operator==(const XYPos &other) const {
    return (x == other.x) && (y == other.y);
}

std::ostream &operator<<(std::ostream &os, const XYPos &xy) {
    os << "XYPos object: x = " << static_cast<int>(xy.x) << ", y = " << xy.y;
    return os;
}

namespace std {
std::size_t hash<XYPos>::operator()(const XYPos &xy) const {
    std::size_t seed = 0;
    seed ^= std::hash<int>()(static_cast<int>(xy.x)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>()(xy.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
} // namespace std
