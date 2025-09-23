#ifndef UAST_GET_SCOPE_ID_H
#define UAST_GET_SCOPE_ID_H

#include <util.h>
#include <uast.h>

static inline bool uast_function_call_get_scope_id(Scope_id* result, const Uast_function_call* call);

static inline bool uast_expr_get_scope_id(Scope_id* result, const Uast_expr* expr);

static inline bool uast_function_call_get_scope_id(Scope_id* result, const Uast_function_call* call) {
    return uast_expr_get_scope_id(result, call->callee);
}

static inline Scope_id uast_member_access_get_scope_id(const Uast_member_access* access) {
    return access->member_name->name.scope_id;
}

static inline bool uast_expr_get_scope_id(Scope_id* result, const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_IF_ELSE_CHAIN:
            todo();
        case UAST_BLOCK:
            todo();
        case UAST_SWITCH:
            todo();
        case UAST_UNKNOWN:
            todo();
        case UAST_OPERATOR:
            todo();
        case UAST_SYMBOL:
            *result = uast_symbol_const_unwrap(expr)->name.scope_id;
            return true;
        case UAST_MEMBER_ACCESS:
            *result = uast_member_access_get_scope_id(uast_member_access_const_unwrap(expr));
            return true;
        case UAST_INDEX:
            todo();
        case UAST_LITERAL:
            return false;
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_ARRAY_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_MACRO:
            todo();
        case UAST_ENUM_ACCESS:
            todo();
        case UAST_ENUM_GET_TAG:
            todo();
    }
    unreachable("");
}

#endif // UAST_GET_SCOPE_ID_H
