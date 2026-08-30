#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/compiler.h"
#include <cassert>
#include <memory>

using namespace std;

static Bytecode compileSource(const char* source) {
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    unique_ptr<ASTNode> root(parser.parse());
    assert(root && !parser.hasErrors());
    Compiler compiler;
    return compiler.compile(root.get());
}

int main() {
    assert(compileSource("int[] values = [1]; print(values == 1);").empty());
    assert(!compileSource("int[] a = [1]; int[] b = [2]; print(a == b);").empty());
    assert(compileSource("int[] values = [1]; int[][] matrix = [[1]]; print(values == matrix);").empty());
    assert(compileSource("int[] values = [1]; print(values + 1);").empty());
    assert(compileSource("int[] values = [1]; print(values < 1);").empty());
}
