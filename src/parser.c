#include "../include/common.h"
#include "../include/parser.h"
#include "../include/scanner.h"


static Expr* expression();
static Expr* equality();
static Expr* comparison();
static Expr* term();
static Expr* factor();
static Expr* unary();
static Expr* primary();
static Expr* variableExpr(Token name);

static Token current;
static Token previous;

static void advance()
{
    previous = current;
    current = scanToken();
}

static bool check(TokenType type)
{
    return current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;

    advance();
    return true;
}


static void error(const char* message)
{
    printf("Parser error: %s\n", message);
    exit(1);
}

static Expr* literalExpr(const char* value, int length, TokenType type) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->literal.value = value;
    expr->literal.length = length;
    expr->literal.type = type; 
    return expr;
}

static Expr* groupingExpr(Expr* expression)
{
    Expr* expr = malloc(sizeof(Expr));

    expr->type = EXPR_GROUPING;
    expr->grouping.expression = expression;

    return expr;
}

static Expr* unaryExpr(Token operator, Expr* right)
{
    Expr* expr = malloc(sizeof(Expr));

    expr->type = EXPR_UNARY;
    expr->unary.operator = operator;
    expr->unary.right = right;

    return expr;
}

static Expr* binaryExpr(Expr* left,
                        Token operator,
                        Expr* right)
{
    Expr* expr = malloc(sizeof(Expr));

    expr->type = EXPR_BINARY;
    expr->binary.left = left;
    expr->binary.operator = operator;
    expr->binary.right = right;

    return expr;
}

static Expr* unary()
{
    if (match(TOKEN_BANG) || match(TOKEN_MINUS))
    {
        Token operator = previous;
        Expr* right = unary();

        return unaryExpr(operator, right);
    }

    return primary();
}

static Expr* factor()
{
    Expr* expr = unary();

    while (match(TOKEN_STAR) ||
           match(TOKEN_SLASH) ||
           match(TOKEN_PERCENT))
    {
        Token operator = previous;
        Expr* right = unary();

        expr = binaryExpr(expr,
                          operator,
                          right);
    }

    return expr;
}

static Expr* term()
{
    Expr* expr = factor();

    while (match(TOKEN_PLUS) ||
           match(TOKEN_MINUS))
    {
        Token operator = previous;
        Expr* right = factor();

        expr = binaryExpr(expr,
                          operator,
                          right);
    }

    return expr;
}

static Expr* comparison()
{
    Expr* expr = term();

    while (match(TOKEN_GREATER) ||
           match(TOKEN_GREATER_EQUAL) ||
           match(TOKEN_LESS) ||
           match(TOKEN_LESS_EQUAL))
    {
        Token operator = previous;
        Expr* right = term();

        expr = binaryExpr(expr,
                          operator,
                          right);
    }

    return expr;
}

static Expr* equality()
{
    Expr* expr = comparison();

    while (match(TOKEN_BANG_EQUAL) || match(TOKEN_EQUAL_EQUAL))
    {
        Token operator = previous;
        Expr* right = comparison();

        expr = binaryExpr(expr,operator,right);
    }

    return expr;
}


static Expr* primary()
{
    if (match(TOKEN_NUMBER)) 
        return literalExpr(previous.start, previous.length, TOKEN_NUMBER);
    
    if (match(TOKEN_STRING)) 
        return literalExpr(previous.start, previous.length, TOKEN_STRING);

    if (match(TOKEN_TRUE))   return literalExpr("true", 4, TOKEN_TRUE);
    if (match(TOKEN_FALSE))  return literalExpr("false", 5, TOKEN_FALSE);
    if (match(TOKEN_VOID))    return literalExpr("void", 3, TOKEN_VOID);

    if (match(TOKEN_LEFT_PAREN)) {
        Expr* expr = expression();
        if (!match(TOKEN_RIGHT_PAREN)) error("Expect ')' after expression.");
        return groupingExpr(expr);
    }

    if (match(TOKEN_IDENTIFIER))
    {
        return variableExpr(previous);
    }

    error("Expect expression.");
    return NULL;
}

static Expr* variableExpr(Token name) {
    Expr* expr = malloc(sizeof(Expr));
    if (expr == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    expr->type = EXPR_VARIABLE;
    expr->variable.name = name;
    return expr;
}

static Expr* assignment() {
    Expr* expr = equality();

    if (match(TOKEN_EQUAL)) {
        Expr* value = assignment();

        if (expr->type == EXPR_VARIABLE) {
            Token name = expr->variable.name;
            Expr* assign = malloc(sizeof(Expr));
            assign->type = EXPR_ASSIGN;
            assign->assign.name = name;
            assign->assign.value = value;
            return assign;
        }
        printf("Invalid assignment target.\n");
    }
    return expr;
}

static Expr* expression()
{
    return assignment();
}

static Token consume(TokenType type, const char* message) {
    if (check(type)) {
        advance();
        return previous;
    }

    error(message);
    return previous; 
}

static Stmt* printStmt(Expr* expression)
{
    Stmt* stmt = malloc(sizeof(Stmt));

    stmt->type = STMT_PRINT;
    stmt->print.expression = expression;

    return stmt;
}

static Stmt* exprStmt(Expr* expression)
{
    Stmt* stmt = malloc(sizeof(Stmt));

    stmt->type = STMT_EXPR;
    stmt->expr.expression = expression;

    return stmt;
}

static Stmt* expressionStatement()
{
    Expr* expr = expression();

    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");

    return exprStmt(expr);
}

static Stmt* printStatement()
{
    Expr* value = expression();

    consume(TOKEN_SEMICOLON,
            "Expect ';' after value.");

    return printStmt(value);
}

static Stmt* varDeclaration()
{
    Token name = consume(TOKEN_IDENTIFIER,"Expect variable name.");

    Expr* initializer = NULL;


    if (match(TOKEN_EQUAL))
    {
        initializer = expression();
    }

    consume(TOKEN_SEMICOLON,"Expect ';' after variable.");
    Stmt* stmt =malloc(sizeof(Stmt));
    stmt->type = STMT_VAR;

    stmt->var.name = name;
    stmt->var.initializer = initializer;

    return stmt;
}

static Stmt* statement()
{
    if (match(TOKEN_PRINT))
        return printStatement();

    if (match(TOKEN_LET))
        return varDeclaration();

    return expressionStatement();
}

bool parserAtEnd() {
    return current.type == TOKEN_EOF;
}

void initParser() {
    advance();
}

Stmt* parseNext() {
    return statement();
}


Stmt* parse()
{
    advance();
    return statement();
}


