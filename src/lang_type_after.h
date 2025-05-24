#ifndef LANG_TYPE_AFTER_H
#define LANG_TYPE_AFTER_H

#include <msg_todo.h>
#include <lang_type_get_pos.h>

// TODO: do these things properly
int64_t str_view_to_int64_t(const Pos pos, Str_view str_view);

static inline Lang_type_atom lang_type_primitive_get_atom_normal(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR: {
            // TODO: remove lang_type_atom from lang_type_char?
            Lang_type_atom atom = lang_type_char_const_unwrap(lang_type).atom;
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            // TODO: remove this if statement
            return atom;
        }
        case LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "i");
            string_extend_int64_t(&a_main, &string, lang_type_signed_int_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_signed_int_const_unwrap(lang_type).pointer_depth
            );
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "f");
            string_extend_int64_t(&a_main, &string, lang_type_float_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_float_const_unwrap(lang_type).pointer_depth
            );
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_UNSIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "u");
            string_extend_int64_t(&a_main, &string, lang_type_unsigned_int_const_unwrap(lang_type).bit_width);
            Lang_type_atom atom = lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_unsigned_int_const_unwrap(lang_type).pointer_depth
            );
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_ANY: {
            // TODO: remove atom from LANG_TYPE_ANY
            Lang_type_atom atom = lang_type_any_const_unwrap(lang_type).atom;
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
    }
    unreachable("");
}

static inline Lang_type_atom lang_type_primitive_get_atom_c(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            return lang_type_char_const_unwrap(lang_type).atom;
        case LANG_TYPE_FLOAT: {
            String string = {0};
            uint32_t bit_width = lang_type_float_const_unwrap(lang_type).bit_width;
            if (bit_width == 32) {
                string_extend_cstr(&a_main, &string, "float");
            } else if (bit_width == 64) {
                string_extend_cstr(&a_main, &string, "double");
            } else {
                    msg_todo("bit widths other than 32 or 64 (for floating point numbers) with the c backend", lang_type_primitive_get_pos(lang_type));
            }
            return lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_float_const_unwrap(lang_type).pointer_depth
            );
        }
        case LANG_TYPE_SIGNED_INT: {
            String string = {0};
            uint32_t bit_width = lang_type_signed_int_const_unwrap(lang_type).bit_width;
            if (bit_width == 1) {
                string_extend_cstr(&a_main, &string, "bool");
            } else {
                string_extend_cstr(&a_main, &string, "int");
                if (bit_width == 1) {
                    // TODO: overflow may not happen correctly; maybe remove i1/u1 in earlier passes
                    string_extend_cstr(&a_main, &string, "bool");
                } else if (bit_width == 8) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 16) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 32) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 64) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else {
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 (for integers) with the c backend", lang_type_primitive_get_pos(lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_signed_int_const_unwrap(lang_type).pointer_depth
            );
        }
        case LANG_TYPE_UNSIGNED_INT: {
            // TODO: deduplicate this and above case?
            // TODO: bit width of 1 here?
            String string = {0};
            uint32_t bit_width = lang_type_unsigned_int_const_unwrap(lang_type).bit_width;
            if (bit_width == 1) {
                string_extend_cstr(&a_main, &string, "bool");
            } else {
                string_extend_cstr(&a_main, &string, "uint");
                if (bit_width == 8) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 16) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 32) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else if (bit_width == 64) {
                    string_extend_int64_t(&a_main, &string, bit_width);
                } else {
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 with the c backend", lang_type_primitive_get_pos(lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return lang_type_atom_new(
                name_new((Str_view) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                lang_type_unsigned_int_const_unwrap(lang_type).pointer_depth
            );
        }
        case LANG_TYPE_ANY:
            todo();
            return lang_type_any_const_unwrap(lang_type).atom;
    }
    unreachable("");
}

