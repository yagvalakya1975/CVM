#include "lexer/lexer.h"
#include <iostream>

Lexer::Lexer(std::string source) : source(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start = current;
        scanToken();
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line));
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        // Single-character tokens
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '+': addToken(TokenType::PLUS); break;
        case '-': addToken(TokenType::MINUS); break;
        case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break;
        case '%': addToken(TokenType::MODULO); break;
        case ';': addToken(TokenType::SEMICOLON); break;

        // Ignore whitespace
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line++;
            break;

        default:
            if (isDigit(c)) {
                number();
            } else {
                // In a production compiler, we'd throw a proper error here
                std::cerr << "LexerError: line " << line << ": Unexpected character '" << c << "'\n";
            }
            break;
    }
}

void Lexer::number() {
    bool isFloat = false;

    // Consume all standard digits
    while (isDigit(peek())) advance();

    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance(); // Consume the "."

        while (isDigit(peek())) advance();
    }

    std::string value = source.substr(start, current - start);
    addToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, value);
}

// --- Utility Functions ---

bool Lexer::isAtEnd() {
    return current >= source.length();
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

void Lexer::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, line));
}

void Lexer::addToken(TokenType type, std::string text) { // particularly used here for number tokens
    tokens.push_back(Token(type, text, line));
}