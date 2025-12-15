#include <lang_type_print.h>
#include <lang_type_after.h>
#include <ulang_type.h>
#include <resolve_generics.h>
#include <ast_msg.h>

static void extend_lang_type_lit_tag_to_string(String* buf, Lang_type_lit lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_INT_LIT:
            string_extend_cstr(&a_temp, buf, "int");
            return;
        case LANG_TYPE_STRUCT_LIT:
            string_extend_cstr(&a_temp, buf, "struct_lit");
            return;
        case LANG_TYPE_STRING_LIT:
            string_extend_cstr(&a_temp, buf, "string_lit");
            return;
        case LANG_TYPE_FN_LIT:
            string_extend_cstr(&a_temp, buf, "fn");
            return;
        case LANG_TYPE_FLOAT_LIT:
            string_extend_cstr(&a_temp, buf, "float");
            return;
    }
    unreachable("");
}

void extend_lang_type_tag_to_string(String* buf, Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&a_temp, buf, "primitive");
            return;
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&a_temp, buf, "struct");
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&a_temp, buf, "raw_union");
            return;
        case LANG_TYPE_ENUM:
            string_extend_cstr(&a_temp, buf, "enum");
            return;
        case LANG_TYPE_TUPLE:
            string_extend_cstr(&a_temp, buf, "tuple");
            return;
        case LANG_TYPE_VOID:
            string_extend_cstr(&a_temp, buf, "void");
            return;
        case LANG_TYPE_FN:
            string_extend_cstr(&a_temp, buf, "fn");
            return;
        case LANG_TYPE_ARRAY:
            string_extend_cstr(&a_temp, buf, "array");
            return;
        case LANG_TYPE_LIT:
            extend_lang_type_lit_tag_to_string(buf, lang_type_lit_const_unwrap(lang_type));
            return;
        case LANG_TYPE_REMOVED:
            string_extend_cstr(&a_temp, buf, "removed");
            return;
    }
    unreachable("");
}

Strv lang_type_vec_print_internal(Lang_type_vec types) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_lang_type_to_string(&buf, LANG_TYPE_MODE_MSG, vec_at(types, idx));
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

Strv lang_type_print_internal(LANG_TYPE_MODE mode, Lang_type lang_type) {
    String buf = {0};
    extend_lang_type_to_string(&buf, mode, lang_type);
    switch (mode) {
        case LANG_TYPE_MODE_EMIT_LLVM:
            break;
        case LANG_TYPE_MODE_EMIT_C:
            break;
        case LANG_TYPE_MODE_MSG:
            break;
        case LANG_TYPE_MODE_LOG:
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

static void extend_lang_type_lit_to_string(String* string, Lang_type_lit lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_INT_LIT:
            string_extend_int64_t(&a_temp, string, lang_type_int_lit_const_unwrap(lang_type).data);
            return;
        case LANG_TYPE_FLOAT_LIT:
            string_extend_double(&a_temp, string, lang_type_float_lit_const_unwrap(lang_type).data);
            return;
        case LANG_TYPE_STRING_LIT:
            serialize_strv_actual(string, lang_type_string_lit_const_unwrap(lang_type).data);
            return;
        case LANG_TYPE_STRUCT_LIT:
            // TODO: this looks ugly
            string_extend_strv(&a_temp, string, uast_expr_print_internal(lang_type_struct_lit_const_unwrap(lang_type).lit, 0));
            return;
        case LANG_TYPE_FN_LIT:
            extend_name(NAME_MSG, string, lang_type_fn_lit_const_unwrap(lang_type).name);
            return;
    }
    unreachable("");
}

static NAME_MODE lang_type_mode_to_name_mode(LANG_TYPE_MODE mode) {
    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            return NAME_LOG;
        case LANG_TYPE_MODE_MSG:
            return NAME_MSG;
        case LANG_TYPE_MODE_EMIT_LLVM:
            return NAME_EMIT_C;
        case LANG_TYPE_MODE_EMIT_C:
            return NAME_EMIT_IR;
    }
    unreachable("");
}

void extend_lang_type_to_string(String* string, LANG_TYPE_MODE mode, Lang_type lang_type) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '<');
    }

    switch (mode) {
        case LANG_TYPE_MODE_LOG:
            extend_lang_type_tag_to_string(string, lang_type);
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

    if (lang_type.type == LANG_TYPE_PRIMITIVE) {
        // TODO: uncomment this?
        //unwrap(!strv_is_equal(lang_type_get_atom(LANG_TYPE_MODE_LOG, lang_type).str.base, sv("void")));
    }

    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, "(");
            }
            Lang_type_vec lang_types = lang_type_tuple_const_unwrap(lang_type).lang_types;
            for (size_t idx = 0; idx < lang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&a_main, string, ", ");
                }
                extend_lang_type_to_string(string, mode, vec_at(lang_types, idx));
            }
            if (mode == LANG_TYPE_MODE_MSG) {
                string_extend_cstr(&a_main, string, ")");
            }
            break;
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            string_extend_cstr(&a_main, string, "fn");
            extend_lang_type_to_string(string, mode, lang_type_tuple_const_wrap(fn.params));
            extend_lang_type_to_string(string, mode, *fn.return_type);
            if (fn.pointer_depth != 1) {
                todo();
            }
            break;
        }
        case LANG_TYPE_ARRAY: {
            Lang_type_array array = lang_type_array_const_unwrap(lang_type);
            extend_lang_type_to_string(string, mode, *array.item_type);
            string_extend_cstr(&a_temp, string, "[");
            string_extend_int64_t(&a_temp, string, array.count);
            string_extend_cstr(&a_temp, string, "]");
            for (int16_t idx = 0; idx < array.pointer_depth; idx++) {
                vec_append(&a_temp, string, '*');
            }
            break;
        }
        case LANG_TYPE_ENUM:
            fallthrough;
        case LANG_TYPE_RAW_UNION:
            fallthrough;
        case LANG_TYPE_STRUCT:
            // TODO: uncomment below assert?
            //assert(!strv_is_equal(lang_type_get_atom(mode, lang_type).str.base, sv("void")));
            fallthrough;
        case LANG_TYPE_VOID: {
            Name name = {0};
            if (!lang_type_get_name(&name, mode, lang_type)) {
                msg_todo("", lang_type_get_pos(lang_type));
                break;
            }

            extend_name(lang_type_mode_to_name_mode(mode), string, name);
            break;
        }
        case LANG_TYPE_PRIMITIVE: {
            //Strv base = lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type).base;
            Name name = {0};
            if (!lang_type_get_name(&name, mode, lang_type)) {
                msg_todo("", lang_type_get_pos(lang_type));
                break;
            }
            assert(strv_is_equal(name.mod_path, MOD_PATH_BUILTIN));
            assert(name.base.count >= 1);
            extend_name(lang_type_mode_to_name_mode(mode), string, name);
            for (int16_t idx = 0; idx < lang_type_get_pointer_depth(lang_type); idx++) {
                vec_append(&a_temp, string, '*');
            }
            break;
        }
        case LANG_TYPE_REMOVED:
            break;
        case LANG_TYPE_LIT:
            extend_lang_type_lit_to_string(string, lang_type_lit_const_unwrap(lang_type));
            break;
        default:
            unreachable("");
    }

    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&a_temp, string, '>');
    }
}

