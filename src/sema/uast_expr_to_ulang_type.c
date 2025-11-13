#include <uast_expr_to_ulang_type.h>
#include <lang_type_from_ulang_type.h>
#include <ast_msg.h>

typedef enum {
    EXPR_TO_ULANG_TYPE_NORMAL,
    EXPR_TO_ULANG_TYPE_PTR_DEPTH,
    EXPR_TO_ULANG_TYPE_ERROR,

    // for static asserts
    EXPR_TO_ULANG_TYPE_COUNT,
} EXPR_TO_ULANG_TYPE;

static EXPR_TO_ULANG_TYPE uast_expr_to_ulang_type_internal(Ulang_type* result, int16_t* pointer_depth, const Uast_expr* expr);

static EXPR_TO_ULANG_TYPE uast_operator_to_ulang_type_internal(Ulang_type* result, int16_t* pointer_depth, const Uast_operator* oper) {
    switch (oper->type) {
        case UAST_BINARY: {
            const Uast_binary* bin = uast_binary_const_unwrap(oper);
            if (bin->token_type != BINARY_MULTIPLY) {
                msg_todo("interpreting this expression as a type", bin->pos);
                return EXPR_TO_ULANG_TYPE_ERROR;
            }

            int16_t rhs_ptr_depth = 0;
            Ulang_type rhs_lang_type = {0};
            static_assert(EXPR_TO_ULANG_TYPE_COUNT == 3, "exhausive handling of EXPR_TO_ULANG_TYPE");
            switch (uast_expr_to_ulang_type_internal(&rhs_lang_type, &rhs_ptr_depth, bin->rhs)) {
                case EXPR_TO_ULANG_TYPE_NORMAL:
                    msg_todo("interpreting this expression as a type", bin->pos);
                    return EXPR_TO_ULANG_TYPE_ERROR;
                case EXPR_TO_ULANG_TYPE_PTR_DEPTH:
                    break;
                case EXPR_TO_ULANG_TYPE_ERROR:
                    return EXPR_TO_ULANG_TYPE_ERROR;
                default:
                    unreachable("");
            }

            if (!uast_expr_to_ulang_type(result, bin->lhs)) {
                return EXPR_TO_ULANG_TYPE_ERROR;
            }

            ulang_type_add_pointer_depth(result, 1 + rhs_ptr_depth);
            return EXPR_TO_ULANG_TYPE_NORMAL;
        }
        case UAST_UNARY: {
            const Uast_unary* unary = uast_unary_const_unwrap(oper);
            if (unary->token_type != UNARY_DEREF) {
                msg_todo("interpreting this expression as a type", uast_operator_get_pos(oper));
                return EXPR_TO_ULANG_TYPE_ERROR;
            }

            Ulang_type child_lang_type = {0};
            static_assert(EXPR_TO_ULANG_TYPE_COUNT == 3, "exhausive handling of EXPR_TO_ULANG_TYPE");
            switch (uast_expr_to_ulang_type_internal(&child_lang_type, pointer_depth, unary->child)) {
                case EXPR_TO_ULANG_TYPE_NORMAL:
                    msg_todo("interpreting this expression as a type", uast_operator_get_pos(oper));
                    return EXPR_TO_ULANG_TYPE_ERROR;
                case EXPR_TO_ULANG_TYPE_PTR_DEPTH:
                    (*pointer_depth)++;
                    return EXPR_TO_ULANG_TYPE_PTR_DEPTH;
                case EXPR_TO_ULANG_TYPE_ERROR:
                    return EXPR_TO_ULANG_TYPE_ERROR;
                default:
                    unreachable("");
            }
            unreachable("");
        }
    }
    unreachable("");
}

static EXPR_TO_ULANG_TYPE uast_symbol_to_ulang_type_internal(Ulang_type* result, const Uast_symbol* sym) {
    Uast_def* sym_def = NULL;
    if (usymbol_lookup(&sym_def, sym->name)) {
        switch (sym_def->type) {
            case UAST_LABEL:
                msg(DIAG_INVALID_TYPE, sym->pos, "label name is not allowed here\n");
                return EXPR_TO_ULANG_TYPE_ERROR;
            case UAST_VOID_DEF:
                *result = ulang_type_new_void(uast_def_get_pos(sym_def));
                return EXPR_TO_ULANG_TYPE_NORMAL;
            case UAST_POISON_DEF:
                todo();
                return EXPR_TO_ULANG_TYPE_ERROR;
            case UAST_IMPORT_PATH:
                unreachable("");
            case UAST_MOD_ALIAS:
                unreachable("");
            case UAST_GENERIC_PARAM:
                break;
            case UAST_FUNCTION_DEF:
                msg(DIAG_INVALID_TYPE, sym->pos, "symbol of function definition is not allowed here\n");
                return EXPR_TO_ULANG_TYPE_ERROR;
            case UAST_VARIABLE_DEF: {
                static uint64_t count = 0;
                if (count == 0) {
                    //todo();
                }
                count++;
                if (!env.silent_generic_resol_errors) {
                    msg(DIAG_INVALID_TYPE, sym->pos, "symbol of variable is not allowed here\n");
                }
                return EXPR_TO_ULANG_TYPE_ERROR;
            }
            case UAST_STRUCT_DEF:
                break;
            case UAST_RAW_UNION_DEF:
                break;
            case UAST_ENUM_DEF:
                break;
            case UAST_LANG_DEF:
                if (!env.silent_generic_resol_errors) {
                    msg_todo("", sym->pos);
                }
                return EXPR_TO_ULANG_TYPE_ERROR;
            case UAST_PRIMITIVE_DEF:
                break;
            case UAST_FUNCTION_DECL:
                todo();
            case UAST_BUILTIN_DEF:
                todo();
        }
    }


    *result = ulang_type_regular_const_wrap(ulang_type_regular_new(
        ulang_type_atom_new(
            name_to_uname(sym->name),
            0
        ),
        sym->pos
    ));

    return EXPR_TO_ULANG_TYPE_NORMAL;
}

