#include "../../include/vm/vm.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

void VM::push(Value v) {
    stack_.push_back(std::move(v));
}

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
    if (std::holds_alternative<int64_t>(v))    return std::to_string(std::get<int64_t>(v));
    if (std::holds_alternative<double>(v)) {
        // Trim trailing zeros for cleaner output
        std::string s = std::to_string(std::get<double>(v));
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s += '0';
        return s;
    }
    return std::get<std::string>(v);
}

bool VM::isTruthy(const Value& v) {
    if (std::holds_alternative<int64_t>(v))    return std::get<int64_t>(v) != 0;
    if (std::holds_alternative<double>(v))     return std::get<double>(v) != 0.0;
    return !std::get<std::string>(v).empty();
}

Value VM::applyBinOp(const std::string& op, Value lhs, Value rhs) {
    // String concatenation with +
    if (op == "+" &&
        (std::holds_alternative<std::string>(lhs) ||
         std::holds_alternative<std::string>(rhs)))
    {
        return valueToString(lhs) + valueToString(rhs);
    }

    // If both are int, stay int
    if (std::holds_alternative<int64_t>(lhs) &&
        std::holds_alternative<int64_t>(rhs))
    {
        int64_t a = std::get<int64_t>(lhs);
        int64_t b = std::get<int64_t>(rhs);

        if (op == "+")  return a + b;
        if (op == "-")  return a - b;
        if (op == "*")  return a * b;
        if (op == "/") {
            if (b == 0) throw std::runtime_error("VMError: division by zero");
            return a / b;
        }
        if (op == "%") {
            if (b == 0) throw std::runtime_error("VMError: modulo by zero");
            return a % b;
        }
        // Comparison → result is int 0/1
        if (op == "==") return int64_t(a == b);
        if (op == "!=") return int64_t(a != b);
        if (op == "<")  return int64_t(a <  b);
        if (op == "<=") return int64_t(a <= b);
        if (op == ">")  return int64_t(a >  b);
        if (op == ">=") return int64_t(a >= b);
    }

    // Promote to double when one side is float
    auto toDouble = [](const Value& v) -> double {
        if (std::holds_alternative<int64_t>(v)) return static_cast<double>(std::get<int64_t>(v));
        if (std::holds_alternative<double>(v))  return std::get<double>(v);
        throw std::runtime_error("VMError: cannot coerce string to number");
    };

    double a = toDouble(lhs);
    double b = toDouble(rhs);

    if (op == "+")  return a + b;
    if (op == "-")  return a - b;
    if (op == "*")  return a * b;
    if (op == "/") {
        if (b == 0.0) throw std::runtime_error("VMError: division by zero");
        return a / b;
    }
    if (op == "%")  return std::fmod(a, b);

    // Comparison
    if (op == "==") return int64_t(a == b);
    if (op == "!=") return int64_t(a != b);
    if (op == "<")  return int64_t(a <  b);
    if (op == "<=") return int64_t(a <= b);
    if (op == ">")  return int64_t(a >  b);
    if (op == ">=") return int64_t(a >= b);

    throw std::runtime_error("VMError: unknown operator '" + op + "'");
}

static std::string opCodeToOp(OpCode op) {
    switch (op) {
        case OpCode::ADD:     return "+";
        case OpCode::SUB:     return "-";
        case OpCode::MUL:     return "*";
        case OpCode::DIV:     return "/";
        case OpCode::MOD:     return "%";
        case OpCode::CMP_EQ:  return "==";
        case OpCode::CMP_NEQ: return "!=";
        case OpCode::CMP_LT:  return "<";
        case OpCode::CMP_LE:  return "<=";
        case OpCode::CMP_GT:  return ">";
        case OpCode::CMP_GE:  return ">=";
        default:              return "?";
    }
}

int VM::run(const Bytecode& code) {
    stack_.clear();
    vars_.clear();

    int ip = 0; // instruction pointer
    const int codeSize = static_cast<int>(code.size());

    try {
        while (ip < codeSize) {
            const Instruction& instr = code[ip];

            switch (instr.op) {

                case OpCode::PUSH_INT:
                    push(std::get<int64_t>(instr.operand));
                    break;

                case OpCode::PUSH_FLOAT:
                    push(std::get<double>(instr.operand));
                    break;

                case OpCode::PUSH_STRING:
                case OpCode::PUSH_CHAR:
                    push(std::get<std::string>(instr.operand));
                    break;

                case OpCode::PUSH_BOOL:
                    push(std::get<int64_t>(instr.operand)); // 0 or 1
                    break;

                case OpCode::LOAD_VAR: {
                    const std::string& name = std::get<std::string>(instr.operand);
                    auto it = vars_.find(name);
                    if (it == vars_.end())
                        throw std::runtime_error(
                            "VMError: undefined variable '" + name + "'");
                    push(it->second);
                    break;
                }

                case OpCode::STORE_VAR: {
                    const std::string& name = std::get<std::string>(instr.operand);
                    vars_[name] = pop();
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
                case OpCode::CMP_GE: {
                    Value rhs = pop();
                    Value lhs = pop();
                    push(applyBinOp(opCodeToOp(instr.op), std::move(lhs), std::move(rhs)));
                    break;
                }

                case OpCode::JUMP: {
                    int target = static_cast<int>(std::get<int64_t>(instr.operand));
                    ip = target;
                    continue; // skip ip++ below
                }

                case OpCode::JUMP_IF_FALSE: {
                    Value cond = pop();
                    if (!isTruthy(cond)) {
                        int target = static_cast<int>(std::get<int64_t>(instr.operand));
                        ip = target;
                        continue; // skip ip++ below
                    }
                    break;
                }

                case OpCode::PRINT: {
                    Value v = pop();
                    std::cout << valueToString(v) << "\n";
                    break;
                }

                case OpCode::INPUT: {
                    const std::string& prompt = std::get<std::string>(instr.operand);
                    std::cout << prompt;
                    std::string line;
                    std::getline(std::cin, line);
                    push(line);
                    break;
                }

                case OpCode::POP:
                    pop();
                    break;

                case OpCode::HALT:
                    return 0;

                default:
                    throw std::runtime_error(
                        "VMError: unknown opcode " +
                        std::to_string(static_cast<int>(instr.op)));
            }

            ++ip;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}