#include <ulang_type_serialize.h>
#include <parser_utils.h>
#include <ulang_type.h>
#include <name.h>

Strv serialize_ulang_type_atom(Ulang_type_atom atom, bool include_scope) {
    Name temp = {0};
    unwrap(name_from_uname( &temp, atom.str));
    Strv serialized = {0};
    if (include_scope) {
        serialized = serialize_name(temp);
    } else {
        serialized = serialize_name_symbol_table(temp);
    }

    String name = {0};
    string_extend_size_t(&a_main, &name, atom.pointer_depth);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_size_t(&a_main, &name, serialized.count);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, serialized);
    string_extend_cstr(&a_main, &name, "__");
    return string_to_strv(name);
}

Name serialize_ulang_type_fn(Strv mod_path, Ulang_type_fn ulang_type, bool include_scope) {
    String name = {0};
    extend_name(NAME_LOG, &name, serialize_ulang_type_tuple(mod_path, ulang_type.params, include_scope));
    string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, *ulang_type.return_type, include_scope)));
    return name_new(mod_path, string_to_strv(name), (Ulang_type_vec) {0}, 0 /* TODO */);
}

Name serialize_ulang_type_tuple(Strv mod_path, Ulang_type_tuple ulang_type, bool include_scope) {
    String name = {0};
    for (size_t idx = 0; idx < ulang_type.ulang_types.info.count; idx++) {
        Ulang_type curr = vec_at_const(ulang_type.ulang_types, idx);
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, curr, include_scope)));
    }
    return name_new(mod_path, string_to_strv(name), (Ulang_type_vec) {0}, 0);
}

Name serialize_ulang_type_regular(Strv mod_path, Ulang_type_regular ulang_type, bool include_scope) {
    return name_new(mod_path, serialize_ulang_type_atom(ulang_type.atom, include_scope), (Ulang_type_vec) {0}, 0 /* TODO */);
}

Strv serialize_ulang_type_vec(Strv mod_path, Ulang_type_vec vec, bool include_scope) {
    String name = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, vec_at(&vec, idx), include_scope)));
    }
    return string_to_strv(name);
}

Name serialize_ulang_type(Strv mod_path, Ulang_type ulang_type, bool include_scope) {
    switch (ulang_type.type) {
        case ULANG_TYPE_REGULAR:
            return serialize_ulang_type_regular(mod_path, ulang_type_regular_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_FN:
            return serialize_ulang_type_fn( mod_path, ulang_type_fn_const_unwrap(ulang_type), include_scope);
        case ULANG_TYPE_TUPLE:
            todo();
    }
    unreachable("");
}

