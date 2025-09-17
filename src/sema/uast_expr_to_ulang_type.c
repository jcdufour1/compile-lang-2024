#include <uast_expr_to_ulang_type.h>
#include <lang_type_from_ulang_type.h>

bool uast_expr_to_ulang_type(Ulang_type* result, const Uast_expr* expr) {
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
        case UAST_SYMBOL: {
            Name sym_name = uast_symbol_const_unwrap(expr)->name;
            *result = ulang_type_regular_const_wrap(ulang_type_regular_new(
                ulang_type_atom_new(
                    name_to_uname(sym_name),
                    0
                ),
                //name_to_uname(uast_symbol_const_unwrap(expr)->name),
                uast_symbol_const_unwrap(expr)->pos
            ));

            return true;
        }
        case UAST_MEMBER_ACCESS:
            todo();
        case UAST_INDEX:
            todo();
        case UAST_LITERAL:
            todo();
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
