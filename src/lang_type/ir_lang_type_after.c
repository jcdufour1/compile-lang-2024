#include <ir_lang_type_after.h>
#include <str_and_num_utils.h>
#include <ast_msg.h>
#include <name.h>
#include <bits_print.h>

static Name ir_lang_type_primitive_get_name_normal(Ir_lang_type_primitive ir_lang_type) {
    Strv new_base = {0};
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_SIGNED_INT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "i"FMT, bits_print(ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width));
            break;
        }
        case IR_LANG_TYPE_FLOAT: {
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "f"FMT, bits_print(ir_lang_type_float_const_unwrap(ir_lang_type).bit_width));
            break;
        }
        case IR_LANG_TYPE_UNSIGNED_INT:
            // TODO: use hashtable, etc. to reduce allocations?
            new_base = strv_from_f(&a_main, "u"FMT, bits_print(ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width));
            break;
        case IR_LANG_TYPE_OPAQUE: {
            new_base = sv("opaque");
            break;
        }
        default:
            unreachable("");
    }

    assert(new_base.count > 0);
    Name new_name = name_new(
        MOD_PATH_BUILTIN,
        new_base,
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
    assert(!strv_is_equal(new_name.base, sv("void")));
    assert(new_name.base.count > 0);
    return new_name;
}

static Name ir_lang_type_primitive_get_name_c(Ir_lang_type_primitive ir_lang_type) {
    Strv new_base = {0};
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_FLOAT: {
            Bits bit_width = ir_lang_type_float_const_unwrap(ir_lang_type).bit_width;
            if (bits_is_equal(bit_width, bits_new(32))) {
                new_base = sv("float");
            } else if (bits_is_equal(bit_width, bits_new(64))) {
                new_base = sv("double");
            } else if (bits_is_equal(bit_width, bits_new(128))) {
                new_base = sv("long double");
            } else {
                msg_todo(
                    "bit widths other than 32, 64, or 128 (for floating point numbers) with the c backend",
                    ir_lang_type_primitive_get_pos(ir_lang_type)
                );
                return util_literal_name_new_poison();
            }
            break;
        }
        case IR_LANG_TYPE_SIGNED_INT: {
            Bits bit_width = ir_lang_type_signed_int_const_unwrap(ir_lang_type).bit_width;
            if (
                bits_is_equal(bit_width, bits_new(8)) || 
                bits_is_equal(bit_width, bits_new(16)) || 
                bits_is_equal(bit_width, bits_new(32)) ||
                bits_is_equal(bit_width, bits_new(64))
            ) {
                new_base = strv_from_f(&a_main, "int"FMT"_t", bits_print(bit_width));
            } else {
                msg_todo(
                    "bit widths other than 8, 16, 32, or 64 (for signed integers) with the c backend",
                    ir_lang_type_primitive_get_pos(ir_lang_type)
                );
                return util_literal_name_new_poison();
            }
            break;
        }
        case IR_LANG_TYPE_UNSIGNED_INT: {
            Bits bit_width = ir_lang_type_unsigned_int_const_unwrap(ir_lang_type).bit_width;
            if (bits_is_equal(bit_width, bits_new(1))) {
                // TODO: overflow may not work correctly when using bool
                new_base = sv("bool");
            } else if (
                bits_is_equal(bit_width, bits_new(8)) || 
                bits_is_equal(bit_width, bits_new(16)) || 
                bits_is_equal(bit_width, bits_new(32)) ||
                bits_is_equal(bit_width, bits_new(64))
            ) {
                new_base = strv_from_f(&a_main, "uint"FMT"_t", bits_print(bit_width));
            } else {
                msg_todo(
                    "bit widths other than 1, 8, 16, 32, or 64 (for unsigned integers) with the c backend",
                    ir_lang_type_primitive_get_pos(ir_lang_type)
                );
                return util_literal_name_new_poison();
            }
            break;
        }
        case IR_LANG_TYPE_OPAQUE:
            new_base = sv("void");
            break;
    }
//
    assert(new_base.count > 0);
    Name new_name = name_new(
        MOD_PATH_EXTERN_C,
        new_base,
        (Ulang_type_darr) {0},
        SCOPE_TOP_LEVEL
    );
    assert(new_name.base.count > 0);
    return new_name;
}

Name ir_lang_type_primitive_get_name(LANG_TYPE_MODE mode, Ir_lang_type_primitive ir_lang_type) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return ir_lang_type_primitive_get_name_normal(ir_lang_type);
        case LANG_TYPE_MODE_MSG:
            return ir_lang_type_primitive_get_name_normal(ir_lang_type);
        case LANG_TYPE_MODE_EMIT_LLVM:
            todo();
        case LANG_TYPE_MODE_EMIT_C:
            return ir_lang_type_primitive_get_name_c(ir_lang_type);
    }
    unreachable("");

}

bool ir_lang_type_get_name(Name* result, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_PRIMITIVE:
            *result = ir_lang_type_primitive_get_name(mode, ir_lang_type_primitive_const_unwrap(ir_lang_type));
            return true;
        case IR_LANG_TYPE_STRUCT:
            *result = ir_lang_type_struct_const_unwrap(ir_lang_type).name;
            return true;
        case IR_LANG_TYPE_TUPLE: {
            return false;
        }
        case IR_LANG_TYPE_FN: {
            return false;
        }
        case IR_LANG_TYPE_VOID: {
            *result = name_new(
                MOD_PATH_BUILTIN,
                sv("void"),
                (Ulang_type_darr) {0},
                SCOPE_TOP_LEVEL
            );
            return true;
        }
        default:
            unreachable("");
    }
    unreachable("");
}

