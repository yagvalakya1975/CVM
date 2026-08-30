#include "vm/vm.h"
#include "vm/vm_error.h"
#include "parser/parser.h"
#include <cmath>
#include <iostream>

using namespace std;

void VM::push(Value v) { stack_.push_back(move(v)); }
Value VM::pop() {
    if (stack_.empty()) throw VMError(VMErrorKind::StackUnderflow, "stack underflow");
    Value v = move(stack_.back()); 
    stack_.pop_back(); 
    return v;
}
const Value& VM::peek() const {
    if (stack_.empty()) throw VMError(VMErrorKind::StackUnderflow, "peek on empty stack");
    return stack_.back();
}

string VM::valueToString(const Value& v) {
    switch (v.tag()) {
        case ValueTag::INT: return to_string(v.asInt());
        case ValueTag::FLOAT: {
            string text = to_string(v.asFloat());
            text.erase(text.find_last_not_of('0') + 1, string::npos);
            if (text.back() == '.') text += '0';
            return text;
        }
        case ValueTag::BOOL: return v.asBool() ? "true" : "false";
        case ValueTag::CHAR: return string(1, static_cast<char>(v.asChar()));
        case ValueTag::STRING: return v.asString();
        case ValueTag::ARRAY: {
            const ArrayPtr& array = v.asArray();
            if (!array) throw VMError(VMErrorKind::InvalidOperand, "null array reference");
            string result = "[";
            for (size_t i = 0; i < array->elements.size(); ++i) {
                if (i > 0) result += ", ";
                result += valueToString(array->elements[i]);
            }
            return result + "]";
        }
    }
    throw VMError(VMErrorKind::InvalidOperand, "unknown value tag");
}

bool VM::isTruthy(const Value& v) { return v.asBool(); }

namespace {
bool isIntegralNumeric(const Value& value) {
    return value.tag() == ValueTag::INT || value.tag() == ValueTag::CHAR;
}
int64_t integralValue(const Value& value) {
    if (value.tag() == ValueTag::INT) return value.asInt();
    if (value.tag() == ValueTag::CHAR) return static_cast<int64_t>(value.asChar());
    throw VMError(VMErrorKind::TypeMismatch, string("expected numeric value, got ") + valueTagName(value.tag()));
}
double numericValue(const Value& value) {
    switch (value.tag()) {
        case ValueTag::INT: return static_cast<double>(value.asInt());
        case ValueTag::FLOAT: return value.asFloat();
        case ValueTag::CHAR: return static_cast<double>(value.asChar());
        default: throw VMError(VMErrorKind::TypeMismatch, string("expected numeric value, got ") + valueTagName(value.tag()));
    }
}
bool numericPair(const Value& lhs, const Value& rhs) { return lhs.isNumeric() && rhs.isNumeric(); }
[[noreturn]] void invalidOperands(const string& op, const Value& lhs, const Value& rhs) {
    throw VMError(VMErrorKind::TypeMismatch, "operator '" + op + "' does not support " +
                             valueTagName(lhs.tag()) + " and " + valueTagName(rhs.tag()));
}

template <typename T>
const T& operandAs(const Instruction& instr) {
    if (const T* operand = get_if<T>(&instr.operand)) return *operand;
    throw VMError(VMErrorKind::MalformedBytecode,
                  "malformed operand for opcode " + opCodeName(instr.op));
}

int checkedJumpTarget(const Instruction& instr, size_t codeSize) {
    const int64_t target = operandAs<int64_t>(instr);
    if (target < 0 || static_cast<uint64_t>(target) >= codeSize)
        throw VMError(VMErrorKind::InvalidOperand,
                      "invalid jump target " + to_string(target));
    return static_cast<int>(target);
}
}

Value VM::applyBinOp(const string& op, Value lhs, Value rhs) {
    if (op == "+" && lhs.tag() == ValueTag::STRING && rhs.tag() == ValueTag::STRING)
        return Value::string(lhs.asString() + rhs.asString());

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
            if (b == 0) throw VMError(VMErrorKind::DivisionByZero, "division by zero"); 
            return Value::integer(a / b); 
        }
        if (op == "%") 
        { 
            if (b == 0) throw VMError(VMErrorKind::DivisionByZero, "modulo by zero"); 
            return Value::integer(a % b); 
        }
    }
    const double a = numericValue(lhs), b = numericValue(rhs);
    if (op == "+") return Value::floating(a + b);
    if (op == "-") return Value::floating(a - b);
    if (op == "*") return Value::floating(a * b);
    if (op == "/") { if (b == 0.0) 
        throw VMError(VMErrorKind::DivisionByZero, "division by zero"); 
    return Value::floating(a / b); }
    if (op == "%") { if (b == 0.0) 
        throw VMError(VMErrorKind::DivisionByZero, "modulo by zero"); 
    return Value::floating(fmod(a, b)); }
    throw VMError(VMErrorKind::TypeMismatch, "unknown operator '" + op + "'");
}

