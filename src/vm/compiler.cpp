#include "vm/compiler.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace std;


Bytecode Compiler::compile(const ASTNode* root) {
    code_.clear();
    scopes_.clear();
    functionTable_.clear();
    nextSlot_ = 0;
    topFrameSize_ = 0;
    currentReturnType_ = ValueType::INVALID;
    hadError_ = false;
    if (!root) 
    {
        cerr << "CompilerError: null AST root.\n";
        return {};
    }
    checkNode(root);
    if (hadError_) return {};
    const int skipFunctions = emit(Instruction(OpCode::JUMP, int64_t(0)));
    for (const ASTNode* child : root->SUB_STATEMENTS)
        if (child->type == NODE_TYPE::FUNCTION_DECL) compileFunctionDecl(child);
    patchJump(skipFunctions, currentIndex());
    emit(Instruction(OpCode::ENTER_FRAME, static_cast<int64_t>(topFrameSize_)));
    for (const ASTNode* child : root->SUB_STATEMENTS)
        if (child->type != NODE_TYPE::FUNCTION_DECL) compileNode(child);
    emit(Instruction(OpCode::HALT));
    return code_;
}

Compiler::VariableInfo* Compiler::resolve(const string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

bool Compiler::numeric(ValueType t) const {
    return t >= ValueType::BYTE && t <= ValueType::CHAR;
}

bool Compiler::assignable(ValueType target, ValueType source) const {
    if (target == source) return true;
    if (!numeric(target) || !numeric(source)) return false;
    auto rank = [](ValueType t) {
        switch (t) 
        { 
            case ValueType::BYTE: return 0; 
            case ValueType::SHORT: return 1;
            case ValueType::CHAR: return 1; 
            case ValueType::INT: return 2; 
            case ValueType::LONG: return 3;
            case ValueType::FLOAT: return 4; 
            case ValueType::DOUBLE: return 5; 
            default: return -1; }
    };
    return rank(source) <= rank(target) && !(source == ValueType::CHAR && target == ValueType::SHORT);
}

ValueType Compiler::promoted(ValueType a, ValueType b) const {
    if (a == ValueType::DOUBLE || b == ValueType::DOUBLE) return ValueType::DOUBLE;
    if (a == ValueType::FLOAT || b == ValueType::FLOAT) return ValueType::FLOAT;
    if (a == ValueType::LONG || b == ValueType::LONG) return ValueType::LONG;
    return ValueType::INT;
}

void Compiler::emitStorageConversion(ValueType target, ValueType source) {
    if (numeric(target) && numeric(source))
        emit(Instruction(OpCode::CONVERT, static_cast<int64_t>(target)));
}

void Compiler::typeError(const ASTNode* node, const string& message) {
    cerr << "TypeError line " << (node ? node->line : 0) << ": " << message << "\n";
    hadError_ = true;
}

ValueType Compiler::checkNode(const ASTNode* node) {
    if (!node) return ValueType::INVALID;
    switch (node->type) {
        case NODE_TYPE::ROOT:
            scopes_.emplace_back();
            nextSlot_ = 0;
            for (const auto* child : node->SUB_STATEMENTS) {
                if (child->type != NODE_TYPE::FUNCTION_DECL) {
                    checkNode(child);
                    continue;
                }
                if (functionTable_.count(child->value)) {
                    typeError(child, "function '" + child->value + "' is already declared");
                    continue;
                }
                // Register this declaration immediately before checking its body.
                // This permits direct recursion, but not calls to later declarations.
                FunctionInfo info;
                info.returnType = child->dataType;
                for (const ASTNode* parameter : child->SUB_STATEMENTS)
                    info.parameterTypes.push_back(parameter->dataType);
                functionTable_.emplace(child->value, move(info));
                const int savedTopNextSlot = nextSlot_;
                scopes_.emplace_back();
                nextSlot_ = 0;
                currentReturnType_ = child->dataType;
                for (const ASTNode* parameter : child->SUB_STATEMENTS) {
                    if (scopes_.back().count(parameter->value))
                        typeError(parameter, "parameter '" + parameter->value + "' is already declared");
                    auto [it, inserted] = scopes_.back().emplace(parameter->value,
                        VariableInfo{parameter->dataType, 0, nextSlot_++});
                    parameter->localSlot = it->second.slot;
                }
                checkNode(child->left);
                if (child->dataType != ValueType::VOID) {
                    const ASTNode* body = child->left;
                    if (!body || body->SUB_STATEMENTS.empty() ||
                        body->SUB_STATEMENTS.back()->type != NODE_TYPE::RETURN_STMT)
                        typeError(child, "non-void function '" + child->value + "' must end with return");
                }
                const_cast<ASTNode*>(child)->localSlot = nextSlot_;
                scopes_.pop_back();
                currentReturnType_ = ValueType::INVALID;
                nextSlot_ = savedTopNextSlot;
            }
            topFrameSize_ = nextSlot_;
            scopes_.pop_back();
            return ValueType::INVALID;
        case NODE_TYPE::BLOCK_STMT:
            scopes_.emplace_back();
            for (const auto* child : node->SUB_STATEMENTS)
                checkNode(child);
            scopes_.pop_back();
            return ValueType::INVALID;
        case NODE_TYPE::DECL_STMT: {
            if (scopes_.back().count(node->value))
                typeError(node, "variable '" + node->value + "' is already declared");
            ValueType actual = checkExpr(node->right);
            if (node->arrayDimensions > 0) {
                if (!node->right || node->right->type != NODE_TYPE::ARRAY_LITERAL)
                    typeError(node, "array declaration requires an array literal initializer");
                else {
                    if (node->right->arrayDimensions != node->arrayDimensions)
                        typeError(node, "array initializer has incompatible dimensions");
                    if (!assignable(node->dataType, actual))
                        typeError(node, "array initializer has incompatible element type");
                }
                auto [it, inserted] = scopes_.back().emplace(node->value, VariableInfo{node->dataType, node->arrayDimensions, nextSlot_++});
                node->localSlot = it->second.slot;
                return ValueType::INVALID;
            }
            bool constantNarrowing = false;
            if (actual == ValueType::INT && node->right && node->right->type == NODE_TYPE::INT_LITERAL &&
                (node->dataType == ValueType::BYTE || node->dataType == ValueType::SHORT || node->dataType == ValueType::CHAR)) {
                try {
                    long long v = stoll(node->right->value);
                    constantNarrowing = (node->dataType == ValueType::BYTE && v >= -128 && v <= 127) ||
                                        (node->dataType == ValueType::SHORT && v >= -32768 && v <= 32767) ||
                                        (node->dataType == ValueType::CHAR && v >= 0 && v <= 65535);
                } 
                catch (...) 
                { 
                    typeError(node, "integer literal is out of range"); 
                }
            }
            if (!assignable(node->dataType, actual) && !constantNarrowing) 
                typeError(node, "cannot initialize " + string(valueTypeName(node->dataType)) + " with " + valueTypeName(actual));
            auto [it, inserted] = scopes_.back().emplace(node->value, VariableInfo{node->dataType, 0, nextSlot_++});
            node->localSlot = it->second.slot;
            return ValueType::INVALID;
        }
        case NODE_TYPE::PRINT_STMT: 
            if (checkExpr(node->left) == ValueType::VOID)
                typeError(node, "cannot print a void expression");
            return ValueType::INVALID;
        case NODE_TYPE::EXPR_STMT: 
            checkExpr(node->left); 
            return ValueType::INVALID;
        case NODE_TYPE::FUNCTION_DECL:
            typeError(node, "functions may only be declared at top level");
            return ValueType::INVALID;
        case NODE_TYPE::RETURN_STMT: {
            if (currentReturnType_ == ValueType::INVALID) {
                typeError(node, "return outside a function");
            } else if (!node->left) {
                if (currentReturnType_ != ValueType::VOID)
                    typeError(node, "non-void function must return a value");
            } else {
                ValueType actual = checkExpr(node->left);
                if (currentReturnType_ == ValueType::VOID)
                    typeError(node, "void function cannot return a value");
                else if (!assignable(currentReturnType_, actual))
                    typeError(node, "cannot return " + string(valueTypeName(actual)) +
                                    " from " + valueTypeName(currentReturnType_) + " function");
            }
            return ValueType::INVALID;
        }
        case NODE_TYPE::IF_STMT: 
        case NODE_TYPE::WHILE_STMT:
            if (checkExpr(node->left) != ValueType::BOOLEAN) 
                typeError(node->left, "condition must have type boolean");
            checkNode(node->right); 
            if (node->alternate) 
                checkNode(node->alternate); 
            return ValueType::INVALID;
        default:
            return checkExpr(node);
    }
}

ValueType Compiler::checkExpr(const ASTNode* node) {
    if (!node) return ValueType::INVALID;
    ValueType result = ValueType::INVALID;
    switch (node->type) {
        case NODE_TYPE::INT_LITERAL: 
            result=ValueType::INT; 
            break; 
        case NODE_TYPE::LONG_LITERAL: 
            result=ValueType::LONG; 
            break;
        case NODE_TYPE::FLOAT_LITERAL: 
            result=ValueType::FLOAT; 
            break; 
        case NODE_TYPE::DOUBLE_LITERAL: 
            result=ValueType::DOUBLE; 
            break;
        case NODE_TYPE::STRING_LITERAL: 
        case NODE_TYPE::INPUT_EXPR: 
            result=ValueType::STRING; 
            break;
        case NODE_TYPE::CHAR_LITERAL: 
            result=ValueType::CHAR; 
            break; 
        case NODE_TYPE::BOOL_LITERAL: 
            result=ValueType::BOOLEAN; 
            break;
        case NODE_TYPE::IDENTIFIER: 
        {
            auto* info = resolve(node->value);
            if(!info)
                typeError(node,"undefined variable '"+node->value+"'"); 
            else 
            {
                result=info->type;
                node->arrayDimensions=info->arrayDimensions;
                node->localSlot=info->slot;
            } 
            break;
        }
        case NODE_TYPE::CALL_EXPR: {
            auto found = functionTable_.find(node->value);
            if (found == functionTable_.end()) {
                typeError(node, "undefined function '" + node->value + "'");
                break;
            }
            const FunctionInfo& function = found->second;
            if (node->SUB_STATEMENTS.size() != function.parameterTypes.size()) {
                typeError(node, "function '" + node->value + "' expects " +
                                to_string(function.parameterTypes.size()) + " argument(s)");
            }
            const size_t count = min(node->SUB_STATEMENTS.size(), function.parameterTypes.size());
            for (size_t i = 0; i < node->SUB_STATEMENTS.size(); ++i) {
                ValueType actual = checkExpr(node->SUB_STATEMENTS[i]);
                if (i < count && !assignable(function.parameterTypes[i], actual))
                    typeError(node->SUB_STATEMENTS[i], "argument " + to_string(i + 1) +
                                                   " cannot be passed to " + node->value);
            }
            result = function.returnType;
            break;
        }
        case NODE_TYPE::CAST_EXPR: {
            ValueType from=checkExpr(node->left); 
            if(!numeric(from)||!numeric(node->dataType)) 
                typeError(node,"casts are only supported between numeric types"); 
            result=node->dataType; 
            break;
        }
        case NODE_TYPE::ASSIGN_EXPR: {
            auto* info = resolve(node->value);
            ValueType rhs=checkExpr(node->right);
            if(!info)
                typeError(node,"undefined variable '"+node->value+"'");
            else 
            { 
                if(info->arrayDimensions != node->right->arrayDimensions || !assignable(info->type,rhs))
                    typeError(node,"cannot assign incompatible value to "+node->value); 
                result=info->type;
                node->arrayDimensions=info->arrayDimensions;
                node->localSlot=info->slot;
            } 
            break;
        }
        case NODE_TYPE::ARRAY_LITERAL: {
            if (node->SUB_STATEMENTS.empty()) {
                typeError(node, "array literal cannot be empty because its element type is ambiguous");
                break;
            }
            ValueType elementType = ValueType::INVALID;
            int childDimensions = -1;
            for (const ASTNode* element : node->SUB_STATEMENTS) {
                ValueType current = checkExpr(element);
                if (elementType == ValueType::INVALID) elementType = current;
                else if (!assignable(elementType, current) && !assignable(current, elementType))
                    typeError(element, "array literal elements must have compatible types");
                if (childDimensions == -1) childDimensions = element->arrayDimensions;
                else if (element->arrayDimensions != childDimensions)
                    typeError(element, "array literal elements must have matching dimensions");
            }
            result = elementType;
            node->arrayDimensions = childDimensions + 1;
            break;
        }
        case NODE_TYPE::ARRAY_ACCESS: {
            ValueType arrayElementType = checkExpr(node->left);
            ValueType indexType = checkExpr(node->right);
            if (node->left->arrayDimensions <= 0)
                typeError(node->left, "indexing requires an array operand");
            if (node->right->arrayDimensions != 0 ||
                (indexType != ValueType::BYTE && indexType != ValueType::SHORT &&
                 indexType != ValueType::INT && indexType != ValueType::LONG &&
                 indexType != ValueType::CHAR))
                typeError(node->right, "array index must have an integral scalar type");
            result = arrayElementType;
            node->arrayDimensions = max(0, node->left->arrayDimensions - 1);
            break;
        }
        case NODE_TYPE::ARRAY_ASSIGN_EXPR: {
            ValueType targetType = checkExpr(node->left);
            ValueType rhsType = checkExpr(node->right);
            if (node->left->arrayDimensions != node->right->arrayDimensions ||
                !assignable(targetType, rhsType))
                typeError(node, "cannot assign incompatible value to array element");
            result = targetType;
            node->arrayDimensions = node->left->arrayDimensions;
            break;
        }
        case NODE_TYPE::BINARY_EXPR: {
            ValueType a=checkExpr(node->left), b=checkExpr(node->right);
            const bool scalarOperands = node->left->arrayDimensions == 0 &&
                                        node->right->arrayDimensions == 0;
            const bool sameArrayShape = node->left->arrayDimensions > 0 &&
                                        node->right->arrayDimensions == node->left->arrayDimensions;
            const bool compatibleBaseTypes = a == b || (numeric(a) && numeric(b));

            if (node->op == "+" && scalarOperands && a == ValueType::STRING && b == ValueType::STRING)
                result = ValueType::STRING;
            else if (node->op == "+" && (a == ValueType::STRING || b == ValueType::STRING)) {
                typeError(node, "string concatenation requires two String operands");
                result = ValueType::INVALID;
            }
            else if(node->op=="==" || node->op=="!=") {
                const bool comparable = (scalarOperands || sameArrayShape) && compatibleBaseTypes;
                if (!comparable) {
                    typeError(node,"incompatible equality operands"); result=ValueType::BOOLEAN; }
                result = ValueType::BOOLEAN;
            }
            else if(node->op=="<"||node->op=="<="||node->op==">"||node->op==">=") {
                if(!scalarOperands || !numeric(a)||!numeric(b))
                    typeError(node,"comparison requires numeric operands");
                result=ValueType::BOOLEAN;
            }
            else {
                if(!scalarOperands || !numeric(a)||!numeric(b))
                    typeError(node,"arithmetic requires numeric operands");
                result=promoted(a,b);
            }
            node->arrayDimensions = 0;
            break;
        }
        default: 
            typeError(node,"invalid expression"); 
            break;
    }
    node->dataType=result;
    return result;
}

int Compiler::emit(Instruction instr) {
    int idx = static_cast<int>(code_.size());
    code_.push_back(move(instr));
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
        case NODE_TYPE::RETURN_STMT:   compileReturnStmt(node); break;
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
//  → compile expr (leaves value on stack) → STORE_LOCAL slot
void Compiler::compileDeclStmt(const ASTNode* node) {
    compileExpr(node->right);                         // evaluate initialiser
    if (node->arrayDimensions == 0)
        emitStorageConversion(node->dataType, node->right->dataType);
    emit(Instruction(OpCode::STORE_LOCAL, static_cast<int64_t>(node->localSlot)));
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
    if (node->left->dataType != ValueType::VOID)
        emit(Instruction(OpCode::POP));
} 

void Compiler::compileFunctionDecl(const ASTNode* node) {
    FunctionInfo& function = functionTable_.at(node->value);
    function.entryAddress = currentIndex();

    emit(Instruction(OpCode::ENTER_FRAME, static_cast<int64_t>(node->localSlot)));
    for (auto it = node->SUB_STATEMENTS.rbegin(); it != node->SUB_STATEMENTS.rend(); ++it)
        emit(Instruction(OpCode::STORE_LOCAL, static_cast<int64_t>((*it)->localSlot)));
    const ValueType savedReturnType = currentReturnType_;
    currentReturnType_ = node->dataType;
    compileNode(node->left);
    currentReturnType_ = savedReturnType;
    if (node->dataType == ValueType::VOID)
        emit(Instruction(OpCode::RETURN, int64_t(0)));
}

void Compiler::compileReturnStmt(const ASTNode* node) {
    if (node->left) {
        compileExpr(node->left);
        emitStorageConversion(currentReturnType_, node->left->dataType);
    }
    emit(Instruction(OpCode::RETURN, node->left ? int64_t(1) : int64_t(0)));
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
        cerr << "CompilerError: null expression node.\n";
        return;
    }

    switch (node->type) {
        // ── Literals ──────────────────────────────────────────────────────
        case NODE_TYPE::INT_LITERAL:
            emit(Instruction(OpCode::PUSH_INT,
                             static_cast<int64_t>(stoll(node->value))));
            break;

        case NODE_TYPE::LONG_LITERAL:
            emit(Instruction(OpCode::PUSH_INT,
                             static_cast<int64_t>(stoll(node->value))));
            break;

        case NODE_TYPE::FLOAT_LITERAL:
            emit(Instruction(OpCode::PUSH_FLOAT, stod(node->value)));
            break;

        case NODE_TYPE::DOUBLE_LITERAL:
            emit(Instruction(OpCode::PUSH_FLOAT, stod(node->value)));
            break;

        case NODE_TYPE::STRING_LITERAL:
            code_.stringConstants.push_back(node->value);
            emit(Instruction(OpCode::PUSH_STRING,
                             static_cast<int64_t>(code_.stringConstants.size() - 1)));
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
            emit(Instruction(OpCode::LOAD_LOCAL, static_cast<int64_t>(node->localSlot)));
            break;

        case NODE_TYPE::CALL_EXPR: {
            const FunctionInfo& function = functionTable_.at(node->value);
            for (size_t i = 0; i < node->SUB_STATEMENTS.size(); ++i) {
                const ASTNode* argument = node->SUB_STATEMENTS[i];
                compileExpr(argument);
                if (i < function.parameterTypes.size() && argument->arrayDimensions == 0)
                    emitStorageConversion(function.parameterTypes[i], argument->dataType);
            }
            emit(Instruction(OpCode::CALL, static_cast<int64_t>(function.entryAddress)));
            break;
        }

        // ── Input expression ──────────────────────────────────────────────
        case NODE_TYPE::INPUT_EXPR:
            code_.stringConstants.push_back(node->value);
            emit(Instruction(OpCode::INPUT,
                             static_cast<int64_t>(code_.stringConstants.size() - 1)));
            break;

        case NODE_TYPE::ARRAY_LITERAL:
            for (const ASTNode* element : node->SUB_STATEMENTS) compileExpr(element);
            emit(Instruction(OpCode::BUILD_ARRAY, static_cast<int64_t>(node->SUB_STATEMENTS.size())));
            break;

        case NODE_TYPE::ARRAY_ACCESS:
            compileArrayAccess(node);
            break;

        // ── Assignment expression  id = expr ─────────────────────────────
        case NODE_TYPE::ASSIGN_EXPR:
            compileExpr(node->right);
            emitStorageConversion(node->dataType, node->right->dataType);
            emit(Instruction(OpCode::STORE_LOCAL, static_cast<int64_t>(node->localSlot)));
            // Assignment leaves a value on the stack (it's an expression)
            emit(Instruction(OpCode::LOAD_LOCAL, static_cast<int64_t>(node->localSlot)));
            break;

        case NODE_TYPE::ARRAY_ASSIGN_EXPR:
            compileArrayAssign(node);
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
            cerr << "CompilerError: unexpected expression node type "
                      << static_cast<int>(node->type) << ".\n";
            break;
    }
}

void Compiler::compileArrayAccess(const ASTNode* node) {
    compileExpr(node->left);
    compileExpr(node->right);
    emit(Instruction(OpCode::LOAD_ARRAY_ELEMENT));
}

void Compiler::compileArrayAssign(const ASTNode* node) {
    const ASTNode* access = node->left;
    compileExpr(access->left);
    compileExpr(access->right);
    compileExpr(node->right);
    if (node->arrayDimensions == 0)
        emitStorageConversion(node->dataType, node->right->dataType);
    emit(Instruction(OpCode::STORE_ARRAY_ELEMENT));
}

void Compiler::compileBinaryExpr(const ASTNode* node) {
    const bool numericOperands = node->left && node->right &&
        numeric(node->left->dataType) && numeric(node->right->dataType);

    compileExpr(node->left);
    // CHAR remains a distinct runtime value for storage and text operations.
    // Numeric operators normalize it to the canonical INT runtime representation,
    // which keeps future typed integer opcodes free of CHAR-specific variants.
    if (numericOperands && node->left->dataType == ValueType::CHAR)
        emit(Instruction(OpCode::CONVERT, static_cast<int64_t>(ValueType::INT)));

    compileExpr(node->right);
    if (numericOperands && node->right->dataType == ValueType::CHAR)
        emit(Instruction(OpCode::CONVERT, static_cast<int64_t>(ValueType::INT)));

    const string& op = node->op;

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
        cerr << "CompilerError: unknown binary operator '" << op << "'.\n";
    }
}

