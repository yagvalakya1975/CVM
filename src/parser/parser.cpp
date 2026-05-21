#include "../include/parser/parser.h"
#include "../include/lexer/lexer.h"
#include <iostream>

Parser::Parser(std::vector<Token>& tokens)
    : parserTokens(tokens),
      index(0),
      limit(tokens.size()),
      current(parserTokens.empty() ? Token(TokenType::END_OF_FILE, "", 1) : parserTokens[0])
{
    if (parserTokens.empty()) {
        std::cerr << "ParserError: No tokens to parse.\n";
        return;
    }
}

Token* Parser::proceed(enum TokenType expectedType) {
    if (current.type == expectedType) 
    {
        if (index < limit - 1) {
            index++;
            current = parserTokens[index];
        } else {
            current = Token(TokenType::END_OF_FILE, "", current.line);
        }
        return &parserTokens[index - 1]; 
    } 
    else 
    {
        std::cerr << "ParserError: Expected token type " << tokenTypeToString(expectedType)
                  << " but found " << tokenTypeToString(current.type)
                  << " at line " << current.line << ".\n";
        return nullptr;
    }
}

ASTNode* Parser::parseExpression() {
    return new ASTNode();;
}

ASTNode* Parser::parse() {
    
    ASTNode* ROOT = new ASTNode();
    ROOT->type = NODE_TYPE::ROOT_NODE;

    while(current.type != TokenType::END_OF_FILE) {

        // We are now parsing one statement at a time here.
        switch (current.type)
        {
            case TokenType::INT_LITERAL :
            {
                ROOT->SUB_STATEMENTS.push_back(parseExpression());
                break;
            }
            default:
                break;
        }
        proceed(TokenType::SEMICOLON);
    }

    return ROOT;
}
