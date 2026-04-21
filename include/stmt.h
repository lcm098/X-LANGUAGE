#ifndef X_STMT_H
#define X_STMT_H

/* Forward declaration breaks the stmt.h ↔ expr.h circular include. */
typedef struct Expr Expr;
#include "token_struct.h"

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
    STMT_FUNCTION

} StmtType;

typedef struct Stmt
{

    StmtType type;

    union
    {

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