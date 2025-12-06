#include <ulang_type_is_equal.h>
#include <uast_expr_to_ulang_type.h>
#include <ulang_type_remove_expr.h>

bool ulang_type_atom_is_equal(Ulang_type_atom a, Ulang_type_atom b) {
    return uname_is_equal(a.str, b.str) && a.pointer_depth == b.pointer_depth;
}

bool ulang_type_regular_is_equal(Ulang_type_regular a, Ulang_type_regular b) {
    return ulang_type_atom_is_equal(a.atom, b.atom);
}

bool ulang_type_tuple_is_equal(Ulang_type_tuple a, Ulang_type_tuple b) {
    vec_foreach(idx, Ulang_type, curr, a.ulang_types) {
        if (!ulang_type_is_equal(curr, vec_at(b.ulang_types, idx))) {
            return false;
        }
    }
    return true;
}

bool ulang_type_fn_is_equal(Ulang_type_fn a, Ulang_type_fn b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    if (!ulang_type_tuple_is_equal(a.params, b.params)) {
        return false;
    }
    return ulang_type_is_equal(*a.return_type, *b.return_type);
}

bool ulang_type_array_is_equal(Ulang_type_array a, Ulang_type_array b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    if (!ulang_type_is_equal(*a.item_type, *b.item_type)) {
        return false;
    }

    if (a.count->type != UAST_LITERAL || b.count->type != UAST_LITERAL) {
        msg_todo("", uast_expr_get_pos(a.count));
        msg_todo("", uast_expr_get_pos(b.count));
        return false;
    }
    Uast_literal* a_lit = uast_literal_unwrap(a.count);
    Uast_literal* b_lit = uast_literal_unwrap(b.count);
    if (a_lit->type != UAST_INT || b_lit->type != UAST_INT) {
        msg_todo("", uast_literal_get_pos(a_lit));
        msg_todo("", uast_literal_get_pos(a_lit));
        return false;
    }

    return uast_int_unwrap(a_lit)->data == uast_int_unwrap(b_lit)->data;
}

// TODO: move this function elsewhere?
static bool uast_literal_is_equal(const Uast_literal* a, const Uast_literal* b) {
    if (a->type != b->type) {
        return false;
    }

    switch (a->type) {
        case UAST_INT:
            return uast_int_const_unwrap(a)->data == uast_int_const_unwrap(b)->data;
        case UAST_STRING:
            todo();
        case UAST_VOID:
            return true;
        case UAST_FLOAT:
            return uast_float_const_unwrap(a)->data == uast_float_const_unwrap(b)->data;
    }
    unreachable("");
}

#define local_todo(expr) \
    msg_todo( \
        "this type of expression in struct literal in this situation", \
        uast_expr_get_pos(expr) \
    )

// TODO: move this function elsewhere?
// TODO: if default arguments are implemented for struct literals, this function could fail
static bool uast_struct_literal_is_equal(const Uast_struct_literal* a, const Uast_struct_literal* b) {
    if (a->members.info.count != b->members.info.count) {
        return false;
    }

    vec_foreach(idx, Uast_expr*, curr_a, a->members) {
        Uast_expr* curr_b = vec_at(b->members, idx);
        if (curr_a->type != curr_b->type) {
            return false;
        }

        switch (curr_a->type) {
            case UAST_IF_ELSE_CHAIN:
                local_todo(curr_a);
                return false;
            case UAST_BLOCK:
                local_todo(curr_a);
                return false;
            case UAST_SWITCH:
                local_todo(curr_a);
                return false;
            case UAST_UNKNOWN:
                local_todo(curr_a);
                return false;
            case UAST_OPERATOR:
                local_todo(curr_a);
                return false;
            case UAST_SYMBOL:
                return name_is_equal(uast_symbol_unwrap(curr_a)->name, uast_symbol_unwrap(curr_b)->name);
            case UAST_MEMBER_ACCESS:
                local_todo(curr_a);
                return false;
            case UAST_INDEX:
                local_todo(curr_a);
                return false;
            case UAST_LITERAL:
                if (!uast_literal_is_equal(uast_literal_unwrap(curr_a), uast_literal_unwrap(curr_b))) {
                    return false;
                }
                continue;
            case UAST_FUNCTION_CALL:
                local_todo(curr_a);
                return false;
            case UAST_STRUCT_LITERAL:
                if (!uast_struct_literal_is_equal(uast_struct_literal_unwrap(curr_a), uast_struct_literal_unwrap(curr_b))) {
                    return false;
                }
                continue;
            case UAST_ARRAY_LITERAL:
                local_todo(curr_a);
                return false;
            case UAST_TUPLE:
                local_todo(curr_a);
                return false;
            case UAST_MACRO:
                local_todo(curr_a);
                return false;
            case UAST_ENUM_ACCESS:
                local_todo(curr_a);
                return false;
            case UAST_ENUM_GET_TAG:
                local_todo(curr_a);
                return false;
            case UAST_ORELSE:
                local_todo(curr_a);
                return false;
            case UAST_FN:
                local_todo(curr_a);
                return false;
            case UAST_QUESTION_MARK:
                local_todo(curr_a);
                return false;
            case UAST_UNDERSCORE:
                local_todo(curr_a);
                return false;
            case UAST_EXPR_REMOVED:
                local_todo(curr_a);
                return false;
        }
        unreachable("");
    }

    return true;
}

