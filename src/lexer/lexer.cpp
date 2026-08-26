#include "lexer/lexer.h"
#include <iostream>

using namespace std;

Lexer::Lexer(string source) : source(std::move(source)) {
    keywords = {
        {"byte", TokenType::KW_BYTE}, {"short", TokenType::KW_SHORT},
        {"int", TokenType::KW_INT}, {"long", TokenType::KW_LONG},
        {"float", TokenType::KW_FLOAT}, {"double", TokenType::KW_DOUBLE},
        {"char", TokenType::KW_CHAR}, {"String", TokenType::KW_STRING},
        {"boolean", TokenType::KW_BOOLEAN}, {"if", TokenType::KW_IF},
        {"else", TokenType::KW_ELSE}, {"while", TokenType::KW_WHILE},
        {"print", TokenType::KW_PRINT}, {"input", TokenType::KW_INPUT},
        {"true", TokenType::KW_TRUE}, {"false", TokenType::KW_FALSE},
        {"string", TokenType::KW_LEGACY_STRING}, {"bool", TokenType::KW_LEGACY_BOOL}
    };
}

vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) { start = current; scanToken(); }
    tokens.emplace_back(TokenType::END_OF_FILE, "", line);
    return tokens;
}

void Lexer::scanToken() {
    switch (char c = advance()) {
        case '(': addToken(TokenType::LEFT_PAREN); break; case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break; case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break; case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break; case ';': addToken(TokenType::SEMICOLON); break;
        case '+': addToken(TokenType::PLUS); break;
        case '-': addToken(TokenType::MINUS); break; case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break; case '%': addToken(TokenType::MODULO); break;
        case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
        case '!': if (match('=')) addToken(TokenType::BANG_EQUAL); else cerr << "LexerError line " << line << ": unexpected '!'.\n"; break;
        case '<': addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS); break;
        case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
        case '"': check_string(); break; case '\'': check_character(); break;
        case ' ': case '\r': case '\t': break; case '\n': ++line; break;
        default:
            if (isDigit(c)) check_number(); else if (isAlpha(c)) check_identifier();
            else cerr << "LexerError line " << line << ": unexpected character '" << c << "'.\n";
    }
}

void Lexer::check_identifier() {
    while (isAlphaNumeric(peek())) advance();
    string text = source.substr(start, current - start);
    auto found = keywords.find(text);
    if (found != keywords.end()) {
        if (found->second == TokenType::KW_LEGACY_STRING || found->second == TokenType::KW_LEGACY_BOOL)
            cerr << "LexerError line " << line << ": '" << text << "' is no longer a type; use '"
                 << (text == "string" ? "String" : "boolean") << "'.\n";
        addToken(found->second, text);
    } else addToken(TokenType::IDENTIFIER, text);
}

void Lexer::check_string() {
    while (peek() != '"' && !isAtEnd() && peek() != '\n') advance();
    if (isAtEnd() || peek() == '\n') { cerr << "LexerError line " << line << ": unterminated string.\n"; return; }
    advance();
    addToken(TokenType::STRING_LITERAL, source.substr(start + 1, current - start - 2));
}

void Lexer::check_character() {
    if (isAtEnd() || peek() == '\'' || peek() == '\n') { cerr << "LexerError line " << line << ": empty character literal.\n"; return; }
    advance();
    if (peek() != '\'') { cerr << "LexerError line " << line << ": character literal must contain one raw character.\n"; return; }
    advance();
    addToken(TokenType::CHAR_LITERAL, source.substr(start + 1, current - start - 2));
}

void Lexer::check_number() {
    while (isDigit(peek())) advance();
    bool decimal = false;
    if (peek() == '.' && isDigit(peekNext())) { decimal = true; advance(); while (isDigit(peek())) advance(); }
    char suffix = peek();
    if (suffix == 'L' || suffix == 'l' || suffix == 'F' || suffix == 'f' || suffix == 'D' || suffix == 'd') advance();
    string value = source.substr(start, current - start);
    if (suffix == 'L' || suffix == 'l') addToken(TokenType::LONG_LITERAL, value.substr(0, value.size()-1));
    else if (suffix == 'F' || suffix == 'f') addToken(TokenType::FLOAT_LITERAL, value.substr(0, value.size()-1));
    else if (suffix == 'D' || suffix == 'd') addToken(TokenType::DOUBLE_LITERAL, value.substr(0, value.size()-1));
    else addToken(decimal ? TokenType::DOUBLE_LITERAL : TokenType::INT_LITERAL, value);
}

bool Lexer::isAtEnd() { return current >= static_cast<int>(source.length()); }
char Lexer::advance() { return source[current++]; }
char Lexer::peek() { return isAtEnd() ? '\0' : source[current]; }
char Lexer::peekNext() { return current + 1 >= static_cast<int>(source.length()) ? '\0' : source[current + 1]; }
bool Lexer::match(char expected) { if (isAtEnd() || source[current] != expected) return false; ++current; return true; }
bool Lexer::isDigit(char c) { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool Lexer::isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }
void Lexer::addToken(TokenType type) { addToken(type, source.substr(start, current - start)); }
void Lexer::addToken(TokenType type, string text) { tokens.emplace_back(type, std::move(text), line); }
