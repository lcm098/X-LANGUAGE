#ifndef X_STMT_H
#define X_STMT_H

#include "expr.h"

typedef enum {

    STMT_PRINT,
    STMT_EXPR,
    STMT_VAR,
    STMT_BLOCK

} StmtType;

typedef struct Stmt {

    StmtType type;

    union {

        struct {
            Expr* expression;
        } print;

        struct {
            Expr* expression;
        } expr;

        struct {
            Token name;
            Expr* initializer;
        } var;

        struct {
            struct Stmt** statements;
            int count;
        } block;

    };

} Stmt;

#endif