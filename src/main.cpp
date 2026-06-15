#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "../include/lexer/lexer.h"
#include "../include/lexer/token.h"
#include "../include/parser/parser.h"
#include "../include/vm/compiler.h"
#include "../include/vm/vm.h"
static void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <source.pi> [--dump-ast] [--dump-bytecode]\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { printUsage(argv[0]); return 1; }

    bool dumpAST      = false;
    bool dumpBytecode = false;
    std::string sourceFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-ast")      dumpAST      = true;
        else if (arg == "--dump-bytecode") dumpBytecode = true;
        else                          sourceFile   = arg;
    }

    if (sourceFile.empty()) { printUsage(argv[0]); return 1; }

    std::ifstream fs(sourceFile);
    if (!fs) {
        std::cerr << "Error: cannot open '" << sourceFile << "'\n";
        return 1;
    }
    std::ostringstream buf;
    buf << fs.rdbuf();
    std::string src = buf.str();

    Lexer lexer(src);
    std::vector<Token> tokens = lexer.scanTokens();

    for (const auto& token : tokens) {
        std::cout << "Token: " << tokenTypeToString(token.type) 
                  << " | Lexeme: '" << token.lexeme 
                  << "' | Line: " << token.line << "\n";
    }

    Parser parser(tokens);
    ASTNode* root = parser.parse();
    if (!root) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }

    std::cout << "\nAST:\n";
    parser.printAST(root);

    if (dumpAST) {
        std::cout << "=== AST ===\n";
        parser.printAST(root);
        std::cout << "\n";
    }

    Compiler compiler;
    Bytecode bytecode = compiler.compile(root);
    if (bytecode.empty()) {
        std::cerr << "Compilation failed.\n";
        return 1;
    }

    if (dumpBytecode)
        Compiler::disassemble(bytecode);

    VM vm;
    return vm.run(bytecode);
}