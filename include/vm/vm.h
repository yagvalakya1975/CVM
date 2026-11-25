#pragma once
#include "vm/bytecode.h"
#include "vm/value.h"
#include <vector>
#include <string>

using namespace std;

class VM {
public:
    int run(const Bytecode& code);

private:
    vector<Value> stack_;
    vector<Value> locals_;
    vector<bool> initializedLocals_;
    struct CallFrame { int returnAddress; size_t base; size_t size; };
    vector<CallFrame> frames_;
    size_t currentBase_ = 0;
    size_t currentFrameSize_ = 0;

    void        push(Value v);
    Value       pop();
    const Value& peek() const;
    Value applyBinOp(const string& op, Value lhs, Value rhs);
    static string valueToString(const Value& v);
    static bool        isTruthy    (const Value& v);
};
