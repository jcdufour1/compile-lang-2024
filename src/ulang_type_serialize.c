#include <ulang_type_serialize.h>

Str_view serialize_ulang_type_atom(Ulang_type_atom atom) {
    String name = {0};
    string_extend_size_t(&a_main, &name, atom.pointer_depth);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_size_t(&a_main, &name, atom.str.count);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, atom.str);
    string_extend_cstr(&a_main, &name, "__");
    return string_to_strv(name);
}

Str_view serialize_ulang_type_tuple(Env* env, Ulang_type_tuple ulang_type) {
    String name = {0};
    for (size_t idx = 0; idx < ulang_type.ulang_types.info.count; idx++) {
        Ulang_type curr = vec_at_const(ulang_type.ulang_types, idx);
        string_extend_strv(&a_main, &name, serialize_ulang_type(env, curr));
    }
    return string_to_strv(name);
}

Str_view serialize_ulang_type_fn(Env* env, Ulang_type_fn ulang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_ulang_type_tuple(env, ulang_type.params));
    string_extend_strv(&a_main, &name, serialize_ulang_type(env, *ulang_type.return_type));
    return string_to_strv(name);
}

Str_view serialize_ulang_type_regular(Ulang_type_regular ulang_type) {
    return serialize_ulang_type_atom(ulang_type.atom);
}

