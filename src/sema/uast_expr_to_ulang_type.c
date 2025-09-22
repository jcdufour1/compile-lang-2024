#include <uast_expr_to_ulang_type.h>
#include <lang_type_from_ulang_type.h>

bool uast_expr_to_ulang_type(Ulang_type* result, const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_IF_ELSE_CHAIN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_BLOCK:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_SWITCH:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_UNKNOWN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_OPERATOR:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
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
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_INDEX:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_FUNCTION_CALL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_STRUCT_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_ARRAY_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_TUPLE:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_MACRO:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_ENUM_ACCESS:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
        case UAST_ENUM_GET_TAG:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
    }
    unreachable("");
}
