#ifndef X_EXPR_H
#define X_EXPR_H
#include "./token_struct.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_GROUPING,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_VARIABLE,
    EXPR_ASSIGN
} ExprType;

typedef struct Expr {
    ExprType type;
    union {
        struct {
            const char* value;
            int length;
            TokenType type; 
        } literal;

        struct {
            struct Expr* expression;
        } grouping;

        struct {
            Token operator;
            struct Expr* right;
        } unary;

        struct {
            struct Expr* left;
            Token operator;
            struct Expr* right;
        } binary;

        struct {
            Token name;
        } variable;

        struct {
            Token name;
            struct Expr* value;
        } assign;

    };
} Expr;

#endif