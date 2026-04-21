
#ifndef X_PARSER_H
#define X_PARSER_H

#include "expr.h"
#include "stmt.h"

Stmt* parse();
void  initParser();
bool  parserAtEnd();
Stmt* parseNext();
Stmt* parse();

#endif