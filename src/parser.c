#include "../include/common.h"
#include "../include/parser.h"
#include "../include/scanner.h"

#define MAX_MACROS 128
#define MAX_PARAMS 128

typedef struct
{
    char name[128];
    char params[MAX_PARAMS][128];
    int paramCount;
    Stmt **body;
    int bodyCount;
} MacroDef;

static MacroDef macros[MAX_MACROS];
static int macroCount = 0;

static MacroDef *findMacro(const char *name)
{
    for (int i = 0; i < macroCount; i++)
        if (strcmp(macros[i].name, name) == 0)
            return &macros[i];
    return NULL;
}


static Expr *expression();
static Expr *assignment();
static Expr *logic_or();
static Expr *logic_and();
static Expr *equality();
static Expr *comparison();
static Expr *term();
static Expr *factor();
static Expr *unary();
static Expr *postfix();
static Expr *primary();
static Expr *variableExpr(Token name);
static Stmt *block();
static Token consume(TokenType type, const char *message);
static Stmt *statement();
static bool isAtEnd();
static Stmt *macroDefinition();
static Stmt *returnStatement();
static Expr *macroInvocationExpr();

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

static void error(const char *message)
{
    printf("Parser error: %s\n", message);
    exit(1);
}

static Expr *literalExpr(const char *value, int length, TokenType type)
{
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->literal.value = value;
    expr->literal.length = length;
    expr->literal.type = type;
    return expr;
}

static Expr *groupingExpr(Expr *expression)
{
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_GROUPING;
    expr->grouping.expression = expression;
    return expr;
}

static Expr *unaryExpr(Token operator, Expr *right)
{
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->unary.operator = operator;
    expr->unary.right = right;
    return expr;
}

static Expr *binaryExpr(Expr *left, Token operator, Expr *right)
{
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->binary.left = left;
    expr->binary.operator = operator;
    expr->binary.right = right;
    return expr;
}

// --- Precedence chain (low → high) ---
// expression → assignment → logic_or → logic_and → equality
// → comparison → term → factor → unary → postfix → primary

static Expr *postfix()
{
    Expr *expr = primary();

    for (;;)
    {
        if (match(TOKEN_LEFT_BRACKET))
        {
            Expr *index = expression();
            consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");
            Expr *e = malloc(sizeof(Expr));
            e->type = EXPR_INDEX;
            e->index_expr.object = expr;
            e->index_expr.index = index;
            expr = e;
        }
        else if (expr->type == EXPR_VARIABLE &&
                 (match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS)))
        {
            Expr *post = malloc(sizeof(Expr));
            post->type = EXPR_POSTFIX;
            post->postfix.operator = previous; // FIX: 'previous' is a variable, not a function call
            post->postfix.name = expr->variable.name;
            return post;
        }
        else
        {
            break;
        }
    }
    return expr;
}

static Expr *unary()
{
    if (match(TOKEN_PLUS_PLUS) || match(TOKEN_MINUS_MINUS))
    {
        Expr *expr = malloc(sizeof(Expr));
        expr->type = EXPR_PREFIX;
        expr->prefix.operator = previous; // FIX: variable, not function call
        expr->prefix.name = consume(TOKEN_IDENTIFIER, "Expect variable name.");
        return expr;
    }
    if (match(TOKEN_BANG) || match(TOKEN_MINUS))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = unary();
        return unaryExpr(operator, right);
    }
    return postfix();
}

static Expr *factor()
{
    Expr *expr = unary();

    while (match(TOKEN_STAR) ||
           match(TOKEN_SLASH) ||
           match(TOKEN_PERCENT))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = unary();
        expr = binaryExpr(expr, operator, right);
    }

    return expr;
}

static Expr *term()
{
    Expr *expr = factor();

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = factor();
        expr = binaryExpr(expr, operator, right);
    }

    return expr;
}

