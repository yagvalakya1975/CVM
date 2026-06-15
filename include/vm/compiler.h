#pragma once
#include "../parser/parser.h"
#include "bytecode.h"


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
};