static string opCodeToOp(OpCode op) {
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
    frames_.clear();
    currentBase_ = 0;
    currentFrameSize_ = 0;
    int ip = 0;
    while (ip < static_cast<int>(code.size())) {
            const Instruction& instr = code[ip];
            switch (instr.op) {
                case OpCode::PUSH_INT: push(Value::integer(operandAs<int64_t>(instr))); break;
                case OpCode::PUSH_FLOAT: push(Value::floating(operandAs<double>(instr))); break;
                case OpCode::PUSH_STRING: {
                    const int64_t index = operandAs<int64_t>(instr);
                    if (index < 0 || static_cast<size_t>(index) >= code.stringConstants.size())
                        throw VMError(VMErrorKind::InvalidOperand, "invalid string constant index " + to_string(index));
                    push(Value::string(code.stringConstants[static_cast<size_t>(index)]));
                    break;
                }
                case OpCode::PUSH_CHAR: push(Value::character(operandAs<char16_t>(instr))); break;
                case OpCode::PUSH_BOOL: push(Value::boolean(operandAs<int64_t>(instr) != 0)); break;
                case OpCode::BUILD_ARRAY: 
                {
                    const int64_t count = operandAs<int64_t>(instr);
                    if (count < 0 || static_cast<size_t>(count) > stack_.size()) throw VMError(VMErrorKind::InvalidOperand, "invalid array element count");
                    auto array = make_shared<Array>();
                    array->elements.resize(static_cast<size_t>(count), Value::integer(0));
                    for (int64_t i = count - 1; i >= 0; --i) 
                        array->elements[static_cast<size_t>(i)] = pop();
                    push(Value::array(move(array))); 
                    break;
                }
                case OpCode::LOAD_LOCAL: {
                    const int64_t slot = operandAs<int64_t>(instr);
                    const size_t index = slot < 0 ? 0 : currentBase_ + static_cast<size_t>(slot);
                    if (slot < 0 || index >= locals_.size())
                        throw VMError(VMErrorKind::InvalidOperand, "invalid or uninitialized local slot " + to_string(slot));
                    if (!initializedLocals_[index])
                        throw VMError(VMErrorKind::UninitializedLocal, "invalid or uninitialized local slot " + to_string(slot));
                    push(locals_[index]);
                    break;
                }
                case OpCode::STORE_LOCAL: {
                    const int64_t slot = operandAs<int64_t>(instr);
                    if (slot < 0) throw VMError(VMErrorKind::InvalidOperand, "invalid local slot " + to_string(slot));
                    const size_t index = currentBase_ + static_cast<size_t>(slot);
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
                    move(lhs), 
                    move(rhs))); 
                    break;
                }
                case OpCode::JUMP: 
                    ip = checkedJumpTarget(instr, code.size());
                    continue;
                case OpCode::JUMP_IF_FALSE: {
                    const int jumpTarget = checkedJumpTarget(instr, code.size());
                    if (!isTruthy(pop())) 
                    { 
                        ip = jumpTarget;
                        continue; 
                    }
                    break;
                }
                case OpCode::ENTER_FRAME: {
                    const int64_t size = operandAs<int64_t>(instr);
                    if (size < 0) throw VMError(VMErrorKind::InvalidOperand, "invalid frame size");
                    const size_t end = currentBase_ + static_cast<size_t>(size);
                    if (end > locals_.size()) {
                        locals_.resize(end, Value::integer(0));
                        initializedLocals_.resize(end, false);
                    }
                    for (size_t i = currentBase_; i < end; ++i) initializedLocals_[i] = false;
                    currentFrameSize_ = static_cast<size_t>(size);
                    break;
                }
                case OpCode::CALL: {
                    const int target = static_cast<int>(operandAs<int64_t>(instr));
                    if (target < 0 || target >= static_cast<int>(code.size()))
                        throw VMError(VMErrorKind::InvalidOperand, "invalid call target " + to_string(target));
                    frames_.push_back(CallFrame{ip + 1, currentBase_, currentFrameSize_});
                    currentBase_ += currentFrameSize_;
                    ip = target;
                    continue;
                }
                case OpCode::RETURN: {
                    const int64_t hasValue = operandAs<int64_t>(instr);
                    if (hasValue != 0 && hasValue != 1) throw VMError(VMErrorKind::InvalidOperand, "invalid return operand");
                    Value value = hasValue ? pop() : Value::integer(0);
                    if (frames_.empty()) throw VMError(VMErrorKind::MalformedBytecode, "return with no active call");
                    CallFrame frame = frames_.back();
                    frames_.pop_back();
                    currentBase_ = frame.base;
                    currentFrameSize_ = frame.size;
                    if (hasValue) push(move(value));
                    ip = frame.returnAddress;
                    continue;
                }
                case OpCode::PRINT: 
                    cout << valueToString(pop()) << "\n"; 
                    break;
                case OpCode::INPUT: 
                { 
                    const int64_t index = operandAs<int64_t>(instr);
                    if (index < 0 || static_cast<size_t>(index) >= code.stringConstants.size())
                        throw VMError(VMErrorKind::InvalidOperand, "invalid string constant index " + to_string(index));
                    cout << code.stringConstants[static_cast<size_t>(index)];
                    string line; getline(cin, line); 
                    push(Value::string(move(line))); 
                    break; 
                }
                case OpCode::CONVERT: 
                {
                    Value value = pop(); 
                    const auto target = static_cast<ValueType>(operandAs<int64_t>(instr));
                    switch (target) {
                    case ValueType::BYTE:
                    case ValueType::SHORT:
                    case ValueType::INT:
                    case ValueType::LONG: {
                        const double number = numericValue(value);
                        push(Value::integer(static_cast<int64_t>(number)));
                        break;
                    }
                    case ValueType::FLOAT:
                    case ValueType::DOUBLE: {
                        const double number = numericValue(value);
                        push(Value::floating(number));
                        break;
                    }
                    case ValueType::CHAR: {
                        const double number = numericValue(value);
                        push(Value::character(static_cast<char16_t>(static_cast<int64_t>(number))));
                        break;
                    }
                    default:
                        throw VMError(VMErrorKind::InvalidOperand, "invalid cast target");
                    }
                    break;
                }
                case OpCode::POP: pop(); break;
                case OpCode::HALT: return 0;
                default: throw VMError(VMErrorKind::UnknownOpcode, "unknown opcode " + to_string(static_cast<int>(instr.op)));
            }
            ++ip;
    }
    return 0;
}
