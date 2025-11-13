#ifndef ULANG_TYPE_REMOVE_EXPR_H
#define ULANG_TYPE_REMOVE_EXPR_H

#include <ulang_type.h>

// TODO: remove this forward decl?
bool uast_expr_to_ulang_type(Ulang_type* result, const Uast_expr* expr);

// will report error
static bool ulang_type_remove_expr(Ulang_type* new_lang_type, Ulang_type lang_type) {
    if (lang_type.type != ULANG_TYPE_EXPR) {
        *new_lang_type = lang_type;
        return true;
    }
    Ulang_type_expr expr = ulang_type_expr_const_unwrap(lang_type);

    Ulang_type inner = {0};
    if (!uast_expr_to_ulang_type(&inner, expr.expr)) {
        return false;
    }
    ulang_type_add_pointer_depth(&inner, expr.pointer_depth);
    assert(inner.type != ULANG_TYPE_EXPR);
    *new_lang_type = inner;
    return true;
}

#endif // ULANG_TYPE_REMOVE_EXPR_H
