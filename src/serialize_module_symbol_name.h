#ifndef SERIALIZE_MODULE_SYMBOL_NAME_H
#define SERIALIZE_MODULE_SYMBOL_NAME_H
// TODO: rename this header file

#include <util.h>
#include <parser_utils.h>

// TODO: remove this
bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

static inline Str_view serialize_name(Name name) {
    String name = {0};

    string_extend_size_t(&a_main, &name, mod_name.count);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, mod_name);
    string_extend_cstr(&a_main, &name, "_");
    string_extend_strv(&a_main, &name, base_name);

    return string_to_strv(name);
}

static inline Name deserialize_name(Str_view serialized) {
    size_t mod_len = 0;
    unwrap(try_str_view_consume_size_t(&mod_len, &serialized, false));
    unwrap(str_view_try_consume(&serialized, '_'));
    *mod_name = str_view_consume_count(&serialized, mod_len);

    size_t base_len = 0;
    unwrap(str_view_try_consume(&serialized, '_'));
    *base_name = serialized;
}

#endif // SERIALIZE_MODULE_SYMBOL_NAME_H
