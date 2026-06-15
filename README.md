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
