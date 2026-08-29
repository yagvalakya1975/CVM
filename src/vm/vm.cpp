#include "../../include/vm/vm.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void VM::push(Value v) { stack_.push_back(std::move(v)); }
Value VM::pop() {
    if (stack_.empty()) throw std::runtime_error("VMError: stack underflow");
    Value v = std::move(stack_.back()); 
    stack_.pop_back(); 
    return v;
}
const Value& VM::peek() const {
    if (stack_.empty()) throw std::runtime_error("VMError: peek on empty stack");
    return stack_.back();
}

std::string VM::valueToString(const Value& v) {
    switch (v.tag()) {
        case ValueTag::INT: return std::to_string(v.asInt());
        case ValueTag::FLOAT: {
            std::string text = std::to_string(v.asFloat());
            text.erase(text.find_last_not_of('0') + 1, std::string::npos);
            if (text.back() == '.') text += '0';
            return text;
        }
        case ValueTag::BOOL: return v.asBool() ? "true" : "false";
        case ValueTag::CHAR: return std::string(1, static_cast<char>(v.asChar()));
        case ValueTag::STRING: return v.asString();
        case ValueTag::ARRAY: {
            const ArrayPtr& array = v.asArray();
            if (!array) throw std::runtime_error("VMError: null array reference");
            std::string result = "[";
            for (size_t i = 0; i < array->elements.size(); ++i) {
                if (i > 0) result += ", ";
                result += valueToString(array->elements[i]);
            }
            return result + "]";
        }
        case ValueTag::REFERENCE: throw std::runtime_error("VMError: unsupported generic reference value");
    }
    throw std::runtime_error("VMError: unknown value tag");
}

bool VM::isTruthy(const Value& v) { return v.asBool(); }

namespace {
bool isIntegralNumeric(const Value& value) {
    return value.tag() == ValueTag::INT || value.tag() == ValueTag::CHAR;
}
int64_t integralValue(const Value& value) {
    if (value.tag() == ValueTag::INT) return value.asInt();
    if (value.tag() == ValueTag::CHAR) return static_cast<int64_t>(value.asChar());
    throw std::runtime_error(std::string("VMError: expected numeric value, got ") + valueTagName(value.tag()));
}
double numericValue(const Value& value) {
    switch (value.tag()) {
        case ValueTag::INT: return static_cast<double>(value.asInt());
        case ValueTag::FLOAT: return value.asFloat();
        case ValueTag::CHAR: return static_cast<double>(value.asChar());
        default: throw std::runtime_error(std::string("VMError: expected numeric value, got ") + valueTagName(value.tag()));
    }
}
bool numericPair(const Value& lhs, const Value& rhs) { return lhs.isNumeric() && rhs.isNumeric(); }
[[noreturn]] void invalidOperands(const std::string& op, const Value& lhs, const Value& rhs) {
    throw std::runtime_error("VMError: operator '" + op + "' does not support " +
                             valueTagName(lhs.tag()) + " and " + valueTagName(rhs.tag()));
}
}

Value VM::applyBinOp(const std::string& op, Value lhs, Value rhs) {
    if (op == "+" && (lhs.tag() == ValueTag::STRING || rhs.tag() == ValueTag::STRING))
        return Value::string(valueToString(lhs) + valueToString(rhs));

    if (op == "==" || op == "!=") {
        bool equal = false;
        if (numericPair(lhs, rhs)) equal = numericValue(lhs) == numericValue(rhs);
        else if (lhs.tag() == ValueTag::STRING && rhs.tag() == ValueTag::STRING) equal = lhs.asString() == rhs.asString();
        else if (lhs.tag() == ValueTag::BOOL && rhs.tag() == ValueTag::BOOL) equal = lhs.asBool() == rhs.asBool();
        else if (lhs.tag() == ValueTag::ARRAY && rhs.tag() == ValueTag::ARRAY) equal = lhs.asArray() == rhs.asArray();
        else invalidOperands(op, lhs, rhs);
        return Value::boolean(op == "==" ? equal : !equal);
    }
    if (!numericPair(lhs, rhs)) invalidOperands(op, lhs, rhs);
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        const double a = numericValue(lhs), b = numericValue(rhs);
        if (op == "<") return Value::boolean(a < b);
        if (op == "<=") return Value::boolean(a <= b);
        if (op == ">") return Value::boolean(a > b);
        return Value::boolean(a >= b);
    }
    if (isIntegralNumeric(lhs) && isIntegralNumeric(rhs)) {
        const int64_t a = integralValue(lhs), b = integralValue(rhs);
        if (op == "+") return Value::integer(a + b);
        if (op == "-") return Value::integer(a - b);
        if (op == "*") return Value::integer(a * b);
        if (op == "/") 
        { 
            if (b == 0) throw std::runtime_error("VMError: division by zero"); 
            return Value::integer(a / b); 
        }
        if (op == "%") 
        { 
            if (b == 0) throw std::runtime_error("VMError: modulo by zero"); 
            return Value::integer(a % b); 
        }
    }
    const double a = numericValue(lhs), b = numericValue(rhs);
    if (op == "+") return Value::floating(a + b);
    if (op == "-") return Value::floating(a - b);
    if (op == "*") return Value::floating(a * b);
    if (op == "/") { if (b == 0.0) 
        throw std::runtime_error("VMError: division by zero"); 
    return Value::floating(a / b); }
    if (op == "%") { if (b == 0.0) 
        throw std::runtime_error("VMError: modulo by zero"); 
    return Value::floating(std::fmod(a, b)); }
    throw std::runtime_error("VMError: unknown operator '" + op + "'");
}

