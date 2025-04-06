
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
        case ULANG_TYPE_REG_GENERIC: {
            Ulang_type after_res = {0};
            if (!resolve_generics_ulang_type_generic(
                &after_res, env, ulang_type_generic_const_unwrap(lang_type)
            )) {
                return false;
            }
            log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, lang_type));
            log(LOG_DEBUG, TAST_FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, after_res));
            log(LOG_DEBUG, TAST_FMT"\n", name_print(ulang_type_regular_const_unwrap(after_res).atom.str));
            log(LOG_DEBUG, TAST_FMT"\n", str_view_print(ulang_type_regular_const_unwrap(after_res).atom.str.mod_path));
            log(LOG_DEBUG, TAST_FMT"\n", str_view_print(ulang_type_regular_const_unwrap(after_res).atom.str.base));
            return try_lang_type_from_ulang_type(new_lang_type, env, after_res, pos);
        }
    }
    unreachable("");
}
