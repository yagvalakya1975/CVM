#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/compiler.h"
#include <cassert>

int main() {
    Lexer lexer("char c = 'A'; print(c + 1); print(c < 'B'); print(\"x\" + c);");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    ASTNode* root = parser.parse();
    assert(root && !parser.hasErrors());

    Compiler compiler;
    Bytecode code = compiler.compile(root);
    assert(!code.empty());

    int numericCharConversions = 0;
    for (const Instruction& instruction : code) {
        if (instruction.op == OpCode::CONVERT &&
            std::get<int64_t>(instruction.operand) == static_cast<int64_t>(ValueType::INT))
            ++numericCharConversions;
    }
    // c + 1 and c < 'B' each normalize their CHAR operand(s); String + c does not.
    assert(numericCharConversions == 3);
}