bool uast_expr_is_equal(const Uast_expr* a, const Uast_expr* b) {
    if (a->type != b->type) {
        return false;
    }

    if (a->type == UAST_STRUCT_LITERAL) {
        return uast_struct_literal_is_equal(
            uast_struct_literal_const_unwrap(a),
            uast_struct_literal_const_unwrap(b)
        );
    }

    if (a->type != UAST_OPERATOR) {
        msg_todo("", uast_expr_get_pos(a));
        msg_todo("", uast_expr_get_pos(b));
        return false;
    }
    const Uast_operator* oper_a = uast_operator_const_unwrap(a);
    const Uast_operator* oper_b = uast_operator_const_unwrap(b);

    if (oper_a->type != UAST_UNARY || oper_b->type != UAST_UNARY) {
        msg_todo("", uast_expr_get_pos(a));
        msg_todo("", uast_expr_get_pos(b));
        return false;
    }
    const Uast_unary* unary_a = uast_unary_const_unwrap(oper_a);
    const Uast_unary* unary_b = uast_unary_const_unwrap(oper_b);

    if (!ulang_type_is_equal(unary_a->lang_type, unary_b->lang_type)) {
        return false;
    }
    return uast_expr_is_equal(unary_a->child, unary_b->child);
}

bool ulang_type_struct_lit_is_equal(Ulang_type_struct_lit a, Ulang_type_struct_lit b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return uast_expr_is_equal(a.expr, b.expr);
}

bool ulang_type_fn_lit_is_equal(Ulang_type_fn_lit a, Ulang_type_fn_lit b) {
    if (a.pointer_depth != b.pointer_depth) {
        return false;
    }
    return name_is_equal(a.name, b.name);
}

bool ulang_type_const_expr_is_equal(Ulang_type_const_expr a, Ulang_type_const_expr b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case ULANG_TYPE_INT:
            return ulang_type_int_const_unwrap(a).data == ulang_type_int_const_unwrap(b).data;
        case ULANG_TYPE_FLOAT_LIT:
            return ulang_type_float_lit_const_unwrap(a).data == ulang_type_float_lit_const_unwrap(b).data;
        case ULANG_TYPE_STRING_LIT:
            return strv_is_equal(ulang_type_string_lit_const_unwrap(a).data, ulang_type_string_lit_const_unwrap(b).data);
        case ULANG_TYPE_STRUCT_LIT:
            return ulang_type_struct_lit_is_equal(
                ulang_type_struct_lit_const_unwrap(a),
                ulang_type_struct_lit_const_unwrap(b)
            );
        case ULANG_TYPE_FN_LIT:
            return ulang_type_fn_lit_is_equal(
                ulang_type_fn_lit_const_unwrap(a),
                ulang_type_fn_lit_const_unwrap(b)
            );
    }
    unreachable("");
}

bool ulang_type_is_equal(Ulang_type a, Ulang_type b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_is_equal(ulang_type_regular_const_unwrap(a), ulang_type_regular_const_unwrap(b));
        case ULANG_TYPE_ARRAY:
            return ulang_type_array_is_equal(ulang_type_array_const_unwrap(a), ulang_type_array_const_unwrap(b));
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_is_equal(ulang_type_tuple_const_unwrap(a), ulang_type_tuple_const_unwrap(b));
        case ULANG_TYPE_FN:
            return ulang_type_fn_is_equal(ulang_type_fn_const_unwrap(a), ulang_type_fn_const_unwrap(b));
        case ULANG_TYPE_EXPR: {
            Ulang_type a_inner = {0};
            Ulang_type b_inner = {0};
            if (!ulang_type_remove_expr(&a_inner, a) || !ulang_type_remove_expr(&b_inner, b)) {
                return false;
            }
            return ulang_type_is_equal(a_inner, b_inner);
        }
        case ULANG_TYPE_CONST_EXPR:
            return ulang_type_const_expr_is_equal(ulang_type_const_expr_const_unwrap(a), ulang_type_const_expr_const_unwrap(b));
        case ULANG_TYPE_REMOVED:
            return ulang_type_removed_const_unwrap(a).pointer_depth == ulang_type_removed_const_unwrap(b).pointer_depth;
    }
    unreachable("");
}

bool ulang_type_vec_is_equal(Ulang_type_vec a, Ulang_type_vec b) {
    if (a.info.count != b.info.count) {
        return false;
    }

    for (size_t idx = 0; idx < a.info.count; idx++) {
        if (!ulang_type_is_equal(vec_at(a, idx), vec_at(b, idx))) {
            return false;
        }
    }

    return true;
}

