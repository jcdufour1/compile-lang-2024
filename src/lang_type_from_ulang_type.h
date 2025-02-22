#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <uast_utils.h>

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type);

// TODO: remove these tow forward decls and replace with better system
bool lang_type_atom_is_signed(Lang_type_atom atom);
bool lang_type_atom_is_unsigned(Lang_type_atom atom);

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_tuple(Env* env, Ulang_type_tuple lang_type) {
    Lang_type_vec new_lang_types = {0};
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        vec_append(&a_main, &new_lang_types, lang_type_from_ulang_type(env, vec_at(&lang_type.ulang_types, idx)));
    }
    return lang_type_tuple_const_wrap(lang_type_tuple_new(new_lang_types));
}

static inline Lang_type lang_type_from_ulang_type_regular_primitive(const Env* env, Ulang_type_regular lang_type, const Uast_primitive_def* def) {
    (void) env;
    (void) def;

    Lang_type_atom atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);

    if (lang_type_atom_is_signed(atom)) {
        Lang_type_signed_int new_int = lang_type_signed_int_new(str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1)), atom.pointer_depth);
        return lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(new_int));
    } else if (str_view_cstr_is_equal(atom.str, "void")) {
        todo();
    } else if (lang_type_atom_is_equal(atom, lang_type_atom_new_from_cstr("u8", 0))) {
        return lang_type_primitive_const_wrap(lang_type_string_const_wrap(lang_type_string_new(atom)));
    } else if (str_view_cstr_is_equal(atom.str, "u8")) {
        // TODO: does this make sense for u8**, etc.?
        return lang_type_primitive_const_wrap(lang_type_string_const_wrap(lang_type_string_new(atom)));
    } else if (str_view_cstr_is_equal(atom.str, "any")) {
        // TODO: does this make sense?
        return lang_type_primitive_const_wrap(lang_type_string_const_wrap(lang_type_string_new(atom)));
    } else {
        log(LOG_DEBUG, TAST_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));
        todo();
    }
    todo();
}

static inline Lang_type lang_type_from_ulang_type_regular(Env* env, Ulang_type_regular lang_type) {
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));
    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, env, lang_type.atom.str)) {
        unreachable(LANG_TYPE_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));
    }
    log(LOG_DEBUG, TAST_FMT, uast_def_print(result));
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));

    Lang_type_atom new_atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));
    switch (result->type) {
        case UAST_STRUCT_DEF:
            log(LOG_DEBUG, "thing 19\n");
            return lang_type_struct_const_wrap(lang_type_struct_new(new_atom));
        case UAST_RAW_UNION_DEF:
            log(LOG_DEBUG, "thing 20\n");
            return lang_type_raw_union_const_wrap(lang_type_raw_union_new(new_atom));
        case UAST_ENUM_DEF:
            log(LOG_DEBUG, "thing 21\n");
            return lang_type_enum_const_wrap(lang_type_enum_new(new_atom));
        case UAST_SUM_DEF:
            log(LOG_DEBUG, "thing 22\n");
            return lang_type_sum_const_wrap(lang_type_sum_new(new_atom));
        case UAST_PRIMITIVE_DEF:
            return lang_type_from_ulang_type_regular_primitive(env, lang_type, uast_primitive_def_unwrap(result));
        case UAST_LITERAL_DEF:
            log(LOG_DEBUG, "thing 24\n");
            try(uast_literal_def_const_unwrap(result)->type == UAST_VOID_DEF);
            return lang_type_void_const_wrap(lang_type_void_new(0));
        default:
            unreachable(UAST_FMT, uast_def_print(result));
    }
    unreachable("");
}

static inline Lang_type lang_type_from_ulang_type(Env* env, Ulang_type lang_type) {
    log(LOG_DEBUG, TAST_FMT, ulang_type_print(lang_type));
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return lang_type_from_ulang_type_regular(env, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE:
            return lang_type_from_ulang_type_tuple(env, ulang_type_tuple_const_unwrap(lang_type));
    }
    unreachable("");
}

static inline Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            todo();
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
    }
    unreachable("");
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
