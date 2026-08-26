#pragma once
#include "../parser/parser.h"
#include "bytecode.h"
#include <unordered_map>


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
    void compileBlock      (const ASTNode* node);
    void compileExpr       (const ASTNode* node);
    void compileBinaryExpr (const ASTNode* node);
    int  emit(Instruction instr);         
    void patchJump(int instrIdx, int target); 
    int  currentIndex() const;             
    struct VariableInfo { ValueType type; int arrayDimensions; };
    std::unordered_map<std::string, VariableInfo> variables_;
    bool hadError_ = false;
    ValueType checkNode(const ASTNode* node);
    ValueType checkExpr(const ASTNode* node);
    bool assignable(ValueType target, ValueType source) const;
    bool numeric(ValueType type) const;
    ValueType promoted(ValueType a, ValueType b) const;
    void typeError(const ASTNode* node, const std::string& message);
};
