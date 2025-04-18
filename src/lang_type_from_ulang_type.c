
#include <lang_type_from_ulang_type.h>
#include <extend_name.h>
#include <serialize_module_symbol_name.h>

bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Ulang_type lang_type, Pos pos) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            if (!try_lang_type_from_ulang_type_regular(new_lang_type,  ulang_type_regular_const_unwrap(lang_type), pos)) {
                return false;
            }
            return true;
        case ULANG_TYPE_TUPLE: {
            Lang_type_tuple new_tuple = {0};
            if (!try_lang_type_from_ulang_type_tuple(&new_tuple,  ulang_type_tuple_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_tuple_const_wrap(new_tuple);
            return true;
        }
        case ULANG_TYPE_FN: {
            Lang_type_fn new_fn = {0};
            if (!try_lang_type_from_ulang_type_fn(&new_fn,  ulang_type_fn_const_unwrap(lang_type), pos)) {
                return false;
            }
            *new_lang_type = lang_type_fn_const_wrap(new_fn);
            return true;
        }
    }
    unreachable("");
}

bool name_from_uname(Name* new_name, Uname name) {
    (void) new_name;
    (void) name;
    todo();
}

Uname name_to_uname(Name name) {
    Uast_mod_alias* new_alias = uast_mod_alias_new((Pos) {0} /* TODO */, (Name) {.base = util_literal_name_new()}, (Name) {.base = name.mod_path});
    return (Uname) {.mod_alias = new_alias->name.base, .base = name.base, .gen_args = name.gen_args};
}

Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            return ulang_type_tuple_const_wrap(lang_type_tuple_to_ulang_type_tuple( lang_type_tuple_const_unwrap(lang_type)));
        case LANG_TYPE_VOID:
            todo();
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
        case LANG_TYPE_SUM:
            // fallthrough
            // TODO: change (Pos) {0} below to lang_type_get_pos(lang_type)
            return ulang_type_regular_const_wrap(ulang_type_regular_new(ulang_type_atom_new(name_to_uname(lang_type_get_str(lang_type)), lang_type_get_pointer_depth(lang_type)), (Pos) {0}));
        case LANG_TYPE_FN:
            todo();
    }
    unreachable("");
}