static Expr *comparison()
{
    Expr *expr = term();

    while (match(TOKEN_GREATER) ||
           match(TOKEN_GREATER_EQUAL) ||
           match(TOKEN_LESS) ||
           match(TOKEN_LESS_EQUAL))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = term();
        expr = binaryExpr(expr, operator, right);
    }

    return expr;
}

static Expr *equality()
{
    Expr *expr = comparison();

    while (match(TOKEN_BANG_EQUAL) || match(TOKEN_EQUAL_EQUAL))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = comparison();
        expr = binaryExpr(expr, operator, right);
    }

    return expr;
}

static Expr *logic_and()
{
    Expr *expr = equality();

    while (match(TOKEN_AND))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = equality();

        Expr *newExpr = malloc(sizeof(Expr));
        newExpr->type = EXPR_LOGICAL;
        newExpr->logical.left = expr;
        newExpr->logical.operator = operator;
        newExpr->logical.right = right;
        expr = newExpr;
    }

    return expr;
}

static Expr *logic_or()
{
    Expr *expr = logic_and();

    while (match(TOKEN_OR))
    {
        Token operator = previous; // FIX: variable, not function call
        Expr *right = logic_and();

        Expr *newExpr = malloc(sizeof(Expr));
        newExpr->type = EXPR_LOGICAL;
        newExpr->logical.left = expr;
        newExpr->logical.operator = operator;
        newExpr->logical.right = right;
        expr = newExpr;
    }

    return expr;
}

static Expr *ExpressionVarDeclaration()
{
    Token name = consume(TOKEN_IDENTIFIER, "Expect variable name.");
    Expr *initializer = NULL;

    if (match(TOKEN_EQUAL))
        initializer = expression();

    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_VAR;
    expr->var_expr.name = name;
    expr->var_expr.initializer = initializer;
    return expr;
}

static Expr *primary()
{
    if (match(TOKEN_NUMBER))
        return literalExpr(previous.start, previous.length, TOKEN_NUMBER);

    if (match(TOKEN_STRING))
        return literalExpr(previous.start, previous.length, TOKEN_STRING);

    if (match(TOKEN_TRUE))
        return literalExpr("true", 4, TOKEN_TRUE);
    if (match(TOKEN_FALSE))
        return literalExpr("false", 5, TOKEN_FALSE);
    if (match(TOKEN_VOID))
        return literalExpr("void", 4, TOKEN_VOID);

    if (match(TOKEN_LEFT_PAREN))
    {
        Expr *expr = expression();
        if (!match(TOKEN_RIGHT_PAREN))
            error("Expect ')' after expression.");
        return groupingExpr(expr);
    }

    if (match(TOKEN_IDENTIFIER))
        return variableExpr(previous);

    if (match(TOKEN_VAR))
        return ExpressionVarDeclaration();

    if (match(TOKEN_VECTOR))
    {
        consume(TOKEN_LEFT_BRACKET, "Expect '[' after 'vector'.");

        Expr **items = NULL;
        int count = 0;

        if (!check(TOKEN_RIGHT_BRACKET))
        {
            do
            {
                items = realloc(items, sizeof(Expr *) * (count + 1));
                items[count++] = expression();
            } while (match(TOKEN_COMMA));
        }

        consume(TOKEN_RIGHT_BRACKET, "Expect ']' after vector items.");

        Expr *expr = malloc(sizeof(Expr));
        expr->type = EXPR_VECTOR;
        expr->vec.items = (struct Expr **)items;
        expr->vec.count = count;
        return expr;
    }

    if (match(TOKEN_HASHMAP))
    {
        consume(TOKEN_LEFT_BRACE, "Expect '{' after 'hashmap'.");

        Expr **keys = NULL;
        Expr **values = NULL;
        int count = 0;

        if (!check(TOKEN_RIGHT_BRACE))
        {
            do
            {
                keys = realloc(keys, sizeof(Expr *) * (count + 1));
                values = realloc(values, sizeof(Expr *) * (count + 1));
                keys[count] = expression();
                consume(TOKEN_COLON, "Expect ':' after key.");
                values[count] = expression();
                count++;
            } while (match(TOKEN_COMMA));
        }

        consume(TOKEN_RIGHT_BRACE, "Expect '}' after hashmap entries.");

        Expr *expr = malloc(sizeof(Expr));
        expr->type = EXPR_HASHMAP;
        expr->hashmap.keys = (struct Expr **)keys;
        expr->hashmap.values = (struct Expr **)values;
        expr->hashmap.count = count;
        return expr;
    }

    if (match(TOKEN_AT))
        return macroInvocationExpr();

    error("Expect expression.");
    return NULL;
}

