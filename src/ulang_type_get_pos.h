#ifndef ULANG_TYPE_GET_POS_H
#define ULANG_TYPE_GET_POS_H

static inline Pos ulang_type_get_pos(Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_const_unwrap(lang_type).pos;
        case ULANG_TYPE_TUPLE:
            return ulang_type_tuple_const_unwrap(lang_type).pos;
        case ULANG_TYPE_FN:
            return ulang_type_fn_const_unwrap(lang_type).pos;
    }
    unreachable("");
}

// TODO: move this function?
static inline bool ulang_type_is_equal(Ulang_type a, Ulang_type b) {
    if (a.type != b.type) {
        return false;
    }
    todo();
}

// TODO: move this function
static inline bool name_is_equal(Name a, Name b) {
    if (!str_view_is_equal(a.mod_path, b.mod_path) || !str_view_is_equal(a.base, b.base)) {
        return false;
    }

    if (a.gen_args.info.count != b.gen_args.info.count) {
        return false;
    }
    for (size_t idx = 0; idx < a.gen_args.info.count; idx++) {
        if (!ulang_type_is_equal(vec_at(&a.gen_args, idx), vec_at(&b.gen_args, idx))) {
            return false;
        }
    }

    return true;
}
#endif // ULANG_TYPE_GET_POS_H
