#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "vm/compiler.h"
#include "vm/vm_error.h"
#include "vm/vm.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
static void printUsage(const char* prog) {
    cerr << "Usage: " << prog << " <source.pi> [--dump-ast] [--dump-bytecode] [--parse-only] [--tokens]\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { printUsage(argv[0]); return 1; }

    bool dumpAST = true;
    bool dumpBytecode = true;
    bool parseOnly = false;
    bool dumpTokens = false;
    string sourceFile;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--dump-ast") dumpAST = true;
        else if (arg == "--dump-bytecode") dumpBytecode = true;
        else if (arg == "--parse-only") parseOnly = true;
        else if (arg == "--tokens") dumpTokens = true;
        else if (!arg.empty() && arg[0] == '-') {
            cerr << "Error: unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        } else if (!sourceFile.empty()) {
            cerr << "Error: multiple source files provided.\n";
            printUsage(argv[0]);
            return 1;
        } else sourceFile = arg;
    }

    if (sourceFile.empty()) { printUsage(argv[0]); return 1; }

    ifstream fs(sourceFile);
    if (!fs) {
        cerr << "Error: cannot open '" << sourceFile << "'\n";
        return 1;
    }
    ostringstream buf;
    buf << fs.rdbuf();
    string src = buf.str();

    Lexer lexer(src);
    vector<Token> tokens = lexer.scanTokens();

    if(dumpTokens)
    {
        for (const auto& token : tokens) {
            cout << "Token: " << tokenTypeToString(token.type) 
                      << " | Lexeme: '" << token.lexeme 
                      << "' | Line: " << token.line << "\n";
        }
    }

    Parser parser(tokens);
    unique_ptr<ASTNode> root(parser.parse());
    if (!root || parser.hasErrors()) {
        cerr << "Parsing failed.\n";
        return 1;
    }
    
    if (dumpAST) {
        cout << "AST\n";
        parser.printAST(root.get());
        cout << "\n";
    }

    if (parseOnly) return 0;

    Compiler compiler;
    Bytecode bytecode = compiler.compile(root.get());
    if (bytecode.empty()) {
        cerr << "Compilation failed.\n";
        return 1;
    }

    if (dumpBytecode)
        Compiler::disassemble(bytecode);

    VM vm;
    try { return vm.run(bytecode); }
    catch (const VMError& e) { cerr << "Runtime error: " << e.what() << "\n"; return 1; }
    catch (const exception& e) { cerr << "Internal error: " << e.what() << "\n"; return 1; }
}
