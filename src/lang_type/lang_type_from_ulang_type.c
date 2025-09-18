
#include <lang_type_from_ulang_type.h>
#include <symbol_log.h>
#include <symbol_iter.h>
#include <expand_lang_def.h>
#include <parser_utils.h>

bool try_lang_type_from_ulang_type(Lang_type* new_lang_type, Ulang_type lang_type, Pos pos) {
    if (!expand_def_ulang_type(&lang_type, pos)) {
        return false;
    }

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
        case ULANG_TYPE_GEN_PARAM:
            unreachable("ulang_type_gen_param should not be converted from ulang_type to lang_type");
    }
    unreachable("");
}

bool name_from_uname(Name* new_name, Uname name) {
    if (name.mod_alias.base.count < 1) {
        Uast_def* result = NULL;
        if (usymbol_lookup(&result, name_new((Strv) {0} /* TODO */, name.base, name.gen_args, name.scope_id))) {
            *new_name = name_new((Strv) {0} /* TODO */, name.base, name.gen_args, name.scope_id);
            return true;
        }
        *new_name = name_new(name.mod_alias.mod_path, name.base, name.gen_args, name.scope_id /* TODO: either remove Uname.scope_id, or fix bugs with Uname->scope_id */);
        return true;
    }

    Uast_def* alias_ = NULL;
    if (!usymbol_lookup(&alias_, name_new(name.mod_alias.mod_path /* TODO */, name.mod_alias.base, (Ulang_type_vec) {0}, name.mod_alias.scope_id))) {
        usymbol_log(LOG_DEBUG, name.mod_alias.scope_id);
        // TODO: expected failure case
        todo();
    }

    switch (alias_->type) {
        case UAST_MOD_ALIAS: {
            Uast_mod_alias* alias = uast_mod_alias_unwrap(alias_);
            *new_name = name_new(alias->mod_path, name.base, name.gen_args, alias->mod_path_scope /* TODO */);
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
            return ulang_type_tuple_const_wrap(lang_type_tuple_to_ulang_type_tuple( lang_type_tuple_const_unwrap(lang_type)));
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
                ulang_type_atom_new(name_to_uname(lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)), lang_type_get_pointer_depth(lang_type)), lang_type_get_pos(lang_type)
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
    }
    unreachable("");
}

