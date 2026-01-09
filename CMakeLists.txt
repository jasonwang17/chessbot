cmake_minimum_required(VERSION 3.20)
project(chessbot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(chessbot
        src/main.cpp
        src/board.cpp
        src/uci.cpp
        src/perft.cpp
)

target_include_directories(chessbot PRIVATE include)

if (MSVC)
    target_compile_options(chessbot PRIVATE /W4)
else()
    target_compile_options(chessbot PRIVATE -Wall -Wextra -Wpedantic)
endif()
