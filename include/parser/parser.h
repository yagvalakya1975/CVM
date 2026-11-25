#pragma once
#include "lexer/lexer.h"
#include <string>
#include <vector>

using namespace std;

enum class ValueType { INVALID, VOID, BYTE, SHORT, INT, LONG, FLOAT, DOUBLE, CHAR, STRING, BOOLEAN };
inline const char* valueTypeName(ValueType t) {
    switch (t) {
        case ValueType::VOID: return "void"; case ValueType::BYTE: return "byte"; case ValueType::SHORT: return "short";
        case ValueType::INT: return "int"; case ValueType::LONG: return "long";
        case ValueType::FLOAT: return "float"; case ValueType::DOUBLE: return "double";
        case ValueType::CHAR: return "char"; case ValueType::STRING: return "String";
        case ValueType::BOOLEAN: return "boolean"; default: return "<invalid>";
    }
}

enum class NODE_TYPE {
    ROOT, DECL_STMT, FUNCTION_DECL, RETURN_STMT, PRINT_STMT, EXPR_STMT, BLOCK_STMT, IF_STMT, WHILE_STMT,
    BINARY_EXPR, ASSIGN_EXPR, ARRAY_ASSIGN_EXPR, CAST_EXPR,
    INT_LITERAL, LONG_LITERAL, FLOAT_LITERAL, DOUBLE_LITERAL, STRING_LITERAL,
    CHAR_LITERAL, BOOL_LITERAL, IDENTIFIER, CALL_EXPR, INPUT_EXPR, ARRAY_LITERAL, ARRAY_ACCESS
};

inline const char* nodeTypeName(NODE_TYPE type) {
    switch (type) {
        case NODE_TYPE::ROOT:           return "ROOT";
        case NODE_TYPE::DECL_STMT:      return "DECL_STMT";
        case NODE_TYPE::FUNCTION_DECL:  return "FUNCTION_DECL";
        case NODE_TYPE::RETURN_STMT:    return "RETURN_STMT";
        case NODE_TYPE::PRINT_STMT:     return "PRINT_STMT";
        case NODE_TYPE::EXPR_STMT:      return "EXPR_STMT";
        case NODE_TYPE::BLOCK_STMT:     return "BLOCK_STMT";
        case NODE_TYPE::IF_STMT:        return "IF_STMT";
        case NODE_TYPE::WHILE_STMT:     return "WHILE_STMT";
        case NODE_TYPE::BINARY_EXPR:    return "BINARY_EXPR";
        case NODE_TYPE::ASSIGN_EXPR:    return "ASSIGN_EXPR";
        case NODE_TYPE::ARRAY_ASSIGN_EXPR: return "ARRAY_ASSIGN_EXPR";
        case NODE_TYPE::CAST_EXPR:      return "CAST_EXPR";
        case NODE_TYPE::INT_LITERAL:    return "INT_LITERAL";
        case NODE_TYPE::LONG_LITERAL:   return "LONG_LITERAL";
        case NODE_TYPE::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case NODE_TYPE::DOUBLE_LITERAL: return "DOUBLE_LITERAL";
        case NODE_TYPE::STRING_LITERAL: return "STRING_LITERAL";
        case NODE_TYPE::CHAR_LITERAL:   return "CHAR_LITERAL";
        case NODE_TYPE::BOOL_LITERAL:   return "BOOL_LITERAL";
        case NODE_TYPE::IDENTIFIER:     return "IDENTIFIER";
        case NODE_TYPE::CALL_EXPR:      return "CALL_EXPR";
        case NODE_TYPE::INPUT_EXPR:     return "INPUT_EXPR";
        case NODE_TYPE::ARRAY_LITERAL:  return "ARRAY_LITERAL";
        case NODE_TYPE::ARRAY_ACCESS:   return "ARRAY_ACCESS";
    }
    return "UNKNOWN_NODE_TYPE";
}

struct ASTNode {
    NODE_TYPE type;
    string value, op;
    // Semantic-analysis annotations. They may be filled in while an AST is
    // traversed through a const pointer.
    mutable ValueType dataType = ValueType::INVALID;
    mutable int localSlot = -1;
    // For an array declaration, dataType is the element type. Multi-dimensional
    // arrays are represented by arrayDimensions; runtime support is compiler-owned.
    mutable int arrayDimensions = 0;
    int line = 0;
    ASTNode* left = nullptr;
    ASTNode* right = nullptr;
    ASTNode* alternate = nullptr;
    vector<ASTNode*> SUB_STATEMENTS;
    ASTNode() : type(NODE_TYPE::ROOT) {}
    explicit ASTNode(NODE_TYPE t) : type(t) {}
    ASTNode(NODE_TYPE t, string v, int n = 0) : type(t), value(move(v)), line(n) {}
    ~ASTNode() {
        delete left;
        delete right;
        delete alternate;
        for (ASTNode* child : SUB_STATEMENTS)
            delete child;
    }
};

class Parser {
public:
    Parser(vector<Token>& tokens);
    ASTNode* parse();
    bool hasErrors() const { return hadError_; }
    void printAST(const ASTNode* node, int depth = 0);
private:
    vector<Token> parserTokens;
    int index = 0, limit = 0;
    Token current{TokenType::END_OF_FILE, "", 1};
    bool hadError_ = false;
    Token* proceed(TokenType expected);
    bool check(TokenType t) const;
    bool isTypeKeyword() const;
    ValueType parseType();
    ASTNode* parseStatement(); ASTNode* parseDeclStmt(); ASTNode* parsePrintStmt(); ASTNode* parseReturnStmt();
    ASTNode* parseExprStmt(); ASTNode* parseBlock(); ASTNode* parseIfStmt(); ASTNode* parseWhileStmt();
    ASTNode* parseCondition(); ASTNode* parseRelational(); ASTNode* parseExpression();
    ASTNode* parseTerm(); ASTNode* parseFactor(); ASTNode* parseArrayLiteral();
};
