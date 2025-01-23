#ifndef LANG_TYPE_FROM_ULANG_TYPE
#define LANG_TYPE_FROM_ULANG_TYPE

#include <ulang_type.h>
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
            return lang_type_wrap_struct_const(lang_type_struct_new(lang_type_atom_new(uast_unwrap_struct_def(result)->base.name, 0)));
        case UAST_PRIMITIVE_DEF:
            return lang_type_wrap_primitive_const(lang_type_primitive_new(lang_type_atom_new(lang_type_get_str(uast_unwrap_primitive_def(result)->lang_type), 0)));
        default:
            unreachable(UAST_FMT, uast_def_print(result));
    }
    unreachable("");
}

static inline Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    (void) lang_type;
    todo();
}

#endif // LANG_TYPE_FROM_ULANG_TYPE
