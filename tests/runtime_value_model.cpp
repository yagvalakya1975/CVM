#include "vm/vm.h"
#include <cassert>
#include <stdexcept>

int main() {
    Value integer = Value::integer(7);
    Value boolean = Value::boolean(true);
    Value character = Value::character(u'A');
    assert(integer.tag() == ValueTag::INT);
    assert(boolean.tag() == ValueTag::BOOL && boolean.asBool());
    assert(character.isNumeric());
    bool rejectedWrongAccessor = false;
    try { (void)boolean.asInt(); } catch (const std::runtime_error&) { rejectedWrongAccessor = true; }
    assert(rejectedWrongAccessor);

    VM vm;
    Bytecode valid = {
        Instruction(OpCode::PUSH_CHAR, char16_t(u'A')),
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::POP),
        Instruction(OpCode::PUSH_BOOL, int64_t(1)),
        Instruction(OpCode::JUMP_IF_FALSE, int64_t(8)),
        Instruction(OpCode::PUSH_STRING, std::string("ok=")),
        Instruction(OpCode::PUSH_BOOL, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::PRINT),
        Instruction(OpCode::HALT),
    };
    assert(vm.run(valid) == 0);

    Bytecode arrayIdentity = {
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::BUILD_ARRAY, int64_t(1)),
        Instruction(OpCode::STORE_LOCAL, int64_t(0)),
        Instruction(OpCode::LOAD_LOCAL, int64_t(0)),
        Instruction(OpCode::LOAD_LOCAL, int64_t(0)),
        Instruction(OpCode::CMP_EQ),
        Instruction(OpCode::POP),
        Instruction(OpCode::HALT),
    };
    assert(vm.run(arrayIdentity) == 0);

    Bytecode invalidCondition = {
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::JUMP_IF_FALSE, int64_t(2)),
        Instruction(OpCode::HALT),
    };
    assert(vm.run(invalidCondition) == 1);

    Bytecode invalidArithmetic = {
        Instruction(OpCode::PUSH_BOOL, int64_t(1)),
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::HALT),
    };
    assert(vm.run(invalidArithmetic) == 1);

    Bytecode invalidCast = {
        Instruction(OpCode::PUSH_STRING, std::string("7")),
        Instruction(OpCode::CONVERT, int64_t(3)),
        Instruction(OpCode::HALT),
    };
    assert(vm.run(invalidCast) == 1);
}
