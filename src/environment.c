#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/environment.h"

void initEnvironment(Environment* env, Environment* enclosing) {
    env->head = NULL;
    env->enclosing = enclosing;
}

void defineVariable(Environment* env, const char* name, Value value) {
    EnvEntry* entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->value = value;
    entry->next = env->head;
    env->head = entry;
}

// FIX: parameter was 'char*' but header declares 'const char*' — made consistent
Value getVariable(Environment* env, const char* name) {
    Environment* currentEnv = env;
    while (currentEnv != NULL)
    {
        EnvEntry* entry = currentEnv->head;
        while (entry != NULL)
        {
            if (strcmp(entry->name, name) == 0)
            {
                return entry->value;
            }
            entry = entry->next;
        }
        currentEnv = currentEnv->enclosing;
    }

    printf("Runtime Error: Undefined variable '%s'\n", name);
    return voidVal();
}

void assignVariable(Environment* env, const char* name, Value value) {
    Environment* currentEnv = env;

    while (currentEnv != NULL)
    {
        EnvEntry* entry = currentEnv->head;
        while (entry != NULL)
        {
            if (strcmp(entry->name, name) == 0)
            {
                entry->value = value;
                return;
            }
            entry = entry->next;
        }
        currentEnv = currentEnv->enclosing;
    }

    printf("Runtime Error: Undefined variable '%s'\n", name);
}