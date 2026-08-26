#include "parser/parser.h"
#include <iostream>

Parser::Parser(std::vector<Token>& tokens) : parserTokens(tokens), limit(static_cast<int>(tokens.size())) {
    if (limit) current = parserTokens[0];
}
bool Parser::check(TokenType t) const { return current.type == t; }
Token* Parser::proceed(TokenType expected) {
    if (!check(expected)) { hadError_ = true; std::cerr << "ParserError line " << current.line << ": expected " << tokenTypeToString(expected) << ", found " << tokenTypeToString(current.type) << ".\n"; return nullptr; }
    Token* out = &parserTokens[index];
    current = ++index < limit ? parserTokens[index] : Token(TokenType::END_OF_FILE, "", out->line);
    return out;
}
bool Parser::isTypeKeyword() const {
    return current.type >= TokenType::KW_BYTE && current.type <= TokenType::KW_BOOLEAN;
}
ValueType Parser::parseType() {
    TokenType t = current.type; proceed(t);
    switch (t) {
        case TokenType::KW_BYTE: return ValueType::BYTE; case TokenType::KW_SHORT: return ValueType::SHORT;
        case TokenType::KW_INT: return ValueType::INT; case TokenType::KW_LONG: return ValueType::LONG;
        case TokenType::KW_FLOAT: return ValueType::FLOAT; case TokenType::KW_DOUBLE: return ValueType::DOUBLE;
        case TokenType::KW_CHAR: return ValueType::CHAR; case TokenType::KW_STRING: return ValueType::STRING;
        case TokenType::KW_BOOLEAN: return ValueType::BOOLEAN; default: return ValueType::INVALID;
    }
}
ASTNode* Parser::parse() {
    auto* root = new ASTNode(NODE_TYPE::ROOT);
    while (!check(TokenType::END_OF_FILE)) {
        ASTNode* stmt = parseStatement();
        if (stmt) root->SUB_STATEMENTS.push_back(stmt);
        else { while (!check(TokenType::SEMICOLON) && !check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) proceed(current.type); if (check(TokenType::SEMICOLON)) proceed(TokenType::SEMICOLON); }
    }
    return root;
}
ASTNode* Parser::parseStatement() {
    if (check(TokenType::KW_LEGACY_STRING) || check(TokenType::KW_LEGACY_BOOL)) { hadError_ = true; std::cerr << "ParserError line " << current.line << ": legacy type is not supported.\n"; return nullptr; }
    if (check(TokenType::LEFT_BRACE)) return parseBlock(); if (check(TokenType::KW_IF)) return parseIfStmt();
    if (check(TokenType::KW_WHILE)) return parseWhileStmt(); if (isTypeKeyword()) return parseDeclStmt();
    if (check(TokenType::KW_PRINT)) return parsePrintStmt(); return parseExprStmt();
}
ASTNode* Parser::parseDeclStmt() {
    int line = current.line; ValueType type = parseType(); int dimensions = 0;
    while (check(TokenType::LEFT_BRACKET)) {
        proceed(TokenType::LEFT_BRACKET);
        if (!proceed(TokenType::RIGHT_BRACKET)) return nullptr;
        ++dimensions;
    }
    Token* name = proceed(TokenType::IDENTIFIER);
    if (!name || !proceed(TokenType::EQUAL)) return nullptr; ASTNode* init = parseCondition();
    if (!init || !proceed(TokenType::SEMICOLON)) return nullptr;
    auto* n = new ASTNode(NODE_TYPE::DECL_STMT, name->lexeme, line); n->dataType = type; n->arrayDimensions = dimensions; n->right = init; return n;
}
ASTNode* Parser::parseBlock() {
    int line = current.line; proceed(TokenType::LEFT_BRACE); auto* b = new ASTNode(NODE_TYPE::BLOCK_STMT, "", line);
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) { if (ASTNode* s = parseStatement()) b->SUB_STATEMENTS.push_back(s); else break; }
    if (!proceed(TokenType::RIGHT_BRACE)) return nullptr; return b;
}
ASTNode* Parser::parseIfStmt() {
    int line = current.line; proceed(TokenType::KW_IF); if (!proceed(TokenType::LEFT_PAREN)) return nullptr;
    ASTNode* c = parseCondition(); if (!c || !proceed(TokenType::RIGHT_PAREN)) return nullptr; ASTNode* yes = parseStatement();
    ASTNode* no = nullptr; if (check(TokenType::KW_ELSE)) { proceed(TokenType::KW_ELSE); no = parseStatement(); }
    auto* n = new ASTNode(NODE_TYPE::IF_STMT, "", line); n->left=c; n->right=yes; n->alternate=no; return n;
}
ASTNode* Parser::parseWhileStmt() {
    int line=current.line; proceed(TokenType::KW_WHILE); if(!proceed(TokenType::LEFT_PAREN)) return nullptr;
    ASTNode* c=parseCondition(); if(!c || !proceed(TokenType::RIGHT_PAREN)) return nullptr; ASTNode* body=parseStatement();
    auto* n=new ASTNode(NODE_TYPE::WHILE_STMT,"",line); n->left=c; n->right=body; return n;
}
ASTNode* Parser::parsePrintStmt() { int line=current.line; proceed(TokenType::KW_PRINT); if(!proceed(TokenType::LEFT_PAREN)) return nullptr; ASTNode* x=parseCondition(); if(!x||!proceed(TokenType::RIGHT_PAREN)||!proceed(TokenType::SEMICOLON)) return nullptr; auto* n=new ASTNode(NODE_TYPE::PRINT_STMT,"",line); n->left=x; return n; }
ASTNode* Parser::parseExprStmt() {
    ASTNode* e=parseCondition();
    if (!e) return nullptr;
    if (check(TokenType::EQUAL)) {
        proceed(TokenType::EQUAL); ASTNode* rhs=parseCondition();
        if (!rhs || !proceed(TokenType::SEMICOLON)) return nullptr;
        ASTNode* assignment = nullptr;
        if (e->type == NODE_TYPE::IDENTIFIER) {
            assignment = new ASTNode(NODE_TYPE::ASSIGN_EXPR, e->value, e->line);
            assignment->right = rhs;
        } else if (e->type == NODE_TYPE::ARRAY_ACCESS) {
            assignment = new ASTNode(NODE_TYPE::ARRAY_ASSIGN_EXPR, "", e->line);
            assignment->left = e;
            assignment->right = rhs;
        } else {
            hadError_ = true;
            std::cerr << "ParserError line " << e->line << ": invalid assignment target.\n";
            return nullptr;
        }
        auto* n = new ASTNode(NODE_TYPE::EXPR_STMT); n->left = assignment; return n;
    }
    if(!proceed(TokenType::SEMICOLON)) return nullptr; auto* n=new ASTNode(NODE_TYPE::EXPR_STMT); n->left=e; return n;
}
ASTNode* Parser::parseCondition() {
    ASTNode* left=parseRelational(); while(check(TokenType::EQUAL_EQUAL)||check(TokenType::BANG_EQUAL)) { Token op=current; proceed(current.type); auto* n=new ASTNode(NODE_TYPE::BINARY_EXPR,"",op.line); n->op=op.lexeme; n->left=left; n->right=parseRelational(); left=n; } return left;
}
ASTNode* Parser::parseRelational() {
    ASTNode* left=parseExpression(); while(check(TokenType::LESS)||check(TokenType::LESS_EQUAL)||check(TokenType::GREATER)||check(TokenType::GREATER_EQUAL)) { Token op=current; proceed(current.type); auto* n=new ASTNode(NODE_TYPE::BINARY_EXPR,"",op.line); n->op=op.lexeme; n->left=left; n->right=parseExpression(); left=n; } return left;
}
ASTNode* Parser::parseExpression() { ASTNode* left=parseTerm(); while(check(TokenType::PLUS)||check(TokenType::MINUS)) { Token op=current; proceed(current.type); auto* n=new ASTNode(NODE_TYPE::BINARY_EXPR,"",op.line); n->op=op.lexeme; n->left=left; n->right=parseTerm(); left=n; } return left; }
ASTNode* Parser::parseTerm() { ASTNode* left=parseFactor(); while(check(TokenType::STAR)||check(TokenType::SLASH)||check(TokenType::MODULO)) { Token op=current; proceed(current.type); auto* n=new ASTNode(NODE_TYPE::BINARY_EXPR,"",op.line); n->op=op.lexeme; n->left=left; n->right=parseFactor(); left=n; } return left; }
ASTNode* Parser::parseFactor() {
    Token tok=current;
    auto literal=[&](NODE_TYPE type){ proceed(tok.type); return new ASTNode(type,tok.lexeme,tok.line); };
    switch(tok.type) {
        case TokenType::INT_LITERAL:return literal(NODE_TYPE::INT_LITERAL); case TokenType::LONG_LITERAL:return literal(NODE_TYPE::LONG_LITERAL);
        case TokenType::FLOAT_LITERAL:return literal(NODE_TYPE::FLOAT_LITERAL); case TokenType::DOUBLE_LITERAL:return literal(NODE_TYPE::DOUBLE_LITERAL);
        case TokenType::STRING_LITERAL:return literal(NODE_TYPE::STRING_LITERAL); case TokenType::CHAR_LITERAL:return literal(NODE_TYPE::CHAR_LITERAL);
        case TokenType::KW_TRUE: case TokenType::KW_FALSE:return literal(NODE_TYPE::BOOL_LITERAL);
        case TokenType::IDENTIFIER: {
            ASTNode* base = literal(NODE_TYPE::IDENTIFIER);
            while (check(TokenType::LEFT_BRACKET)) {
                proceed(TokenType::LEFT_BRACKET);
                ASTNode* subscript = parseCondition();
                if (!subscript || !proceed(TokenType::RIGHT_BRACKET)) return nullptr;
                auto* access = new ASTNode(NODE_TYPE::ARRAY_ACCESS, "", base->line);
                access->left = base;
                access->right = subscript;
                base = access;
            }
            return base;
        }
        case TokenType::LEFT_BRACKET:return parseArrayLiteral();
        case TokenType::KW_INPUT: { proceed(TokenType::KW_INPUT); if(!proceed(TokenType::LEFT_PAREN)) return nullptr; Token* p=proceed(TokenType::STRING_LITERAL); if(!p||!proceed(TokenType::RIGHT_PAREN)) return nullptr; return new ASTNode(NODE_TYPE::INPUT_EXPR,p->lexeme,p->line); }
        case TokenType::LEFT_PAREN: {
            proceed(TokenType::LEFT_PAREN);
            if (isTypeKeyword()) { ValueType t=parseType(); if(!proceed(TokenType::RIGHT_PAREN)) return nullptr; auto* n=new ASTNode(NODE_TYPE::CAST_EXPR,"",tok.line); n->dataType=t; n->left=parseFactor(); return n; }
            ASTNode* e=parseCondition(); if(!proceed(TokenType::RIGHT_PAREN)) return nullptr; return e;
        }
        default: hadError_ = true; std::cerr << "ParserError line " << tok.line << ": expected expression.\n"; return nullptr;
    }
}

