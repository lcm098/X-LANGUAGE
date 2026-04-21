#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/interpreter.h"
#include "../include/token.h"
#include "../include/environment.h"

Environment globals;

static char* tokenName(Token token) {
    char* name = malloc(token.length + 1);
    memcpy(name, token.start, token.length);
    name[token.length] = '\0';
    return name;
}

static Value stringVal(const char* start, int length) {
    Value v;
    v.type = VAL_STRING;
    // +1 for null terminator. We subtract 2 from length to remove quotes ("...")
    char* buffer = malloc(length - 1); 
    memcpy(buffer, start + 1, length - 2);
    buffer[length - 2] = '\0';
    v.string = buffer;
    return v;
}

// --- Helpers ---

// Safely checks equality between two Values based on their internal types
static int valuesEqual(Value a, Value b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case VAL_BOOL:   return a.boolean == b.boolean;
        case VAL_VOID:    return 1; // nil is always equal to nil
        case VAL_NUMBER: return a.number == b.number;
        case VAL_STRING: return strcmp(a.string, b.string) == 0;
        default:         return 0;
    }
}

static Value evaluate(Expr* expr);

// --- Evaluation Logic ---

static Value evalLiteral(Expr* expr) {
    switch (expr->literal.type) {
        case TOKEN_NUMBER: {
            // strtod is used because expr->literal.value points to a buffer 
            // that isn't null-terminated (part of the larger source string).
            char* endPtr;
            double value = strtod(expr->literal.value, &endPtr);
            return numberVal(value);
        }
        case TOKEN_TRUE:  return boolVal(1);
        case TOKEN_FALSE: return boolVal(0);
        case TOKEN_VOID:   return voidVal();
        case TOKEN_STRING: return stringVal(expr->literal.value, expr->literal.length);
        default:          return voidVal();
    }
}

static Value evalGrouping(Expr* expr) {
    return evaluate(expr->grouping.expression);
}

static Value evalUnary(Expr* expr) {
    Value right = evaluate(expr->unary.right);

    switch (expr->unary.operator.type) {
        case TOKEN_MINUS:
            // Ensure we only negate numbers
            if (right.type != VAL_NUMBER) return voidVal(); 
            return numberVal(-right.number);

        case TOKEN_BANG:
            // Invert the boolean "truthiness"
            return boolVal(!right.boolean);

        default:
            return voidVal();
    }
}

static double asNumber(Value value) {
    if (value.type == VAL_NUMBER) return value.number;
    if (value.type == VAL_BOOL)   return value.boolean ? 1.0 : 0.0;
    return 0.0; // Nil or other types
}

static Value evalBinary(Expr* expr) {
    Value left = evaluate(expr->binary.left);
    Value right = evaluate(expr->binary.right);

    switch (expr->binary.operator.type) {

        // --- Arithmetic Operators (Require Numbers) ---
        case TOKEN_PLUS:
            // Handle String Concatenation
            if (left.type == VAL_STRING && right.type == VAL_STRING) {
                int lenL = strlen(left.string);
                int lenR = strlen(right.string);
                char* dest = malloc(lenL + lenR + 1);
                memcpy(dest, left.string, lenL);
                memcpy(dest + lenL, right.string, lenR);
                dest[lenL + lenR] = '\0';
                
                Value v; v.type = VAL_STRING; v.string = dest;
                return v;
            }
            return numberVal(asNumber(left) + asNumber(right));

        case TOKEN_MINUS:
            return numberVal(asNumber(left) - asNumber(right));

        case TOKEN_STAR:
            return numberVal(asNumber(left) * asNumber(right));

        case TOKEN_SLASH: {
            double divisor = asNumber(right);
            if (divisor == 0) {
                printf("Runtime Error: Division by zero.\n");
                return voidVal();
            }
            return numberVal(asNumber(left) / divisor);
        }

        case TOKEN_PERCENT: {
            // Convert both sides to numbers automatically (true becomes 1.0)
            double leftNum = asNumber(left);
            double rightNum = asNumber(right);

            if (rightNum == 0) {
                printf("Runtime Error: Division by zero in modulo.\n");
                return voidVal();
            }
            
            return numberVal((int)leftNum % (int)rightNum);
        }

        // --- Comparison Operators (Require Numbers) ---
        case TOKEN_GREATER:
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return boolVal(left.number > right.number);
            }
            printf("Runtime Error: Operands must be numbers for comparison.\n");
            return voidVal();

        case TOKEN_GREATER_EQUAL:
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return boolVal(left.number >= right.number);
            }
            printf("Runtime Error: Operands must be numbers for comparison.\n");
            return voidVal();

        case TOKEN_LESS:
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return boolVal(left.number < right.number);
            }
            printf("Runtime Error: Operands must be numbers for comparison.\n");
            return voidVal();

        case TOKEN_LESS_EQUAL:
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return boolVal(left.number <= right.number);
            }
            printf("Runtime Error: Operands must be numbers for comparison.\n");
            return voidVal();

        // --- Equality Operators (Type Safe) ---
        case TOKEN_EQUAL_EQUAL:
            return boolVal(valuesEqual(left, right));

        case TOKEN_BANG_EQUAL:
            return boolVal(!valuesEqual(left, right));

        default:
            return voidVal();
    }
}

// The core recursive visitor for the expression tree
static Value evaluate(Expr* expr) {
    if (expr == NULL) return voidVal();

    switch (expr->type) {
        case EXPR_LITERAL:  return evalLiteral(expr);
        case EXPR_GROUPING: return evalGrouping(expr);
        case EXPR_UNARY:    return evalUnary(expr);
        case EXPR_BINARY:   return evalBinary(expr);
        case EXPR_VARIABLE: {
            char* name = tokenName(expr->variable.name);
            Value v = getVariable(&globals, name);
            free(name);
            return v;
        }
        case EXPR_ASSIGN: {
            Value value = evaluate(expr->assign.value);
            char* name = tokenName(expr->assign.name);
            assignVariable(&globals, name, value);
            free(name);
            return value;
        }
    }

    return voidVal();
}

static void printValue(Value value) {
    switch (value.type) {
        case VAL_NUMBER:
            printf("%g", value.number);
            break;
        case VAL_BOOL:
            printf(value.boolean ? "true" : "false");
            break;
        case VAL_STRING: 
            printf("%s", value.string); 
            break;
        case VAL_VOID:
            printf("void");
            break;
    }
}

static void execute(Stmt* stmt)
{
    switch (stmt->type)
    {
        case STMT_PRINT:
        {
            Value value = evaluate(stmt->print.expression);
            printValue(value);
            printf("\n");
            break;
        }

        case STMT_VAR:
        {
            Value value = voidVal();
            if (stmt->var.initializer != NULL)
                value = evaluate(stmt->var.initializer);

            char* name = tokenName(stmt->var.name);
            defineVariable(&globals, name, value);
            free(name);
            break;
        }

        case STMT_EXPR:
        {
            evaluate(stmt->expr.expression);
            break;
        }
    }
}

void interpret(Stmt* stmt) {
    static int initialized = 0;
    if (!initialized) {
        initEnvironment(&globals);
        initialized = 1;
    }
    execute(stmt);
}