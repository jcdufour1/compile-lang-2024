
#include <lang_type_from_ulang_type.h>
#include <symbol_log.h>
#include <symbol_iter.h>
#include <uast_expr_to_ulang_type.h>

static inline bool try_lang_type_from_ulang_type_expr(Lang_type* new_lang_type, Ulang_type_expr lang_type) {
    Ulang_type inner = {0};
    if (!uast_expr_to_ulang_type(&inner, lang_type.expr)) {
        return false;
    }
    return try_lang_type_from_ulang_type(new_lang_type, inner);
}

bool try_lang_type_from_ulang_type_lit(Lang_type* new_lang_type, Ulang_type_lit lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_INT_LIT: {
            Ulang_type_int_lit lit = ulang_type_int_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_lit_const_wrap(lang_type_int_lit_const_wrap(lang_type_int_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
            return true;
        }
        case ULANG_TYPE_STRING_LIT: {
            Ulang_type_string_lit lit = ulang_type_string_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_lit_const_wrap(lang_type_string_lit_const_wrap(
                lang_type_string_lit_new(lit.pos, lit.data, lit.pointer_depth)
            ));
            return true;
        }
        case ULANG_TYPE_STRUCT_LIT: {
            Ulang_type_struct_lit lit = ulang_type_struct_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_lit_const_wrap(lang_type_struct_lit_const_wrap(
                lang_type_struct_lit_new(lit.pos, lit.expr, lit.pointer_depth)
            ));
            return true;
        }
        case ULANG_TYPE_FN_LIT: {
            Ulang_type_fn_lit lit = ulang_type_fn_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_lit_const_wrap(lang_type_fn_lit_const_wrap(
                lang_type_fn_lit_new(lit.pos, lit.name, lit.pointer_depth)
            ));
            return true;
        }
        case ULANG_TYPE_FLOAT_LIT: {
            Ulang_type_float_lit lit = ulang_type_float_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_lit_const_wrap(lang_type_float_lit_const_wrap(lang_type_float_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
            return true;
        }
    }
    unreachable("");
}

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
        case ULANG_TYPE_REMOVED:
            msg(
                DIAG_TYPE_COULD_NOT_BE_INFERED,
                ulang_type_get_pos(lang_type),
                "could not infer the type of this variable definition\n"
            );
            return false;
        case ULANG_TYPE_ARRAY:
            return try_lang_type_from_ulang_type_array(new_lang_type, ulang_type_array_const_unwrap(lang_type));
        case ULANG_TYPE_EXPR:
            return try_lang_type_from_ulang_type_expr(new_lang_type, ulang_type_expr_const_unwrap(lang_type));
        case ULANG_TYPE_LIT:
            return try_lang_type_from_ulang_type_lit(new_lang_type, ulang_type_lit_const_unwrap(lang_type));
    }
    unreachable("");
}

// TODO: move this function
bool name_from_uname(Name* new_name, Uname name, Pos name_pos) {
    unwrap(name.mod_alias.base.count > 0);

    Uast_def* alias_ = NULL;
    Name alias_name = name_new(
        name.mod_alias.mod_path,
        name.mod_alias.base,
        (Ulang_type_darr) {0},
        name.mod_alias.scope_id
    );

    if (!usymbol_lookup(&alias_, alias_name)) {
        msg(
            DIAG_UNDEFINED_SYMBOL, name_pos, "module alias `"FMT"` is not defined\n",
            name_print(NAME_MSG, alias_name)
        );
        unwrap(usymbol_add(uast_poison_def_wrap(uast_poison_def_new(name_pos, alias_name))));
        return false;
    }

    switch (alias_->type) {
        case UAST_MOD_ALIAS: {
            Uast_mod_alias* alias = uast_mod_alias_unwrap(alias_);

            assert(env.is_printing || env.mod_path_curr_file.count > 0);
            if (strv_is_equal(env.mod_path_curr_file, alias->mod_path) && name.scope_id != SCOPE_NOT) {
                *new_name = name_new(alias->mod_path, name.base, name.gen_args, name.scope_id);
                assert(new_name->scope_id != SCOPE_NOT);
                return true;
            }
            *new_name = name_new(alias->mod_path, name.base, name.gen_args, alias->mod_path_scope);
            assert(new_name->scope_id != SCOPE_NOT);
            return true;
        }
        case UAST_POISON_DEF:
            return false;
        case UAST_IMPORT_PATH:
            fallthrough;
        case UAST_STRUCT_DEF:
            fallthrough;
        case UAST_RAW_UNION_DEF:
            fallthrough;
        case UAST_ENUM_DEF:
            fallthrough;
        case UAST_PRIMITIVE_DEF:
            fallthrough;
        case UAST_FUNCTION_DEF:
            fallthrough;
        case UAST_FUNCTION_DECL:
            fallthrough;
        case UAST_VARIABLE_DEF:
            fallthrough;
        case UAST_GENERIC_PARAM:
            fallthrough;
        case UAST_LANG_DEF:
            fallthrough;
        case UAST_VOID_DEF:
            fallthrough;
        case UAST_LABEL:
            fallthrough;
        case UAST_BUILTIN_DEF:
            msg_todo("error message for this situation", uast_def_get_pos(alias_));
            return false;
    }
    unreachable("");
}

Ulang_type lang_type_lit_to_ulang_type(Lang_type_lit lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_INT_LIT: {
            Lang_type_int_lit lit = lang_type_int_lit_const_unwrap(lang_type);
            return ulang_type_lit_const_wrap(ulang_type_int_lit_const_wrap(ulang_type_int_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_FLOAT_LIT: {
            Lang_type_float_lit lit = lang_type_float_lit_const_unwrap(lang_type);
            return ulang_type_lit_const_wrap(ulang_type_float_lit_const_wrap(ulang_type_float_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_STRING_LIT: {
            Lang_type_string_lit lit = lang_type_string_lit_const_unwrap(lang_type);
            return ulang_type_lit_const_wrap(ulang_type_string_lit_const_wrap(ulang_type_string_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_STRUCT_LIT: {
            Lang_type_struct_lit lit = lang_type_struct_lit_const_unwrap(lang_type);
            return ulang_type_lit_const_wrap(ulang_type_struct_lit_const_wrap(ulang_type_struct_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_FN_LIT: {
            Lang_type_fn_lit lit = lang_type_fn_lit_const_unwrap(lang_type);
            return ulang_type_lit_const_wrap(ulang_type_fn_lit_const_wrap(ulang_type_fn_lit_new(
                lit.pos,
                lit.name,
                lit.pointer_depth
            )));
        }
    }
    unreachable("");
    
}

Ulang_type lang_type_to_ulang_type(Lang_type lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            return ulang_type_tuple_const_wrap(lang_type_tuple_to_ulang_type_tuple(lang_type_tuple_const_unwrap(lang_type)));
        case LANG_TYPE_VOID:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(
                lang_type_void_const_unwrap(lang_type).pos,
                uname_new(MOD_ALIAS_BUILTIN, sv("void"), (Ulang_type_darr) {0}, SCOPE_TOP_LEVEL),
                0
            ));
        case LANG_TYPE_PRIMITIVE:
            fallthrough;
        case LANG_TYPE_STRUCT:
            fallthrough;
        case LANG_TYPE_RAW_UNION:
            fallthrough;
        case LANG_TYPE_ENUM: {
            Name name = {0};
            if (!lang_type_get_name(&name, lang_type)) {
                msg_todo("", lang_type_get_pos(lang_type));
                return ulang_type_new_void(lang_type_get_pos(lang_type));
            }
            return ulang_type_regular_const_wrap(ulang_type_regular_new(
                lang_type_get_pos(lang_type),
                name_to_uname(name),
                lang_type_get_pointer_depth(lang_type)
            ));
        }
        case LANG_TYPE_FN: {
            Lang_type_fn fn = lang_type_fn_const_unwrap(lang_type);
            Ulang_type* new_rtn_type = arena_alloc(&a_main, sizeof(*new_rtn_type));
            *new_rtn_type = lang_type_to_ulang_type(*fn.return_type);
            return ulang_type_fn_const_wrap(ulang_type_fn_new(
                fn.pos,
                lang_type_tuple_to_ulang_type_tuple(fn.params),
                new_rtn_type,
                1/*TODO*/
            ));
        }
        case LANG_TYPE_ARRAY: {
            Lang_type_array array = lang_type_array_const_unwrap(lang_type);
            Ulang_type new_item_type = lang_type_to_ulang_type(*array.item_type);
            return ulang_type_array_const_wrap(ulang_type_array_new(
                array.pos,
                arena_dup(&a_main, &new_item_type),
                uast_literal_wrap(uast_int_wrap(uast_int_new(array.pos, array.count))),
                array.pointer_depth
            ));
        }
        case LANG_TYPE_LIT: {
            return lang_type_lit_to_ulang_type(lang_type_lit_const_unwrap(lang_type));
        }
        case LANG_TYPE_REMOVED:
            unreachable("");
    }
    unreachable("");
}

bool try_lang_type_from_ulang_type_array(Lang_type* new_lang_type, Ulang_type_array lang_type) {
    Lang_type item_type = {0};
    if (!try_lang_type_from_ulang_type(&item_type, *lang_type.item_type)) {
        return false;
    }
    
    if (lang_type.count->type == UAST_UNDERSCORE) {
        if (!env.silent_generic_resol_errors) {
            msg(
                DIAG_STATIC_ARRAY_COUNT_NOT_INFERED,
                uast_expr_get_pos(lang_type.count),
                "could not infer count of elements in the static array\n"
            );
        }
        return false;
    }

    if (!try_lang_type_from_ulang_type_array_is_unsigned_int(lang_type.count)) {
        if (!env.silent_generic_resol_errors) {
            msg(
                DIAG_NON_INT_LITERAL_IN_STATIC_ARRAY,
                uast_expr_get_pos(lang_type.count),
                "count of statically sized array must be an integer literal\n"
            );
        }
        return false;
    }

    int64_t count = uast_int_unwrap(uast_literal_unwrap(lang_type.count))->data;

    *new_lang_type = lang_type_array_const_wrap(lang_type_array_new(
        lang_type.pos,
        arena_dup(&a_main, &item_type),
        count,
        0
    ));

    // TODO: making new array def every time is wasteful. only make array def when nessessary
    symbol_add(tast_array_def_wrap(tast_array_def_new(lang_type.pos, item_type, count)));
    return true;
}

