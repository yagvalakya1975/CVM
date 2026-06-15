# CVM - C++ Virtual Machine

A lightweight interpreter and virtual machine written in C++ that executes a custom high-level programming language. CVM implements a complete pipeline from lexical analysis through parsing to abstract syntax tree generation, designed to demonstrate compiler and interpreter fundamentals.

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Standard](https://img.shields.io/badge/C%2B%2B-17-blue)
![Build System](https://img.shields.io/badge/Build-CMake-green)
![Status](https://img.shields.io/badge/Status-Active%20Development-yellow)

## 📋 Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Language Syntax](#language-syntax)
- [Installation](#installation)
- [Usage](#usage)
- [Example Programs](#example-programs)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Contributing](#contributing)

## ✨ Features

- **Complete Lexer**: Tokenizes source code with support for keywords, literals, and operators
- **Recursive Descent Parser**: Builds an Abstract Syntax Tree (AST) from tokens
- **Multiple Data Types**: int, float, char, string, and bool
- **Control Flow**: if/else statements and while loops
- **Operators**: Arithmetic, bitwise, comparison, and assignment operations
- **I/O Operations**: print and input functions
- **Block Structure**: Proper scoping with braced blocks

## 🏗️ Architecture

CVM follows a classic three-stage compiler pipeline:

```
Source Code → Lexer → Parser → AST
                       ↓
                   Execution/Compilation
```

### Component Overview

1. **Lexer** (`src/lexer/lexer.cpp`)
   - Scans source code character by character
   - Produces a stream of tokens
   - Handles keywords, identifiers, literals, and operators
   - Line number tracking for error reporting

2. **Parser** (`src/parser/parser.cpp`)
   - Implements recursive descent parsing
   - Builds an Abstract Syntax Tree (AST)
   - Enforces language grammar and syntax rules
   - Supports operator precedence

3. **VM/Compiler** (`src/vm/compiler.cpp` & `src/vm/vm.cpp`)
   - Virtual machine execution engine (in development)
   - Bytecode compilation support
   - Stack-based execution model

## 🔤 Language Syntax

### Data Types

```cpp
int count = 10;
float pi = 3.14;
string message = "Hello";
char letter = 'A';
bool flag = true;
```

### Control Flow

**If Statements:**
```cpp
if (x == 5) {
    print("x is 5");
} else {
    print("x is not 5");
}
```

**While Loops:**
```cpp
int i = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}
```

### Operators

| Category | Operators |
|----------|-----------|
| Arithmetic | `+`, `-`, `*`, `/`, `%` |
| Comparison | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Bitwise | `&`, `\|`, `^`, `~`, `<<`, `>>` |
| Assignment | `=` |
| Logical | `!` |

### Built-in Functions

- `print(value)` - Output to console
- `input(prompt)` - Read input from user

## 📦 Installation

### Requirements

- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)

### Building

```bash
# Clone the repository
git clone https://github.com/yagvalakya1975/CVM.git
cd CVM

# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Compile
cmake --build .

# Executable will be in the build directory as 'cvmpp'
```

## 🚀 Usage

Run a `.pi` file (Pi language):

```bash
./cvmpp program.pi
```

The interpreter will:
1. Scan and tokenize the source code
2. Display all tokens with their types and line numbers
3. Parse and generate an Abstract Syntax Tree (AST)
4. Print the AST structure

### Example Usage

```bash
$ ./cvmpp example.pi
Reading from the file : example.pi
Scanning source code: int c = 5;
while(c==5){
    print("Hi");
    c = c + 1;
}
print("loop finished");
int b = 4;
int v = 3;
string d = "24";
print(c*b+v/1);
print(d);

Token: KW_INT | Lexeme: 'int' | Line: 1
Token: IDENTIFIER | Lexeme: 'c' | Line: 1
Token: EQUAL | Lexeme: '=' | Line: 1
Token: INT_LITERAL | Lexeme: '5' | Line: 1
...
```

## 💾 Example Programs

### Simple Output

```cpp
print("Hello, World!");
string name = "CVM";
print(name);
```

### Arithmetic

```cpp
int a = 10;
int b = 20;
print(a + b);
print(a * b);
int c = b / a;
print(c);
```

### Loops

```cpp
int counter = 0;
while (counter < 5) {
    print(counter);
    counter = counter + 1;
}
print("Done!");
```

### Conditional Logic

```cpp
int age = 18;
if (age >= 18) {
    print("You are an adult");
} else {
    print("You are a minor");
}
```

### Mixed Operations

```cpp
int c = 5;
while(c==5){
    print("Hi");
    c = c + 1;
}
print("loop finished");
int b = 4;
int v = 3;
string d = "24";
print(c*b+v/1);
print(d);
```

## 📁 Project Structure

```
CVM/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # This file
├── example.pi                  # Example source code
├── include/                    # Header files
│   ├── lexer/
│   │   ├── lexer.h            # Lexer interface
│   │   └── token.h            # Token definitions
│   ├── parser/
│   │   └── parser.h           # Parser interface & AST nodes
│   └── vm/
│       ├── compiler.h         # Compiler interface
│       └── vm.h               # Virtual machine interface
└── src/                        # Implementation files
    ├── main.cpp               # Entry point
    ├── lexer/
    │   └── lexer.cpp          # Lexer implementation
    ├── parser/
    │   └── parser.cpp         # Parser implementation
    └── vm/
        ├── compiler.cpp       # Compiler implementation
        └── vm.cpp             # VM implementation
```

## 🔧 Technical Details

### Lexer Features

- Multi-line comment and string support
- Numeric literal parsing (int and float)
- Character literal handling
- Keyword recognition and classification
- Comprehensive token type enumeration

### Parser Features

- Recursive descent parsing
- Expression parsing with proper operator precedence
- Control flow statement parsing
- Declaration statement handling
- AST printing for debugging

### Token Types

The lexer recognizes:
- **Structural**: parentheses, braces, brackets, semicolons, commas
- **Operators**: arithmetic, bitwise, comparison, assignment
- **Keywords**: int, float, char, string, bool, if, else, while, print, input, true, false
- **Literals**: integers, floats, strings, characters
- **Identifiers**: variable and function names

## 📝 Requirements

- **C++ Standard**: 17 or higher
- **CMake**: 3.15 or higher
- **Compiler**: Any C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## 🔄 Development Status

Currently implemented:
- ✅ Lexical analysis (Lexer)
- ✅ Parsing (Parser)
- ✅ AST Generation
- 🔄 Virtual Machine Execution (In Progress)
- 🔄 Bytecode Compilation (In Progress)

## 🤝 Contributing

Contributions are welcome! Feel free to:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is open source and available under the MIT License (or specify your license).

## 🙋 Support

For questions or issues, please open an issue on the [GitHub Issues](https://github.com/yagvalakya1975/CVM/issues) page.

---

**Author**: [@yagvalakya1975](https://github.com/yagvalakya1975)

**Last Updated**: June 2026
