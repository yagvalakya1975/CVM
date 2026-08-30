#pragma once

#include <stdexcept>
#include <string>

using namespace std;

enum class VMErrorKind {
    StackUnderflow,
    UninitializedLocal,
    TypeMismatch,
    DivisionByZero,
    UnknownOpcode,
    InvalidOperand,
    MalformedBytecode
};

class VMError : public runtime_error {
public:
    VMError(VMErrorKind kind, const string& message)
        : runtime_error(message), kind_(kind) {}

    VMErrorKind kind() const noexcept { return kind_; }

private:
    VMErrorKind kind_;
};
