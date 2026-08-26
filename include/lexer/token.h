#pragma once
#include <string>
#include <utility>

enum class TokenType {
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, SEMICOLON,
    PLUS, MINUS, STAR, SLASH, MODULO,
    EQUAL, EQUAL_EQUAL, BANG_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    IDENTIFIER, INT_LITERAL, LONG_LITERAL, FLOAT_LITERAL, DOUBLE_LITERAL,
    CHAR_LITERAL, STRING_LITERAL,
    KW_BYTE, KW_SHORT, KW_INT, KW_LONG, KW_FLOAT, KW_DOUBLE, KW_CHAR,
    KW_STRING, KW_BOOLEAN,
    KW_IF, KW_ELSE, KW_WHILE, KW_INPUT, KW_PRINT, KW_TRUE, KW_FALSE,
    KW_LEGACY_STRING, KW_LEGACY_BOOL,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    Token(TokenType t, std::string l, int n) : type(t), lexeme(std::move(l)), line(n) {}
};

inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LEFT_PAREN: return "("; case TokenType::RIGHT_PAREN: return ")";
        case TokenType::LEFT_BRACE: return "{"; case TokenType::RIGHT_BRACE: return "}";
        case TokenType::SEMICOLON: return ";"; case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-"; case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/"; case TokenType::MODULO: return "%";
        case TokenType::EQUAL: return "="; case TokenType::EQUAL_EQUAL: return "==";
        case TokenType::BANG_EQUAL: return "!="; case TokenType::GREATER: return ">";
        case TokenType::GREATER_EQUAL: return ">="; case TokenType::LESS: return "<";
        case TokenType::LESS_EQUAL: return "<="; case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INT_LITERAL: return "INT_LITERAL"; case TokenType::LONG_LITERAL: return "LONG_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL"; case TokenType::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL"; case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::KW_BYTE: return "byte"; case TokenType::KW_SHORT: return "short";
        case TokenType::KW_INT: return "int"; case TokenType::KW_LONG: return "long";
        case TokenType::KW_FLOAT: return "float"; case TokenType::KW_DOUBLE: return "double";
        case TokenType::KW_CHAR: return "char"; case TokenType::KW_STRING: return "String";
        case TokenType::KW_BOOLEAN: return "boolean"; case TokenType::KW_IF: return "if";
        case TokenType::KW_ELSE: return "else"; case TokenType::KW_WHILE: return "while";
        case TokenType::KW_INPUT: return "input"; case TokenType::KW_PRINT: return "print";
        case TokenType::KW_TRUE: return "true"; case TokenType::KW_FALSE: return "false";
        case TokenType::KW_LEGACY_STRING: return "legacy 'string'";
        case TokenType::KW_LEGACY_BOOL: return "legacy 'bool'";
        case TokenType::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}