static EXPR_TO_ULANG_TYPE uast_expr_to_ulang_type_internal(Ulang_type* result, int16_t* pointer_depth, const Uast_expr* expr) {
    switch (expr->type) {
        case UAST_IF_ELSE_CHAIN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_BLOCK:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_SWITCH:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_UNKNOWN:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_OPERATOR:
            return uast_operator_to_ulang_type_internal(result, pointer_depth, uast_operator_const_unwrap(expr));
        case UAST_SYMBOL:
            return uast_symbol_to_ulang_type_internal(result, uast_symbol_const_unwrap(expr));
        case UAST_MEMBER_ACCESS: {
            // TODO: make a helper function to convert member access to uname?
            const Uast_member_access* access = uast_member_access_const_unwrap(expr);
            if (access->callee->type != UAST_SYMBOL) {
                msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
                return EXPR_TO_ULANG_TYPE_ERROR;
            }

            Name memb_name = access->member_name->name;

            Uname new_name = uname_new(uast_symbol_unwrap(access->callee)->name, memb_name.base, memb_name.gen_args, memb_name.scope_id);
            Ulang_type_regular reg = ulang_type_regular_new(ulang_type_atom_new(new_name, 0), access->pos);
            *result = ulang_type_regular_const_wrap(reg);
            return EXPR_TO_ULANG_TYPE_NORMAL;
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

            //return EXPR_TO_ULANG_TYPE_NORMAL;
        case UAST_INDEX: {
            const Uast_index* index = uast_index_const_unwrap(expr);
            if (index->index->type != UAST_EXPR_REMOVED) {
                msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
                return EXPR_TO_ULANG_TYPE_ERROR;
            }

            Ulang_type item_type = {0};
            int16_t dummy = 0;
            EXPR_TO_ULANG_TYPE status = uast_expr_to_ulang_type_internal(&item_type, &dummy, index->callee);
            if (status != EXPR_TO_ULANG_TYPE_NORMAL) {
                return status;
            }
            *result = ulang_type_new_slice(index->pos, item_type, 0 /* TODO */);
            return EXPR_TO_ULANG_TYPE_NORMAL;
        }
        case UAST_LITERAL: {
            const Uast_literal* lit = uast_literal_const_unwrap(expr);
            if (lit->type != UAST_INT) {
                msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
                return EXPR_TO_ULANG_TYPE_ERROR;
            }
            const Uast_int* lang_int = uast_int_const_unwrap(lit);
            *result = ulang_type_int_const_wrap(ulang_type_int_new(lang_int->data, 0, lang_int->pos));
            return EXPR_TO_ULANG_TYPE_NORMAL;
        }
        case UAST_FUNCTION_CALL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_STRUCT_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_ARRAY_LITERAL:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_TUPLE:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_MACRO:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_ENUM_ACCESS:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_ENUM_GET_TAG:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return EXPR_TO_ULANG_TYPE_ERROR;
        case UAST_EXPR_REMOVED:
            *pointer_depth = 0;
            return EXPR_TO_ULANG_TYPE_PTR_DEPTH;
    }
    unreachable("");
}

bool uast_expr_to_ulang_type(Ulang_type* result, const Uast_expr* expr) {
    int16_t pointer_depth = 0;
    static_assert(EXPR_TO_ULANG_TYPE_COUNT == 3, "exhausive handling of EXPR_TO_ULANG_TYPE");
    switch (uast_expr_to_ulang_type_internal(result, &pointer_depth, expr)) {
        case EXPR_TO_ULANG_TYPE_NORMAL:
            return true;
        case EXPR_TO_ULANG_TYPE_PTR_DEPTH:
            msg_todo("interpreting this expression as a type", uast_expr_get_pos(expr));
            return false;
        case EXPR_TO_ULANG_TYPE_ERROR:
            return false;
        default:
            unreachable("");
    }
    unreachable("");
}

