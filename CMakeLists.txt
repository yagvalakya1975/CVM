cmake_minimum_required(VERSION 3.15)

project(CVMpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(cvmpp
    src/main.cpp
    src/lexer/lexer.cpp
)