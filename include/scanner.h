#ifndef X_SCANNER_H
#define X_SCANNER_H

#include "token_struct.h"

void initScanner(const char* source);
Token scanToken();

#endif