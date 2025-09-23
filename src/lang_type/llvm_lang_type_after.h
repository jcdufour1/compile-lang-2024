#ifndef LLVM_LANG_TYPE_AFTER_H
#define LLVM_LANG_TYPE_AFTER_H

#include <msg_todo.h>
#include <llvm_lang_type_get_pos.h>

// TODO: do these things properly
int64_t strv_to_int64_t(const Pos pos, Strv strv);

static inline Llvm_lang_type_atom llvm_lang_type_primitive_get_atom_normal(Llvm_lang_type_primitive llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "i");
            string_extend_int64_t(&a_main, &string, llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).bit_width);
            Llvm_lang_type_atom atom = llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LLVM_LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "f");
            string_extend_int64_t(&a_main, &string, llvm_lang_type_float_const_unwrap(llvm_lang_type).bit_width);
            Llvm_lang_type_atom atom = llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_float_const_unwrap(llvm_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LLVM_LANG_TYPE_UNSIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations
            String string = {0};
            string_extend_cstr(&a_main, &string, "u");
            string_extend_int64_t(&a_main, &string, llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).bit_width);
            Llvm_lang_type_atom atom = llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).pointer_depth
            );
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LLVM_LANG_TYPE_OPAQUE: {
            // TODO: remove atom from LLVM_LANG_TYPE_OPAQUE
            Llvm_lang_type_atom atom = llvm_lang_type_opaque_const_unwrap(llvm_lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
    }
    unreachable("");
}

static inline Llvm_lang_type_atom llvm_lang_type_primitive_get_atom_c(Llvm_lang_type_primitive llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_FLOAT: {
            String string = {0};
            uint32_t bit_width = llvm_lang_type_float_const_unwrap(llvm_lang_type).bit_width;
            if (bit_width == 32) {
                string_extend_cstr(&a_main, &string, "float");
            } else if (bit_width == 64) {
                string_extend_cstr(&a_main, &string, "double");
            } else {
                    msg_todo("bit widths other than 32 or 64 (for floating point numbers) with the c backend", llvm_lang_type_primitive_get_pos(llvm_lang_type));
            }
            return llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_float_const_unwrap(llvm_lang_type).pointer_depth
            );
        }
        case LLVM_LANG_TYPE_SIGNED_INT: {
            String string = {0};
            uint32_t bit_width = llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).bit_width;
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
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 (for integers) with the c backend", llvm_lang_type_primitive_get_pos(llvm_lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).pointer_depth
            );
        }
        case LLVM_LANG_TYPE_UNSIGNED_INT: {
            // TODO: deduplicate this and above case?
            // TODO: bit width of 1 here?
            String string = {0};
            uint32_t bit_width = llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).bit_width;
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
                    msg_todo("bit widths other than 1, 8, 16, 32, or 64 with the c backend", llvm_lang_type_primitive_get_pos(llvm_lang_type));
                }
                string_extend_cstr(&a_main, &string, "_t");
            }
            return llvm_lang_type_atom_new(
                name_new((Strv) {0}, string_to_strv(string), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).pointer_depth
            );
        }
        case LLVM_LANG_TYPE_OPAQUE:
            return llvm_lang_type_atom_new(
                name_new((Strv) {0}, sv("void"), (Ulang_type_vec) {0}, 0),
                llvm_lang_type_opaque_const_unwrap(llvm_lang_type).atom.pointer_depth
            );
    }
    unreachable("");
}

static inline Llvm_lang_type_atom llvm_lang_type_primitive_get_atom(LANG_TYPE_MODE mode, Llvm_lang_type_primitive llvm_lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return llvm_lang_type_primitive_get_atom_normal(llvm_lang_type);
        case LANG_TYPE_MODE_MSG:
            return llvm_lang_type_primitive_get_atom_normal(llvm_lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            return llvm_lang_type_primitive_get_atom_normal(llvm_lang_type);
        case LANG_TYPE_MODE_EMIT_C:
            return llvm_lang_type_primitive_get_atom_c(llvm_lang_type);
    }
    unreachable("");
}

static inline Llvm_lang_type_atom llvm_lang_type_get_atom(LANG_TYPE_MODE mode, Llvm_lang_type llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_PRIMITIVE: {
            Llvm_lang_type_atom atom = llvm_lang_type_primitive_get_atom(mode, llvm_lang_type_primitive_const_unwrap(llvm_lang_type));
            return atom;
        }
        case LLVM_LANG_TYPE_STRUCT: {
            Llvm_lang_type_atom atom = llvm_lang_type_struct_const_unwrap(llvm_lang_type).atom;
            assert(!strv_is_equal(atom.str.base, sv("void")));
            assert(atom.str.base.count > 0);
            return atom;
        }
        case LLVM_LANG_TYPE_TUPLE: {
            unreachable("");
        }
        case LLVM_LANG_TYPE_FN: {
            Llvm_lang_type_atom atom = llvm_lang_type_atom_new_from_cstr("", 1, 0);
            assert(!strv_is_equal(atom.str.base, sv("void")));
            return atom;
        }
        case LLVM_LANG_TYPE_VOID: {
            Llvm_lang_type_atom atom = llvm_lang_type_atom_new_from_cstr("void", 0, SCOPE_BUILTIN);
            return atom;
        }
    }
    unreachable("");
}