ASTNode* Parser::parseArrayLiteral() {
    int line = current.line;
    if (!proceed(TokenType::LEFT_BRACKET)) return nullptr;
    auto* array = new ASTNode(NODE_TYPE::ARRAY_LITERAL, "", line);
    if (!check(TokenType::RIGHT_BRACKET)) {
        while (true) {
            ASTNode* element = parseCondition();
            if (!element) return nullptr;
            array->SUB_STATEMENTS.push_back(element);
            if (!check(TokenType::COMMA)) break;
            proceed(TokenType::COMMA);
            if (check(TokenType::RIGHT_BRACKET)) {
                hadError_ = true;
                std::cerr << "ParserError line " << current.line << ": trailing comma in array literal.\n";
                return nullptr;
            }
        }
    }
    if (!proceed(TokenType::RIGHT_BRACKET)) return nullptr;
    return array;
}
void Parser::printAST(const ASTNode* n,int depth) { if(!n)return; std::cout<<std::string(depth*2,' ')<<nodeTypeName(n->type); if(!n->value.empty())std::cout<<" "<<n->value; if(n->dataType!=ValueType::INVALID)std::cout<<" :"<<valueTypeName(n->dataType); for(int i=0;i<n->arrayDimensions;++i)std::cout<<"[]"; std::cout<<"\n"; printAST(n->left,depth+1); printAST(n->right,depth+1); printAST(n->alternate,depth+1); for(auto* s:n->SUB_STATEMENTS)printAST(s,depth+1); }
