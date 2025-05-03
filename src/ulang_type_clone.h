#ifndef ULANG_TYPE_CLONE_H
#define ULANG_TYPE_CLONE_H

static inline Ulang_type ulang_type_clone(Ulang_type lang_type, Scope_id new_scope);

static inline Ulang_type_vec ulang_type_vec_clone(Ulang_type_vec vec, Scope_id new_scope) {
    Ulang_type_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, ulang_type_clone(vec_at(&vec, idx), new_scope));
    }
    return new_vec;
}

static inline Ulang_type_tuple ulang_type_tuple_clone(Ulang_type_tuple lang_type, Scope_id new_scope) {
    return ulang_type_tuple_new(ulang_type_vec_clone(lang_type.ulang_types, new_scope), lang_type.pos);
}

static inline Ulang_type_regular ulang_type_regular_clone(Ulang_type_regular lang_type, Scope_id new_scope) {
    // TODO: this is not actually cloning
    lang_type.atom.str.scope_id = new_scope;
    return lang_type;
}

static inline Ulang_type_fn ulang_type_fn_clone(Ulang_type_fn lang_type, Scope_id new_scope) {
    Ulang_type* rtn_type = arena_alloc(&a_main, sizeof(*rtn_type));
    *rtn_type = ulang_type_clone(*lang_type.return_type, new_scope);
    return ulang_type_fn_new(ulang_type_tuple_clone(lang_type.params, new_scope), rtn_type, lang_type.pos);
}

static inline Ulang_type ulang_type_clone(Ulang_type lang_type, Scope_id new_scope) {
    switch (lang_type.type) {
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_wrap(ulang_type_fn_clone(
                ulang_type_fn_const_unwrap(lang_type), new_scope
            ));
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_wrap(ulang_type_regular_clone(
                ulang_type_regular_const_unwrap(lang_type), new_scope
            ));
    }
}

#endif // ULANG_TYPE_CLONE_H
