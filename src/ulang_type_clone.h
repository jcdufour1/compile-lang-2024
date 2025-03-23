#ifndef ULANG_TYPE_CLONE_H
#define ULANG_TYPE_CLONE_H

static inline Ulang_type ulang_type_clone(Ulang_type lang_type);

static inline Ulang_type_generic ulang_type_generic_clone(Ulang_type_generic lang_type);

static inline Ulang_type_vec ulang_type_vec_clone(Ulang_type_vec vec) {
    Ulang_type_vec new_vec = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        vec_append(&a_main, &new_vec, ulang_type_clone(vec_at(&vec, idx)));
    }
    return new_vec;
}

static inline Ulang_type_regular ulang_type_regular_clone(Ulang_type_regular lang_type) {
    return lang_type;
}

static inline Ulang_type_resol ulang_type_resol_clone(Ulang_type_resol lang_type) {
    return ulang_type_resol_new(
        ulang_type_clone(*lang_type.original),
        ulang_type_regular_clone(lang_type.resolved),
        lang_type.pos
    );
}

static inline Ulang_type_generic ulang_type_generic_clone(Ulang_type_generic lang_type) {
    Ulang_type_vec new_args = {0};
    for (size_t idx = 0; idx < lang_type.generic_args.info.count; idx++) {
        vec_append(&a_main, &new_args, ulang_type_clone(vec_at(&lang_type.generic_args, idx)));
    }
    return (Ulang_type_generic) {
        .atom = lang_type.atom,
        .generic_args = new_args,
        .pos = lang_type.pos
    };
}

static inline Ulang_type ulang_type_clone(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REG_GENERIC:
            return ulang_type_generic_const_wrap(ulang_type_generic_clone(
                ulang_type_generic_const_unwrap(lang_type)
            ));
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_wrap(ulang_type_regular_clone(
                ulang_type_regular_const_unwrap(lang_type)
            ));
        case ULANG_TYPE_RESOL:
            return ulang_type_resol_const_wrap(ulang_type_resol_clone(
                ulang_type_resol_const_unwrap(lang_type)
            ));
    }
}

#endif // ULANG_TYPE_CLONE_H
