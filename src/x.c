#include "../include/common.h"
#include "../include/x.h"
#include "../include/scanner.h"
#include "../include/debug_token.h"
#include "../include/parser.h"
#include "../include/ast_printer.h"
#include "../include/interpreter.h"


static void run(const char* source)
{
    initScanner(source);
    Stmt* stmt = parse();
    interpret(stmt);

}

void runPrompt() {   
    int capacity = 8;
    char *line = (char *)malloc(capacity * sizeof(char));

    if(line == NULL) {
        fprintf(stderr, "Memory Allocation failed\n");
        exit(1);
    }

    while(true) {
        int length = 0;
        printf(">>> ");

        while (true) {
            int ch = getchar();

            if (ch == '~') { 
                free(line);
                return; 
            }

            if(ch == '\n' || ch == EOF) {
                line[length] = '\0';
                break;
            }

            if(length + 1 >= capacity) {
                capacity *= 2;
                char *temp = (char *)realloc(line, capacity * sizeof(char));
                if (temp == NULL) {
                    free(line);
                    exit(1);
                }
                line = temp;
            }
            line[length++] = (char)ch;
        }

        if (length > 0) {
            run(line);
        }
    }
}

static void runSource(const char* source)
{
    initScanner(source);
    initParser();
    while (!parserAtEnd()) {
        Stmt* stmt = parseNext();
        interpret(stmt);
    }
}

void runFile(const char *path)
{
    FILE *file = fopen(path, "rb");

    if(!file) {
        fprintf(stderr, "Error opening source code file of X");
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer)
    {
        printf("Not enough memory. to read source code\n");
        exit(74);
    }

    size_t readSize = fread(buffer, sizeof(char), size, file);
    if (readSize < size)
    {
        printf("Could not read file.\n");
        exit(74);
    }
    buffer[readSize] = '\0';
    fclose(file);
    runSource(buffer);
    free(buffer);
}