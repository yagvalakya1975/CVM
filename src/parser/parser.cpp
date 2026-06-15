#include "parser/parser.h"
#include "lexer/lexer.h"
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

Token* Parser::proceed(TokenType expected) {
    if (current.type == expected) {
        Token* consumed = &parserTokens[index];
        if (index < limit - 1) {
            current = parserTokens[++index];
        } else {
            current = Token(TokenType::END_OF_FILE, "", current.line);
        }
        return consumed;
    }
 
    std::cerr << "ParserError [line " << current.line << "]: expected '"
              << tokenTypeToString(expected) << "' but found '"
              << tokenTypeToString(current.type) << "' ('" << current.lexeme << "').\n";
    return nullptr;
}

ASTNode* Parser::parse() {
    ASTNode* root = new ASTNode(NODE_TYPE::ROOT);
 
    while (!check(TokenType::END_OF_FILE)) {
        ASTNode* stmt = parseStatement();
        if (stmt)
            root->SUB_STATEMENTS.push_back(stmt);
        else {
            // Error recovery: skip to the next semicolon and carry on
            std::cerr << "ParserError [line " << current.line
                      << "]: skipping bad statement.\n";
            while (!check(TokenType::SEMICOLON) &&
                   !check(TokenType::END_OF_FILE))
                proceed(current.type);          // consume whatever is there
            if (check(TokenType::SEMICOLON))
                proceed(TokenType::SEMICOLON);
        }
    }
    return root;
}

bool Parser::check(TokenType t) const {
    return current.type == t;
}
 
bool Parser::isTypeKeyword() const {
    return current.type == TokenType::KW_INT    ||
           current.type == TokenType::KW_FLOAT  ||
           current.type == TokenType::KW_CHAR   ||
           current.type == TokenType::KW_STRING ||
           current.type == TokenType::KW_BOOL;
}

ASTNode* Parser::parseStatement() {
    if (check(TokenType::LEFT_BRACE))
        return parseBlock();

    if (check(TokenType::KW_IF))
        return parseIfStmt();

    if (check(TokenType::KW_WHILE))
        return parseWhileStmt();

    if (isTypeKeyword())
        return parseDeclStmt();
        
    if (check(TokenType::KW_PRINT))
        return parsePrintStmt();
 
    if (check(TokenType::END_OF_FILE)) {
        std::cerr << "ParserError: unexpected end of file.\n";
        return nullptr;
    }

    if (check(TokenType::SEMICOLON)) {
        std::cerr << "ParserError [line " << current.line
                  << "]: empty statement.\n";
        proceed(TokenType::SEMICOLON);  // consume the ';' and move on
        return nullptr;
    }
    
    return parseExprStmt();
}

ASTNode* Parser::parseDeclStmt() { //  declStmt → TYPE IDENTIFIER = expression ;
    // Consume the type keyword
    Token* typeTok = proceed(current.type);   // safe: isTypeKeyword() was true
    if (!typeTok) return nullptr;

    Token* nameTok = proceed(TokenType::IDENTIFIER);
    if (!nameTok) return nullptr;

    if (!proceed(TokenType::EQUAL)) return nullptr;
 
    ASTNode* initExpr = parseCondition();
    if (!initExpr) return nullptr;

    if (!proceed(TokenType::SEMICOLON)) return nullptr;
 
    // Build the node
    ASTNode* node   = new ASTNode(NODE_TYPE::DECL_STMT, nameTok->lexeme);
    node->dataType  = typeTok->lexeme;   // "int", "float", "string", "char"
    node->right     = initExpr;          // the rhs expression
    return node;
}

ASTNode* Parser::parseBlock() {
    proceed(TokenType::LEFT_BRACE);
    ASTNode* blockNode = new ASTNode(NODE_TYPE::BLOCK_STMT);
    
    // Keep parsing statements until we hit the closing brace
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
        ASTNode* stmt = parseStatement();
        if (stmt) blockNode->SUB_STATEMENTS.push_back(stmt);
    }
    
    proceed(TokenType::RIGHT_BRACE);
    return blockNode;
}

ASTNode* Parser::parseIfStmt() {
    proceed(TokenType::KW_IF);
    proceed(TokenType::LEFT_PAREN);
    ASTNode* condition = parseCondition();
    proceed(TokenType::RIGHT_PAREN);

    ASTNode* trueBranch = parseStatement();
    ASTNode* falseBranch = nullptr;

    // Check if there is an 'else' attached
    if (check(TokenType::KW_ELSE)) {
        proceed(TokenType::KW_ELSE);
        falseBranch = parseStatement();
    }

    ASTNode* node = new ASTNode(NODE_TYPE::IF_STMT);
    node->left = condition;      // The condition to evaluate
    node->right = trueBranch;    // What to do if true
    node->alternate = falseBranch; // What to do if false (can be nullptr)
    return node;
}

