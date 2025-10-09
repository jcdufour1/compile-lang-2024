
#include <lang_type_from_ulang_type.h>
#include <symbol_log.h>
#include <symbol_iter.h>
#include <expand_lang_def.h>

bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR:
            return try_lang_type_from_ulang_type_regular(new_lang_type, ulang_type_regular_const_unwrap(lang_type));
        case ULANG_TYPE_TUPLE: {
            Lang_type_tuple new_tuple = {0};
            if (!try_lang_type_from_ulang_type_tuple(&new_tuple, ulang_type_tuple_const_unwrap(lang_type))) {
                return false;
            }
            *new_lang_type = lang_type_tuple_const_wrap(new_tuple);
            return true;
        }
        case ULANG_TYPE_FN: {
            Lang_type_fn new_fn = {0};
            if (!try_lang_type_from_ulang_type_fn(&new_fn, ulang_type_fn_const_unwrap(lang_type))) {
                return false;
            }
            *new_lang_type = lang_type_fn_const_wrap(new_fn);
            return true;
        }
        case ULANG_TYPE_GEN_PARAM:
            // TODO: print error?
            return false;
        case ULANG_TYPE_ARRAY:
            return try_lang_type_from_ulang_type_array(new_lang_type, ulang_type_array_const_unwrap(lang_type));
    }
    unreachable("");
}

// TODO: move this function
bool name_from_uname(Name* new_name, Uname name, Pos name_pos) {
    assert(name.mod_alias.base.count > 0);

    Uast_def* alias_ = NULL;
    Name temp_name = name_new(
        name.mod_alias.mod_path,
        name.mod_alias.base,
        (Ulang_type_vec) {0},
        name.mod_alias.scope_id
    );
    if (!usymbol_lookup(&alias_, temp_name)) {
        msg(
            DIAG_UNDEFINED_SYMBOL, name_pos, "module alias `"FMT"` is not defined\n",
            name_print(NAME_MSG, temp_name)
        );
        unwrap(usymbol_add(uast_poison_def_wrap(uast_poison_def_new(name_pos, temp_name))));
        return false;
    }

    switch (alias_->type) {
        case UAST_MOD_ALIAS: {
            Uast_mod_alias* alias = uast_mod_alias_unwrap(alias_);
            *new_name = name_new(alias->mod_path, name.base, name.gen_args, alias->mod_path_scope);
            return true;
        }
        case UAST_IMPORT_PATH:
            todo();
        case UAST_STRUCT_DEF:
            todo();
        case UAST_RAW_UNION_DEF:
            todo();
        case UAST_ENUM_DEF:
            todo();
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            todo();
        case UAST_VARIABLE_DEF:
            log(LOG_DEBUG, FMT"\n", uast_def_print(alias_));
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_POISON_DEF:
            return false;
        case UAST_LANG_DEF:
            todo();
        case UAST_VOID_DEF:
            todo();
        case UAST_LABEL:
            todo();
    }
    unreachable("");
}

Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            return ulang_type_tuple_const_wrap(lang_type_tuple_to_ulang_type_tuple(lang_type_tuple_const_unwrap(lang_type)));
        case LANG_TYPE_VOID:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(
                ulang_type_atom_new_from_cstr("void", 0),
                (Pos) {0}
            ));
        case LANG_TYPE_PRIMITIVE:
            // fallthrough
        case LANG_TYPE_STRUCT:
            // fallthrough
        case LANG_TYPE_RAW_UNION:
            // fallthrough
        case LANG_TYPE_ENUM:
            // fallthrough
            return ulang_type_regular_const_wrap(ulang_type_regular_new(
                ulang_type_atom_new(
                    name_to_uname(lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)),
                    lang_type_get_pointer_depth(lang_type)
                ),
                lang_type_get_pos(lang_type)
            ));
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            Ulang_type* new_rtn_type = arena_alloc(&a_main, sizeof(*new_rtn_type));
            *new_rtn_type = lang_type_to_ulang_type(*fn.return_type);
            return ulang_type_fn_const_wrap(ulang_type_fn_new(
                lang_type_tuple_to_ulang_type_tuple(fn.params),
                new_rtn_type,
                fn.pos
            ));
        }
        case LANG_TYPE_ARRAY:
            todo();
    }
    unreachable("");
}

