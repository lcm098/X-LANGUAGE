#include "../include/common.h"
#include "../include/ast_printer.h"

static void printParenthesized(const char* name, Expr* left, Expr* right)
{
    printf("(%s ", name);
    if (left != NULL) {
        printExpr(left);
        printf(" ");
    }
    printExpr(right);
    printf(")");
}

static void printLiteral(Expr* expr)
{
    printf("%s", expr->literal.value);
}

static void printGrouping(Expr* expr)
{
    printf("(group ");
    printExpr(expr->grouping.expression);
    printf(")");
}

static void printUnary(Expr* expr)
{
    printf("(%.*s ",
           expr->unary.operator.length,
           expr->unary.operator.start);

    printExpr(expr->unary.right);
    printf(")");
}

static void printBinary(Expr* expr)
{
    // Convert the operator token to a temporary string for the helper
    char opName[16]; 
    snprintf(opName, sizeof(opName), "%.*s", 
             expr->binary.operator.length, 
             expr->binary.operator.start);

    printParenthesized(opName, expr->binary.left, expr->binary.right);
}

void printExpr(Expr* expr)
{
    switch (expr->type)
    {
        case EXPR_LITERAL:
            printLiteral(expr);
            break;

        case EXPR_GROUPING:
            printGrouping(expr);
            break;

        case EXPR_UNARY:
            printUnary(expr);
            break;

        case EXPR_BINARY:
            printBinary(expr);
            break;

        case EXPR_VARIABLE:
            
            printf("(var %.*s)", expr->variable.name.length, expr->variable.name.start);
            break;

        case EXPR_ASSIGN:
            printf("(assign %.*s ", expr->assign.name.length, expr->assign.name.start);
            printExpr(expr->assign.value);
            printf(")");
            break;

    }
}