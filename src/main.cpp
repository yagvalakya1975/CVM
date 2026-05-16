#include <iostream>
#include <vector>
#include "lexer/lexer.h"
#include "lexer/token.h"

// Quick helper function to print token types as text instead of numbers
std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "UNKNOWN";
    }
}

int main() {
    // Our test source code
    std::string sourceCode = "(50 + 3.14) * 2;";
    
    std::cout << "Scanning source: " << sourceCode << "\n\n";

    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.scanTokens();

    for (const auto& token : tokens) {
        std::cout << "Token: " << tokenTypeToString(token.type) 
                  << " | Lexeme: '" << token.lexeme 
                  << "' | Line: " << token.line << "\n";
    }

    return 0;
}