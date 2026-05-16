#pragma once
#include <string>
#include <vector>
#include "lexer/token.h"

class Lexer {
public:
    Lexer(std::string source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    std::vector<Token> tokens;
    
    int start = 0;   // Points to the first character of the lexeme being scanned
    int current = 0; // Points to the character currently being considered
    int line = 1;    // Tracks what line we are on for error reporting

    // Helper methods
    void scanToken();
    void addToken(TokenType type);
    void addToken(TokenType type, std::string text);
    
    void number();
    
    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();
    bool isDigit(char c);
};