// TODO: remove this function?
static inline void llvm_lang_type_primitive_set_atom(Llvm_lang_type_primitive* llvm_lang_type, Llvm_lang_type_atom atom) {
    switch (llvm_lang_type->type) {
        case LLVM_LANG_TYPE_SIGNED_INT:
            llvm_lang_type_signed_int_unwrap(llvm_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            llvm_lang_type_signed_int_unwrap(llvm_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LLVM_LANG_TYPE_UNSIGNED_INT:
            llvm_lang_type_unsigned_int_unwrap(llvm_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            llvm_lang_type_unsigned_int_unwrap(llvm_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LLVM_LANG_TYPE_FLOAT:
            llvm_lang_type_float_unwrap(llvm_lang_type)->bit_width = strv_to_int64_t(
                POS_BUILTIN,
                strv_slice(atom.str.base, 1, atom.str.base.count - 1)
            );
            llvm_lang_type_float_unwrap(llvm_lang_type)->pointer_depth = atom.pointer_depth;
            return;
        case LLVM_LANG_TYPE_OPAQUE:
            llvm_lang_type_opaque_unwrap(llvm_lang_type)->atom = atom;
            return;
    }
    unreachable("");
}

static inline void llvm_lang_type_set_atom(Llvm_lang_type* llvm_lang_type, Llvm_lang_type_atom atom) {
    switch (llvm_lang_type->type) {
        case LLVM_LANG_TYPE_PRIMITIVE:
            llvm_lang_type_primitive_set_atom( llvm_lang_type_primitive_unwrap(llvm_lang_type), atom);
            return;
        case LLVM_LANG_TYPE_STRUCT:
            llvm_lang_type_struct_unwrap(llvm_lang_type)->atom = atom;
            return;
        case LLVM_LANG_TYPE_TUPLE:
            unreachable("");
        case LLVM_LANG_TYPE_FN:
            unreachable("");
        case LLVM_LANG_TYPE_VOID:
            todo();
    }
    unreachable("");
}

static inline Name llvm_lang_type_get_str(LANG_TYPE_MODE mode, Llvm_lang_type llvm_lang_type) {
    return llvm_lang_type_get_atom(mode, llvm_lang_type).str;
}

static inline int16_t llvm_lang_type_get_pointer_depth(Llvm_lang_type llvm_lang_type) {
    return llvm_lang_type_get_atom(LANG_TYPE_MODE_LOG, llvm_lang_type).pointer_depth;
}

static inline int16_t llvm_lang_type_primitive_get_pointer_depth(LANG_TYPE_MODE mode, Llvm_lang_type_primitive llvm_lang_type) {
    return llvm_lang_type_primitive_get_atom(mode, llvm_lang_type).pointer_depth;
}

static inline int32_t llvm_lang_type_primitive_get_bit_width(Llvm_lang_type_primitive llvm_lang_type) {
    switch (llvm_lang_type.type) {
        case LLVM_LANG_TYPE_UNSIGNED_INT:
            return llvm_lang_type_unsigned_int_const_unwrap(llvm_lang_type).bit_width;
        case LLVM_LANG_TYPE_SIGNED_INT:
            return llvm_lang_type_signed_int_const_unwrap(llvm_lang_type).bit_width;
        case LLVM_LANG_TYPE_FLOAT:
            return llvm_lang_type_float_const_unwrap(llvm_lang_type).bit_width;
        case LLVM_LANG_TYPE_OPAQUE:
            unreachable("");
    }
    unreachable("");
}

static inline int32_t llvm_lang_type_get_bit_width(Llvm_lang_type llvm_lang_type) {
    return llvm_lang_type_primitive_get_bit_width(llvm_lang_type_primitive_const_unwrap(llvm_lang_type));
}

static inline void llvm_lang_type_set_pointer_depth(Llvm_lang_type* llvm_lang_type, int16_t pointer_depth) {
    Llvm_lang_type_atom atom = llvm_lang_type_get_atom(LANG_TYPE_MODE_LOG, *llvm_lang_type);
    atom.pointer_depth = pointer_depth;
    llvm_lang_type_set_atom( llvm_lang_type, atom);
}

static inline Llvm_lang_type llvm_lang_type_new_ux(int32_t bit_width) {
    return llvm_lang_type_primitive_const_wrap(llvm_lang_type_unsigned_int_const_wrap(
        llvm_lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)
    ));
}

static inline Llvm_lang_type llvm_lang_type_new_u8(void) {
    return llvm_lang_type_new_ux(8);
}

static inline Llvm_lang_type llvm_lang_type_new_usize(void) {
    return llvm_lang_type_new_ux(64 /* TODO: change based on target */);
}

#endif // LLVM_LANG_TYPE_AFTER_H
