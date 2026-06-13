#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "lexer/token.h"

using namespace std;

class Lexer {
public:
    Lexer(std::string source);
    vector<Token> scanTokens();

private:
    string source;
    vector<Token> tokens;
    
    int start = 0;
    int current = 0; 
    int line = 1;    

    unordered_map<string, TokenType> keywords;

    // Helper methods
    void scanToken();
    void addToken(TokenType type);
    void addToken(TokenType type, string text);
    
    void check_number();
    void check_string();
    void check_character();
    void check_identifier();
    
    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();

    bool isDigit(char c);
    bool isAlpha(char c);
    bool isAlphaNumeric(char c);
};