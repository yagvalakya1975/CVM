#include "../../include/vm/compiler.h"
#include <iostream>
#include <stdexcept>
#include <iomanip>


Bytecode Compiler::compile(const ASTNode* root) {
    code_.clear();
    variables_.clear();
    hadError_ = false;
    if (!root) {
        std::cerr << "CompilerError: null AST root.\n";
        return {};
    }
    checkNode(root);
    if (hadError_) return {};
    compileNode(root);
    emit(Instruction(OpCode::HALT));
    return code_;
}

bool Compiler::numeric(ValueType t) const {
    return t >= ValueType::BYTE && t <= ValueType::CHAR;
}

bool Compiler::assignable(ValueType target, ValueType source) const {
    if (target == source) return true;
    if (!numeric(target) || !numeric(source)) return false;
    auto rank = [](ValueType t) {
        switch (t) { case ValueType::BYTE: return 0; case ValueType::SHORT: return 1;
        case ValueType::CHAR: return 1; case ValueType::INT: return 2; case ValueType::LONG: return 3;
        case ValueType::FLOAT: return 4; case ValueType::DOUBLE: return 5; default: return -1; }
    };
    return rank(source) <= rank(target) && !(source == ValueType::CHAR && target == ValueType::SHORT);
}

ValueType Compiler::promoted(ValueType a, ValueType b) const {
    if (a == ValueType::DOUBLE || b == ValueType::DOUBLE) return ValueType::DOUBLE;
    if (a == ValueType::FLOAT || b == ValueType::FLOAT) return ValueType::FLOAT;
    if (a == ValueType::LONG || b == ValueType::LONG) return ValueType::LONG;
    return ValueType::INT;
}

void Compiler::typeError(const ASTNode* node, const std::string& message) {
    std::cerr << "TypeError line " << (node ? node->line : 0) << ": " << message << "\n";
    hadError_ = true;
}

ValueType Compiler::checkNode(const ASTNode* node) {
    if (!node) return ValueType::INVALID;
    switch (node->type) {
        case NODE_TYPE::ROOT: case NODE_TYPE::BLOCK_STMT:
            for (const auto* child : node->SUB_STATEMENTS) checkNode(child);
            return ValueType::INVALID;
        case NODE_TYPE::DECL_STMT: {
            if (variables_.count(node->value)) typeError(node, "variable '" + node->value + "' is already declared");
            ValueType actual = checkExpr(node->right);
            bool constantNarrowing = false;
            if (actual == ValueType::INT && node->right && node->right->type == NODE_TYPE::INT_LITERAL &&
                (node->dataType == ValueType::BYTE || node->dataType == ValueType::SHORT || node->dataType == ValueType::CHAR)) {
                try {
                    long long v = std::stoll(node->right->value);
                    constantNarrowing = (node->dataType == ValueType::BYTE && v >= -128 && v <= 127) ||
                                        (node->dataType == ValueType::SHORT && v >= -32768 && v <= 32767) ||
                                        (node->dataType == ValueType::CHAR && v >= 0 && v <= 65535);
                } catch (...) { typeError(node, "integer literal is out of range"); }
            }
            if (!assignable(node->dataType, actual) && !constantNarrowing) typeError(node, "cannot initialize " + std::string(valueTypeName(node->dataType)) + " with " + valueTypeName(actual));
            variables_[node->value] = node->dataType;
            return ValueType::INVALID;
        }
        case NODE_TYPE::PRINT_STMT: checkExpr(node->left); return ValueType::INVALID;
        case NODE_TYPE::EXPR_STMT: checkExpr(node->left); return ValueType::INVALID;
        case NODE_TYPE::IF_STMT: case NODE_TYPE::WHILE_STMT:
            if (checkExpr(node->left) != ValueType::BOOLEAN) typeError(node->left, "condition must have type boolean");
            checkNode(node->right); if (node->alternate) checkNode(node->alternate); return ValueType::INVALID;
        default: return checkExpr(node);
    }
}

