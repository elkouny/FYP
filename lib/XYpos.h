//
// Created by Ahmed Elkouny on 28/01/2025.
//
#ifndef XY_POS_H
#define XY_POS_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// Enum class to represent index values
enum class Index {
    a = 1,
    b = 2,
    c = 3,
    d = 4,
    e = 5,
    f = 6,
    g = 7,
    h = 8,
};

// Operator overloading for subtraction
Index operator-(Index a, Index b);

Index operator-(Index a, int b);

// Operator overloading for addition
Index operator+(Index a, Index b);

Index operator+(Index a, int b);

// Class to represent XY position
class XYPos {
public:
    Index x;
    int y;

    // Constructors
    XYPos(int x, int y);

    XYPos(Index x, int y);

    // Operator overloads
    XYPos operator+(const XYPos &other);

    XYPos operator-(const XYPos &other);

    XYPos operator*(const int &other);

    // Equality check for hashing
    bool operator==(const XYPos &other);

    // Used for printing XYPos objects
    friend std::ostream &operator<<(std::ostream &os, const XYPos &xy);

    // Destructor
    ~XYPos();
};

// Specialization of std::hash for XYPos to enable hashing
namespace std {
    template<>
    struct hash<XYPos> {
        std::size_t operator()(const XYPos &xy);
    };
}

#endif // XY_POS_H

