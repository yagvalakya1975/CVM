#pragma once
#include "parser/parser.h"
#include "vm/bytecode.h"
#include <unordered_map>
#include <vector>

using namespace std;


class Compiler {
public:

    Bytecode compile(const ASTNode* root);
    static void disassemble(const Bytecode& code);

private:
    Bytecode code_;
    void compileNode(const ASTNode* node);
    void compileDeclStmt   (const ASTNode* node);
    void compilePrintStmt  (const ASTNode* node);
    void compileExprStmt   (const ASTNode* node);
    void compileIfStmt     (const ASTNode* node);
    void compileWhileStmt  (const ASTNode* node);
    void compileFunctionDecl(const ASTNode* node);
    void compileReturnStmt(const ASTNode* node);
    void compileBlock      (const ASTNode* node);
    void compileExpr       (const ASTNode* node);
    void compileBinaryExpr (const ASTNode* node);
    void emitStorageConversion(ValueType target, ValueType source);
    int  emit(Instruction instr);         
    void patchJump(int instrIdx, int target); 
    int  currentIndex() const;             
    struct VariableInfo { ValueType type; int arrayDimensions; int slot; };
    using Scope = unordered_map<string, VariableInfo>;
    struct FunctionInfo {
        ValueType returnType = ValueType::INVALID;
        vector<ValueType> parameterTypes;
        int entryAddress = -1;
    };
    vector<Scope> scopes_;
    unordered_map<string, FunctionInfo> functionTable_;
    int nextSlot_ = 0;
    int topFrameSize_ = 0;
    ValueType currentReturnType_ = ValueType::INVALID;
    bool hadError_ = false;
    ValueType checkNode(const ASTNode* node);
    ValueType checkExpr(const ASTNode* node);
    bool assignable(ValueType target, ValueType source) const;
    bool numeric(ValueType type) const;
    ValueType promoted(ValueType a, ValueType b) const;
    void typeError(const ASTNode* node, const string& message);
    VariableInfo* resolve(const string& name);
};
