#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <uast_utils.h>
#include <ulang_type_print.h>
#include <resolve_generics.h>
#include <ulang_type_get_pos.h>
#include <msg.h>

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type);

// TODO: remove Pos parameter
bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Env* env, Ulang_type lang_type, Pos pos);

// TODO: remove these tow forward decls and replace with better system
bool lang_type_atom_is_signed(Lang_type_atom atom);
bool lang_type_atom_is_unsigned(Lang_type_atom atom);

static inline Ulang_type lang_type_to_ulang_type(Lang_type lang_type);

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

static inline bool try_lang_type_from_ulang_type_generic(
    Lang_type* new_lang_type,
    Env* env,
    Ulang_type_generic lang_type,
    Pos pos
);

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_tuple(Env* env, Ulang_type_tuple lang_type) {
    Lang_type_tuple new_tuple = {0};
    unwrap(try_lang_type_from_ulang_type_tuple(&new_tuple, env, lang_type, (Pos) {0}));
    return lang_type_tuple_const_wrap(new_tuple);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_fn(Env* env, Ulang_type_fn lang_type) {
    Lang_type_fn new_fn = {0};
    unwrap(try_lang_type_from_ulang_type_fn(&new_fn, env, lang_type, (Pos) {0}));
    return lang_type_fn_const_wrap(new_fn);
}

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_generic(Env* env, Ulang_type_generic lang_type) {
    Lang_type new_gen = {0};
    unwrap(try_lang_type_from_ulang_type_generic(&new_gen, env, lang_type, (Pos) {0}));
    return new_gen;
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
    *new_lang_type = lang_type_tuple_new(pos, new_lang_types);
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
    *new_lang_type = lang_type_fn_new(pos, new_params, new_rtn_type);
    return true;
}

// TODO: figure out way to reduce duplicate vec allocations
static inline bool try_lang_type_from_ulang_type_generic(
    Lang_type* new_lang_type,
    Env* env,
    Ulang_type_generic lang_type,
    Pos pos
) {
    Ulang_type after_res = {0};
    if (!resolve_generics_ulang_type_generic(&after_res, env, lang_type)) {
        return false;
    }
    return try_lang_type_from_ulang_type(new_lang_type, env, after_res, pos);
}

static inline Lang_type lang_type_from_ulang_type_regular_primitive(const Env* env, Ulang_type_regular lang_type, const Uast_primitive_def* def) {
    (void) env;
    (void) def;
    (void) lang_type;

    Lang_type_atom atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);

    if (lang_type_atom_is_signed(atom)) {
        Lang_type_signed_int new_int = lang_type_signed_int_new(
            lang_type.pos,
            str_view_to_int64_t(env, POS_BUILTIN, str_view_slice(atom.str.base, 1, atom.str.base.count - 1)),
            atom.pointer_depth
        );
        return lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(new_int));
    } else if (lang_type_atom_is_unsigned(atom)) {
        Lang_type_unsigned_int new_int = lang_type_unsigned_int_new(
            lang_type.pos,
            str_view_to_int64_t(env, POS_BUILTIN, str_view_slice(atom.str.base, 1, atom.str.base.count - 1)),
            atom.pointer_depth
        );
        return lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(new_int));
    } else if (str_view_cstr_is_equal(atom.str.base, "void")) {
        todo();
    } else if (lang_type_atom_is_equal(atom, lang_type_atom_new_from_cstr("u8", 0))) {
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type.pos, atom)));
    } else if (str_view_cstr_is_equal(atom.str.base, "u8")) {
        // TODO: does this make sense for u8**, etc.?
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type.pos, atom)));
    } else if (str_view_cstr_is_equal(atom.str.base, "any")) {
        // TODO: does this make sense?
        return lang_type_primitive_const_wrap(lang_type_char_const_wrap(lang_type_char_new(lang_type.pos, atom)));
    } else {
        log(LOG_DEBUG, TAST_FMT, ulang_type_print(LANG_TYPE_MODE_LOG, ulang_type_regular_const_wrap(lang_type)));
        todo();
    }
    unreachable("");
}

