#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <uast_utils.h>

static inline Lang_type lang_type_from_ulang_type(const Env* env, Ulang_type lang_type);

// TODO: figure out way to reduce duplicate vec allocations
static inline Lang_type lang_type_from_ulang_type_tuple(const Env* env, Ulang_type_tuple lang_type) {
    Lang_type_vec new_lang_types = {0};
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        vec_append(&a_main, &new_lang_types, lang_type_from_ulang_type(env, vec_at(&lang_type.ulang_types, idx)));
    }
    return lang_type_tuple_const_wrap(lang_type_tuple_new(new_lang_types));
}

static inline Lang_type lang_type_from_ulang_type_regular(const Env* env, Ulang_type_regular lang_type) {
    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, env, lang_type.atom.str)) {
        unreachable(LANG_TYPE_FMT, ulang_type_print(ulang_type_regular_const_wrap(lang_type)));
    }

    Lang_type_atom new_atom = lang_type_atom_new(lang_type.atom.str, lang_type.atom.pointer_depth);
    switch (result->type) {
        case UAST_STRUCT_DEF:
            return lang_type_struct_const_wrap(lang_type_struct_new(new_atom));
        case UAST_RAW_UNION_DEF:
            return lang_type_raw_union_const_wrap(lang_type_raw_union_new(new_atom));
        case UAST_ENUM_DEF:
            return lang_type_enum_const_wrap(lang_type_enum_new(new_atom));
        case UAST_SUM_DEF:
            return lang_type_sum_const_wrap(lang_type_sum_new(new_atom));
        case UAST_PRIMITIVE_DEF:
            return lang_type_primitive_const_wrap(lang_type_primitive_new(new_atom));
        case UAST_LITERAL_DEF:
            try(uast_literal_def_const_unwrap(result)->type == UAST_VOID_DEF);
            return lang_type_void_const_wrap(lang_type_void_new(0));
        default:
            unreachable(UAST_FMT, uast_def_print(result));
    }
    unreachable("");
}

static inline Lang_type lang_type_from_ulang_type(const Env* env, Ulang_type lang_type) {
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
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
