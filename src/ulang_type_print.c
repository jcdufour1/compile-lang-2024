#include <ulang_type_print.h>

Str_view ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type) {
    String buf = {0};
    extend_ulang_type_to_string(&buf, mode, lang_type);
    if (mode == LANG_TYPE_MODE_LOG) {
        string_extend_cstr(&print_arena, &buf, "\n");
    }
    return string_to_strv(buf);
}

void extend_ulang_type_atom_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_atom atom) {
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '<');
    }

    if (atom.str.count > 1) {
        string_extend_strv(&print_arena, string, atom.str);
    } else {
        string_extend_cstr(&print_arena, string, "<null>");
    }
    if (atom.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
        vec_append(&print_arena, string, '*');
    }
    if (mode == LANG_TYPE_MODE_LOG) {
        vec_append(&print_arena, string, '>');
    }
    return;
}

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            extend_ulang_type_atom_to_string(string, mode, ulang_type_regular_const_unwrap(lang_type).atom);
            return;
        case ULANG_TYPE_TUPLE: {
            vec_append(&print_arena, string, '(');
            Ulang_type_tuple tuple = ulang_type_tuple_const_unwrap(lang_type);
            for (size_t idx = 0; idx < tuple.ulang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&print_arena, string, ", ");
                }
                extend_ulang_type_to_string(string, mode, vec_at(&tuple.ulang_types, idx));
            }
            vec_append(&print_arena, string, ')');
            return;
        }
        case ULANG_TYPE_FN: {
            string_extend_cstr(&a_main, string, "fn");
            Ulang_type_fn fn = ulang_type_fn_const_unwrap(lang_type);
            extend_ulang_type_to_string(string, mode, ulang_type_tuple_const_wrap(fn.params));
            extend_ulang_type_to_string(string, mode, *fn.return_type);
            return;
        }
        case ULANG_TYPE_REG_GENERIC: {
            Ulang_type_reg_generic reg_gen = ulang_type_reg_generic_const_unwrap(lang_type);
            extend_ulang_type_atom_to_string(string, mode, reg_gen.atom);
            string_extend_cstr(&a_main, string, "(<");
            for (size_t idx = 0; idx < reg_gen.generic_args.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&print_arena, string, ", ");
                }
                extend_ulang_type_to_string(string, mode, vec_at(&reg_gen.generic_args, idx));
            }
            string_extend_cstr(&a_main, string, ">)");
            return;
        }
    }
    unreachable("");
}

