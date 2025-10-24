#include <ir_lang_type_print.h>
#include <ir_lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>

void extend_ir_lang_type_tag_to_string(String* buf, IR_LANG_TYPE_TYPE type) {
    switch (type) {
        case IR_LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&a_print, buf, "primitive");
            return;
        case IR_LANG_TYPE_STRUCT:
            string_extend_cstr(&a_print, buf, "struct");
            return;
        case IR_LANG_TYPE_TUPLE:
            string_extend_cstr(&a_print, buf, "tuple");
            return;
        case IR_LANG_TYPE_VOID:
            string_extend_cstr(&a_print, buf, "void");
            return;
        case IR_LANG_TYPE_FN:
            string_extend_cstr(&a_print, buf, "fn");
            return;
    }
    unreachable("");
}

Strv ir_lang_type_vec_print_internal(Ir_lang_type_vec types) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, vec_at(types, idx));
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

void extend_ir_lang_type_atom(String* string, LANG_TYPE_MODE mode, Ir_lang_type_atom atom) {
    Name temp = atom.str;

    if (atom.str.base.count > 1) {
        switch (mode) {
            case LANG_TYPE_MODE_LOG:
                extend_name(NAME_LOG, string, atom.str);
                break;
            case LANG_TYPE_MODE_MSG:
                extend_name(NAME_MSG, string, atom.str);
                break;
            case LANG_TYPE_MODE_EMIT_C:
                extend_name(NAME_EMIT_C, string, atom.str);
                break;
            case LANG_TYPE_MODE_EMIT_LLVM:
                extend_name(NAME_EMIT_IR, string, atom.str);
                break;
            default:
                unreachable("");
        }
    } else {
        string_extend_cstr(&a_print, string, "void");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&a_print, string, '*');
    }

    if (mode == LANG_TYPE_MODE_EMIT_LLVM) {
        if (temp.gen_args.info.count > 0) {
            todo();
        }
    }
}

Strv ir_lang_type_print_internal(LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    String buf = {0};
    extend_ir_lang_type_to_string(&buf, mode, ir_lang_type);
    switch (mode) {
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_EMIT_C:
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_LOG:
            string_extend_cstr(&a_print, &buf, "\n");
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

Strv ir_lang_type_atom_print_internal(Ir_lang_type_atom atom, LANG_TYPE_MODE mode) {
    String buf = {0};
    extend_ir_lang_type_atom(&buf, mode, atom);
    return string_to_strv(buf);
}

void extend_ir_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Ir_lang_type ir_lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_print, string, '<');
    }

    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            extend_ir_lang_type_tag_to_string(string, ir_lang_type.type);
            string_extend_cstr(&a_main, string, " ");
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_EMIT_C:
            break;
        default:
            unreachable("");
    }

    switch (ir_lang_type.type) {
        case IR_LANG_TYPE_TUPLE:
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, "(");
            }
            Ir_lang_type_vec ir_lang_types = ir_lang_type_tuple_const_unwrap(ir_lang_type).ir_lang_types;
            for (size_t idx = 0; idx < ir_lang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&a_main, string, ", ");
                }
                extend_ir_lang_type_to_string(string, mode, vec_at(ir_lang_types, idx));
            }
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, ")");
            }
            goto end;
        case IR_LANG_TYPE_FN: {
            Ir_lang_type_fn fn = ir_lang_type_fn_const_unwrap(ir_lang_type);
            string_extend_cstr(&a_main, string, "fn");
            extend_ir_lang_type_to_string(string, mode, ir_lang_type_tuple_const_wrap(fn.params));
            extend_ir_lang_type_to_string(string, mode, *fn.return_type);
            goto end;
        }
        case IR_LANG_TYPE_STRUCT:
            // fallthrough
            unwrap(!strv_is_equal(ir_lang_type_get_atom(mode, ir_lang_type).str.base, sv("void")));
        case IR_LANG_TYPE_VOID:
            // fallthrough
        case IR_LANG_TYPE_PRIMITIVE:
            extend_ir_lang_type_atom(string, mode, ir_lang_type_get_atom(mode, ir_lang_type));
            goto end;
    }
    unreachable("");

end:
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_print, string, '>');
    }
}

