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

// Quick helper function to print token types as text instead of numbers
inline std::string tokenTypeToString(TokenType type) {
    switch (type) {

        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::MODULO: return "MODULO";
        case TokenType::SEMICOLON: return "SEMICOLON";
        
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_FLOAT: return "KW_FLOAT";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_PRINT: return "KW_PRINT";
        case TokenType::KW_INPUT: return "KW_INPUT";
        
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        
        case TokenType::EQUAL: return "EQUAL";
        
        case TokenType::END_OF_FILE: return "EOF";
        
        default: return "UNKNOWN";
    }
}