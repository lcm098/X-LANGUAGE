#ifndef X_EXPR_H
#define X_EXPR_H
#include "./token_struct.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_GROUPING,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_VARIABLE,
    EXPR_ASSIGN,
    EXPR_VAR,
    EXPR_PREFIX,
    EXPR_POSTFIX,
    EXPR_VECTOR,
    EXPR_HASHMAP,
    EXPR_INDEX,
    EXPR_INDEX_ASSIGN,


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

        struct {
            Token name;
            struct Expr* initializer;
        } var_expr;

        struct {
            Token operator;
            Token name;
        } prefix;

        struct {
            Token operator;
            Token name;
        } postfix;

        struct {
            struct Expr** items;
            int count;
        } vec;

        struct {
            struct Expr** keys;
            struct Expr** values;
            int count;
        } hashmap;

        struct {
            struct Expr* object;
            struct Expr* index;
        } index_expr;

        struct {
            struct Expr* object;
            struct Expr* index;
            struct Expr* value;
        } index_assign;
    };
} Expr;

#endif