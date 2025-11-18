#include <ulang_type_is_equal.h>
#include <uast_expr_to_ulang_type.h>
#include <ulang_type_remove_expr.h>

bool ulang_type_atom_is_equal(Ulang_type_atom a, Ulang_type_atom b) {
    return uname_is_equal(a.str, b.str) && a.pointer_depth == b.pointer_depth;
}

bool ulang_type_regular_is_equal(Ulang_type_regular a, Ulang_type_regular b) {
    return ulang_type_atom_is_equal(a.atom, b.atom);
}

bool ulang_type_is_equal(Ulang_type a, Ulang_type b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_is_equal(ulang_type_regular_const_unwrap(a), ulang_type_regular_const_unwrap(b));
        case ULANG_TYPE_ARRAY:
            // TODO
            todo();
        case ULANG_TYPE_TUPLE:
            // TODO
            todo();
        case ULANG_TYPE_FN:
            // TODO
            todo();
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

