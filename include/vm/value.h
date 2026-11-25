#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include "vm/vm_error.h"

using namespace std;

struct Array;
using ArrayPtr = shared_ptr<Array>;

enum class ValueTag { INT, FLOAT, BOOL, CHAR, STRING, ARRAY };

inline const char* valueTagName(ValueTag tag) {
    switch (tag) {
        case ValueTag::INT:       return "INT";
        case ValueTag::FLOAT:     return "FLOAT";
        case ValueTag::BOOL:      return "BOOL";
        case ValueTag::CHAR:      return "CHAR";
        case ValueTag::STRING:    return "STRING";
        case ValueTag::ARRAY:     return "ARRAY";
    }
    return "UNKNOWN";
}

class Value {
public:
    static Value integer(int64_t value) { return Value(ValueTag::INT, value); }
    static Value floating(double value) { return Value(ValueTag::FLOAT, value); }
    static Value boolean(bool value) { return Value(ValueTag::BOOL, value); }
    static Value character(char16_t value) { return Value(ValueTag::CHAR, value); }
    static Value string(std::string value) { return Value(ValueTag::STRING, move(value)); }
    static Value array(ArrayPtr value) { return Value(ValueTag::ARRAY, move(value)); }

    ValueTag tag() const noexcept { return tag_; }
    bool isNumeric() const noexcept {
        return tag_ == ValueTag::INT || tag_ == ValueTag::FLOAT || tag_ == ValueTag::CHAR;
    }
    bool isReference() const noexcept { return tag_ == ValueTag::ARRAY; }

    int64_t asInt() const { return get<int64_t>(ValueTag::INT); }
    double asFloat() const { return get<double>(ValueTag::FLOAT); }
    bool asBool() const { return get<bool>(ValueTag::BOOL); }
    char16_t asChar() const { return get<char16_t>(ValueTag::CHAR); }
    const std::string& asString() const { return get<std::string>(ValueTag::STRING); }
    const ArrayPtr& asArray() const { return get<ArrayPtr>(ValueTag::ARRAY); }

private:
    using Storage = variant<int64_t, double, bool, char16_t, std::string, ArrayPtr>;

    template <typename T>
    Value(ValueTag tag, T value) : tag_(tag), storage_(move(value)) {}

    template <typename T>
    const T& get(ValueTag expected) const {
        if (tag_ != expected)
            throw VMError(VMErrorKind::TypeMismatch, std::string("expected ") + valueTagName(expected) +
                                     ", got " + valueTagName(tag_));
        return std::get<T>(storage_);
    }

    ValueTag tag_;
    Storage storage_;
};

struct Array {
    vector<Value> elements;
};