static inline Lang_type_atom lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Lang_type_primitive lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return lang_type_primitive_get_atom_normal(lang_type);
        case LANG_TYPE_MODE_MSG:
            return lang_type_primitive_get_atom_normal(lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            return lang_type_primitive_get_atom_normal(lang_type);
        case LANG_TYPE_MODE_EMIT_C:
            return lang_type_primitive_get_atom_c(lang_type);
    }
    unreachable("");
}

static inline Lang_type_atom lang_type_get_atom(LANG_TYPE_MODE mode, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE: {
            Lang_type_atom atom = lang_type_primitive_get_atom(mode, lang_type_primitive_const_unwrap(lang_type));
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            return atom;
        }
        case LANG_TYPE_ENUM: {
            Lang_type_atom atom = lang_type_enum_const_unwrap(lang_type).atom;
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_STRUCT: {
            Lang_type_atom atom = lang_type_struct_const_unwrap(lang_type).atom;
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_RAW_UNION: {
            Lang_type_atom atom = lang_type_raw_union_const_unwrap(lang_type).atom;
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LANG_TYPE_TUPLE: {
            unreachable("");
        }
        case LANG_TYPE_FN: {
            Lang_type_atom atom = lang_type_atom_new_from_cstr("", 1, 0);
            assert(!str_view_cstr_is_equal(atom.str.base, "void"));
            return atom;
        }
        case LANG_TYPE_VOID: {
            Lang_type_atom atom = lang_type_atom_new_from_cstr("void", 0, SCOPE_BUILTIN);
            return atom;
        }
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
            lang_type_signed_int_unwrap(lang_type)->bit_width = str_view_to_int64_t(
                POS_BUILTIN,
                str_view_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_signed_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_UNSIGNED_INT:
            lang_type_unsigned_int_unwrap(lang_type)->bit_width = str_view_to_int64_t(
                POS_BUILTIN,
                str_view_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_unsigned_int_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LANG_TYPE_FLOAT:
            lang_type_float_unwrap(lang_type)->bit_width = str_view_to_int64_t(
                POS_BUILTIN,
                str_view_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            lang_type_float_unwrap(lang_type)->pointer_depth = atom.pointer_depth;
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
            lang_type_primitive_set_atom( lang_type_primitive_unwrap(lang_type), atom);
            return;
        case LANG_TYPE_ENUM:
            lang_type_enum_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_STRUCT:
            lang_type_struct_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_RAW_UNION:
            lang_type_raw_union_unwrap(lang_type)->atom = atom;
            return;
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_FN:
            unreachable("");
        case LANG_TYPE_VOID:
            todo();
    }
    unreachable("");
}

static inline Name lang_type_get_str(LANG_TYPE_MODE mode, Lang_type lang_type) {
    return lang_type_get_atom(mode, lang_type).str;
}

static inline int16_t lang_type_get_pointer_depth(Lang_type lang_type) {
    return lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type).pointer_depth;
}

static inline int16_t lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE mode, Lang_type_primitive lang_type) {
    return lang_type_primitive_get_atom(mode, lang_type).pointer_depth;
}

static inline int32_t lang_type_primitive_get_bit_width(Lang_type_primitive lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_CHAR:
            unreachable("");
        case LANG_TYPE_UNSIGNED_INT:
            return lang_type_unsigned_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_SIGNED_INT:
            return lang_type_signed_int_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_FLOAT:
            return lang_type_float_const_unwrap(lang_type).bit_width;
        case LANG_TYPE_ANY:
            unreachable("");
    }
    unreachable("");
}

static inline int32_t lang_type_get_bit_width(Lang_type lang_type) {
    return lang_type_primitive_get_bit_width(lang_type_primitive_const_unwrap(lang_type));
}

static inline void lang_type_set_pointer_depth(Lang_type* lang_type, int16_t pointer_depth) {
    Lang_type_atom atom = lang_type_get_atom(LANG_TYPE_MODE_LOG, *lang_type);
    atom.pointer_depth = pointer_depth;
    lang_type_set_atom( lang_type, atom);
}

#endif // LANG_TYPE_AFTER_H
