#ifndef X_INTERPRETER_H
#define X_INTERPRETER_H

#include "expr.h"
#include "stmt.h"

typedef enum {
    VAL_NUMBER,
    VAL_BOOL,
    VAL_VOID,
    VAL_STRING,
    VAL_VECTOR,
    VAL_HASHMAP

} ValueType;

struct Value;
typedef struct Value {
    ValueType type;
    union {
        double number;
        int boolean;
        char* string;
        struct {
            struct Value* items;
            int count;
            int capacity;
        } vector;

        struct {
            char** keys;
            struct Value* values;
            int count;
            int capacity;
        } hashmap;
    };
} Value;

static inline Value numberVal(double value) {
    Value v;
    v.type = VAL_NUMBER;
    v.number = value;
    return v;
}

static inline Value boolVal(int value) {
    Value v;
    v.type = VAL_BOOL;
    v.boolean = value;
    return v;
}

static inline Value voidVal() {
    Value v;
    v.type = VAL_VOID;
    return v;
}

void interpret(Stmt* stmt);

#endif