#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"

int main(int argc, char ** argv) {
    // Our test source code
    if(argc < 2)
    {
        std::cerr<<"Error: Please Supply the source file";
        return 1;
    }
    std::cout << "Reading from the file : "<< argv[1] << std::endl;
    std::ifstream sourceFileStream(argv[1]);

    std::stringstream buffer;
    buffer << sourceFileStream.rdbuf(); 
    std::string sourceCode = buffer.str();
    
    std::cout << "Scanning source code: " << sourceCode << "\n\n";

    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.scanTokens();

    for (const auto& token : tokens) {
        std::cout << "Token: " << tokenTypeToString(token.type) 
                  << " | Lexeme: '" << token.lexeme 
                  << "' | Line: " << token.line << "\n";
    }

    Parser parser(tokens);
    ASTNode* root = parser.parse();
    if (!root) {
        std::cerr << "\nParsing failed.\n";
        return 1;
    }
    std::cout << "\nAST:\n";
    parser.printAST(root);

    return 0;
}