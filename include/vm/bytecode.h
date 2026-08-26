#pragma once
#include <string>
#include <variant>
#include <vector>
#include <cstdint>


enum class OpCode : uint8_t {
    PUSH_INT,       // operand: int constant
    PUSH_FLOAT,     // operand: double  constant
    PUSH_STRING,    // operand: string  constant
    PUSH_CHAR,      // operand: string  constant (single char)
    PUSH_BOOL,      // operand: int  0=false 1=true
    LOAD_VAR,       // operand: variable name  → push value on stack
    STORE_VAR,      // operand: variable name  ← pop value from stack
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    CMP_EQ,
    CMP_NEQ,
    CMP_LT,
    CMP_LE,
    CMP_GT,
    CMP_GE,
    JUMP,           // operand: absolute instruction index
    JUMP_IF_FALSE,  // operand: absolute instruction index; pops condition
    PRINT,          // pop top of stack, print it
    INPUT,          // operand: prompt string; push string result
    CONVERT,        // operand: ValueType numeric code; convert top stack value
    POP,            // discard top of stack
    HALT
};

inline std::string opCodeName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:      return "PUSH_INT";
        case OpCode::PUSH_FLOAT:    return "PUSH_FLOAT";
        case OpCode::PUSH_STRING:   return "PUSH_STRING";
        case OpCode::PUSH_CHAR:     return "PUSH_CHAR";
        case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
        case OpCode::LOAD_VAR:      return "LOAD_VAR";
        case OpCode::STORE_VAR:     return "STORE_VAR";
        case OpCode::ADD:           return "ADD";
        case OpCode::SUB:           return "SUB";
        case OpCode::MUL:           return "MUL";
        case OpCode::DIV:           return "DIV";
        case OpCode::MOD:           return "MOD";
        case OpCode::CMP_EQ:        return "CMP_EQ";
        case OpCode::CMP_NEQ:       return "CMP_NEQ";
        case OpCode::CMP_LT:        return "CMP_LT";
        case OpCode::CMP_LE:        return "CMP_LE";
        case OpCode::CMP_GT:        return "CMP_GT";
        case OpCode::CMP_GE:        return "CMP_GE";
        case OpCode::JUMP:          return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::INPUT:         return "INPUT";
        case OpCode::CONVERT:       return "CONVERT";
        case OpCode::POP:           return "POP";
        case OpCode::HALT:          return "HALT";
        default:                    return "???";
    }
}


using Value = std::variant<int64_t, double, char16_t, std::string>;


struct Instruction {
    OpCode  op;
    Value   operand;
    explicit Instruction(OpCode o)                : op(o), operand(int64_t(0)) {}
    Instruction(OpCode o, int64_t v)              : op(o), operand(v) {}
    Instruction(OpCode o, double  v)              : op(o), operand(v) {}
    Instruction(OpCode o, char16_t v)             : op(o), operand(v) {}
    Instruction(OpCode o, std::string v)          : op(o), operand(std::move(v)) {}
};

using Bytecode = std::vector<Instruction>;