static Expr *variableExpr(Token name)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    expr->type = EXPR_VARIABLE;
    expr->variable.name = name;
    return expr;
}

static Expr *assignment()
{
    Expr *expr = logic_or(); // FIX: was equality(), must flow through logic_or → logic_and

    if (match(TOKEN_EQUAL))
    {
        Expr *value = assignment();

        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->variable.name;
            Expr *assign = malloc(sizeof(Expr));
            assign->type = EXPR_ASSIGN;
            assign->assign.name = name;
            assign->assign.value = value;
            return assign;
        }

        if (expr->type == EXPR_INDEX)
        {
            Expr *assign = malloc(sizeof(Expr));
            assign->type = EXPR_INDEX_ASSIGN;
            assign->index_assign.object = expr->index_expr.object;
            assign->index_assign.index = expr->index_expr.index;
            assign->index_assign.value = value;
            return assign;
        }

        printf("Invalid assignment target.\n");
    }
    return expr;
}

static Expr *expression()
{
    return assignment();
}

static Token consume(TokenType type, const char *message)
{
    if (check(type))
    {
        advance();
        return previous;
    }
    error(message);
    return previous;
}

static Stmt *printStmt(Expr *expression)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_PRINT;
    stmt->print.expression = expression;
    return stmt;
}

static Stmt *exprStmt(Expr *expression)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->expr.expression = expression;
    return stmt;
}

static Stmt *expressionStatement()
{
    Expr *expr = expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    return exprStmt(expr);
}

static Stmt *printStatement()
{
    Expr *value = expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    return printStmt(value);
}

static Stmt *varDeclaration()
{
    Token name = consume(TOKEN_IDENTIFIER, "Expect variable name.");
    Expr *initializer = NULL;

    if (match(TOKEN_EQUAL))
        initializer = expression();

    consume(TOKEN_SEMICOLON, "Expect ';' after variable.");

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_VAR;
    stmt->var.name = name;
    stmt->var.initializer = initializer;
    return stmt;
}

static Stmt *block()
{
    Stmt **statements = NULL;
    int count = 0;

    while (!check(TOKEN_RIGHT_BRACE) && !isAtEnd())
    {
        statements = realloc(statements, sizeof(Stmt *) * (count + 1));
        statements[count++] = statement();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_BLOCK;
    stmt->block.statements = statements;
    stmt->block.count = count;
    return stmt;
}

static Stmt *ifStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Expr *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    Stmt *thenBranch = statement();
    Stmt *elseBranch = NULL;

    if (match(TOKEN_ELSE))
    {
        // FIX: 'else if' — after consuming 'else', if next token is 'if',
        // just call statement() which will match TOKEN_IF → ifStatement(),
        // producing a chained if/else-if/else naturally.
        elseBranch = statement();
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_IF;
    stmt->ifStmt.condition = condition;
    stmt->ifStmt.thenBranch = thenBranch;
    stmt->ifStmt.elseBranch = elseBranch;
    return stmt;
}

static Stmt *ifNotStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Expr *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    Stmt *thenBranch = statement();
    Stmt *elseBranch = NULL;

    if (match(TOKEN_ELSE))
    {
        // FIX: 'else if' — after consuming 'else', if next token is 'if',
        // just call statement() which will match TOKEN_IF → ifStatement(),
        // producing a chained if/else-if/else naturally.
        elseBranch = statement();
    }

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_IF_NOT;
    stmt->ifNotStmt.condition = condition;
    stmt->ifNotStmt.thenBranch = thenBranch;
    stmt->ifNotStmt.elseBranch = elseBranch;
    return stmt;
}

