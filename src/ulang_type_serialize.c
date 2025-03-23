#include <ulang_type_serialize.h>
#include <parser_utils.h>
#include <ulang_type.h>

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

Str_view serialize_ulang_type_fn(Ulang_type_fn ulang_type) {
    String name = {0};
    string_extend_strv(&a_main, &name, serialize_ulang_type_tuple(ulang_type.params));
    string_extend_strv(&a_main, &name, serialize_ulang_type(*ulang_type.return_type));
    return string_to_strv(name);
}

Str_view serialize_ulang_type_tuple(Ulang_type_tuple ulang_type) {
    String name = {0};
    for (size_t idx = 0; idx < ulang_type.ulang_types.info.count; idx++) {
        Ulang_type curr = vec_at_const(ulang_type.ulang_types, idx);
        string_extend_strv(&a_main, &name, serialize_ulang_type(curr));
    }
    return string_to_strv(name);
}

Str_view serialize_ulang_type_regular(Ulang_type_regular ulang_type) {
    return serialize_ulang_type_atom(ulang_type.atom);
}

Str_view serialize_ulang_type(Ulang_type ulang_type) {
    Str_view name = ulang_type_print_internal(LANG_TYPE_MODE_LOG, ulang_type);
    switch (ulang_type.type) {
        case ULANG_TYPE_REGULAR:
            return serialize_ulang_type_regular(ulang_type_regular_const_unwrap(ulang_type));
        case ULANG_TYPE_REG_GENERIC:
            todo();
        case ULANG_TYPE_FN:
            return serialize_ulang_type_fn(ulang_type_fn_const_unwrap(ulang_type));
        case ULANG_TYPE_TUPLE:
            todo();
    }
    assert(name.count > 1);
    return str_view_slice(name, 0, name.count - 1);
}

Ulang_type deserialize_ulang_type(Str_view serialized) {
    size_t pointer_depth = 0;
    unwrap(try_str_view_consume_size_t(&pointer_depth, &serialized, false));
    unwrap(str_view_try_consume(&serialized, '_'));

    size_t len_base = 0;
    unwrap(try_str_view_consume_size_t(&len_base, &serialized, false));
    unwrap(str_view_try_consume(&serialized, '_'));

    log(LOG_DEBUG, TAST_FMT"\n", str_view_print(serialized));
    Str_view base = str_view_consume_count(&serialized, len_base);
    log(LOG_DEBUG, TAST_FMT"\n", str_view_print(serialized));

    unwrap(str_view_try_consume_count(&serialized, '_', 2));
    if (serialized.count > 0) {
        todo();
    }

    return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(base, pointer_depth), POS_BUILTIN /* TODO */));
}

