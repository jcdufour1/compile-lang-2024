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
    if (!ulang_type_tuple_is_equal(a.params, b.params)) {
        return false;
    }
    if (!ulang_type_is_equal(*a.return_type, *b.return_type)) {
        return false;
    }
    return a.pointer_depth == b.pointer_depth;
}

bool ulang_type_array_is_equal(Ulang_type_array a, Ulang_type_array b) {
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
        case ULANG_TYPE_INT:
            return ulang_type_int_const_unwrap(a).data == ulang_type_int_const_unwrap(b).data;
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