static Stmt *whileStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after while.");
    Expr *condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    Stmt *body = statement();
    Stmt *stmt = malloc(sizeof(Stmt));

    stmt->type = STMT_WHILE;
    stmt->whileStmt.condition = condition;
    stmt->whileStmt.body = body;

    return stmt;
}

static Stmt *doWhileStatement()
{
    Stmt *body = statement();
    consume(TOKEN_WHILE, "Expected 'while' after do block");
    consume(TOKEN_LEFT_PAREN, "Expect '(' after while.");

    Expr *condition = expression();

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    consume(TOKEN_SEMICOLON, "Expected ';' after do-while loop");

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_DO_WHILE;
    stmt->doWhileStmt.condition = condition;
    stmt->doWhileStmt.body = body;

    return stmt;
}

static Stmt *passStatement()
{
    consume(TOKEN_SEMICOLON, "Expect ';' after 'pass'.");
    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_PASS;
    return stmt;
}

static Stmt *forLoopStatement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    Stmt *initializer = NULL;

    if (match(TOKEN_SEMICOLON))
    {
        // no initializer
    }
    else if (match(TOKEN_LET))
    {
        initializer = varDeclaration();
    }
    else
    {
        initializer = expressionStatement();
    }

    // --- Condition clause ---
    Expr *condition = NULL;
    if (!check(TOKEN_SEMICOLON))
        condition = expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after for condition.");

    // --- Increment clause ---
    Expr *increment = NULL;
    if (!check(TOKEN_RIGHT_PAREN))
        increment = expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    Stmt *body = statement();

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FOR;
    stmt->forStmt.initializer = initializer;
    stmt->forStmt.condition = condition;
    stmt->forStmt.increment = increment;
    stmt->forStmt.body = body;
    return stmt;
}

