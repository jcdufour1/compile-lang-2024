#include <uast_clone.h>
#include <parser_utils.h>

Uast_number* uast_number_clone(const Uast_number* lit) {
    return uast_number_new(lit->pos, lit->data);
}

Uast_string* uast_string_clone(const Uast_string* lit) {
    return uast_string_new(lit->pos, lit->data, util_literal_name_new());
}

Uast_literal* uast_literal_clone(const Uast_literal* lit) {
    switch (lit->type) {
        case UAST_NUMBER:
            return uast_number_wrap(uast_number_clone(uast_number_const_unwrap(lit)));
        case UAST_STRING:
            return uast_string_wrap(uast_string_clone(uast_string_const_unwrap(lit)));
        case UAST_ENUM_LIT:
            todo();
        case UAST_CHAR:
            todo();
        case UAST_VOID:
            todo();
    }
    unreachable("");
}

Uast_expr* uast_expr_clone(const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_OPERATOR:
            todo();
        case UAST_SYMBOL:
            todo();
        case UAST_MEMBER_ACCESS:
            todo();
        case UAST_INDEX:
            todo();
        case UAST_LITERAL:
            return uast_literal_wrap(uast_literal_clone(uast_literal_const_unwrap(expr)));
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_SUM_ACCESS: // TODO: remove uast_sum_access if not used
            todo();
    }
    unreachable("");
}

