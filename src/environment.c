#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/environment.h"

void initEnvironment(Environment* env) {
    env->head = NULL;
}

void defineVariable(Environment* env, const char* name, Value value) {
    EnvEntry* entry = malloc(sizeof(EnvEntry));
    entry->name = strdup(name);
    entry->value = value;
    entry->next = env->head;
    env->head = entry;
}

Value getVariable(Environment* env, const char* name) {
    EnvEntry* current = env->head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    printf("Runtime Error: Undefined variable '%s'\n", name);
    return voidVal();
}

void assignVariable(Environment* env, const char* name, Value value) {
    EnvEntry* current = env->head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            current->value = value;
            return;
        }
        current = current->next;
    }
    printf("Runtime Error: Undefined variable '%s'\n", name);
}