ValueType Compiler::checkExpr(const ASTNode* node) {
    if (!node) return ValueType::INVALID;
    ValueType result = ValueType::INVALID;
    switch (node->type) {
        case NODE_TYPE::INT_LITERAL: result=ValueType::INT; break; case NODE_TYPE::LONG_LITERAL: result=ValueType::LONG; break;
        case NODE_TYPE::FLOAT_LITERAL: result=ValueType::FLOAT; break; case NODE_TYPE::DOUBLE_LITERAL: result=ValueType::DOUBLE; break;
        case NODE_TYPE::STRING_LITERAL: case NODE_TYPE::INPUT_EXPR: result=ValueType::STRING; break;
        case NODE_TYPE::CHAR_LITERAL: result=ValueType::CHAR; break; case NODE_TYPE::BOOL_LITERAL: result=ValueType::BOOLEAN; break;
        case NODE_TYPE::IDENTIFIER: {
            auto it=variables_.find(node->value); if(it==variables_.end()) typeError(node,"undefined variable '"+node->value+"'"); else result=it->second; break;
        }
        case NODE_TYPE::CAST_EXPR: {
            ValueType from=checkExpr(node->left); if(!numeric(from)||!numeric(node->dataType)) typeError(node,"casts are only supported between numeric types"); result=node->dataType; break;
        }
        case NODE_TYPE::ASSIGN_EXPR: {
            auto it=variables_.find(node->value); ValueType rhs=checkExpr(node->right);
            if(it==variables_.end()) typeError(node,"undefined variable '"+node->value+"'");
            else { if(!assignable(it->second,rhs)) typeError(node,"cannot assign "+std::string(valueTypeName(rhs))+" to "+valueTypeName(it->second)); result=it->second; } break;
        }
        case NODE_TYPE::BINARY_EXPR: {
            ValueType a=checkExpr(node->left), b=checkExpr(node->right);
            if(node->op=="+" && (a==ValueType::STRING || b==ValueType::STRING)) result=ValueType::STRING;
            else if(node->op=="==" || node->op=="!=") { if(a!=b && !(numeric(a)&&numeric(b))) typeError(node,"incompatible equality operands"); result=ValueType::BOOLEAN; }
            else if(node->op=="<"||node->op=="<="||node->op==">"||node->op==">=") { if(!numeric(a)||!numeric(b)) typeError(node,"comparison requires numeric operands"); result=ValueType::BOOLEAN; }
            else { if(!numeric(a)||!numeric(b)) typeError(node,"arithmetic requires numeric operands"); result=promoted(a,b); } break;
        }
        default: typeError(node,"invalid expression"); break;
    }
    const_cast<ASTNode*>(node)->dataType=result;
    return result;
}

int Compiler::emit(Instruction instr) {
    int idx = static_cast<int>(code_.size());
    code_.push_back(std::move(instr));
    return idx;
}

void Compiler::patchJump(int instrIdx, int target) {
    code_[instrIdx].operand = static_cast<int64_t>(target);
}

int Compiler::currentIndex() const {
    return static_cast<int>(code_.size());
}

void Compiler::compileNode(const ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_TYPE::ROOT:
            for (const ASTNode* stmt : node->SUB_STATEMENTS)
                compileNode(stmt);
            break;

        case NODE_TYPE::DECL_STMT:     compileDeclStmt(node);  break;
        case NODE_TYPE::PRINT_STMT:    compilePrintStmt(node); break;
        case NODE_TYPE::EXPR_STMT:     compileExprStmt(node);  break;
        case NODE_TYPE::IF_STMT:       compileIfStmt(node);    break;
        case NODE_TYPE::WHILE_STMT:    compileWhileStmt(node); break;
        case NODE_TYPE::BLOCK_STMT:    compileBlock(node);     break;

        // Expressions used as statements shouldn't reach here directly,
        // but fall through to compileExpr just in case.
        default:
            compileExpr(node);
            break;
    }
}

