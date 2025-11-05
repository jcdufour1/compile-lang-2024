#ifndef ULANG_TYPE_CLONE_H
#define ULANG_TYPE_CLONE_H

#include <uast_clone.h>

static inline Ulang_type ulang_type_clone(Ulang_type lang_type, bool use_new_scope, Scope_id new_scope);

static inline Ulang_type_vec ulang_type_vec_clone(Ulang_type_vec vec, bool use_new_scope, Scope_id new_scope) {
    Ulang_type_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, ulang_type_clone(vec_at(vec, idx), use_new_scope, new_scope));
    }
    return new_vec;
}

static inline Ulang_type_gen_param ulang_type_gen_param_clone(Ulang_type_gen_param lang_type) {
    return ulang_type_gen_param_new(lang_type.pos);
}

static inline Ulang_type_expr ulang_type_expr_clone(Ulang_type_expr lang_type) {
    return ulang_type_expr_new(
        uast_expr_clone(lang_type.expr, false, 0, uast_expr_get_pos(lang_type.expr)),
        lang_type.pointer_depth,
        lang_type.pos
    );
}

static inline Ulang_type_int ulang_type_int_clone(Ulang_type_int lang_type) {
    return ulang_type_int_new(lang_type.data, lang_type.pointer_depth, lang_type.pos);
}

static inline Ulang_type_tuple ulang_type_tuple_clone(Ulang_type_tuple lang_type, bool use_new_scope, Scope_id new_scope) {
    return ulang_type_tuple_new(ulang_type_vec_clone(lang_type.ulang_types, use_new_scope, new_scope), lang_type.pos);
}

static inline Ulang_type_regular ulang_type_regular_clone(Ulang_type_regular lang_type, bool use_new_scope, Scope_id new_scope) {
    return ulang_type_regular_new(
        ulang_type_atom_new(uname_clone(lang_type.atom.str, use_new_scope, new_scope), lang_type.atom.pointer_depth),
        lang_type.pos
    );
}

static inline Ulang_type_array ulang_type_array_clone(Ulang_type_array lang_type, bool use_new_scope, Scope_id new_scope) {
    Ulang_type new_item_type = ulang_type_clone(*lang_type.item_type, use_new_scope, new_scope);
    return ulang_type_array_new(arena_dup(&a_main, &new_item_type), lang_type.count, lang_type.pos);
}

static inline Ulang_type_fn ulang_type_fn_clone(Ulang_type_fn lang_type, bool use_new_scope, Scope_id new_scope) {
    Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type));
    *rtn_type = ulang_type_clone(*lang_type.return_type, use_new_scope, new_scope);
    return ulang_type_fn_new(ulang_type_tuple_clone(lang_type.params, use_new_scope, new_scope), rtn_type, lang_type.pos);
}

static inline Ulang_type ulang_type_clone(Ulang_type lang_type, bool use_new_scope, Scope_id new_scope) {
    switch (lang_type.type) {
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_const_wrap(ulang_type_tuple_clone(
                ulang_type_tuple_const_unwrap(lang_type), use_new_scope, new_scope
            ));
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_wrap(ulang_type_fn_clone(
                ulang_type_fn_const_unwrap(lang_type), use_new_scope, new_scope
            ));
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_wrap(ulang_type_regular_clone(
                ulang_type_regular_const_unwrap(lang_type), use_new_scope, new_scope
            ));
        case ULANG_TYPE_ARRAY:
            return ulang_type_array_const_wrap(ulang_type_array_clone(
                ulang_type_array_const_unwrap(lang_type), use_new_scope, new_scope
            ));
        case ULANG_TYPE_GEN_PARAM:
            return ulang_type_gen_param_const_wrap(ulang_type_gen_param_clone(
                ulang_type_gen_param_const_unwrap(lang_type)
            ));
        case ULANG_TYPE_EXPR:
            return ulang_type_expr_const_wrap(ulang_type_expr_clone(
                ulang_type_expr_const_unwrap(lang_type)
            ));
        case ULANG_TYPE_INT:
            return ulang_type_int_const_wrap(ulang_type_int_clone(
                ulang_type_int_const_unwrap(lang_type)
            ));
    }
}

#endif // ULANG_TYPE_CLONE_H
