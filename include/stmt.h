#ifndef X_STMT_H
#define X_STMT_H

/* Forward declaration breaks the stmt.h ↔ expr.h circular include. */
typedef struct Expr Expr;
#include "token_struct.h"

typedef struct StructFieldAST
{
    Token typeToken;      // TOKEN_VAR, TOKEN_LET, or TOKEN_STRUCT
    Token structTypeName; // Valid only if typeToken == TOKEN_STRUCT
    Token fieldName;      // The property name
} StructFieldAST;

typedef struct SwitchCase
{
    struct Expr **matchValues; // Array to support stacked cases: case 1: case 2:
    int matchCount;
    struct Stmt **statements; // The body to execute
    int stmtCount;
} SwitchCase;

typedef enum
{

    STMT_PRINT,
    STMT_EXPR,
    STMT_VAR,
    STMT_BLOCK,
    STMT_IF,
    STMT_IF_NOT,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_PASS,
    STMT_FOR,
    STMT_RETURN,
    STMT_FUNCTION,
    STMT_LABEL,
    STMT_JUMP,
    STMT_STRUCT_DEF,
    STMT_STRUCT_DECL,
    STMT_SWITCH,
    STMT_BREAK,
    STMT_CONTINUE

} StmtType;

typedef struct Stmt
{

    StmtType type;

    union
    {

        struct
        {
            struct Expr *condition;
            SwitchCase *cases;
            int caseCount;
            struct Stmt **defaultBranch;
            int defaultCount;
        } switchStmt;

        struct
        {
            Token name;
            StructFieldAST *fields;
            int fieldCount;
        } structDef;

        struct
        {
            Token structName;
            Token instanceName;
        } structDecl;

        struct
        {
            Token name;
            struct Stmt *body; /* NULL if it's just a bare label marker */
        } labelStmt;

        struct
        {
            Token name;
        } jumpStmt;

        struct
        {
            Token name;
            Token *params;
            Expr **defaults;
            int paramCount;
            struct Stmt *body;
        } function;

        struct
        {
            Expr *value; /* NULL = bare 'return;' */
        } returnStmt;

        struct
        {
            struct Stmt *initializer; /* NULL = no init clause  */
            Expr *condition;          /* NULL = infinite loop   */
            Expr *increment;          /* NULL = no increment    */
            struct Stmt *body;
        } forStmt;

        struct
        {
            Expr *condition;
            struct Stmt *body;
        } doWhileStmt;

        struct
        {
            Expr *condition;
            struct Stmt *body;
        } whileStmt;

        struct
        {
            Expr *expression;
        } print;

        struct
        {
            Expr *expression;
        } expr;

        struct
        {
            Token name;
            Expr *initializer;
        } var;

        struct
        {
            struct Stmt **statements;
            int count;
        } block;

        struct
        {
            Expr *condition;
            struct Stmt *thenBranch;
            struct Stmt *elseBranch;
        } ifStmt;

        struct
        {
            Expr *condition;
            struct Stmt *thenBranch;
            struct Stmt *elseBranch;
        } ifNotStmt;
    };

} Stmt;

#endif