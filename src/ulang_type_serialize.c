#include <ulang_type_serialize.h>
#include <parser_utils.h>
#include <ulang_type.h>
#include <serialize_module_symbol_name.h>

Str_view serialize_ulang_type_atom(Env* env, Ulang_type_atom atom) {
    Name temp = {0};
    unwrap(name_from_uname(env, &temp, atom.str));
    Str_view serialized = serialize_name(temp);

    String name = {0};
    string_extend_size_t(&a_main, &name, atom.pointer_depth);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_size_t(&a_main, &name, serialized.count);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, serialized);
    string_extend_cstr(&a_main, &name, "__");
    return string_to_strv(name);
}

Name serialize_ulang_type_fn(Str_view mod_path, Ulang_type_fn ulang_type) {
    String name = {0};
    extend_name(false, &name, serialize_ulang_type_tuple(mod_path, ulang_type.params));
    string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, *ulang_type.return_type)));
    return (Name) {.mod_path = mod_path, .base = string_to_strv(name)};
}

Name serialize_ulang_type_tuple(Str_view mod_path, Ulang_type_tuple ulang_type) {
    String name = {0};
    for (size_t idx = 0; idx < ulang_type.ulang_types.info.count; idx++) {
        Ulang_type curr = vec_at_const(ulang_type.ulang_types, idx);
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, curr)));
    }
    return (Name) {.mod_path = mod_path, .base = string_to_strv(name)};
}

Name serialize_ulang_type_regular(Env* env, Str_view mod_path, Ulang_type_regular ulang_type) {
    return (Name) {.mod_path = mod_path, serialize_ulang_type_atom(env, ulang_type.atom)};
}

Str_view serialize_ulang_type_vec(Str_view mod_path, Ulang_type_vec vec) {
    String name = {0};
    for (size_t idx = 0; idx < vec.info.count; idx++) {
        string_extend_strv(&a_main, &name, serialize_name(serialize_ulang_type(mod_path, vec_at(&vec, idx))));
    }
    return string_to_strv(name);
}

Name serialize_ulang_type(Env* env, Str_view mod_path, Ulang_type ulang_type) {
    Str_view name = ulang_type_print_internal(LANG_TYPE_MODE_LOG, ulang_type);
    switch (ulang_type.type) {
        case ULANG_TYPE_REGULAR:
            return serialize_ulang_type_regular(env, mod_path, ulang_type_regular_const_unwrap(ulang_type));
        case ULANG_TYPE_FN:
            return serialize_ulang_type_fn(env, mod_path, ulang_type_fn_const_unwrap(ulang_type));
        case ULANG_TYPE_TUPLE:
            todo();
    }
    assert(name.count > 1);
    return (Name) {.mod_path = mod_path, .base = str_view_slice(name, 0, name.count - 1)};
}

