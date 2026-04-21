#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/interpreter.h"
#include "../include/token.h"
#include "../include/environment.h"

Environment globals;
Environment* environment = &globals;

// FIX: 'execute()' calls 'executeBlock()' before its definition — forward-declare it
static void executeBlock(Stmt** statements, int count, Environment* newEnv);

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

static Value indexGet(Value object, Value key) {
    if (object.type == VAL_VECTOR) {
        if (key.type != VAL_NUMBER) {
            printf("Runtime Error: Vector index must be a number.\n");
            return voidVal();
        }
        int i = (int)key.number;
        if (i < 0 || i >= object.vector.count) {
            printf("Runtime Error: Vector index %d out of bounds.\n", i);
            return voidVal();
        }
        return object.vector.items[i];
    }
    if (object.type == VAL_HASHMAP) {
        if (key.type != VAL_STRING) {
            printf("Runtime Error: Hashmap key must be a string.\n");
            return voidVal();
        }
        for (int i = 0; i < object.hashmap.count; i++) {
            if (strcmp(object.hashmap.keys[i], key.string) == 0)
                return object.hashmap.values[i];
        }
        printf("Runtime Error: Key '%s' not found.\n", key.string);
        return voidVal();
    }
    printf("Runtime Error: Value is not indexable.\n");
    return voidVal();
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
            Value v = getVariable(environment, name);
            free(name);
            return v;
        }
        case EXPR_ASSIGN: {
            Value value = evaluate(expr->assign.value);
            char* name = tokenName(expr->assign.name);
            assignVariable(environment, name, value);
            free(name);
            return value;
        }
        case EXPR_VAR: {
            Value value = voidVal();

            if (expr->var_expr.initializer != NULL) {
                value = evaluate(expr->var_expr.initializer);
            }

            char* name = tokenName(expr->var_expr.name);
            defineVariable(environment, name, value);
            free(name);
            return boolVal(1);
        }

        case EXPR_PREFIX: {
            char* name = tokenName(expr->prefix.name);
            Value val = getVariable(environment, name);
            double newVal = val.number + (expr->prefix.operator.type == TOKEN_PLUS_PLUS ? 1 : -1);
            Value result = numberVal(newVal);
            assignVariable(environment, name, result);
            free(name);
            return result; // returns new value
        }
        case EXPR_POSTFIX: {
            char* name = tokenName(expr->postfix.name);
            Value old = getVariable(environment, name);
            double newVal = old.number + (expr->postfix.operator.type == TOKEN_PLUS_PLUS ? 1 : -1);
            assignVariable(environment, name, numberVal(newVal));
            free(name);
            return old; // returns original value
        }

        case EXPR_VECTOR: {
            Value v;
            v.type = VAL_VECTOR;
            v.vector.count    = expr->vec.count;
            v.vector.capacity = expr->vec.count;
            v.vector.items    = malloc(sizeof(Value) * expr->vec.count);
            for (int i = 0; i < expr->vec.count; i++)
                v.vector.items[i] = evaluate(expr->vec.items[i]);
            return v;
        }

        case EXPR_HASHMAP: {
            Value v;
            v.type = VAL_HASHMAP;
            v.hashmap.count    = expr->hashmap.count;
            v.hashmap.capacity = expr->hashmap.count;
            v.hashmap.keys     = malloc(sizeof(char*)  * expr->hashmap.count);
            v.hashmap.values   = malloc(sizeof(Value)  * expr->hashmap.count);
            for (int i = 0; i < expr->hashmap.count; i++) {
                Value key = evaluate(expr->hashmap.keys[i]);
                if (key.type != VAL_STRING) {
                    printf("Runtime Error: Hashmap keys must be strings.\n");
                    return voidVal();
                }
                v.hashmap.keys[i]   = key.string;
                v.hashmap.values[i] = evaluate(expr->hashmap.values[i]);
            }
            return v;
        }

        case EXPR_INDEX: {
            Value object = evaluate(expr->index_expr.object);
            Value key    = evaluate(expr->index_expr.index);
            return indexGet(object, key);
        }

        case EXPR_INDEX_ASSIGN: {
            Expr* obj = expr->index_assign.object;

            Expr* root = obj;
            while (root->type == EXPR_INDEX) root = root->index_expr.object;
            if (root->type != EXPR_VARIABLE) {
                printf("Runtime Error: Invalid assignment target.\n");
                return voidVal();
            }
            char* rootName = tokenName(root->variable.name);
            Value container = getVariable(environment, rootName);

            
            Expr* indexChain[64];
            int   chainLen = 0;
            Expr* cur = expr->index_assign.object;
            // push the final index
            indexChain[chainLen++] = expr->index_assign.index;
            // walk up collecting intermediate indices
            while (cur->type == EXPR_INDEX) {
                indexChain[chainLen++] = cur->index_expr.index;
                cur = cur->index_expr.object;
            }
            // indexChain is now in reverse order (innermost first), reverse it
            for (int i = 0, j = chainLen - 1; i < j; i++, j--) {
                Expr* tmp = indexChain[i];
                indexChain[i] = indexChain[j];
                indexChain[j] = tmp;
            }

            
            // We need to work with pointers into the actual stored arrays
            // Re-fetch a mutable reference by walking the live data
            Value newValue = evaluate(expr->index_assign.value);

            // For single-level: just set directly on the fetched container
            if (chainLen == 1) {
                Value key = evaluate(indexChain[0]);
                if (container.type == VAL_VECTOR) {
                    int i = (int)key.number;
                    if (i < 0 || i >= container.vector.count) {
                        printf("Runtime Error: Index %d out of bounds.\n", i);
                        free(rootName); return voidVal();
                    }
                    container.vector.items[i] = newValue;
                } else if (container.type == VAL_HASHMAP) {
                    int found = 0;
                    for (int i = 0; i < container.hashmap.count; i++) {
                        if (strcmp(container.hashmap.keys[i], key.string) == 0) {
                            container.hashmap.values[i] = newValue;
                            found = 1; break;
                        }
                    }
                    if (!found) {
                        // Insert new key
                        int c = container.hashmap.count;
                        container.hashmap.keys   = realloc(container.hashmap.keys,   sizeof(char*)  * (c + 1));
                        container.hashmap.values = realloc(container.hashmap.values, sizeof(Value)  * (c + 1));
                        container.hashmap.keys[c]   = strdup(key.string);
                        container.hashmap.values[c] = newValue;
                        container.hashmap.count++;
                    }
                }
                assignVariable(environment, rootName, container);
                free(rootName);
                return newValue;
            }

            Value* target = &container;
            for (int i = 0; i < chainLen - 1; i++) {
                Value key = evaluate(indexChain[i]);
                if (target->type == VAL_VECTOR)
                    target = &target->vector.items[(int)key.number];
                else if (target->type == VAL_HASHMAP) {
                    int found = 0;
                    for (int j = 0; j < target->hashmap.count; j++) {
                        if (strcmp(target->hashmap.keys[j], key.string) == 0) {
                            target = &target->hashmap.values[j];
                            found = 1; break;
                        }
                    }
                    if (!found) { printf("Runtime Error: Key not found.\n"); free(rootName); return voidVal(); }
                }
            }
            Value lastKey = evaluate(indexChain[chainLen - 1]);
            if (target->type == VAL_VECTOR)
                target->vector.items[(int)lastKey.number] = newValue;
            else if (target->type == VAL_HASHMAP) {
                int found = 0;
                for (int i = 0; i < target->hashmap.count; i++) {
                    if (strcmp(target->hashmap.keys[i], lastKey.string) == 0) {
                        target->hashmap.values[i] = newValue;
                        found = 1; break;
                    }
                }
                if (!found) {
                    int c = target->hashmap.count;
                    target->hashmap.keys   = realloc(target->hashmap.keys,   sizeof(char*)  * (c + 1));
                    target->hashmap.values = realloc(target->hashmap.values, sizeof(Value)  * (c + 1));
                    target->hashmap.keys[c]   = strdup(lastKey.string);
                    target->hashmap.values[c] = newValue;
                    target->hashmap.count++;
                }
            }
            assignVariable(environment, rootName, container);
            free(rootName);
            return newValue;
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
        case VAL_VECTOR:
            printf("[");
            for (int i = 0; i < value.vector.count; i++) {
                if (i > 0) printf(", ");
                printValue(value.vector.items[i]);
            }
            printf("]");
            break;

        case VAL_HASHMAP:
            printf("{");
            for (int i = 0; i < value.hashmap.count; i++) {
                if (i > 0) printf(", ");
                printf("\"%s\": ", value.hashmap.keys[i]);
                printValue(value.hashmap.values[i]);
            }
            printf("}");
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
            defineVariable(environment, name, value);
            free(name);
            break;
        }

        case STMT_EXPR:
        {
            evaluate(stmt->expr.expression);
            break;
        }
        case STMT_BLOCK:
        {
            Environment newEnv;
            initEnvironment(&newEnv, environment);
            executeBlock(stmt->block.statements, stmt->block.count, &newEnv);
            break;
        }
    }
}

static void executeBlock(Stmt** statements, int count, Environment* newEnv)
{
    Environment* previous = environment;
    environment = newEnv;
    for (int i = 0; i < count; i++)
    {
        execute(statements[i]);
    }
    environment = previous;
}

void interpret(Stmt* stmt) {
    static int initialized = 0;
    if (!initialized) {
        initEnvironment(&globals, NULL);
        initialized = 1;
    }
    execute(stmt);
}