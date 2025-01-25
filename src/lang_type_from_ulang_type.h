#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
#include <lang_type.h>
#include <uast_utils.h>

static inline Lang_type lang_type_from_ulang_type(const Env* env, Ulang_type lang_type) {
    (void) lang_type;
    (void) env;

    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, env, ulang_type_unwrap_regular_const(lang_type).atom.str)) {
        todo();
    }

    switch (result->type) {
        case UAST_STRUCT_DEF:
            return lang_type_wrap_struct_const(lang_type_struct_new(lang_type_atom_new(ulang_type_unwrap_regular_const(lang_type).atom.str, ulang_type_unwrap_regular_const(lang_type).atom.pointer_depth)));
        case UAST_RAW_UNION_DEF:
            return lang_type_wrap_raw_union_const(lang_type_raw_union_new(lang_type_atom_new(ulang_type_unwrap_regular_const(lang_type).atom.str, ulang_type_unwrap_regular_const(lang_type).atom.pointer_depth)));
        case UAST_ENUM_DEF:
            return lang_type_wrap_enum_const(lang_type_enum_new(lang_type_atom_new(ulang_type_unwrap_regular_const(lang_type).atom.str, ulang_type_unwrap_regular_const(lang_type).atom.pointer_depth)));
        case UAST_PRIMITIVE_DEF:
            return lang_type_wrap_primitive_const(lang_type_primitive_new(lang_type_atom_new(ulang_type_unwrap_regular_const(lang_type).atom.str, ulang_type_unwrap_regular_const(lang_type).atom.pointer_depth)));
        case UAST_LITERAL_DEF:
            try(uast_unwrap_literal_def_const(result)->type == UAST_VOID_DEF);
            return lang_type_wrap_void_const(lang_type_void_new(0));
        default:
            unreachable(UAST_FMT, uast_def_print(result));
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
            return ulang_type_wrap_regular_const(ulang_type_regular_new(ulang_type_atom_new(lang_type_get_str(lang_type), lang_type_get_pointer_depth(lang_type))));
    }
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
