#ifndef X_ENVIRONMENT_H
#define X_ENVIRONMENT_H

#include "token.h"
#include "interpreter.h"

typedef struct EnvEntry {
    char* name;
    Value value;
    struct EnvEntry* next;
} EnvEntry;

typedef struct {
    EnvEntry* head;
} Environment;

void initEnvironment(Environment* env);
void defineVariable(Environment* env, const char* name, Value value);
Value getVariable(Environment* env, const char* name);
void assignVariable(Environment* env, const char* name, Value value);

#endif