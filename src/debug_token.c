#include "../include/debug_token.h"

void printToken(Token token)
{
    printf("Line %d: Type %d -> ",
           token.line,
           token.type);

    for (int i = 0; i < token.length; i++)
        printf("%c", token.start[i]);

    printf("\n");
}