//  TYPE id = expr ;
//  → compile expr (leaves value on stack) → STORE_VAR id
void Compiler::compileDeclStmt(const ASTNode* node) {
    compileExpr(node->right);                         // evaluate initialiser
    emit(Instruction(OpCode::STORE_VAR, node->value)); // pop and store in name
}

//  print ( expr ) ;
//  → compile expr → PRINT
void Compiler::compilePrintStmt(const ASTNode* node) {
    compileExpr(node->left);
    emit(Instruction(OpCode::PRINT));
}

//  expr ;
//  → compile expr → POP  (discard result)
void Compiler::compileExprStmt(const ASTNode* node) {
    compileExpr(node->left);
    emit(Instruction(OpCode::POP));
}

//  { stmts }
void Compiler::compileBlock(const ASTNode* node) {
    for (const ASTNode* stmt : node->SUB_STATEMENTS)
        compileNode(stmt);
}

//  if ( cond ) trueBranch [ else falseBranch ]
//
//  Bytecode layout:
//    <condition>
//    JUMP_IF_FALSE  →  elseLabel
//    <trueBranch>
//    JUMP           →  endLabel
//  elseLabel:
//    <falseBranch>   (or nothing)
//  endLabel:
void Compiler::compileIfStmt(const ASTNode* node) {
    compileExpr(node->left);                         // condition

    int jumpIfFalseIdx = emit(Instruction(OpCode::JUMP_IF_FALSE, int64_t(0)));

    compileNode(node->right);                        // true branch

    if (node->alternate) {
        int jumpEndIdx = emit(Instruction(OpCode::JUMP, int64_t(0)));
        patchJump(jumpIfFalseIdx, currentIndex());   // else label
        compileNode(node->alternate);                // false branch
        patchJump(jumpEndIdx,     currentIndex());   // end label
    } else {
        patchJump(jumpIfFalseIdx, currentIndex());   // end label (no else)
    }
}

//  while ( cond ) body
//
//  Bytecode layout:
//  loopStart:
//    <condition>
//    JUMP_IF_FALSE  →  loopEnd
//    <body>
//    JUMP           →  loopStart
//  loopEnd:
void Compiler::compileWhileStmt(const ASTNode* node) {
    int loopStart = currentIndex();

    compileExpr(node->left);                         // condition
    int jumpIfFalseIdx = emit(Instruction(OpCode::JUMP_IF_FALSE, int64_t(0)));

    compileNode(node->right);                        // body
    emit(Instruction(OpCode::JUMP, static_cast<int64_t>(loopStart)));

    patchJump(jumpIfFalseIdx, currentIndex());       // patch loop-exit jump
}

