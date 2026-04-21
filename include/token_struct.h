#ifndef X_TOKEN_STRUCT_H
#define X_TOKEN_STRUCT_H

#include "token.h"

typedef struct {

    TokenType type;
    const char* start;
    int length;
    int line;

} Token;

#endif