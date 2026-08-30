#include "vm/vm.h"
#include <cassert>
#include <stdexcept>

using namespace std;

int main() {
    Value integer = Value::integer(7);
    Value boolean = Value::boolean(true);
    Value character = Value::character(u'A');
    assert(integer.tag() == ValueTag::INT);
    assert(boolean.tag() == ValueTag::BOOL && boolean.asBool());
    assert(character.isNumeric());
    bool rejectedWrongAccessor = false;
    try { (void)boolean.asInt(); } catch (const runtime_error&) { rejectedWrongAccessor = true; }
    assert(rejectedWrongAccessor);

    VM vm;
    auto fails = [](VM& machine, const Bytecode& code) {
        try {
            return machine.run(code) != 0;
        } catch (const runtime_error&) {
            return true;
        }
    };
    auto failsWithInvalidOperand = [](VM& machine, const Bytecode& code) {
        try {
            (void)machine.run(code);
        } catch (const VMError& error) {
            return error.kind() == VMErrorKind::InvalidOperand;
        } catch (const runtime_error&) {
        }
        return false;
    };
    Bytecode valid = {
        Instruction(OpCode::PUSH_CHAR, char16_t(u'A')),
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::POP),
        Instruction(OpCode::PUSH_BOOL, int64_t(1)),
        Instruction(OpCode::JUMP_IF_FALSE, int64_t(10)),
        Instruction(OpCode::PUSH_STRING, int64_t(0)),
        Instruction(OpCode::PUSH_STRING, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::PRINT),
        Instruction(OpCode::HALT),
    };
    valid.stringConstants = {"ok=", "true"};
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
    assert(fails(vm, invalidCondition));

    for (int64_t target : {int64_t(-1), int64_t(3), int64_t(INT64_MAX)}) {
        Bytecode invalidJump = {
            Instruction(OpCode::JUMP, target),
            Instruction(OpCode::HALT),
        };
        assert(failsWithInvalidOperand(vm, invalidJump));
    }

    for (int64_t target : {int64_t(-1), int64_t(3), int64_t(INT64_MAX)}) {
        for (int64_t condition : {0, 1}) {
            Bytecode invalidConditionalJump = {
                Instruction(OpCode::PUSH_BOOL, condition),
                Instruction(OpCode::JUMP_IF_FALSE, target),
                Instruction(OpCode::HALT),
            };
            assert(failsWithInvalidOperand(vm, invalidConditionalJump));
        }
    }

    Bytecode invalidArithmetic = {
        Instruction(OpCode::PUSH_BOOL, int64_t(1)),
        Instruction(OpCode::PUSH_INT, int64_t(1)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::HALT),
    };
    assert(fails(vm, invalidArithmetic));

    Bytecode invalidStringAddition = {
        Instruction(OpCode::PUSH_STRING, int64_t(0)),
        Instruction(OpCode::PUSH_INT, int64_t(7)),
        Instruction(OpCode::ADD),
        Instruction(OpCode::HALT),
    };
    invalidStringAddition.stringConstants = {"ok="};
    assert(fails(vm, invalidStringAddition));

    for (int64_t target : {0, 1, 9, 10}) {
        Bytecode invalidTarget = {
            Instruction(OpCode::PUSH_INT, int64_t(7)),
            Instruction(OpCode::CONVERT, target),
            Instruction(OpCode::HALT),
        };
        assert(fails(vm, invalidTarget));
    }

    Bytecode invalidSource = {
        Instruction(OpCode::PUSH_STRING, int64_t(0)),
        Instruction(OpCode::CONVERT, int64_t(4)),
        Instruction(OpCode::HALT),
    };
    invalidSource.stringConstants = {"7"};
    assert(fails(vm, invalidSource));
}