void Compiler::compileExpr(const ASTNode* node) {
    if (!node) {
        std::cerr << "CompilerError: null expression node.\n";
        return;
    }

    switch (node->type) {
        // ── Literals ──────────────────────────────────────────────────────
        case NODE_TYPE::INT_LITERAL:
            emit(Instruction(OpCode::PUSH_INT,
                             static_cast<int64_t>(std::stoll(node->value))));
            break;

        case NODE_TYPE::LONG_LITERAL:
            emit(Instruction(OpCode::PUSH_INT,
                             static_cast<int64_t>(std::stoll(node->value))));
            break;

        case NODE_TYPE::FLOAT_LITERAL:
            emit(Instruction(OpCode::PUSH_FLOAT, std::stod(node->value)));
            break;

        case NODE_TYPE::DOUBLE_LITERAL:
            emit(Instruction(OpCode::PUSH_FLOAT, std::stod(node->value)));
            break;

        case NODE_TYPE::STRING_LITERAL:
            emit(Instruction(OpCode::PUSH_STRING, node->value));
            break;

        case NODE_TYPE::CHAR_LITERAL:
            emit(Instruction(OpCode::PUSH_CHAR,
                             static_cast<char16_t>(static_cast<unsigned char>(node->value.at(0)))));
            break;

        case NODE_TYPE::BOOL_LITERAL:
            emit(Instruction(OpCode::PUSH_BOOL,
                             node->value == "true" ? int64_t(1) : int64_t(0)));
            break;

        // ── Variable read ─────────────────────────────────────────────────
        case NODE_TYPE::IDENTIFIER:
            emit(Instruction(OpCode::LOAD_VAR, node->value));
            break;

        // ── Input expression ──────────────────────────────────────────────
        case NODE_TYPE::INPUT_EXPR:
            emit(Instruction(OpCode::INPUT, node->value)); // value = prompt
            break;

        // ── Assignment expression  id = expr ─────────────────────────────
        case NODE_TYPE::ASSIGN_EXPR:
            compileExpr(node->right);
            emit(Instruction(OpCode::STORE_VAR, node->value));
            // Assignment leaves a value on the stack (it's an expression)
            emit(Instruction(OpCode::LOAD_VAR,  node->value));
            break;

        case NODE_TYPE::CAST_EXPR:
            compileExpr(node->left);
            emit(Instruction(OpCode::CONVERT, static_cast<int64_t>(node->dataType)));
            break;

        // ── Binary expression ─────────────────────────────────────────────
        case NODE_TYPE::BINARY_EXPR:
            compileBinaryExpr(node);
            break;

        default:
            std::cerr << "CompilerError: unexpected expression node type "
                      << static_cast<int>(node->type) << ".\n";
            break;
    }
}

void Compiler::compileBinaryExpr(const ASTNode* node) {
    if (node->op == "=" && node->left &&
        node->left->type == NODE_TYPE::IDENTIFIER)
    {
        compileExpr(node->right);
        emit(Instruction(OpCode::STORE_VAR, node->left->value));
        emit(Instruction(OpCode::LOAD_VAR,  node->left->value));
        return;
    }

    compileExpr(node->left);
    compileExpr(node->right);

    const std::string& op = node->op;

    if      (op == "+")  emit(Instruction(OpCode::ADD));
    else if (op == "-")  emit(Instruction(OpCode::SUB));
    else if (op == "*")  emit(Instruction(OpCode::MUL));
    else if (op == "/")  emit(Instruction(OpCode::DIV));
    else if (op == "%")  emit(Instruction(OpCode::MOD));
    else if (op == "==") emit(Instruction(OpCode::CMP_EQ));
    else if (op == "!=") emit(Instruction(OpCode::CMP_NEQ));
    else if (op == "<")  emit(Instruction(OpCode::CMP_LT));
    else if (op == "<=") emit(Instruction(OpCode::CMP_LE));
    else if (op == ">")  emit(Instruction(OpCode::CMP_GT));
    else if (op == ">=") emit(Instruction(OpCode::CMP_GE));
    else {
        std::cerr << "CompilerError: unknown binary operator '" << op << "'.\n";
    }
}

void Compiler::disassemble(const Bytecode& code) {
    std::cout << "\n=== BYTECODE DISASSEMBLY ===\n";
    for (int i = 0; i < static_cast<int>(code.size()); ++i) {
        const Instruction& instr = code[i];
        std::cout << std::setw(4) << i << "  " << std::left
                  << std::setw(16) << opCodeName(instr.op);

        // Print operand if meaningful
        if (std::holds_alternative<int64_t>(instr.operand)) {
            int64_t v = std::get<int64_t>(instr.operand);
            if (v != 0 || instr.op == OpCode::PUSH_INT ||
                          instr.op == OpCode::PUSH_BOOL ||
                          instr.op == OpCode::JUMP      ||
                          instr.op == OpCode::JUMP_IF_FALSE)
                std::cout << v;
        } else if (std::holds_alternative<double>(instr.operand)) {
            std::cout << std::get<double>(instr.operand);
        } else {
            const std::string& s = std::get<std::string>(instr.operand);
            if (!s.empty()) std::cout << '"' << s << '"';
        }

        std::cout << "\n";
    }
    std::cout << "============================\n\n";
}
