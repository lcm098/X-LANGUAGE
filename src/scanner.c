
#include "../include/common.h"
#include "../include/scanner.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
}Scanner;

Scanner scanner;

void initScanner(const char *source_code)
{
    scanner.start = source_code;
    scanner.current = source_code;
    scanner.line = 1;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static char peekNext()
{
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* message)
{
    Token token;

    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner.line;

    return token;
}

static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static Token number()
{
    while (isDigit(peek()))
        advance();

    if (peek() == '.' && isDigit(peekNext()))
    {
        advance();

        while (isDigit(peek()))
            advance();
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string()
{
    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
            scanner.line++;

        advance();
    }

    if (isAtEnd())
        return errorToken("Unterminated string.");

    advance();

    return makeToken(TOKEN_STRING);
}


static TokenType checkKeyword(int start, int length, const char* rest,TokenType type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start,rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}


static TokenType identifierType()
{
    switch (scanner.start[0])
    {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
        
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
                    case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
            break;

        case 'd':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e': 
                        if (scanner.current - scanner.start > 2) {
                            switch (scanner.start[2]) {
                                case 'f': 
                                    // Distinguish 'default' vs 'define'
                                    if (scanner.current - scanner.start > 3 && scanner.start[3] == 'a')
                                        return checkKeyword(4, 3, "ult", TOKEN_DEFAULT);
                                    return checkKeyword(3, 3, "ine", TOKEN_DEFINE);
                            }
                        }
                        break;
                    case 'o': return checkKeyword(2, 0, "", TOKEN_DO);
                }
            }
            break;

        case 'e':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l': return checkKeyword(2, 2, "se", TOKEN_ELSE);
                    case 'n': return checkKeyword(2, 3, "dif", TOKEN_ENDIF);
                }
            }
            break;

        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                }
            }
            break;

        case 'i':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'f':
                        if (scanner.current - scanner.start == 2) return TOKEN_IF;
                        // Handle if_not, ifdef, ifndef
                        if (scanner.start[2] == '_') return checkKeyword(3, 3, "not", TOKEN_IF_NOT);
                        if (scanner.start[2] == 'd') return checkKeyword(3, 2, "ef", TOKEN_IFDEF);
                        if (scanner.start[2] == 'n') return checkKeyword(3, 4, "def", TOKEN_IFNDEF);
                        break;
                    case 'm': return checkKeyword(2, 2, "pl", TOKEN_IMPL);
                    case 'n': return checkKeyword(2, 5, "clude", TOKEN_INCLUDE);
                }
            } else {
                return TOKEN_IF; 
            }
            break;

        case 'j': return checkKeyword(1, 3, "ump", TOKEN_JUMP);
        case 'l':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'e': return checkKeyword(2, 1, "t", TOKEN_LET);
                    case 'a': return checkKeyword(2, 3, "bel", TOKEN_LABEL);
                }
            }
            break;

        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 2, "ss", TOKEN_PASS);
                    case 'r': return checkKeyword(2, 3, "int", TOKEN_PRINT);
                }
            }
            break;

        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 5, "witch", TOKEN_SWITCH);
        case 't': return checkKeyword(1, 3, "rue", TOKEN_TRUE);
        case 'v':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 1, "r",    TOKEN_VAR);
                    case 'o': return checkKeyword(2, 2, "id",   TOKEN_VOID);
                    case 'e': return checkKeyword(2, 4, "ctor", TOKEN_VECTOR);
                }
            }
            break;
        case 'h': return checkKeyword(1, 6, "ashmap", TOKEN_HASHMAP);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
        advance();

    return makeToken(identifierType());
}

static void skipWhitespace()
{
    for (;;)
    {
        char c = peek();

        switch (c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                scanner.line++;
                advance();
                break;

            case '/':
                if (peekNext() == '/') 
                {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } 
                else if (peekNext() == '*') 
                {
                    advance(); // Consume '/'
                    advance(); // Consume '*'

                    while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) 
                    {
                        if (peek() == '\n') scanner.line++;
                        advance();
                    }

                    if (!isAtEnd()) 
                    {
                        advance(); // Consume '*'
                        advance(); // Consume '/'
                    }
                } 
                else 
                {
                    return;
                }
                break;

            default:
                return;
        }
    }
}

static bool match(char expected)
{
    if (isAtEnd()) return false;

    if (*scanner.current != expected)
        return false;

    scanner.current++;

    return true;
}

Token scanToken()
{
    skipWhitespace();

    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);

    char c = advance();

    if (isAlpha(c))
        return identifier();

    if (isDigit(c))
        return number();

    switch (c)
    {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case '"':
            return string();

        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case ']': return makeToken(TOKEN_RIGHT_BRACKET);
        case '[': return makeToken(TOKEN_LEFT_BRACKET);
        case ':': return makeToken(TOKEN_COLON);
        case '+': return match('+') ? makeToken(TOKEN_PLUS_PLUS)  : makeToken(TOKEN_PLUS);
        case '-': return match('-') ? makeToken(TOKEN_MINUS_MINUS) : makeToken(TOKEN_MINUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '%': return makeToken(TOKEN_PERCENT);

        case '!':
            return makeToken(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG
            );

        case '=':
            return makeToken(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL
            );

        case '<':
            return makeToken(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS
            );

        case '>':
            return makeToken(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER
            );
    }

    return errorToken("Unexpected character.");
}