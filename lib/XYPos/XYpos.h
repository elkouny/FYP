//
// Created by Ahmed Elkouny on 28/01/2025.
//
#ifndef XY_POS_H
#define XY_POS_H

#include <iostream>
#include <string>
#include <vector>
#include <array>

// Enum class to represent index values
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

// Operator overloading for subtraction
Index operator-(Index a, Index b);

Index operator-(Index a, int b);

// Operator overloading for addition
Index operator+(Index a, Index b);

Index operator+(Index a, int b);

// Class to represent XY position
class XYPos
{
public:
    Index x;
    int y;

    // Constructors
    XYPos(int x, int y);

    XYPos();

    XYPos(Index x, int y);

    XYPos(std::array<int, 2> xy);

    // Operator overloads
    XYPos operator+(XYPos &other);

    XYPos operator-(XYPos &other);

    XYPos operator*(int &other);

    friend XYPos operator*(int &lhs, XYPos &rhs);

    // Equality check for hashing
    bool operator==(XYPos &other);

    // Used for printing XYPos objects
    friend std::ostream &operator<<(std::ostream &os, XYPos &xy);

    // Destructor
    ~XYPos();
};

// Specialization of std::hash for XYPos to enable hashing
namespace std
{
    template <>
    struct hash<XYPos>
    {
        std::size_t operator()(XYPos &xy);
    };
}

#endif // XY_POS_H