void Compiler::disassemble(const Bytecode& code) {
    cout << "\n=== BYTECODE DISASSEMBLY ===\n";
    for (int i = 0; i < static_cast<int>(code.size()); ++i) 
    {
        const Instruction& instr = code[i];
        cout << setw(4) << i << "  " << left
                  << setw(16) << opCodeName(instr.op);

        // Print operand if meaningful
        if (holds_alternative<int64_t>(instr.operand)) 
        {
            int64_t v = get<int64_t>(instr.operand);
            if (v != 0 || instr.op == OpCode::PUSH_INT ||
                          instr.op == OpCode::PUSH_BOOL ||
                          instr.op == OpCode::PUSH_STRING ||
                          instr.op == OpCode::JUMP      ||
                          instr.op == OpCode::JUMP_IF_FALSE ||
                          instr.op == OpCode::CALL ||
                          instr.op == OpCode::RETURN ||
                          instr.op == OpCode::ENTER_FRAME ||
                          instr.op == OpCode::LOAD_LOCAL ||
                          instr.op == OpCode::STORE_LOCAL ||
                          instr.op == OpCode::INPUT)
                cout << v;

            if (instr.op == OpCode::PUSH_STRING || instr.op == OpCode::INPUT) {
                if (v >= 0 && static_cast<size_t>(v) < code.stringConstants.size())
                    cout << "   ; \"" << code.stringConstants[static_cast<size_t>(v)] << '\"';
                else
                    cout << "   ; <invalid string constant>";
            }
        } 
        else if (holds_alternative<double>(instr.operand)) 
        {
            cout << get<double>(instr.operand);
        } 
        else if (holds_alternative<char16_t>(instr.operand)) 
        {
            cout << "'" << static_cast<char>(get<char16_t>(instr.operand)) << "'";
        } 
        cout << "\n";
    }
    cout << "============================\n\n";
}
