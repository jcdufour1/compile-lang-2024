
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

bool try_lang_type_from_ulang_type_const_expr(Lang_type* new_lang_type, Ulang_type_const_expr lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_INT: {
            Ulang_type_int lit = ulang_type_int_const_unwrap(lang_type);

            *new_lang_type = lang_type_const_expr_const_wrap(lang_type_int_const_wrap(lang_type_int_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
            return true;
        }
        case ULANG_TYPE_STRING_LIT: {
            Ulang_type_string_lit lit = ulang_type_string_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_const_expr_const_wrap(lang_type_string_lit_const_wrap(
                lang_type_string_lit_new(lit.pos, lit.data, lit.pointer_depth)
            ));
            return true;
        }
        case ULANG_TYPE_STRUCT_LIT: {
            Ulang_type_struct_lit lit = ulang_type_struct_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_const_expr_const_wrap(lang_type_struct_lit_const_wrap(
                lang_type_struct_lit_new(
                    lit.pos,
                    lit.expr,
                    lit.pointer_depth
                )
            ));
            return true;
        }
        case ULANG_TYPE_FN_LIT: {
            Ulang_type_fn_lit lit = ulang_type_fn_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_const_expr_const_wrap(lang_type_fn_lit_const_wrap(
                lang_type_fn_lit_new(lit.pos, lit.name, lit.pointer_depth)
            ));
            return true;
        }
        case ULANG_TYPE_FLOAT_LIT: {
            Ulang_type_float_lit lit = ulang_type_float_lit_const_unwrap(lang_type);

            *new_lang_type = lang_type_const_expr_const_wrap(lang_type_float_lit_const_wrap(lang_type_float_lit_new(
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
        case ULANG_TYPE_CONST_EXPR:
            return try_lang_type_from_ulang_type_const_expr(new_lang_type, ulang_type_const_expr_const_unwrap(lang_type));
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
        (Ulang_type_vec) {0},
        name.mod_alias.scope_id,
        (Attrs) {0}
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
            *new_name = name_new(alias->mod_path, name.base, name.gen_args, alias->mod_path_scope, (Attrs) {0});
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

Ulang_type lang_type_const_expr_to_ulang_type(Lang_type_const_expr lang_type) {
    switch (lang_type.type) {
        case LANG_TYPE_INT: {
            // TODO: rename LANG_TYPE_INT to LANG_TYPE_INT_LIT
            Lang_type_int lit = lang_type_int_const_unwrap(lang_type);
            return ulang_type_const_expr_const_wrap(ulang_type_int_const_wrap(ulang_type_int_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_FLOAT_LIT: {
            Lang_type_float_lit lit = lang_type_float_lit_const_unwrap(lang_type);
            return ulang_type_const_expr_const_wrap(ulang_type_float_lit_const_wrap(ulang_type_float_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_STRING_LIT: {
            Lang_type_string_lit lit = lang_type_string_lit_const_unwrap(lang_type);
            return ulang_type_const_expr_const_wrap(ulang_type_string_lit_const_wrap(ulang_type_string_lit_new(
                lit.pos,
                lit.data,
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_STRUCT_LIT: {
            Lang_type_struct_lit lit = lang_type_struct_lit_const_unwrap(lang_type);
            return ulang_type_const_expr_const_wrap(ulang_type_struct_lit_const_wrap(ulang_type_struct_lit_new(
                lit.pos,
                lit.expr, // TODO: change Lang_type_struct_lit.lit to Lang_type_struct_lit.data
                lit.pointer_depth
            )));
        }
        case LANG_TYPE_FN_LIT: {
            Lang_type_fn_lit lit = lang_type_fn_lit_const_unwrap(lang_type);
            return ulang_type_const_expr_const_wrap(ulang_type_fn_lit_const_wrap(ulang_type_fn_lit_new(
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
                POS_BUILTIN,
                ulang_type_atom_new_from_cstr("void", 0)
            ));
        case LANG_TYPE_PRIMITIVE:
            fallthrough;
        case LANG_TYPE_STRUCT:
            fallthrough;
        case LANG_TYPE_RAW_UNION:
            fallthrough;
        case LANG_TYPE_ENUM:
            return ulang_type_regular_const_wrap(ulang_type_regular_new(
                lang_type_get_pos(lang_type),
                ulang_type_atom_new(
                    name_to_uname(lang_type_get_str(LANG_TYPE_MODE_LOG, lang_type)),
                    lang_type_get_pointer_depth(lang_type)
                )
            ));
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
        case LANG_TYPE_CONST_EXPR: {
            return lang_type_const_expr_to_ulang_type(lang_type_const_expr_const_unwrap(lang_type));
        }
        case LANG_TYPE_REMOVED:
            unreachable("");
    }
    unreachable("");
}
