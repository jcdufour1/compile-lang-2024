#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <uast_utils.h>
#include <ulang_type_print.h>

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type);

// TODO: remove these tow forward decls and replace with better system
bool lang_type_atom_is_signed(Lang_type_atom atom);
bool lang_type_atom_is_unsigned(Lang_type_atom atom);

static inline Ulang_type lang_type_to_ulang_type(Lang_type lang_type);

static inline bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Env* env, Ulang_type lang_type, Pos pos);

static inline bool try_lang_type_from_ulang_type_tuple(
    Lang_type_tuple* new_lang_type,
    Env* env,
    Ulang_type_tuple lang_type,
    Pos pos
);

static inline bool try_lang_type_from_ulang_type_fn(
    Lang_type_fn* new_lang_type,
    Env* env,
    Ulang_type_fn lang_type,
    Pos pos
);

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_tuple(Env* env, Ulang_type_tuple lang_type) {
    Lang_type_tuple new_tuple = {0};
    try(try_lang_type_from_ulang_type_tuple(&new_tuple, env, lang_type, (Pos) {0}));
    return lang_type_tuple_const_wrap(new_tuple);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_fn(Env* env, Ulang_type_fn lang_type) {
    Lang_type_fn new_fn = {0};
    try(try_lang_type_from_ulang_type_fn(&new_fn, env, lang_type, (Pos) {0}));
    return lang_type_fn_const_wrap(new_fn);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline bool try_lang_type_from_ulang_type_tuple(
    Lang_type_tuple* new_lang_type,
    Env* env,
    Ulang_type_tuple lang_type,
    Pos pos
) {
    Lang_type_vec new_lang_types = {0};
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        Lang_type new_child = {0};
        if (!try_lang_type_from_ulang_type(&new_child, env, vec_at(&lang_type.ulang_types, idx), pos)) {
            return false;
        }
        vec_append(&a_main, &new_lang_types, new_child);
    }
    *new_lang_type = lang_type_tuple_new(new_lang_types);
    return true;
}

// TODO: figure out way to reduce duplicate vec allocations
static inline bool try_lang_type_from_ulang_type_fn(
    Lang_type_fn* new_lang_type,
    Env* env,
    Ulang_type_fn lang_type,
    Pos pos
) {
    Lang_type_tuple new_params = {0};
    if (!try_lang_type_from_ulang_type_tuple(&new_params, env, lang_type.params, pos)) {
        return false;
    }
    Lang_type* new_rtn_type = arena_alloc(&a_main, sizeof(*new_rtn_type));
    if (!try_lang_type_from_ulang_type(new_rtn_type, env, *lang_type.return_type, pos)) {
        return false;
    }
    *new_lang_type = lang_type_fn_new(new_params, new_rtn_type);
    return true;
}

static inline Lang_type lang_type_from_ulang_type_regular_primitive(const Env* env, Ulang_type_regular lang_type, const Uast_primitive_def* def) {
    (void) env;
    (void) def;

    Lang_type_atom atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);

    if (lang_type_atom_is_signed(atom)) {
        Lang_type_signed_int new_int = lang_type_signed_int_new(str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1)), atom.pointer_depth);
        return lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(new_int));
    } else if (lang_type_atom_is_unsigned(atom)) {
        Lang_type_unsigned_int new_int = lang_type_unsigned_int_new(str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1)), atom.pointer_depth);
        return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(new_int));
    } else if (str_view_cstr_is_equal(atom.str, "void")) {
        todo();
    } else if (lang_type_atom_is_equal(atom, lang_type_atom_new_from_cstr("u8", 0))) {
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(atom)));
    } else if (str_view_cstr_is_equal(atom.str, "u8")) {
        // TODO: does this make sense for u8**, etc.?
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(atom)));
    } else if (str_view_cstr_is_equal(atom.str, "any")) {
        // TODO: does this make sense?
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(atom)));
    } else {
        log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(lang_type)));
        todo();
    }
    todo();
}

