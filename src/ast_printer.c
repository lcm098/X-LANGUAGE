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

        case EXPR_VAR:
            printf("(var %.*s", expr->var_expr.name.length, expr->var_expr.name.start);
            if (expr->var_expr.initializer != NULL) {
                printf(" ");
                printExpr(expr->var_expr.initializer);
            }
            printf(")");
            break;

        case EXPR_PREFIX:
            printf("(%.*s %.*s)",
                expr->prefix.operator.length, expr->prefix.operator.start,
                expr->prefix.name.length,     expr->prefix.name.start);
            break;

        case EXPR_POSTFIX:
            printf("(%.*s %.*s)",
                expr->postfix.name.length,     expr->postfix.name.start,
                expr->postfix.operator.length, expr->postfix.operator.start);
            break;

        case EXPR_VECTOR:
            printf("(vector");
            for (int i = 0; i < expr->vec.count; i++) {
                printf(" ");
                printExpr(expr->vec.items[i]);
            }
            printf(")");
            break;

        case EXPR_HASHMAP:
            printf("(hashmap");
            for (int i = 0; i < expr->hashmap.count; i++) {
                printf(" (");
                printExpr(expr->hashmap.keys[i]);
                printf(": ");
                printExpr(expr->hashmap.values[i]);
                printf(")");
            }
            printf(")");
            break;

        case EXPR_INDEX:
            printf("(index ");
            printExpr(expr->index_expr.object);
            printf(" ");
            printExpr(expr->index_expr.index);
            printf(")");
            break;

        case EXPR_INDEX_ASSIGN:
            printf("(index-assign ");
            printExpr(expr->index_assign.object);
            printf(" ");
            printExpr(expr->index_assign.index);
            printf(" ");
            printExpr(expr->index_assign.value);
            printf(")");
            break;

        case EXPR_LOGICAL:
            printf("(%.*s ", expr->logical.operator.length, expr->logical.operator.start);
            printExpr(expr->logical.left);
            printf(" ");
            printExpr(expr->logical.right);
            printf(")");
            break;

        case EXPR_BLOCK:
            printf("(block");
            for (int i = 0; i < expr->block.count; i++)
            {
                printf(" stmt%d", i);
            }
            printf(")");
            break;

        case EXPR_CALL:
        {
            printf("(call ");
            printExpr(expr->call.callee);
            for (int i = 0; i < expr->call.argCount; i++)
            {
                printf(" ");
                if (expr->call.argNames[i])
                    printf("%s=", expr->call.argNames[i]);
                printExpr((Expr *)expr->call.argValues[i]);
            }
            printf(")");
            break;
        }

        case EXPR_LAMBDA:
        {
            printf("(lambda (");
            for (int i = 0; i < expr->lambda.paramCount; i++)
            {
                if (i > 0)
                    printf(", ");
                printf("%.*s", expr->lambda.params[i].length,
                       expr->lambda.params[i].start);
            }
            printf(") ...)");
            break;
        }
        }
}