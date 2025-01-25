
#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

static inline Lang_type_atom lang_type_get_atom(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return lang_type_unwrap_primitive_const(lang_type).atom;
        case LANG_TYPE_SUM:
            return lang_type_unwrap_sum_const(lang_type).atom;
        case LANG_TYPE_STRUCT:
            return lang_type_unwrap_struct_const(lang_type).atom;
        case LANG_TYPE_RAW_UNION:
            return lang_type_unwrap_raw_union_const(lang_type).atom;
        case LANG_TYPE_ENUM:
            return lang_type_unwrap_enum_const(lang_type).atom;
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_VOID:
            return (Lang_type_atom) {0};
    }
    unreachable("");
}

static inline Str_view lang_type_get_str(Lang_type lang_type) {
    return lang_type_get_atom(lang_type).str;
}

static inline int16_t lang_type_get_pointer_depth(Lang_type lang_type) {
    return lang_type_get_atom(lang_type).pointer_depth;
}

#endif // LANG_TYPE_AFTER_H
