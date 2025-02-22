
#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

static inline Lang_type_atom lang_type_primitive_get_atom(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            return lang_type_char_const_unwrap(lang_type).atom;
        case LANG_TYPE_STRING:
            return lang_type_string_const_unwrap(lang_type).atom;
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(lang_type).atom;
        case LANG_TYPE_ANY:
            return lang_type_any_const_unwrap(lang_type).atom;
    }
    unreachable("");
}

static inline Lang_type_atom lang_type_get_atom(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            return lang_type_primitive_get_atom(lang_type_primitive_const_unwrap(lang_type));
        case LANG_TYPE_SUM:
            return lang_type_sum_const_unwrap(lang_type).atom;
        case LANG_TYPE_STRUCT:
            return lang_type_struct_const_unwrap(lang_type).atom;
        case LANG_TYPE_RAW_UNION:
            return lang_type_raw_union_const_unwrap(lang_type).atom;
        case LANG_TYPE_ENUM:
            return lang_type_enum_const_unwrap(lang_type).atom;
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_VOID:
            return (Lang_type_atom) {0};
    }
    unreachable("");
}

static inline void lang_type_primitive_set_atom(Lang_type_primitive* lang_type, Lang_type_atom atom) {
    switch (lang_type->type) {
        case LANG_TYPE_STRING:
            lang_type_string_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_CHAR:
            lang_type_char_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_SIGNED_INT:
            lang_type_signed_int_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_ANY:
            lang_type_any_unwrap(lang_type)->atom = atom;
            return;
    }
    unreachable("");
}

static inline void lang_type_set_atom(Lang_type* lang_type, Lang_type_atom atom) {
    switch (lang_type->type) {
        case LANG_TYPE_PRIMITIVE:
            lang_type_primitive_set_atom(lang_type_primitive_unwrap(lang_type), atom);
            return;
        case LANG_TYPE_SUM:
            lang_type_sum_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_STRUCT:
            lang_type_struct_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_RAW_UNION:
            lang_type_raw_union_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_ENUM:
            lang_type_enum_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_VOID:
            todo();
    }
    unreachable("");
}

static inline Str_view lang_type_get_str(Lang_type lang_type) {
    return lang_type_get_atom(lang_type).str;
}

static inline int16_t lang_type_get_pointer_depth(Lang_type lang_type) {
    return lang_type_get_atom(lang_type).pointer_depth;
}

static inline void lang_type_set_pointer_depth(Lang_type* lang_type, int16_t pointer_depth) {
    Lang_type_atom atom = lang_type_get_atom(*lang_type);
    atom.pointer_depth = pointer_depth;
    lang_type_set_atom(lang_type, atom);
}

#endif // LANG_TYPE_AFTER_H
