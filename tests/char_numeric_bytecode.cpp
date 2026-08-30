#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/compiler.h"
#include <cassert>
#include <memory>

using namespace std;

int main() {
    Lexer lexer("char c = 'A'; print(c + 1); print(c < 'B'); print(\"x\" + \"y\");");
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    unique_ptr<ASTNode> root(parser.parse());
    assert(root && !parser.hasErrors());

    Compiler compiler;
    Bytecode code = compiler.compile(root.get());
    assert(!code.empty());

    int numericCharConversions = 0;
    for (const Instruction& instruction : code) {
        if (instruction.op == OpCode::CONVERT &&
            get<int64_t>(instruction.operand) == static_cast<int64_t>(ValueType::INT))
            ++numericCharConversions;
    }
    // c + 1 and c < 'B' each normalize their CHAR operand(s).
    assert(numericCharConversions == 3);

    for (const char* source : {
             "char c = 'A'; print(\"x\" + c);",
             "print(\"x\" + 1);",
             "print(1 + \"x\");",
         }) {
        Lexer invalidLexer(source);
        auto invalidTokens = invalidLexer.scanTokens();
        Parser invalidParser(invalidTokens);
        unique_ptr<ASTNode> invalidRoot(invalidParser.parse());
        assert(invalidRoot && !invalidParser.hasErrors());

        Compiler invalidCompiler;
        assert(invalidCompiler.compile(invalidRoot.get()).empty());
    }
}
