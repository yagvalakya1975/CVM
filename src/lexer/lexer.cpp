#include "lexer/lexer.h"
#include <iostream>

using namespace std;
    Lexer::Lexer(std::string source) : source(std::move(source)) {
        keywords["int"] = TokenType::KW_INT;
        keywords["float"] = TokenType::KW_FLOAT;
        keywords["string"] = TokenType::KW_STRING;
        keywords["char"] = TokenType::KW_CHAR;
        keywords["print"] = TokenType::KW_PRINT;
        keywords["input"] = TokenType::KW_INPUT;
        keywords["if"] = TokenType::KW_IF;
        keywords["else"] = TokenType::KW_ELSE;
        keywords["while"] = TokenType::KW_WHILE;
    }

    

    vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start = current;
        scanToken();
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line));
    return tokens;
}



void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        // Single-character tokens
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '+': addToken(TokenType::PLUS); break;
        case '-': addToken(TokenType::MINUS); break;
        case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break;
        case '%': addToken(TokenType::MODULO); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        // case '=': addToken(TokenType::EQUAL); break;
        case '"': check_string(); break;
        case '\'': check_character(); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '=': 
            addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); 
            break;
        case '!': 
            addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG); 
            break;
        case '<': 
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS); 
            break;
        case '>': 
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); 
            break;


        // Ignore whitespace
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line++;
            break;

        default:
            if (isDigit(c)) {
                check_number();
            } 
            else if(isAlpha(c)){
                check_identifier();
            }
            else {
                std::cerr << "LexerError: line " << line << ": Unexpected character '" << c << "'\n";
            }
            break;
    }
}

void Lexer::check_identifier(){
    while(isAlphaNumeric(peek())) advance();
    std::string text= source.substr(start, current-start);

    //checking if the scanned word is a reserved keyword or not
    TokenType type;
    auto match = keywords.find(text);
    if (match != keywords.end()) {
        type = match->second; // It's a keyword like 'int' or 'print'
    } else {
        type = TokenType::IDENTIFIER; // It's a user-defined variable name
    }

    addToken(type, text);

}

void Lexer::check_string() {
    // Consume characters until we hit the closing quote
    while (peek() != '"' && !isAtEnd() && peek() != '\n') {
        advance();
    }

    if (isAtEnd() || peek() == '\n') {
        std::cerr << "Error at line " << line << ": Unterminated string.\n";
        return;
    }

    advance(); // Consume the closing '"'

    // Grab the string content, excluding the quotes themselves
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TokenType::STRING_LITERAL, value);
}

void Lexer::check_character() {
    if (isAtEnd() || peek() == '\'') {
        std::cerr << "Error at line " << line << ": Empty character literal.\n";
        return;
    }

    advance(); // Consume the actual character

    if (peek() != '\'') {
        std::cerr << "Error at line " << line << ": Unterminated character literal.\n";
        return;
    }

    advance(); // Consume the closing '\''

    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TokenType::CHAR_LITERAL, value);
}

void Lexer::check_number() {
    bool isFloat = false;

    // Consume all standard digits
    while (isDigit(peek())) advance();

    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance(); // Consume the "."

        while (isDigit(peek())) advance();
    }

    std::string value = source.substr(start, current - start);
    addToken(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, value);
}

// --- Utility Functions ---

bool Lexer::isAtEnd() {
    return current >= source.length();
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return source[current];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++; // Consume the character since it matched!
    return true;
}

char Lexer::peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
            c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

void Lexer::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, line));
}

void Lexer::addToken(TokenType type, std::string text) { // particularly used here for number tokens
    tokens.push_back(Token(type, text, line));
}
 // namespace std