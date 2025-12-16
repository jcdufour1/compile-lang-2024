#include <ulang_type.h>
#include <lang_type.h>
#include <lang_type_from_ulang_type.h>
#include <lang_type_print.h>
#include <env.h>

Strv ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type) {
    bool old_silent_resol_errors = env.silent_generic_resol_errors;
    env.silent_generic_resol_errors = true;
    if (mode == LANG_TYPE_MODE_MSG) {
        Lang_type new_lang_type = {0};
        if (try_lang_type_from_ulang_type(&new_lang_type, lang_type)) {
            env.silent_generic_resol_errors = old_silent_resol_errors;
            return lang_type_print_internal(LANG_TYPE_MODE_MSG, new_lang_type);
        }
    }

    String buf = {0};
    extend_ulang_type_to_string(&buf, mode, lang_type);
    if (mode == LANG_TYPE_MODE_LOG) {
        string_extend_cstr(&a_temp, &buf, "\n");
    }
    env.silent_generic_resol_errors = old_silent_resol_errors;
    return string_to_strv(buf);
}

//void extend_ulang_type_atom_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_atom atom) {
//    // TODO: remove?
//    if (mode == LANG_TYPE_MODE_LOG) {
//        vec_append(&a_temp, string, '<');
//    }
//
//    if (atom.str.base.count > 1) {
//        extend_uname(mode == LANG_TYPE_MODE_MSG ? UNAME_MSG : UNAME_LOG, string, atom.str);
//    } else {
//        string_extend_cstr(&a_temp, string, "<null>");
//    }
//    if (atom.pointer_depth < 0) {
//        todo();
//    }
//    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
//        vec_append(&a_temp, string, '*');
//    }
//
//    // TODO: remove?
//    if (mode == LANG_TYPE_MODE_LOG) {
//        vec_append(&a_temp, string, '>');
//    }
//    return;
//}

static void string_extend_ulang_type_lit(String* string, Ulang_type_lit lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_INT_LIT:
            string_extend_cstr(&a_main, string, "int ");
            string_extend_int64_t(&a_main, string, ulang_type_int_lit_const_unwrap(lang_type).data);
            return;
        case ULANG_TYPE_FLOAT_LIT:
            string_extend_cstr(&a_main, string, "float ");
            string_extend_strv(&a_main, string, serialize_double(ulang_type_float_lit_const_unwrap(lang_type).data));
            return;
        case ULANG_TYPE_STRING_LIT:
            string_extend_cstr(&a_main, string, "string ");
            serialize_strv_actual(string, ulang_type_string_lit_const_unwrap(lang_type).data);
            return;
        case ULANG_TYPE_STRUCT_LIT:
            string_extend_cstr(&a_main, string, "struct ");
            string_extend_strv(&a_main, string, uast_expr_print_internal(
                ulang_type_struct_lit_const_unwrap(lang_type).expr,
                0
            ));
            return;
        case ULANG_TYPE_FN_LIT:
            string_extend_cstr(&a_main, string, "fn ");
            extend_name(NAME_LOG, string, ulang_type_fn_lit_const_unwrap(lang_type).name);
            return;
    }
    unreachable("");
}

// TODO: accept arena argument
void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_ARRAY: {
            Ulang_type_array array = ulang_type_array_const_unwrap(lang_type);
            extend_ulang_type_to_string(string, mode, *array.item_type);
            string_extend_cstr(&a_temp, string, "[");
            string_extend_strv(&a_temp, string, uast_expr_print_internal(array.count, 0));
            string_extend_cstr(&a_temp, string, "]");
            return;
        }
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular reg = ulang_type_regular_const_unwrap(lang_type);
            // TODO: remove?
            if (mode == LANG_TYPE_MODE_LOG) {
                vec_append(&a_temp, string, '<');
            }

            if (reg.name.base.count > 1) {
                extend_uname(mode == LANG_TYPE_MODE_MSG ? UNAME_MSG : UNAME_LOG, string, reg.name);
            } else {
                string_extend_cstr(&a_temp, string, "<null>");
            }
            if (reg.pointer_depth < 0) {
                todo();
            }
            for (int16_t idx = 0; idx < reg.pointer_depth; idx++) {
                vec_append(&a_temp, string, '*');
            }

            // TODO: remove?
            if (mode == LANG_TYPE_MODE_LOG) {
                vec_append(&a_temp, string, '>');
            }
            return;
        }
        case ULANG_TYPE_TUPLE: {
            vec_append(&a_temp, string, '(');
            Ulang_type_tuple tuple = ulang_type_tuple_const_unwrap(lang_type);
            for (size_t idx = 0; idx < tuple.ulang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&a_temp, string, ", ");
                }
                extend_ulang_type_to_string(string, mode, vec_at(tuple.ulang_types, idx));
            }
            vec_append(&a_temp, string, ')');
            return;
        }
        case ULANG_TYPE_FN: {
            string_extend_cstr(&a_main, string, "fn");
            Ulang_type_fn fn = ulang_type_fn_const_unwrap(lang_type);
            extend_ulang_type_to_string(string, mode, ulang_type_tuple_const_wrap(fn.params));
            extend_ulang_type_to_string(string, mode, *fn.return_type);
            return;
        }
        case ULANG_TYPE_REMOVED: {
            string_extend_cstr(&a_main, string, "removed");
            return;
        }
        case ULANG_TYPE_EXPR: {
            string_extend_cstr(&a_main, string, "expr ");
            Ulang_type_expr expr = ulang_type_expr_const_unwrap(lang_type);
            string_extend_strv(&a_main, string, uast_expr_print_internal(expr.expr, 0));
            return;
        }
        case ULANG_TYPE_LIT: {
            string_extend_ulang_type_lit(string, ulang_type_lit_const_unwrap(lang_type));
            return;
        }
    }
    unreachable("");
}

