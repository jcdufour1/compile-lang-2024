#include <tast_utils.h>
#include <type_checking.h>
#include <token_type_to_operator_type.h>
#include <sizeof.h>
#include <uast_utils.h>

Tast_literal* util_tast_literal_new_from_double(double value, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_double(value, pos));
}

Tast_literal* util_tast_literal_new_from_int64_t(int64_t value, TOKEN_TYPE token_type, Pos pos) {
    return try_set_literal_types(util_uast_literal_new_from_int64_t(value, token_type, pos));
}

Tast_operator* util_binary_typed_new(Uast_expr* lhs, Uast_expr* rhs, TOKEN_TYPE operator_type) {
    Uast_binary* binary = uast_binary_new(uast_expr_get_pos(lhs), lhs, rhs, token_type_to_binary_type(operator_type));

    Tast_expr* new_tast;
    unwrap(try_set_binary_types(&new_tast, binary));

    return tast_operator_unwrap(new_tast);
}

Tast_operator* tast_condition_get_default_child(Tast_expr* if_cond_child) {
    Tast_binary* binary = tast_binary_new(
        tast_expr_get_pos(if_cond_child),
        tast_literal_wrap(
            util_tast_literal_new_from_int64_t( 0, TOKEN_INT_LITERAL, tast_expr_get_pos(if_cond_child))
        ),
        if_cond_child,
        BINARY_NOT_EQUAL,
        lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(lang_type_get_pos(tast_expr_get_lang_type(if_cond_child)), 32, 0)))
    );

    return tast_binary_wrap(binary);
}

size_t struct_def_base_get_idx_largest_member(Struct_def_base base) {
    size_t result = 0;
    uint64_t size_result = 0;

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        uint64_t curr_size = sizeof_lang_type(vec_at(&base.members, idx)->lang_type);
        if (curr_size > size_result) {
            size_result = curr_size;
            result = idx;
        }
    }

    return result;
}


