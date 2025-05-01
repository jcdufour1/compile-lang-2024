#ifndef ULANG_TYPE_GET_POS_H
#define ULANG_TYPE_GET_POS_H

static inline bool name_is_equal(Name a, Name b);

static inline bool uname_is_equal(Uname a, Uname b);

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


static inline bool ulang_type_atom_is_equal(Ulang_type_atom a, Ulang_type_atom b) {
    return uname_is_equal(a.str, b.str) && a.pointer_depth == b.pointer_depth;
}

static inline bool ulang_type_regular_is_equal(Ulang_type_regular a, Ulang_type_regular b) {
    return ulang_type_atom_is_equal(a.atom, b.atom);
}

// TODO: move this function?
static inline bool ulang_type_is_equal(Ulang_type a, Ulang_type b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case ULANG_TYPE_REGULAR:
            return ulang_type_regular_is_equal(ulang_type_regular_const_unwrap(a), ulang_type_regular_const_unwrap(b));
        case ULANG_TYPE_TUPLE:
            todo();
        case ULANG_TYPE_FN:
            todo();
    }
    todo();
}

// TODO: move this function
static inline bool name_is_equal(Name a, Name b) {
    log(LOG_DEBUG, TAST_FMT"" TAST_FMT"\n", name_print(a), name_print(b));
    if (!str_view_is_equal(a.mod_path, b.mod_path) || !str_view_is_equal(a.base, b.base)) {
        log(LOG_DEBUG, "%s\n", bool_print(str_view_is_equal(a.mod_path, b.mod_path)));
        log(LOG_DEBUG, TAST_FMT" "TAST_FMT"\n", name_print(a), name_print(b));
        log(LOG_DEBUG, TAST_FMT" "TAST_FMT"\n", str_view_print(a.base), str_view_print(b.base));
        log(LOG_DEBUG, "name_is_equal return 1\n");
        return false;
    }

    if (a.gen_args.info.count != b.gen_args.info.count) {
        log(LOG_DEBUG, "name_is_equal return 2\n");
        return false;
    }
    for (size_t idx = 0; idx < a.gen_args.info.count; idx++) {
        if (!ulang_type_is_equal(vec_at(&a.gen_args, idx), vec_at(&b.gen_args, idx))) {
            log(LOG_DEBUG, "name_is_equal return 3\n");
            return false;
        }
    }

    log(LOG_DEBUG, "name_is_equal return 4\n");
    return true;
}

static inline bool uname_is_equal(Uname a, Uname b) {
    if (!name_is_equal(a.mod_alias, b.mod_alias) || !str_view_is_equal(a.base, b.base)) {
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