// TODO: add Pos as member to Ulang_type and Lang_type?
static inline bool try_lang_type_from_ulang_type_regular(Lang_type* new_lang_type, Env* env, Ulang_type_regular lang_type, Pos pos) {
    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, env, lang_type.atom.str)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_text, pos,
            "undefined type `"TAST_FMT"`\n", ulang_type_print(LANG_TYPE_MODE_MSG, ulang_type_regular_const_wrap(lang_type))
        );
        return false;
    }

    Lang_type_atom new_atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);
    switch (result->type) {
        case UAST_STRUCT_DEF:
            *new_lang_type = lang_type_struct_const_wrap(lang_type_struct_new(new_atom));
            return true;
        case UAST_RAW_UNION_DEF:
            *new_lang_type = lang_type_raw_union_const_wrap(lang_type_raw_union_new(new_atom));
            return true;
        case UAST_ENUM_DEF:
            *new_lang_type = lang_type_enum_const_wrap(lang_type_enum_new(new_atom));
            return true;
        case UAST_SUM_DEF:
            *new_lang_type = lang_type_sum_const_wrap(lang_type_sum_new(new_atom));
            return true;
        case UAST_PRIMITIVE_DEF:
            *new_lang_type = lang_type_from_ulang_type_regular_primitive(env, lang_type, uast_primitive_def_unwrap(result));
            return true;
        case UAST_LITERAL_DEF:
            try(uast_literal_def_const_unwrap(result)->type == UAST_VOID_DEF);
            *new_lang_type = lang_type_void_const_wrap(lang_type_void_new(0));
            return true;
        default:
            unreachable(UAST_FMT, uast_def_print(result));
    }
    unreachable("");
}

static inline Lang_type lang_type_from_ulang_type_regular(Env* env, Ulang_type_regular lang_type) {
    Lang_type new_lang_type = {0};
    try(try_lang_type_from_ulang_type_regular(&new_lang_type, env, lang_type, (Pos) {0}));
    return new_lang_type;
}

static inline bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Env* env, Ulang_type lang_type, Pos pos) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            if (!try_lang_type_from_ulang_type_regular(new_lang_type, env, ulang_type_regular_const_unwrap(lang_type), pos)) {
                return false;
            }
            return true;
        case ULANG_TYPE_TUPLE: {
            Lang_type_tuple new_tuple = {0};
            if (!try_lang_type_from_ulang_type_tuple(&new_tuple, env, ulang_type_tuple_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_tuple_const_wrap(new_tuple);
            return true;
        }
        case ULANG_TYPE_FN: {
            Lang_type_fn new_fn = {0};
            if (!try_lang_type_from_ulang_type_fn(&new_fn, env, ulang_type_fn_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_fn_const_wrap(new_fn);
            return true;
        }
    }
    unreachable("");
}

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return lang_type_from_ulang_type_regular(env, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            return lang_type_from_ulang_type_tuple(env, ulang_type_tuple_const_unwrap(lang_type));
        case ULANG_TYPE_FN:
            return lang_type_from_ulang_type_fn(env, ulang_type_fn_const_unwrap(lang_type));
    }
    unreachable("");
}

static inline Ulang_type_tuple lang_type_tuple_to_ulang_type_tuple(Lang_type_tuple lang_type) {
    // TODO: reduce heap allocations (do sym_tbl_lookup for this?)
    Ulang_type_vec new_types = {0};
    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        vec_append(&a_main, &new_types, lang_type_to_ulang_type(vec_at(&lang_type.lang_types, idx)));
    }
    return ulang_type_tuple_new(new_types);
}

static inline Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            return ulang_type_tuple_const_wrap(lang_type_tuple_to_ulang_type_tuple(lang_type_tuple_const_unwrap(lang_type)));
        case LANG_TYPE_VOID:
            todo();
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(lang_type_get_str(lang_type), lang_type_get_pointer_depth(lang_type))));
        case LANG_TYPE_FN:
            todo();
    }
    unreachable("");
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
