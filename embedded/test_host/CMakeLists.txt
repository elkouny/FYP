cmake_minimum_required(VERSION 3.15)
project(ChessEngineTests)

set(CMAKE_CXX_STANDARD 17)

# GoogleTest (update this path if needed)
# set(GTEST_ROOT "/opt/homebrew/opt/googletest/1.16")
set(GTEST_ROOT /opt/homebrew/Cellar/googletest/1.16.0)
include_directories(${GTEST_ROOT}/include)
link_directories(${GTEST_ROOT}/lib)

include_directories(
    ../lib/Board
    ../lib/Piece
    ../lib/XYPos
    ../lib/Constants
)
include_directories(server/include)
add_executable(server server.cpp
    ../lib/Board/Board.cpp
    ../lib/Piece/Piece.cpp
    ../lib/XYPos/XYPos.cpp
    ../lib/Constants/Constants.h
)



# Add test and source files
add_executable(tests
    tests.cpp
    ../lib/Board/Board.cpp
    ../lib/Piece/Piece.cpp
    ../lib/XYPos/XYPos.cpp
    ../lib/Constants/Constants.h
)


# Link GoogleTest
target_link_libraries(tests gtest gtest_main pthread)
