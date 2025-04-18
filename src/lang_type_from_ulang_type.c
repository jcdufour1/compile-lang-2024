
#include <lang_type_from_ulang_type.h>
#include <extend_name.h>
#include <serialize_module_symbol_name.h>

bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Env* env, Ulang_type lang_type, Pos pos) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            if (!try_lang_type_from_ulang_type_regular(new_lang_type, env, ulang_type_regular_const_unwrap(lang_type), pos)) {
                return false;
            }
            return true;
        case ULANG_TYPE_TUPLE: {
            Lang_type_tuple new_tuple = {0};
            if (!try_lang_type_from_ulang_type_tuple(&new_tuple, env, ulang_type_tuple_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_tuple_const_wrap(new_tuple);
            return true;
        }
        case ULANG_TYPE_FN: {
            Lang_type_fn new_fn = {0};
            if (!try_lang_type_from_ulang_type_fn(&new_fn, env, ulang_type_fn_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_fn_const_wrap(new_fn);
            return true;
        }
    }
    unreachable("");
}

bool name_from_uname(Name* new_name, Uname name) {
}
