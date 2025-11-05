#include <ulang_type_is_equal.h>
#include <uast_expr_to_ulang_type.h>

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
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_GEN_PARAM:
            todo();
        case ULANG_TYPE_EXPR: {
            Ulang_type a_inner = {0};
            Ulang_type b_inner = {0};

            if (!uast_expr_to_ulang_type(&a_inner, ulang_type_expr_const_unwrap(a).expr)) {
                todo();
            }
            ulang_type_add_pointer_depth(&a_inner, ulang_type_expr_const_unwrap(a).pointer_depth);

            if (!uast_expr_to_ulang_type(&b_inner, ulang_type_expr_const_unwrap(b).expr)) {
                todo();
            }
            ulang_type_add_pointer_depth(&b_inner, ulang_type_expr_const_unwrap(b).pointer_depth);

            return ulang_type_is_equal(a_inner, b_inner);
        }
        case ULANG_TYPE_INT:
            todo();
    }
    todo();
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