// TODO: add Pos as member to Ulang_type and Lang_type?
static inline bool try_lang_type_from_ulang_type_regular(Lang_type* new_lang_type, Env* env, Ulang_type_regular lang_type, Pos pos) {
    (void) new_lang_type;
    Ulang_type resolved = {0};
    if (!resolve_generics_ulang_type_regular(&resolved, env, lang_type)) {
        todo();
        return false;
    }
    //log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, lang_type));
    //log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, after_res));
    //log(LOG_DEBUG, TAST_FMT"\n", name_print(ulang_type_regular_const_unwrap(after_res).atom.str));
    //log(LOG_DEBUG, TAST_FMT"\n", str_view_print(ulang_type_regular_const_unwrap(after_res).atom.str.mod_path));
    log(LOG_DEBUG, TAST_FMT"\n", str_view_print(lang_type.atom.str.base));
    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, env, ulang_type_regular_const_unwrap(resolved).atom.str)) {
        msg(
            LOG_ERROR, EXPECT_FAIL_UNDEFINED_TYPE, env->file_path_to_text, pos,
            "undefined type `"TAST_FMT"`\n", ulang_type_print(LANG_TYPE_MODE_MSG, resolved)
        );
        todo();
        return false;
    }

    log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_MSG, ulang_type_regular_const_wrap(lang_type)));
    Lang_type_atom new_atom = lang_type_atom_new(ulang_type_regular_const_unwrap(resolved).atom.str, ulang_type_regular_const_unwrap(resolved).atom.pointer_depth);
    switch (result->type) {
        case UAST_STRUCT_DEF:
            *new_lang_type = lang_type_struct_const_wrap(lang_type_struct_new(lang_type.pos, new_atom));
            return true;
        case UAST_RAW_UNION_DEF:
            *new_lang_type = lang_type_raw_union_const_wrap(lang_type_raw_union_new(lang_type.pos, new_atom));
            return true;
        case UAST_ENUM_DEF:
            *new_lang_type = lang_type_enum_const_wrap(lang_type_enum_new(lang_type.pos, new_atom));
            return true;
        case UAST_SUM_DEF:
            *new_lang_type = lang_type_sum_const_wrap(lang_type_sum_new(lang_type.pos, new_atom));
            return true;
        case UAST_PRIMITIVE_DEF:
            *new_lang_type = lang_type_from_ulang_type_regular_primitive(env, ulang_type_regular_const_unwrap(resolved), uast_primitive_def_unwrap(result));
            return true;
        case UAST_LITERAL_DEF:
            unwrap(uast_literal_def_const_unwrap(result)->type == UAST_VOID_DEF);
            *new_lang_type = lang_type_void_const_wrap(lang_type_void_new(lang_type.pos));
            return true;
        default:
            unreachable(UAST_FMT, uast_def_print(result));
    }
    unreachable("");
}

static inline Lang_type lang_type_from_ulang_type_regular(Env* env, Ulang_type_regular lang_type) {
    Lang_type new_lang_type = {0};
    unwrap(try_lang_type_from_ulang_type_regular(&new_lang_type, env, lang_type, (Pos) {0}));
    return new_lang_type;
}

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return lang_type_from_ulang_type_regular(env, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            return lang_type_from_ulang_type_tuple(env, ulang_type_tuple_const_unwrap(lang_type));
        case ULANG_TYPE_FN:
            return lang_type_from_ulang_type_fn(env, ulang_type_fn_const_unwrap(lang_type));
        case ULANG_TYPE_REG_GENERIC:
            return lang_type_from_ulang_type_generic(env, ulang_type_generic_const_unwrap(lang_type));
    }
    unreachable("");
}

static inline Ulang_type_tuple lang_type_tuple_to_ulang_type_tuple(Lang_type_tuple lang_type) {
    // TODO: reduce heap allocations (do sym_tbl_lookup for this?)
    Ulang_type_vec new_types = {0};
    for (size_t idx = 0; idx < lang_type.lang_types.info.count; idx++) {
        vec_append(&a_main, &new_types, lang_type_to_ulang_type(vec_at(&lang_type.lang_types, idx)));
    }
    return ulang_type_tuple_new(new_types, lang_type.pos);
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
            // TODO: change (Pos) {0} below to lang_type_get_pos(lang_type)
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(lang_type_get_str(lang_type), lang_type_get_pointer_depth(lang_type)), (Pos) {0}));
        case LANG_TYPE_FN:
            todo();
    }
    unreachable("");
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
