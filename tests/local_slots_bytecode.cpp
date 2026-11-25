#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/compiler.h"
#include <cassert>
#include <memory>
#include <string>

using namespace std;

int main() {
    const string source =
        "int x = 1; { int x = 2; print(x); } print(x); x = 3; print(x);";
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    unique_ptr<ASTNode> root(parser.parse());
    assert(root && !parser.hasErrors());

    Compiler compiler;
    Bytecode code = compiler.compile(root.get());
    assert(!code.empty());
    int localCount = 0;
    for (const auto& instruction : code) {
        if (instruction.op == OpCode::LOAD_LOCAL || instruction.op == OpCode::STORE_LOCAL) {
            assert(holds_alternative<int64_t>(instruction.operand));
            assert(get<int64_t>(instruction.operand) >= 0);
            localCount++;
        }
    }
    assert(localCount >= 7);
    return 0;
}
