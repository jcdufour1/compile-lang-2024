
#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

// TODO: do these things properly
int64_t str_view_to_int64_t(Str_view str_view);

static inline Lang_type_atom lang_type_primitive_get_atom(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            return lang_type_char_const_unwrap(lang_type).atom;
        case LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "i");
            string_extend_int64_t(&a_main, &string, lang_type_signed_int_const_unwrap(lang_type).bit_width);
            return lang_type_atom_new(string_to_strv(string), lang_type_signed_int_const_unwrap(lang_type).pointer_depth);
        }
        case LANG_TYPE_UNSIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "u");
            string_extend_int64_t(&a_main, &string, lang_type_unsigned_int_const_unwrap(lang_type).bit_width);
            return lang_type_atom_new(string_to_strv(string), lang_type_unsigned_int_const_unwrap(lang_type).pointer_depth);
        }
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

// TODO: remove this function?
static inline void lang_type_primitive_set_atom(Lang_type_primitive* lang_type, Lang_type_atom atom) {
    switch (lang_type->type) {
        case LANG_TYPE_CHAR:
            lang_type_char_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_SIGNED_INT:
            lang_type_signed_int_unwrap(lang_type)->bit_width = str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1));
            lang_type_signed_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_UNSIGNED_INT:
            lang_type_unsigned_int_unwrap(lang_type)->bit_width = str_view_to_int64_t(str_view_slice(atom.str, 1, atom.str.count - 1));
            lang_type_unsigned_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
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

static inline int16_t lang_type_primitive_get_pointer_depth(Lang_type_primitive lang_type) {
    return lang_type_primitive_get_atom(lang_type).pointer_depth;
}

static inline int32_t lang_type_primitive_get_bit_width(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            unreachable("");
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_ANY:
            unreachable("");
    }
    unreachable("");
}

static inline int32_t lang_type_get_bit_width(Lang_type lang_type) {
    return lang_type_primitive_get_bit_width(lang_type_primitive_const_unwrap(lang_type));
}

static inline void lang_type_set_pointer_depth(Lang_type* lang_type, int16_t pointer_depth) {
    Lang_type_atom atom = lang_type_get_atom(*lang_type);
    atom.pointer_depth = pointer_depth;
    lang_type_set_atom(lang_type, atom);
}

#endif // LANG_TYPE_AFTER_H