ASTNode* Parser::parseWhileStmt() {

    proceed(TokenType::KW_WHILE);
    proceed(TokenType::LEFT_PAREN);

    ASTNode* condition = parseCondition();
    if (!condition) return nullptr;
    proceed(TokenType::RIGHT_PAREN);

    ASTNode* body = parseStatement();

    ASTNode* node = new ASTNode(NODE_TYPE::WHILE_STMT);
    if (!body) return nullptr;      //stop if body has syntax error

    node->left = condition; //  loop condition
    node->right = body;     //  block to loop over
    return node;
}

ASTNode* Parser::parseEquality() {
    ASTNode* left = parseRelational();

    while (check(TokenType::EQUAL_EQUAL) || check(TokenType::BANG_EQUAL)) {
        std::string op = current.lexeme;
        proceed(current.type);
        ASTNode* right = parseRelational();
        
        ASTNode* binary = new ASTNode(NODE_TYPE::BINARY_EXPR);
        binary->op = op;
        binary->left = left;
        binary->right = right;
        left = binary;
    }
    return left;
}

//  condition → relational ( ( "==" | "!=" ) relational )*
ASTNode* Parser::parseCondition() {
    ASTNode* left = parseRelational();

    while (check(TokenType::EQUAL_EQUAL) || check(TokenType::BANG_EQUAL)) {
        std::string op = current.lexeme;
        proceed(current.type);
        ASTNode* right = parseRelational();
        
        ASTNode* binary = new ASTNode(NODE_TYPE::BINARY_EXPR);
        binary->op = op;
        binary->left = left;
        binary->right = right;
        left = binary;
    }
    return left;
}

//  relational → expression ( ( "<" | ">" | "<=" | ">=" ) expression )*
ASTNode* Parser::parseRelational() {
    ASTNode* left = parseExpression(); // Calls your existing math function!

    while (check(TokenType::LESS) || check(TokenType::LESS_EQUAL) ||
           check(TokenType::GREATER) || check(TokenType::GREATER_EQUAL)) {
        std::string op = current.lexeme;
        proceed(current.type);
        ASTNode* right = parseExpression();
        
        ASTNode* binary = new ASTNode(NODE_TYPE::BINARY_EXPR);
        binary->op = op;
        binary->left = left;
        binary->right = right;
        left = binary;
    }
    return left;
}

//  printStmt → print ( expression ) ;
ASTNode* Parser::parsePrintStmt() {
    proceed(TokenType::KW_PRINT);
    if (!proceed(TokenType::LEFT_PAREN))  return nullptr;
 
    ASTNode* arg = parseCondition();
    if (!arg) return nullptr;
 
    if (!proceed(TokenType::RIGHT_PAREN)) return nullptr;
    if (!proceed(TokenType::SEMICOLON))   return nullptr;
 
    ASTNode* node = new ASTNode(NODE_TYPE::PRINT_STMT);
    node->left    = arg;     // the expression to print
    return node;
}

ASTNode* Parser::parseExprStmt() {
    // Assignment statement:  IDENTIFIER = expr ;
    // We peek two tokens ahead without consuming; if the pattern matches
    // we handle it specially so the parser doesn't choke on the '='.
    if (check(TokenType::IDENTIFIER) && index + 1 < limit &&
        parserTokens[index + 1].type == TokenType::EQUAL)
    {
        Token* nameTok = proceed(TokenType::IDENTIFIER);
        if (!nameTok) return nullptr;
        if (!proceed(TokenType::EQUAL)) return nullptr;

        ASTNode* rhs = parseCondition();
        if (!rhs) return nullptr;
        if (!proceed(TokenType::SEMICOLON)) return nullptr;

        // Build an ASSIGN_EXPR node; compiler handles the store
        ASTNode* assign = new ASTNode(NODE_TYPE::ASSIGN_EXPR, nameTok->lexeme);
        assign->op    = "=";
        assign->right = rhs;

        ASTNode* node = new ASTNode(NODE_TYPE::EXPR_STMT);
        node->left    = assign;
        return node;
    }

    ASTNode* expr = parseCondition();
    if (!expr) return nullptr;
    if (!proceed(TokenType::SEMICOLON)) return nullptr;
 
    ASTNode* node = new ASTNode(NODE_TYPE::EXPR_STMT);
    node->left    = expr;
    return node;
}

//  expression     → term expressionTail
//  expressionTail → + term expressionTail | - term expressionTail | ε
ASTNode* Parser::parseExpression() {
    ASTNode* left = parseTerm();
    if (!left) return nullptr;
    return parseExpressionTail(left);
    // return parseEquality();
}
 
ASTNode* Parser::parseExpressionTail(ASTNode* left) {
    if (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = current.lexeme;
        proceed(current.type);                  // consume + or -
 
        ASTNode* right = parseTerm();
        if (!right) return nullptr;
 
        // Build a BINARY_EXPR node and recurse (left-associative)
        ASTNode* binary = new ASTNode(NODE_TYPE::BINARY_EXPR);
        binary->op      = op;
        binary->left    = left;
        binary->right   = right;
 
        return parseExpressionTail(binary);     // tail recursion → left assoc.
    }
    return left;    // ε production
}

