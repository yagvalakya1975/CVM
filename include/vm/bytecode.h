#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>

using namespace std;

// Bytecode operands describe encoded instruction data, not VM runtime values.
using Operand = variant<int64_t, double, char16_t>;


enum class OpCode : uint8_t {
    PUSH_INT,       // operand: int constant
    PUSH_FLOAT,     // operand: double  constant
    PUSH_STRING,    // operand: string constant pool index
    PUSH_CHAR,      // operand: string  constant (single char)
    PUSH_BOOL,      // operand: int  0=false 1=true
    BUILD_ARRAY,    // operand: element count
    LOAD_ARRAY_ELEMENT,  // pop index and array, then push selected element
    STORE_ARRAY_ELEMENT, // pop value, index, and array; update element then push value
    LOAD_LOCAL,     // operand: numeric local slot → push value on stack
    STORE_LOCAL,    // operand: numeric local slot ← pop value from stack
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
    CALL,           // operand: absolute instruction index of function entry
    RETURN,         // operand: 1 if returning a value, 0 otherwise
    ENTER_FRAME,    // operand: local slot count for this frame
    PRINT,          // pop top of stack, print it
    INPUT,          // operand: prompt string constant pool index; push string result
    CONVERT,        // operand: ValueType numeric code; convert top stack value
    POP,            // discard top of stack
    HALT
};

inline string opCodeName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:      return "PUSH_INT";
        case OpCode::PUSH_FLOAT:    return "PUSH_FLOAT";
        case OpCode::PUSH_STRING:   return "PUSH_STRING";
        case OpCode::PUSH_CHAR:     return "PUSH_CHAR";
        case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
        case OpCode::BUILD_ARRAY:   return "BUILD_ARRAY";
        case OpCode::LOAD_ARRAY_ELEMENT: return "LOAD_ARRAY_ELEMENT";
        case OpCode::STORE_ARRAY_ELEMENT: return "STORE_ARRAY_ELEMENT";
        case OpCode::LOAD_LOCAL:    return "LOAD_LOCAL";
        case OpCode::STORE_LOCAL:   return "STORE_LOCAL";
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
        case OpCode::CALL:          return "CALL";
        case OpCode::RETURN:        return "RETURN";
        case OpCode::ENTER_FRAME:   return "ENTER_FRAME";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::INPUT:         return "INPUT";
        case OpCode::CONVERT:       return "CONVERT";
        case OpCode::POP:           return "POP";
        case OpCode::HALT:          return "HALT";
        default:                    return "???";
    }
}


struct Instruction {
    OpCode  op;
    Operand operand;
    explicit Instruction(OpCode o)                : op(o), operand(int64_t(0)) {}
    Instruction(OpCode o, int64_t v)              : op(o), operand(v) {}
    Instruction(OpCode o, double  v)              : op(o), operand(v) {}
    Instruction(OpCode o, char16_t v)             : op(o), operand(v) {}
};

struct Bytecode {
    vector<Instruction> instructions;
    vector<string> stringConstants;

    Bytecode() = default;
    Bytecode(initializer_list<Instruction> initialInstructions)
        : instructions(initialInstructions) {}

    bool empty() const { return instructions.empty(); }
    size_t size() const { return instructions.size(); }
    void clear() { instructions.clear(); stringConstants.clear(); }
    void push_back(Instruction instruction) { instructions.push_back(move(instruction)); }

    Instruction& operator[](size_t index) { return instructions[index]; }
    const Instruction& operator[](size_t index) const { return instructions[index]; }
    auto begin() { return instructions.begin(); }
    auto end() { return instructions.end(); }
    auto begin() const { return instructions.begin(); }
    auto end() const { return instructions.end(); }
};
