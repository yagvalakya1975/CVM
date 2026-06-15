#pragma once
#include "bytecode.h"
#include <unordered_map>
#include <vector>
#include <string>

class VM {
public:
    int run(const Bytecode& code);

private:
    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> vars_;

    void        push(Value v);
    Value       pop();
    const Value& peek() const;
    Value applyBinOp(const std::string& op, Value lhs, Value rhs);
    static std::string valueToString(const Value& v);
    static bool        isTruthy    (const Value& v);
};