//  term     → factor termTail
//  termTail → * factor termTail | / factor termTail | % factor termTail | ε
ASTNode* Parser::parseTerm() {
    ASTNode* left = parseFactor();
    if (!left) return nullptr;
    return parseTermTail(left);
}
 
ASTNode* Parser::parseTermTail(ASTNode* left) {
    if (check(TokenType::STAR)   ||
        check(TokenType::SLASH)  ||
        check(TokenType::MODULO))
    {
        std::string op = current.lexeme;
        proceed(current.type);                  // consume * / or %
 
        ASTNode* right = parseFactor();
        if (!right) return nullptr;
 
        ASTNode* binary = new ASTNode(NODE_TYPE::BINARY_EXPR);
        binary->op      = op;
        binary->left    = left;
        binary->right   = right;
 
        return parseTermTail(binary);           // left-associative
    }
    return left;    // ε production
}

//  factor → INT_LITERAL | FLOAT_LITERAL | STRING_LITERAL | CHAR_LITERAL | IDENTIFIER | t/f | input ( STRING_LITERAL ) | ( expression )
ASTNode* Parser::parseFactor() {
    switch (current.type) {
 
        case TokenType::INT_LITERAL: {
            ASTNode* n = new ASTNode(NODE_TYPE::INT_LITERAL, current.lexeme);
            proceed(TokenType::INT_LITERAL);
            return n;
        }
        case TokenType::FLOAT_LITERAL: {
            ASTNode* n = new ASTNode(NODE_TYPE::FLOAT_LITERAL, current.lexeme);
            proceed(TokenType::FLOAT_LITERAL);
            return n;
        }
        case TokenType::STRING_LITERAL: {
            ASTNode* n = new ASTNode(NODE_TYPE::STRING_LITERAL, current.lexeme);
            proceed(TokenType::STRING_LITERAL);
            return n;
        }
        case TokenType::CHAR_LITERAL: {
            ASTNode* n = new ASTNode(NODE_TYPE::CHAR_LITERAL, current.lexeme);
            proceed(TokenType::CHAR_LITERAL);
            return n;
        }
        case TokenType::IDENTIFIER: {
            ASTNode* n = new ASTNode(NODE_TYPE::IDENTIFIER, current.lexeme);
            proceed(TokenType::IDENTIFIER);
            return n;
        }
        case TokenType::KW_TRUE: {
            ASTNode* n = new ASTNode(NODE_TYPE::BOOL_LITERAL, "true");
            proceed(TokenType::KW_TRUE);
            return n;
        }
        case TokenType::KW_FALSE: {
            ASTNode* n = new ASTNode(NODE_TYPE::BOOL_LITERAL, "false");
            proceed(TokenType::KW_FALSE);
            return n;
        }
        // ── input ( STRING_LITERAL ) ──────────────────────────────
        case TokenType::KW_INPUT:
            return parseInputExpr();
 
        // ── Parenthesised expression ──────────────────────────────
        case TokenType::LEFT_PAREN: {
            proceed(TokenType::LEFT_PAREN);
            ASTNode* inner = parseCondition();
            if (!proceed(TokenType::RIGHT_PAREN)) return nullptr;
            return inner;   // the parens are structural; no extra node needed
        }
 
        default:
            std::cerr << "ParserError [line " << current.line
                      << "]: unexpected token '"
                      << tokenTypeToString(current.type)
                      << "' in expression.\n";
            return nullptr;
    }
}

//  inputExpr → input ( STRING_LITERAL )
ASTNode* Parser::parseInputExpr() {
    proceed(TokenType::KW_INPUT);
    if (!proceed(TokenType::LEFT_PAREN))  return nullptr;
 
    Token* prompt = proceed(TokenType::STRING_LITERAL);
    if (!prompt) return nullptr;
 
    if (!proceed(TokenType::RIGHT_PAREN)) return nullptr;
 
    ASTNode* node = new ASTNode(NODE_TYPE::INPUT_EXPR, prompt->lexeme);
    return node;
}

void Parser::printAST(const ASTNode* node, int depth) {
    if (!node) return;
 
    std::string indent(depth * 4, ' ');
    std::string label = nodeTypeName(node->type);

    if (!node->value.empty())    label += " [" + node->value + "]";
    if (!node->op.empty())       label += " (op: " + node->op + ")";
    if (!node->dataType.empty()) label += " <type: " + node->dataType + ">";
 
   std::cout << indent << " |─ " << label << "\n";

    if (node->left)  printAST(node->left,  depth + 1);
    if (node->right) printAST(node->right, depth + 1);

    // NEW: Print the ELSE branch visibly if it exists
    if (node->alternate) {
        std::cout << indent << "    └─ ELSE_BRANCH:\n";
        printAST(node->alternate, depth + 1);
    }

    for (const ASTNode* child : node->SUB_STATEMENTS)
        printAST(child, depth + 1);
}