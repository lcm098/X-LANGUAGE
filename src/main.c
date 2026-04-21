#include "../include/common.h"
#include "../include/x.h"

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Usage: x [script name]\n");
        exit(1);
    } else if (argc == 2) {
        const char* filename = argv[1];
        const char* dot = strrchr(filename, '.');
        if (dot == NULL || strcmp(dot, ".x") != 0) {
            printf("Error: File must have a '.x' extension.\n");
            exit(1);
        }
        runFile(filename);
    } else {
        runPrompt();
    }
}