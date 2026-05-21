#pragma once
#include "../lexer/lexer.h"

enum NODE_TYPE {
    ROOT_NODE,
    INT_NODE,
    FLOAT_NODE
};
struct ASTNode {
    enum NODE_TYPE type;
    std::string * value;
    ASTNode * child;
    std::vector<ASTNode* > SUB_STATEMENTS;
};

class Parser {
    public:
        Parser(std::vector<Token >& tokens);
        ASTNode* parse();
        Token * proceed(enum TokenType expected);
        ASTNode* parseExpression();

    private:
        Token current;
        int index;
        int limit;
        std::vector<Token > parserTokens;

};