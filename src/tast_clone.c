#include <tast_clone.h>

Tast_expr* tast_expr_clone(const Tast_expr* expr) {
    switch (expr->type) {
        case TAST_ASSIGNMENT:
            todo();
        case TAST_OPERATOR:
            todo();
        case TAST_SYMBOL:
            todo();
        case TAST_MEMBER_ACCESS:
            todo();
        case TAST_INDEX:
            todo();
        case TAST_LITERAL:
            todo();
        case TAST_FUNCTION_CALL:
            todo();
        case TAST_STRUCT_LITERAL:
            todo();
        case TAST_TUPLE:
            todo();
        case TAST_SUM_CALLEE:
            todo();
        case TAST_SUM_CASE:
            todo();
        case TAST_SUM_ACCESS:
            todo();
        case TAST_SUM_GET_TAG:
            todo();
        case TAST_IF_ELSE_CHAIN:
            todo();
        case TAST_MODULE_ALIAS:
            todo();
    }
    unreachable("");
}

