#pragma once
#include "../lexer/lexer.h"
#include <vector>
#include <string>

enum class NODE_TYPE {
    ROOT,               // top-level program node; children = statements
    DECL_STMT,          // TYPE id = expr ;          (declaration + init)
    PRINT_STMT,         // print ( expr ) ;
    EXPR_STMT,          // bare expression used as a statement
    BINARY_EXPR,        // left OP right    (+ - * / %)
    UNARY_EXPR,         // OP operand       (currently only unary minus)
    ASSIGN_EXPR,        // id = expr        (future: assignment statement)
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    BOOL_LITERAL,
    IDENTIFIER,

    INPUT_EXPR,         // input ( STRING_LITERAL )
};

struct ASTNode {
    NODE_TYPE   type;

    std::string value;      // lexeme of the node's own token
    std::string op;         // operator for BINARY_EXPR / UNARY_EXPR
    std::string dataType;   // "int" / "float" / "string" / "char" for DECL_STMT

    ASTNode* left  = nullptr;   // lhs of binary expr, or single child
    ASTNode* right = nullptr;   // rhs of binary expr
    ASTNode*& child = left;
    std::vector<ASTNode*> SUB_STATEMENTS;

    // ── constructors ──────────────────────────────────────────────
    ASTNode() : type(NODE_TYPE::ROOT) {}
    explicit ASTNode(NODE_TYPE t) : type(t) {}
    ASTNode(NODE_TYPE t, std::string v) : type(t), value(std::move(v)) {}
};
class Parser {
public:
    Parser(std::vector<Token>& tokens);
    ASTNode* parse();
    Token* proceed(TokenType expected);
    void printAST(const ASTNode* node, int depth = 0);

private:
    std::vector<Token> parserTokens;
    int   index;
    int   limit;
    Token current;
    bool  check(TokenType t) const;     // peek without consuming
    bool  isTypeKeyword() const;        // is current a type keyword?
    ASTNode* parseStatement();
    ASTNode* parseDeclStmt();           // TYPE id = expr ;
    ASTNode* parsePrintStmt();          // print ( expr ) ;
    ASTNode* parseExprStmt();           // expr ;
    ASTNode* parseExpression();
    ASTNode* parseExpressionTail(ASTNode* left);
    ASTNode* parseTerm();
    ASTNode* parseTermTail(ASTNode* left);
    ASTNode* parseFactor();
    ASTNode* parseInputExpr();          // input ( STRING_LITERAL )
};

static std::string nodeTypeName(NODE_TYPE t) {
    switch (t) {
        case NODE_TYPE::ROOT:           return "ROOT";
        case NODE_TYPE::DECL_STMT:      return "DECL_STMT";
        case NODE_TYPE::PRINT_STMT:     return "PRINT_STMT";
        case NODE_TYPE::EXPR_STMT:      return "EXPR_STMT";
        case NODE_TYPE::BINARY_EXPR:    return "BINARY_EXPR";
        case NODE_TYPE::UNARY_EXPR:     return "UNARY_EXPR";
        case NODE_TYPE::ASSIGN_EXPR:    return "ASSIGN_EXPR";
        case NODE_TYPE::INT_LITERAL:    return "INT_LITERAL";
        case NODE_TYPE::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case NODE_TYPE::STRING_LITERAL: return "STRING_LITERAL";
        case NODE_TYPE::CHAR_LITERAL:   return "CHAR_LITERAL";
        case NODE_TYPE::BOOL_LITERAL:   return "BOOL_LITERAL";
        case NODE_TYPE::IDENTIFIER:     return "IDENTIFIER";
        case NODE_TYPE::INPUT_EXPR:     return "INPUT_EXPR";
        default:                        return "UNKNOWN";
    }
}