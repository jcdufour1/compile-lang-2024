#include <uast_expr_to_ulang_type.h>
#include <lang_type_from_ulang_type.h>

bool uast_expr_to_ulang_type(Ulang_type* result, const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_IF_ELSE_CHAIN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_BLOCK:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_SWITCH:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_UNKNOWN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_OPERATOR:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
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
        case UAST_MEMBER_ACCESS: {
            // TODO: make a helper function to convert member access to uname?
            const Uast_member_access* access = uast_member_access_const_unwrap(expr);
            if (access->callee->type != UAST_SYMBOL) {
                msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
                return false;
            }

            Name memb_name = access->member_name->name;

            Uname new_name = uname_new(uast_symbol_unwrap(access->callee)->name, memb_name.base, memb_name.gen_args, memb_name.scope_id);
            Ulang_type_regular reg = ulang_type_regular_new(ulang_type_atom_new(new_name, 0), access->pos);
            *result = ulang_type_regular_const_wrap(reg);
            return true;
        }
            //Name sym_name = uast_symbol_const_unwrap(expr)->name;
            //*result = ulang_type_regular_const_wrap(ulang_type_regular_new(
            //    ulang_type_atom_new(
            //        name_to_uname(sym_name),
            //        0
            //    ),
            //    //name_to_uname(uast_symbol_const_unwrap(expr)->name),
            //    uast_symbol_const_unwrap(expr)->pos
            //));

            //return true;
        case UAST_INDEX:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_FUNCTION_CALL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_STRUCT_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_ARRAY_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_TUPLE:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_MACRO:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_ENUM_ACCESS:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_ENUM_GET_TAG:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case UAST_EXPR_REMOVED:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
    }
    unreachable("");
}
