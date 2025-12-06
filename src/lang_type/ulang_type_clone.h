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

static inline Ulang_type_expr ulang_type_expr_clone(Ulang_type_expr lang_type) {
    return ulang_type_expr_new(
        lang_type.pos,
        uast_expr_clone(lang_type.expr, false, 0, uast_expr_get_pos(lang_type.expr)),
        lang_type.pointer_depth
    );
}

static inline Ulang_type_int ulang_type_int_clone(Ulang_type_int lang_type) {
    return ulang_type_int_new(lang_type.pos, lang_type.data, lang_type.pointer_depth);
}

static inline Ulang_type_float_lit ulang_type_float_lit_clone(Ulang_type_float_lit lang_type) {
    return ulang_type_float_lit_new(lang_type.pos, lang_type.data, lang_type.pointer_depth);
}

static inline Ulang_type_string_lit ulang_type_string_lit_clone(Ulang_type_string_lit lang_type) {
    return ulang_type_string_lit_new(lang_type.pos, lang_type.data, lang_type.pointer_depth);
}

static inline Ulang_type_struct_lit ulang_type_struct_lit_clone(
    Ulang_type_struct_lit lang_type,
    bool use_new_scope,
    Scope_id new_scope
) {
    return ulang_type_struct_lit_new(
        lang_type.pos,
        uast_expr_clone(lang_type.expr, use_new_scope, new_scope, lang_type.pos),
        lang_type.pointer_depth
    );
}

static inline Ulang_type_fn_lit ulang_type_fn_lit_clone(
    Ulang_type_fn_lit lang_type,
    bool use_new_scope,
    Scope_id new_scope
) {
    return ulang_type_fn_lit_new(
        lang_type.pos,
        name_clone(lang_type.name, use_new_scope, new_scope),
        lang_type.pointer_depth
    );
}

static inline Ulang_type_tuple ulang_type_tuple_clone(Ulang_type_tuple lang_type, bool use_new_scope, Scope_id new_scope) {
    return ulang_type_tuple_new(
        lang_type.pos,
        ulang_type_vec_clone(lang_type.ulang_types, use_new_scope, new_scope),
        lang_type.pointer_depth
    );
}

static inline Ulang_type_regular ulang_type_regular_clone(Ulang_type_regular lang_type, bool use_new_scope, Scope_id new_scope) {
    return ulang_type_regular_new(
        lang_type.pos,
        ulang_type_atom_new(
            uname_clone(lang_type.atom.str, use_new_scope, new_scope),
            lang_type.atom.pointer_depth
        )
    );
}

static inline Ulang_type_array ulang_type_array_clone(Ulang_type_array lang_type, bool use_new_scope, Scope_id new_scope) {
    Ulang_type new_item_type = ulang_type_clone(*lang_type.item_type, use_new_scope, new_scope);
    return ulang_type_array_new(lang_type.pos, arena_dup(&a_main, &new_item_type), lang_type.count, lang_type.pointer_depth);
}

static inline Ulang_type_const_expr ulang_type_const_expr_clone(
    Ulang_type_const_expr lang_type,
    bool use_new_scope,
    Scope_id new_scope
) {
    (void) use_new_scope;
    (void) new_scope;
    switch (lang_type.type) {
        case ULANG_TYPE_INT:
            return ulang_type_int_const_wrap(ulang_type_int_clone(ulang_type_int_const_unwrap(lang_type)));
        case ULANG_TYPE_FLOAT_LIT:
            return ulang_type_float_lit_const_wrap(ulang_type_float_lit_clone(ulang_type_float_lit_const_unwrap(lang_type)));
        case ULANG_TYPE_STRING_LIT:
            return ulang_type_string_lit_const_wrap(ulang_type_string_lit_clone(ulang_type_string_lit_const_unwrap(lang_type)));
        case ULANG_TYPE_STRUCT_LIT:
            return ulang_type_struct_lit_const_wrap(ulang_type_struct_lit_clone(
                ulang_type_struct_lit_const_unwrap(lang_type),
                use_new_scope,
                new_scope
            ));
        case ULANG_TYPE_FN_LIT:
            return ulang_type_fn_lit_const_wrap(ulang_type_fn_lit_clone(
                ulang_type_fn_lit_const_unwrap(lang_type),
                use_new_scope,
                new_scope
            ));
    }
    unreachable("");
}

static inline Ulang_type_fn ulang_type_fn_clone(Ulang_type_fn lang_type, bool use_new_scope, Scope_id new_scope) {
    Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type));
    *rtn_type = ulang_type_clone(*lang_type.return_type, use_new_scope, new_scope);
    return ulang_type_fn_new(
        lang_type.pos,
       ulang_type_tuple_clone(lang_type.params, use_new_scope, new_scope),
       rtn_type,
       lang_type.pointer_depth
    );
}

static inline Ulang_type_removed ulang_type_removed_clone(Ulang_type_removed lang_type, bool use_new_scope, Scope_id new_scope) {
    (void) use_new_scope;
    (void) new_scope;
    return ulang_type_removed_new(lang_type.pos, lang_type.pointer_depth);
}

static inline Ulang_type ulang_type_clone(Ulang_type lang_type, bool use_new_scope, Scope_id new_scope) {
    switch (lang_type.type) {
        case ULANG_TYPE_REMOVED:
            return ulang_type_removed_const_wrap(ulang_type_removed_clone(
                ulang_type_removed_const_unwrap(lang_type), use_new_scope, new_scope
            ));
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
        case ULANG_TYPE_EXPR:
            return ulang_type_expr_const_wrap(ulang_type_expr_clone(
                ulang_type_expr_const_unwrap(lang_type)
            ));
        case ULANG_TYPE_CONST_EXPR:
            return ulang_type_const_expr_const_wrap(ulang_type_const_expr_clone(
                ulang_type_const_expr_const_unwrap(lang_type), use_new_scope, new_scope
            ));
    }
    unreachable("");
}

#endif // ULANG_TYPE_CLONE_H