static std::string opCodeToOp(OpCode op) {
    switch (op) {
        case OpCode::ADD: return "+"; 
        case OpCode::SUB: return "-"; 
        case OpCode::MUL: return "*";
        case OpCode::DIV: return "/"; 
        case OpCode::MOD: return "%"; 
        case OpCode::CMP_EQ: return "==";
        case OpCode::CMP_NEQ: return "!="; 
        case OpCode::CMP_LT: return "<";
        case OpCode::CMP_LE: return "<=";
        case OpCode::CMP_GT: return ">"; 
        case OpCode::CMP_GE: return ">="; 
        default: return "?";
    }
}

int VM::run(const Bytecode& code) {
    stack_.clear(); 
    locals_.clear(); 
    initializedLocals_.clear();
    int ip = 0;
    try {
        while (ip < static_cast<int>(code.size())) {
            const Instruction& instr = code[ip];
            switch (instr.op) {
                case OpCode::PUSH_INT: push(Value::integer(std::get<int64_t>(instr.operand))); break;
                case OpCode::PUSH_FLOAT: push(Value::floating(std::get<double>(instr.operand))); break;
                case OpCode::PUSH_STRING: push(Value::string(std::get<std::string>(instr.operand))); break;
                case OpCode::PUSH_CHAR: push(Value::character(std::get<char16_t>(instr.operand))); break;
                case OpCode::PUSH_BOOL: push(Value::boolean(std::get<int64_t>(instr.operand) != 0)); break;
                case OpCode::BUILD_ARRAY: 
                {
                    const int64_t count = std::get<int64_t>(instr.operand);
                    if (count < 0 || static_cast<size_t>(count) > stack_.size()) throw std::runtime_error("VMError: invalid array element count");
                    auto array = std::make_shared<Array>();
                    array->elements.resize(static_cast<size_t>(count), Value::integer(0));
                    for (int64_t i = count - 1; i >= 0; --i) 
                        array->elements[static_cast<size_t>(i)] = pop();
                    push(Value::array(std::move(array))); 
                    break;
                }
                case OpCode::LOAD_LOCAL: {
                    const int64_t slot = std::get<int64_t>(instr.operand);
                    if (slot < 0 || static_cast<size_t>(slot) >= locals_.size() || !initializedLocals_[static_cast<size_t>(slot)])
                        throw std::runtime_error("VMError: invalid or uninitialized local slot " + std::to_string(slot));
                    push(locals_[static_cast<size_t>(slot)]); 
                    break;
                }
                case OpCode::STORE_LOCAL: {
                    const int64_t slot = std::get<int64_t>(instr.operand);
                    if (slot < 0) throw std::runtime_error("VMError: invalid local slot " + std::to_string(slot));
                    const size_t index = static_cast<size_t>(slot);
                    if (index >= locals_.size()) 
                    { 
                        locals_.resize(index + 1, Value::integer(0)); 
                        initializedLocals_.resize(index + 1, false); 
                    }
                    locals_[index] = pop(); 
                    initializedLocals_[index] = true; 
                    break;
                }
                case OpCode::ADD:
                case OpCode::SUB: 
                case OpCode::MUL: 
                case OpCode::DIV: 
                case OpCode::MOD:
                case OpCode::CMP_EQ: 
                case OpCode::CMP_NEQ: 
                case OpCode::CMP_LT: 
                case OpCode::CMP_LE: 
                case OpCode::CMP_GT: 
                case OpCode::CMP_GE: 
                {
                    Value rhs = pop(), lhs = pop(); 
                    push(applyBinOp(opCodeToOp(instr.op), 
                    std::move(lhs), 
                    std::move(rhs))); 
                    break;
                }
                case OpCode::JUMP: 
                    ip = static_cast<int>(std::get<int64_t>(instr.operand)); 
                    continue;
                case OpCode::JUMP_IF_FALSE:
                    if (!isTruthy(pop())) 
                    { 
                        ip = static_cast<int>(std::get<int64_t>(instr.operand)); 
                        continue; 
                    }
                    break;
                case OpCode::PRINT: 
                    std::cout << valueToString(pop()) << "\n"; 
                    break;
                case OpCode::INPUT: 
                { 
                    std::cout << std::get<std::string>(instr.operand); 
                    std::string line; std::getline(std::cin, line); 
                    push(Value::string(std::move(line))); 
                    break; 
                }
                case OpCode::CONVERT: 
                {
                    Value value = pop(); 
                    const int target = static_cast<int>(std::get<int64_t>(instr.operand)); 
                    const double number = numericValue(value);
                    if (target >= 1 && target <= 4) push(Value::integer(static_cast<int64_t>(number)));
                    else if (target == 5 || target == 6) push(Value::floating(number));
                    else if (target == 7) push(Value::character(static_cast<char16_t>(static_cast<int64_t>(number))));
                    else throw std::runtime_error("VMError: invalid cast target");
                    break;
                }
                case OpCode::POP: pop(); break;
                case OpCode::HALT: return 0;
                default: throw std::runtime_error("VMError: unknown opcode " + std::to_string(static_cast<int>(instr.op)));
            }
            ++ip;
        }
    } 
    catch (const std::exception& e) 
    { 
        std::cerr << e.what() << "\n"; 
        return 1; 
    }
    return 0;
}
