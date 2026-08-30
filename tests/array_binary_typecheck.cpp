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
    assert(!compileSource("int[] values = [1, 2]; print(values[0] + values[1]);").empty());
    assert(!compileSource("int[][] matrix = [[1, 2], [3, 4]]; matrix[1][0] = 9; print(matrix[1][0]);").empty());
    assert(!compileSource("char[] values = ['A']; print(values['A']);").empty());
    assert(compileSource("int[] values = [1]; print(values == 1);").empty());
    assert(!compileSource("int[] a = [1]; int[] b = [2]; print(a == b);").empty());
    assert(compileSource("int[] values = [1]; int[][] matrix = [[1]]; print(values == matrix);").empty());
    assert(compileSource("int[] values = [1]; print(values + 1);").empty());
    assert(compileSource("int[] values = [1]; print(values < 1);").empty());
    assert(compileSource("int[] values = [1]; print(values[0.0]);").empty());
    assert(compileSource("int[] values = [1]; print(values[true]);").empty());
    assert(compileSource("int[] values = [1]; print(values[0][0]);").empty());
    assert(compileSource("int value = 1; print(value[0]);").empty());
    assert(compileSource("int[] values = [1]; values[0] = \"bad\";").empty());
}
