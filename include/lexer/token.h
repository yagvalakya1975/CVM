#pragma once
#include <string>
#include <utility>

// Define all possible token types based on your language specs
enum class TokenType {
    // Structural & Grouping
    LEFT_PAREN, RIGHT_PAREN,       // ( )
    LEFT_BRACE, RIGHT_BRACE,       // { } -> Assuming these for if/while blocks since it's indentation-free
    LEFT_BRACKET, RIGHT_BRACKET,   // [ ] -> For arrays
    SEMICOLON, COMMA,              // ; ,

    // Arithmetic Operators
    PLUS, MINUS, STAR, SLASH, MODULO, // + - * / %

    // Bitwise Operators
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, // & | ^ ~
    LEFT_SHIFT, RIGHT_SHIFT,           // << >> (Standard bitwise additions)

    // Comparisons & Assignment
    EQUAL, EQUAL_EQUAL,            // = ==
    BANG, BANG_EQUAL,              // ! !=  (Adding ! for boolean NOT)
    GREATER, GREATER_EQUAL,        // > >=
    LESS, LESS_EQUAL,              // < <=

    // Literals (Values)
    IDENTIFIER,                    // Variable names (e.g., 'var', 'count')
    INT_LITERAL,                   // e.g., 10, -5
    FLOAT_LITERAL,                 // e.g., 3.14
    CHAR_LITERAL,                  // e.g., 'A'
    STRING_LITERAL,                // e.g., "input text"

    // Keywords - Data Types
    KW_INT, KW_FLOAT, KW_CHAR, KW_STRING, KW_BOOL,

    // Keywords - Control Flow
    KW_IF, KW_ELSE, KW_WHILE, KW_BREAK, KW_CONTINUE,

    // Keywords - I/O & Booleans
    KW_INPUT, KW_PRINT,
    KW_TRUE, KW_FALSE,

    // End of File marker
    END_OF_FILE
};

// The Token struct holds the type, the raw string, and the line number for debugging
struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    // Constructor for clean initialization
    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}
};