static Stmt *macroDefinition()
{
    Token nameToken = consume(TOKEN_IDENTIFIER, "Expect macro name after '#define'.");
    consume(TOKEN_LEFT_BRACKET, "Expect '[' after macro name.");
    consume(TOKEN_LEFT_PAREN, "Expect '(' after '['.");

    if (macroCount >= MAX_MACROS)
        error("Too many macros.");
    MacroDef *macro = &macros[macroCount++];
    macro->paramCount = 0;
    macro->body = NULL;
    macro->bodyCount = 0;

    int nameLen = nameToken.length < 63 ? nameToken.length : 63;
    memcpy(macro->name, nameToken.start, nameLen);
    macro->name[nameLen] = '\0';

    if (!check(TOKEN_RIGHT_PAREN))
    {
        do
        {
            if (macro->paramCount >= MAX_PARAMS)
                error("Too many parameters.");
            Token p = consume(TOKEN_IDENTIFIER, "Expect parameter name.");
            int pLen = p.length < 63 ? p.length : 63;
            memcpy(macro->params[macro->paramCount], p.start, pLen);
            macro->params[macro->paramCount][pLen] = '\0';
            macro->paramCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    while (!check(TOKEN_RIGHT_BRACKET) && !isAtEnd())
    {
        macro->body = realloc(macro->body, sizeof(Stmt *) * (macro->bodyCount + 1));
        macro->body[macro->bodyCount++] = statement();
    }
    consume(TOKEN_RIGHT_BRACKET, "Expect ']' after macro body.");

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_PASS;
    return stmt;
}

// 'return' already consumed. Semicolon is optional (e.g. before ']').
static Stmt *returnStatement()
{
    Expr *value = NULL;
    if (!check(TOKEN_SEMICOLON) && !check(TOKEN_RIGHT_BRACKET) && !isAtEnd())
        value = expression();
    match(TOKEN_SEMICOLON); // optional

    Stmt *stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_RETURN;
    stmt->returnStmt.value = value;
    return stmt;
}

// '@' already consumed. Looks up the macro, builds an EXPR_BLOCK expansion.
static Expr *macroInvocationExpr()
{
    Token nameToken = consume(TOKEN_IDENTIFIER, "Expect macro name after '@'.");

    char macroName[64];
    int nameLen = nameToken.length < 63 ? nameToken.length : 63;
    memcpy(macroName, nameToken.start, nameLen);
    macroName[nameLen] = '\0';

    MacroDef *macro = findMacro(macroName);
    if (!macro)
    {
        error("Undefined macro.");
        return NULL;
    }

    // Parse argument list if the macro has parameters
    Expr **args = NULL;
    int argCount = 0;
    if (macro->paramCount > 0)
    {
        consume(TOKEN_LEFT_PAREN, "Expect '(' for macro arguments.");
        if (!check(TOKEN_RIGHT_PAREN))
        {
            do
            {
                args = realloc(args, sizeof(Expr *) * (argCount + 1));
                args[argCount++] = expression();
            } while (match(TOKEN_COMMA));
        }
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after macro arguments.");
        if (argCount != macro->paramCount)
            error("Wrong number of macro arguments.");
    }
    else if (check(TOKEN_LEFT_PAREN))
    {
        // zero-param macro invoked with explicit ()
        consume(TOKEN_LEFT_PAREN, "");
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after '('.");
    }

    // Build expansion:
    //   let p0 = arg0;
    //   let p1 = arg1;
    //   ...macro body statements (shared, read-only)...
    int total = macro->paramCount + macro->bodyCount;
    Stmt **stmts = malloc(sizeof(Stmt *) * (total > 0 ? total : 1));
    int si = 0;

    for (int i = 0; i < macro->paramCount; i++)
    {
        Stmt *letStmt = malloc(sizeof(Stmt));
        letStmt->type = STMT_VAR;

        // Synthetic token pointing into the stable MacroDef storage
        Token paramToken;
        paramToken.type = TOKEN_IDENTIFIER;
        paramToken.start = macro->params[i];
        paramToken.length = (int)strlen(macro->params[i]);
        paramToken.line = 0;

        letStmt->var.name = paramToken;
        letStmt->var.initializer = args[i];
        stmts[si++] = letStmt;
    }
    for (int i = 0; i < macro->bodyCount; i++)
        stmts[si++] = macro->body[i];

    free(args);

    Expr *expr = malloc(sizeof(Expr));
    expr->type = EXPR_BLOCK;
    expr->block.statements = (struct Stmt **)stmts;
    expr->block.count = total;
    return expr;
}

static Stmt *statement()
{
    if (match(TOKEN_PRINT))
        return printStatement();

    if (match(TOKEN_LET))
        return varDeclaration();

    if (match(TOKEN_LEFT_BRACE))
        return block();

    if (match(TOKEN_IF))
        return ifStatement();

    if (match(TOKEN_IF_NOT))
        return ifNotStatement();

    if (match(TOKEN_WHILE))
        return whileStatement();

    if (match(TOKEN_DO))
        return doWhileStatement();

    if (match(TOKEN_PASS))
        return passStatement();

    if (match(TOKEN_FOR))
        return forLoopStatement();

    if (match(TOKEN_DEFINE))
        return macroDefinition();

    if (match(TOKEN_RETURN))
        return returnStatement();

    return expressionStatement();
}

static bool isAtEnd()
{
    return current.type == TOKEN_EOF;
}

bool parserAtEnd()
{
    return current.type == TOKEN_EOF;
}

void initParser()
{
    advance();
}

Stmt *parseNext()
{
    return statement();
}

Stmt *parse()
{
    advance();
    return statement();
}