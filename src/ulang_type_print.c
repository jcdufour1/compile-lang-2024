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
}

void extend_ulang_type_regular_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_regular lang_type) {
    extend_ulang_type_atom_to_string(string, mode, lang_type.atom);
}

void extend_ulang_type_tuple_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_tuple lang_type) {
    vec_append(&print_arena, string, '(');
    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
            string_extend_cstr(&print_arena, string, ", ");
        }
        extend_ulang_type_to_string(string, mode, vec_at(&lang_type.ulang_types, idx));
    }
    vec_append(&print_arena, string, ')');
}

void extend_ulang_type_fn_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_fn lang_type) {
    string_extend_cstr(&a_main, string, "fn");
    extend_ulang_type_to_string(string, mode, ulang_type_tuple_const_wrap(lang_type.params));
    extend_ulang_type_to_string(string, mode, *lang_type.return_type);
}

void extend_ulang_type_generic_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_generic lang_type) {
    extend_ulang_type_atom_to_string(string, mode, lang_type.atom);
    string_extend_cstr(&a_main, string, "(<");
    for (size_t idx = 0; idx < lang_type.generic_args.info.count; idx++) {
        if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
            string_extend_cstr(&print_arena, string, ", ");
        }
        extend_ulang_type_to_string(string, mode, vec_at(&lang_type.generic_args, idx));
    }
    string_extend_cstr(&a_main, string, ">)");
}

void extend_ulang_type_resol_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type_resol lang_type) {
    string_extend_cstr(&a_main, string, "(");
    extend_ulang_type_regular_to_string(string, mode, lang_type.resolved);
    string_extend_cstr(&a_main, string, ")");
    extend_ulang_type_generic_to_string(string, mode, lang_type.original);
    todo();
}

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            extend_ulang_type_regular_to_string(string, mode, ulang_type_regular_const_unwrap(lang_type));
            return;
        case ULANG_TYPE_TUPLE:
            extend_ulang_type_tuple_to_string(string, mode, ulang_type_tuple_const_unwrap(lang_type));
            return;
        case ULANG_TYPE_FN:
            extend_ulang_type_fn_to_string(string, mode, ulang_type_fn_const_unwrap(lang_type));
            return;
        case ULANG_TYPE_REG_GENERIC:
            extend_ulang_type_generic_to_string(string, mode, ulang_type_generic_const_unwrap(lang_type));
            return;
        case ULANG_TYPE_RESOL:
            extend_ulang_type_resol_to_string(string, mode, ulang_type_resol_const_unwrap(lang_type));
            return;
    }
    unreachable("");
}

