#ifndef SERIALIZE_MODULE_SYMBOL_NAME_H
#define SERIALIZE_MODULE_SYMBOL_NAME_H
// TODO: rename this header file

#include <util.h>
#include <parser_utils.h>
#include <ulang_type_serialize.h>

// TODO: remove this
bool try_str_view_consume_size_t(size_t* result, Str_view* str_view, bool ignore_underscore);

static inline Str_view serialize_name(Env* env, Name name) {
    String buf = {0};

    if (name.mod_path.count > 0) {
        string_extend_cstr(&a_main, &buf, "_");
        string_extend_size_t(&a_main, &buf, name.mod_path.count);
        string_extend_cstr(&a_main, &buf, "_");
        string_extend_strv(&a_main, &buf, name.mod_path);
        string_extend_cstr(&a_main, &buf, "_");
    }

    string_extend_strv(&a_main, &buf, name.base);

    if (name.gen_args.info.count > 0) {
        string_extend_cstr(&a_main, &buf, "____");
        string_extend_size_t(&a_main, &buf, name.gen_args.info.count);
        string_extend_cstr(&a_main, &buf, "_");
        for (size_t idx = 0; idx < name.gen_args.info.count; idx++) {
            string_extend_strv(&a_main, &buf, serialize_name(env, serialize_ulang_type(env, name.mod_path, vec_at(&name.gen_args, idx))));
        }
    }

    log(LOG_DEBUG, TAST_FMT"\n", string_print(buf));
    return string_to_strv(buf);
}

static inline Name deserialize_name(Str_view serialized) {
    todo();
    unwrap(str_view_try_consume(&serialized, '_'));
    size_t mod_len = 0;
    unwrap(try_str_view_consume_size_t(&mod_len, &serialized, false));
    unwrap(str_view_try_consume(&serialized, '_'));
    Str_view mod_path = str_view_consume_count(&serialized, mod_len);

    unwrap(str_view_try_consume(&serialized, '_'));
    Str_view base_name = serialized;

    return (Name) {.mod_path = mod_path, .base = base_name};
}

// TODO: move this macro
static inline Str_view name_print_internal(Env* env, bool serialize, Name name) {
    if (serialize) {
        return serialize_name(env, name);
    }
        
    String buf = {0};
    extend_name(env, false, &buf, name);
    return string_to_strv(buf);
}

#endif // SERIALIZE_MODULE_SYMBOL_NAME_H
