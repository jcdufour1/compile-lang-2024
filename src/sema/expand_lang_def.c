#include <symbol_iter.h>
#include <uast.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <ulang_type_clone.h>
#include <uast_clone.h>
#include <type_checking.h>
#include <msg_todo.h>
#include <lang_type_from_ulang_type.h>
#include <ast_msg.h>
#include <pos_util.h>
#include <ulang_type_is_equal.h>

// TODO: consider if def definition has pointer_depth > 0

typedef enum {
    EXPAND_NAME_ERROR,
    EXPAND_NAME_NORMAL, // only changes name
    EXPAND_NAME_NEW_EXPR, // new_expr must be substituted where the name was
    EXPAND_NAME_NEW_ULANG_TYPE, // new_expr must be substituted where the name was
} EXPAND_NAME_STATUS;

typedef enum {
    EXPAND_EXPR_ERROR,
    EXPAND_EXPR_NEW_EXPR, // new_expr must be substituted where the name was
    EXPAND_EXPR_NEW_ULANG_TYPE, // new_expr must be substituted where the name was
} EXPAND_EXPR_STATUS;

static bool expand_def_block(Uast_block* block);

static bool expand_def_def(Uast_def* def, bool is_rhs, Uast_expr* rhs);

static bool expand_def_block(Uast_block* block);

static EXPAND_NAME_STATUS expand_def_uname(Ulang_type* new_lang_type, Uast_expr** new_expr, Uname* name, Pos pos, Pos dest_pos);

static bool expand_def_expr_vec(Uast_expr_vec* exprs);

static bool expand_def_expr_not_ulang_type(Uast_expr** new_expr, Uast_expr* expr, bool is_rhs, Uast_expr* rhs);

static EXPAND_EXPR_STATUS expand_def_expr(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Uast_expr* expr,
    bool is_rhs,
    Uast_expr* rhs
);

static bool expand_def_generic_param_vec(Uast_generic_param_vec* params);

static bool expand_def_variable_def_vec(Uast_variable_def_vec* defs);

static EXPAND_NAME_STATUS expand_def_symbol(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Uast_symbol* sym,
    bool is_rhs,
    Uast_expr* rhs
);

static bool expand_def_ulang_type(Ulang_type* lang_type, Pos dest_pos, bool is_rhs, Uast_expr* rhs);

static bool expand_def_ulang_type_regular(
    Ulang_type* new_lang_type,
    Ulang_type_regular lang_type,
    Pos dest_pos
);

static bool expand_def_function_def(Uast_function_def* def);

static bool expand_def_switch(Uast_switch* lang_switch);

static bool expand_def_stmt(Uast_stmt** new_stmt, Uast_stmt* stmt);

static bool expand_def_if_else_chain(Uast_if_else_chain* if_else);

static bool expand_def_operator(Uast_operator* oper);

static bool expand_def_defer(Uast_defer* lang_defer);

static EXPAND_NAME_STATUS expand_def_name(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Name* name,
    Pos dest_pos
);

static bool must_expand_again = false;

static bool expand_def_ulang_type_regular(
    Ulang_type* new_lang_type,
    Ulang_type_regular lang_type,
    Pos dest_pos
) {
    Uast_expr* new_expr = NULL;
    Ulang_type result = {0};
    switch (expand_def_uname(&result, &new_expr, &lang_type.atom.str, lang_type.pos, dest_pos)) {
        case EXPAND_NAME_ERROR:
            return false;
        case EXPAND_NAME_NORMAL:
            *new_lang_type = ulang_type_regular_const_wrap(lang_type);
            return true;
        case EXPAND_NAME_NEW_EXPR:
            break;
        case EXPAND_NAME_NEW_ULANG_TYPE:
            if (result.type != ULANG_TYPE_REGULAR) {
                msg_todo("", ulang_type_get_pos(result));
                return false;
            }
            return expand_def_ulang_type_regular(
                new_lang_type,
                ulang_type_regular_const_unwrap(result),
                dest_pos
            );
        default:
            unreachable("");
    }

    switch (new_expr->type) {
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(new_expr);
            if (access->callee->type != UAST_SYMBOL) {
                msg_todo("", uast_expr_get_pos(access->callee));
                return false;
            }

            Uname new_uname = uname_new(
                uast_symbol_unwrap(access->callee)->name,
                access->member_name->name.base,
                lang_type.atom.str.gen_args,
                SCOPE_TOP_LEVEL/*TODO*/
            );
            *new_lang_type = ulang_type_regular_const_wrap(ulang_type_regular_new(
                ulang_type_atom_new(new_uname, lang_type.atom.pointer_depth),
                lang_type.pos
            ));
            return true;
        }
        case UAST_SYMBOL:
            lang_type.atom.str = name_to_uname(uast_symbol_unwrap(new_expr)->name);
            lang_type.pos.expanded_from = uast_symbol_unwrap(new_expr)->pos.expanded_from;
            *new_lang_type = ulang_type_regular_const_wrap(lang_type);
            return true;
        case UAST_INDEX: {
            Uast_index* index = uast_index_unwrap(new_expr);
            if (index->index->type != UAST_EXPR_REMOVED) {
                msg_todo("", index->pos);
                return false;
            }

            Ulang_type index_ulang_type = {0};
            if (index->callee->type != UAST_SYMBOL) {
                msg_todo("", uast_expr_get_pos(index->callee));
                return false;
            }
            if (!expand_def_ulang_type_regular(
                &index_ulang_type,
                ulang_type_regular_new(
                    ulang_type_atom_new(name_to_uname(uast_symbol_unwrap(index->callee)->name), 0),
                    uast_expr_get_pos(index->callee)
                ),
                uast_expr_get_pos(index->callee)
            )) {
                return false;
            }

            *new_lang_type = ulang_type_new_slice(
                index->pos,
                index_ulang_type,
                lang_type.atom.pointer_depth
            );
            return true;
        }
        case UAST_FN:
            *new_lang_type = ulang_type_fn_const_wrap(uast_fn_unwrap(new_expr)->ulang_type);
            return true;
        case UAST_IF_ELSE_CHAIN:
            fallthrough;
        case UAST_BLOCK:
            fallthrough;
        case UAST_SWITCH:
            fallthrough;
        case UAST_UNKNOWN:
            fallthrough;
        case UAST_OPERATOR:
            fallthrough;
        case UAST_LITERAL:
            fallthrough;
        case UAST_FUNCTION_CALL:
            fallthrough;
        case UAST_STRUCT_LITERAL:
            fallthrough;
        case UAST_ARRAY_LITERAL:
            fallthrough;
        case UAST_TUPLE:
            fallthrough;
        case UAST_MACRO:
            fallthrough;
        case UAST_ENUM_ACCESS:
            fallthrough;
        case UAST_ENUM_GET_TAG:
            fallthrough;
        case UAST_ORELSE:
            fallthrough;
        case UAST_QUESTION_MARK:
            fallthrough;
        case UAST_UNDERSCORE:
            fallthrough;
        case UAST_EXPR_REMOVED:
            msg_todo("", uast_expr_get_pos(new_expr));
            return false;
    }
    unreachable("");
}

static bool expand_def_ulang_type_array(
    Ulang_type_array* new_lang_type,
    Ulang_type_array lang_type,
    Pos dest_pos,
    bool is_rhs,
    Uast_expr* rhs
) {
    Uast_expr* item_type_rhs = rhs;
    if (is_rhs) {
        item_type_rhs = rhs/*TODO*/;
        log(LOG_DEBUG, FMT"\n", ulang_type_print(LANG_TYPE_MODE_LOG, *lang_type.item_type));
        log(LOG_DEBUG, FMT"\n", uast_expr_print(rhs));
        //todo();
    }
    if (!expand_def_ulang_type(lang_type.item_type, dest_pos, is_rhs, item_type_rhs)) {
        return false;
    }

    Uast_expr* count_rhs = rhs;
    if (is_rhs) {
        if (rhs->type != UAST_ARRAY_LITERAL) {
            msg_todo("", dest_pos);
            return false;
        }
        Uast_array_literal* rhs_arr = uast_array_literal_unwrap(rhs);
        Uast_int* new_count = uast_int_new(rhs_arr->pos, (int64_t)rhs_arr->members.info.count);
        count_rhs = uast_literal_wrap(uast_int_wrap(new_count));
    }
    
    Ulang_type dummy = {0};
    switch (expand_def_expr(&dummy, &lang_type.count, lang_type.count, is_rhs, count_rhs)) {
        case EXPAND_EXPR_NEW_EXPR:
            *new_lang_type = lang_type;
            return true;
        case EXPAND_EXPR_NEW_ULANG_TYPE:
            msg_got_type_but_expected_expr(uast_expr_get_pos(lang_type.count));
            return false;
        case EXPAND_EXPR_ERROR:
            return false;
    }
    unreachable("");
}

static bool expand_def_ulang_type_fn(
    Ulang_type_fn* new_lang_type,
    Ulang_type_fn lang_type,
    Pos dest_pos,
    bool is_rhs,
    Uast_expr* rhs
) {
    bool status = true;

    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        Uast_expr* gen_rhs = rhs;
        if (is_rhs) {
            todo();
        }
        if (!expand_def_ulang_type(vec_at_ref(&lang_type.params.ulang_types, idx), dest_pos, is_rhs, gen_rhs)) {
            status = false;
        }
    }

    Uast_expr* rtn_rhs = rhs;
    if (is_rhs) {
        todo();
    }
    if (!expand_def_ulang_type(lang_type.return_type, dest_pos, is_rhs, rtn_rhs)) {
        status = false;
    }

    *new_lang_type = lang_type;
    return status;
}

static bool expand_def_ulang_type_tuple(
    Ulang_type_tuple* new_lang_type,
    Ulang_type_tuple lang_type,
    Pos dest_pos
) {
    bool status = true;

    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        todo();
        //if (!expand_def_ulang_type(vec_at_ref(&lang_type.ulang_types, idx), dest_pos)) {
        //    status = false;
        //}
    }

    *new_lang_type = lang_type;
    return status;
}

static bool expand_def_ulang_type_expr(
    Ulang_type* new_lang_type,
    Ulang_type_expr lang_type,
    bool is_rhs,
    Uast_expr* rhs
) {
    switch (expand_def_expr(new_lang_type, &lang_type.expr, lang_type.expr, is_rhs, rhs)) {
        case EXPAND_EXPR_ERROR:
            return false;
        case EXPAND_EXPR_NEW_EXPR:
            *new_lang_type = ulang_type_expr_const_wrap(lang_type);
            return true;
        case EXPAND_EXPR_NEW_ULANG_TYPE:
            return true;
    }
    unreachable("");
}

static bool expand_def_ulang_type_int(
    Ulang_type_int* new_lang_type,
    Ulang_type_int lang_type
) {
    *new_lang_type = lang_type;
    return true;
}

static bool expand_def_ulang_type(Ulang_type* lang_type, Pos dest_pos, bool is_rhs, Uast_expr* rhs) {
    switch (lang_type->type) {
        case ULANG_TYPE_REGULAR: {
            return expand_def_ulang_type_regular(
                lang_type,
                ulang_type_regular_const_unwrap(*lang_type),
                dest_pos
            );
        }
        case ULANG_TYPE_ARRAY: {
            Ulang_type_array new_lang_type = {0};
            if (!expand_def_ulang_type_array(&new_lang_type, ulang_type_array_const_unwrap(*lang_type), dest_pos, is_rhs, rhs)) {
                return false;
            }
            *lang_type = ulang_type_array_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_FN: {
            Ulang_type_fn new_lang_type = {0};
            if (!expand_def_ulang_type_fn(&new_lang_type, ulang_type_fn_const_unwrap(*lang_type), dest_pos, is_rhs, rhs)) {
                return false;
            }
            *lang_type = ulang_type_fn_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_TUPLE: {
            Ulang_type_tuple new_lang_type = {0};
            if (!expand_def_ulang_type_tuple(&new_lang_type, ulang_type_tuple_const_unwrap(*lang_type), dest_pos)) {
                return false;
            }
            *lang_type = ulang_type_tuple_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_REMOVED:
            return true;
        case ULANG_TYPE_EXPR: {
            Ulang_type new_lang_type = {0};
            if (!expand_def_ulang_type_expr(&new_lang_type, ulang_type_expr_const_unwrap(*lang_type), is_rhs, rhs)) {
                return false;
            }
            *lang_type = new_lang_type;
            return true;
        }
        case ULANG_TYPE_INT: {
            Ulang_type_int new_lang_type = {0};
            if (!expand_def_ulang_type_int(&new_lang_type, ulang_type_int_const_unwrap(*lang_type))) {
                return false;
            }
            *lang_type = ulang_type_int_const_wrap(new_lang_type);
            return true;
        }
    }
    unreachable("");
}

static void ulang_type_tuple_append_pos(Ulang_type_tuple* new_tuple, Ulang_type_tuple tuple, Pos new_pos) {
    Pos* new_pos_ptr = &new_pos;
    vec_foreach_ref(idx, Ulang_type, curr, tuple.ulang_types) {
        pos_expanded_from_append(ulang_type_get_pos_ref(curr), arena_dup(&a_main, new_pos_ptr));
    }
    *new_tuple = tuple;
}

static void ulang_type_fn_append_pos(Ulang_type_fn* new_fn, Ulang_type_fn fn, Pos new_pos) {
    ulang_type_tuple_append_pos(&fn.params, fn.params, new_pos);
    Pos* new_pos_ptr = &new_pos;
    pos_expanded_from_append(ulang_type_get_pos_ref(fn.return_type), arena_dup(&a_main, new_pos_ptr));
    *new_fn = fn;
}

static EXPAND_NAME_STATUS expand_def_name_internal(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Name* new_name,
    Name name,
    Pos dest_pos,
    bool always_new_expr,
    bool is_expanded_from,
    Pos* expanded_from
) {
    Uast_def* def = NULL;
    *new_name = name;
    memset(&new_name->gen_args, 0, sizeof(new_name->gen_args));
    if (!usymbol_lookup(&def, *new_name)) {
        new_name->gen_args = name.gen_args;
        return EXPAND_NAME_NORMAL;
    }

    new_name->gen_args = name.gen_args;
    bool gen_arg_status = true;
    for (size_t idx = 0; idx < new_name->gen_args.info.count; idx++) {
        Ulang_type* curr = vec_at_ref(&new_name->gen_args, idx);
        Pos curr_pos = ulang_type_get_pos(*curr);
        gen_arg_status = expand_def_ulang_type(curr, curr_pos, false/*TODO*/, NULL) && gen_arg_status;
    }
    if (!gen_arg_status) {
        return EXPAND_NAME_ERROR;
    }

    switch (def->type) {
        case UAST_POISON_DEF:
            return EXPAND_NAME_ERROR;
        case UAST_LANG_DEF:
            break;
        case UAST_IMPORT_PATH:
            return EXPAND_NAME_NORMAL;
        case UAST_MOD_ALIAS:
            fallthrough;
        case UAST_GENERIC_PARAM:
            fallthrough;
        case UAST_FUNCTION_DEF:
            fallthrough;
        case UAST_VARIABLE_DEF:
            fallthrough;
        case UAST_STRUCT_DEF:
            fallthrough;
        case UAST_RAW_UNION_DEF:
            fallthrough;
        case UAST_ENUM_DEF:
            fallthrough;
        case UAST_PRIMITIVE_DEF:
            fallthrough;
        case UAST_FUNCTION_DECL:
            fallthrough;
        case UAST_VOID_DEF:
            if (always_new_expr) {
                if (is_expanded_from) {
                    dest_pos.expanded_from = expanded_from;
                }
                *new_expr = uast_symbol_wrap(uast_symbol_new(dest_pos /* TODO */, name));
                return EXPAND_NAME_NEW_EXPR;
            }
            return EXPAND_NAME_NORMAL;
        case UAST_LABEL:
            msg_todo("", dest_pos);
            return EXPAND_NAME_ERROR;
        case UAST_BUILTIN_DEF: {
            static_assert(BUILTIN_DEFS_COUNT == 4, "exhausive handling of builtin defs");
            if (strv_is_equal(name.base, sv("usize"))) {
                *new_lang_type = ulang_type_new_usize();
                return EXPAND_NAME_NEW_ULANG_TYPE;
            } else if (strv_is_equal(name.base, sv("buf_at"))) {
                return EXPAND_NAME_NORMAL;
            } else if (strv_is_equal(name.base, sv("static_array_access"))) {
                return EXPAND_NAME_NORMAL;
            } else if (strv_is_equal(name.base, sv("static_array_slice"))) {
                return EXPAND_NAME_NORMAL;
            }
            unreachable("");
        }
    }

    Uast_expr* expr = uast_expr_clone(uast_lang_def_unwrap(def)->expr, true, name.scope_id, dest_pos);
    if (is_expanded_from) {
        pos_expanded_from_append(uast_expr_get_pos_ref(expr), expanded_from);
    }
    if (pos_is_recursion(uast_expr_get_pos(expr))) {
        msg(DIAG_DEF_RECURSION, *uast_expr_get_pos_ref(expr), "def recursion detected\n");
        return EXPAND_NAME_ERROR;
    }

    switch (expr->type) {
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(expr);
            if (access->member_name->name.gen_args.info.count > 0) {
                if (name.gen_args.info.count > 0) {
                    Pos temp_pos = uast_expr_get_pos(expr);
                    *uast_expr_get_pos_ref(expr) = dest_pos;
                    pos_expanded_from_append(uast_expr_get_pos_ref(expr), arena_dup(&a_main, &temp_pos));
                    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name));
                    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, *new_name));
                    msg(
                        DIAG_DEF_DEST_AND_SRC_BOTH_HAVE_GEN_ARGS,
                        uast_expr_get_pos(expr), 
                        "def destination and source both have generic arguments (only one of destination or source is allowed generic arguments)\n"
                    );
                    return EXPAND_NAME_ERROR;
                }
            }


            Uast_def* result = NULL;
            if (access->callee->type == UAST_SYMBOL) {
                if (!usymbol_lookup(&result, uast_symbol_unwrap(access->callee)->name)) {
                    msg_undefined_symbol(uast_symbol_unwrap(access->callee)->name, uast_symbol_unwrap(access->callee)->pos);
                    return EXPAND_NAME_ERROR;
                }
                if (result->type == UAST_MOD_ALIAS) {
                    new_name->mod_path = uast_mod_alias_unwrap(result)->mod_path;
                    new_name->base = access->member_name->name.base;
                    return expand_def_name_internal(
                        new_lang_type,
                        new_expr,
                        new_name,
                        *new_name,
                        dest_pos,
                        true,
                        true,
                        uast_expr_get_pos_ref(expr)
                    );
                }

                if (result->type == UAST_VARIABLE_DEF) {
                    *new_expr = uast_member_access_wrap(access);
                    return EXPAND_NAME_NEW_EXPR;
                }
            }

            *new_expr = uast_member_access_wrap(access);
            return EXPAND_NAME_NEW_EXPR;
        }
        case UAST_SYMBOL: {
            Uast_symbol* sym = uast_symbol_unwrap(expr);
            if (sym->name.gen_args.info.count > 0) {
                if (name.gen_args.info.count > 0) {
                    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name));
                    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, *new_name));
                    Pos temp_pos = uast_expr_get_pos(expr);
                    *uast_expr_get_pos_ref(expr) = dest_pos;
                    pos_expanded_from_append(uast_expr_get_pos_ref(expr), arena_dup(&a_main, &temp_pos));
                    msg(
                        DIAG_DEF_DEST_AND_SRC_BOTH_HAVE_GEN_ARGS,
                        uast_expr_get_pos(expr), 
                        "def destination and source both have generic arguments (only one of destination or source is allowed generic arguments)\n"
                    );
                    return EXPAND_NAME_ERROR;
                }
            }
            *new_name = sym->name;

            return expand_def_name_internal(
                new_lang_type,
                new_expr,
                new_name,
                *new_name,
                dest_pos,
                true,
                true,
                &sym->pos
            );
        }
        case UAST_LITERAL:
            fallthrough;
        case UAST_OPERATOR:
            fallthrough;
        case UAST_INDEX: {
            Pos temp_pos = uast_expr_get_pos(expr);
            *uast_expr_get_pos_ref(expr) = dest_pos;
            pos_expanded_from_append(uast_expr_get_pos_ref(expr), arena_dup(&a_main, &temp_pos));
            *new_expr = expr;
            return EXPAND_NAME_NEW_EXPR;
        }
        case UAST_FN: {
            Uast_fn* uast_fn = uast_fn_unwrap(expr);
            Pos temp_pos = uast_expr_get_pos(expr);
            *uast_expr_get_pos_ref(expr) = dest_pos;
            pos_expanded_from_append(uast_expr_get_pos_ref(expr), arena_dup(&a_main, &temp_pos));
            ulang_type_fn_append_pos(&uast_fn->ulang_type, uast_fn->ulang_type, dest_pos);
            uast_fn->ulang_type.pos = uast_expr_get_pos(expr);
            *new_expr = expr;
            return EXPAND_NAME_NEW_EXPR;
        }
        case UAST_BLOCK:
            fallthrough;
        case UAST_IF_ELSE_CHAIN:
            fallthrough;
        case UAST_SWITCH:
            fallthrough;
        case UAST_UNKNOWN:
            fallthrough;
        case UAST_FUNCTION_CALL:
            fallthrough;
        case UAST_STRUCT_LITERAL:
            fallthrough;
        case UAST_ARRAY_LITERAL:
            fallthrough;
        case UAST_TUPLE:
            fallthrough;
        case UAST_MACRO:
            fallthrough;
        case UAST_ENUM_ACCESS:
            fallthrough;
        case UAST_EXPR_REMOVED:
            fallthrough;
        case UAST_ORELSE:
            fallthrough;
        case UAST_QUESTION_MARK:
            fallthrough;
        case UAST_UNDERSCORE:
            fallthrough;
        case UAST_ENUM_GET_TAG: {
            Pos temp_pos = uast_expr_get_pos(expr);
            *uast_expr_get_pos_ref(expr) = dest_pos;
            pos_expanded_from_append(uast_expr_get_pos_ref(expr), arena_dup(&a_main, &temp_pos));
            msg_todo("", uast_expr_get_pos(expr));
            return EXPAND_NAME_ERROR;
        }
    }
    unreachable("");
}

static EXPAND_NAME_STATUS expand_def_uname(Ulang_type* new_lang_type, Uast_expr** new_expr, Uname* name, Pos pos, Pos dest_pos) {
    Name actual = {0};
    if (!name_from_uname(&actual, *name, pos)) {
        return EXPAND_NAME_ERROR;
    }

    Name new_name = {0};
    switch (expand_def_name_internal(new_lang_type, new_expr, &new_name, actual, dest_pos, false, false, NULL)) {
        case EXPAND_NAME_NORMAL:
            *name = name_to_uname(new_name);
            return EXPAND_NAME_NORMAL;
        case EXPAND_NAME_NEW_EXPR:
            return EXPAND_NAME_NEW_EXPR;
        case EXPAND_NAME_NEW_ULANG_TYPE:
            return EXPAND_NAME_NEW_ULANG_TYPE;
        case EXPAND_NAME_ERROR:
            return EXPAND_NAME_ERROR;
    }
    unreachable("");
}

// TODO: expected failure case for having generic parameters in def definition
static EXPAND_NAME_STATUS expand_def_name(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Name* name,
    Pos dest_pos
) {
    Name new_name = {0};
    switch (expand_def_name_internal(new_lang_type, new_expr, &new_name, *name, dest_pos, false, false, NULL)) {
        case EXPAND_NAME_NORMAL:
            *name = new_name;
            return EXPAND_NAME_NORMAL;
        case EXPAND_NAME_NEW_EXPR:
            return EXPAND_NAME_NEW_EXPR;
        case EXPAND_NAME_NEW_ULANG_TYPE:
            return EXPAND_NAME_NEW_ULANG_TYPE;
        case EXPAND_NAME_ERROR:
            return EXPAND_NAME_ERROR;
    }
    unreachable("");
}

static bool expand_def_variable_def(Uast_variable_def* def, bool is_rhs, Uast_expr* rhs) {
    return expand_def_ulang_type(&def->lang_type, def->pos, is_rhs, rhs);
}

static bool expand_def_case(Uast_case* lang_case) {
    bool status = expand_def_block(lang_case->if_true);
    if (!lang_case->is_default) {
        status = expand_def_expr_not_ulang_type(&lang_case->expr, lang_case->expr, false/*TODO*/, NULL) && status;
    }
    return status;
}

static bool expand_def_function_call(Uast_function_call* call) {
    bool status = expand_def_expr_vec(&call->args);
    status = expand_def_expr_not_ulang_type(&call->callee, call->callee, false/*TODO*/, NULL) && status;
    call->pos.expanded_from = uast_expr_get_pos(call->callee).expanded_from;
    assert(!call->pos.expanded_from || !pos_is_equal(*call->pos.expanded_from, (Pos) {0}));
    return status;
}

static bool expand_def_struct_literal(Uast_struct_literal* lit) {
    return expand_def_expr_vec(&lit->members);
}

static bool expand_def_orelse(Uast_orelse* orelse) {
    if (!expand_def_block(orelse->if_error)) {
        return false;
    }

    todo();
    //Ulang_type dummy = {0};
    //switch (expand_def_expr(&dummy, &orelse->expr_to_unwrap, orelse->expr_to_unwrap)) {
    //    case EXPAND_EXPR_ERROR:
    //        return false;
    //    case EXPAND_EXPR_NEW_EXPR:
    //        return true;
    //    case EXPAND_EXPR_NEW_ULANG_TYPE:
    //        msg_got_type_but_expected_expr(uast_expr_get_pos(orelse->expr_to_unwrap));
    //        return false;
    //}
    unreachable("");
}

static bool expand_def_fn(Uast_fn* fn, bool is_rhs, Uast_expr* rhs) {
    return expand_def_ulang_type_fn(&fn->ulang_type, fn->ulang_type, fn->pos, is_rhs, rhs);
}

static bool expand_def_binary(Uast_binary* bin) {
    bool status = expand_def_expr_not_ulang_type(&bin->lhs, bin->lhs, true, bin->rhs);
    status = expand_def_expr_not_ulang_type(&bin->rhs, bin->rhs, false, NULL) && status;
    return status;
}

static bool expand_def_unary(Uast_unary* unary, bool is_rhs, Uast_expr* rhs) {
    bool status = expand_def_expr_not_ulang_type(&unary->child, unary->child, false, NULL);
    status = expand_def_ulang_type(&unary->lang_type, unary->pos /* TODO */, is_rhs, rhs) && status;
    return status;
}

static bool expand_def_condition(Uast_condition* cond) {
    return expand_def_operator(cond->child);
}

static bool expand_def_if(Uast_if* lang_if) {
    bool status = expand_def_condition(lang_if->condition);
    status = expand_def_block(lang_if->body) && status;
    return status;
}

static EXPAND_EXPR_STATUS expand_def_member_access(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Uast_member_access* access,
    bool is_rhs,
    Uast_expr* rhs
) {
    if (!expand_def_expr_not_ulang_type(&access->callee, access->callee, is_rhs, rhs)) {
        return EXPAND_EXPR_ERROR;
    }

    Uast_def* callee_def = NULL;
    switch (access->callee->type) {
        case UAST_SYMBOL: {
            Uast_symbol* sym = uast_symbol_unwrap(access->callee);
            Ulang_type_vec old_gen_args = sym->name.gen_args;
            memset(&sym->name.gen_args, 0, sizeof(sym->name.gen_args));
            if (!usymbol_lookup(&callee_def, sym->name)) {
                sym->name.gen_args = old_gen_args;
                *new_expr = uast_member_access_wrap(access);
                return EXPAND_EXPR_NEW_EXPR;
            }
            sym->name.gen_args = old_gen_args;
            break;
        }
        default:
            *new_expr = uast_member_access_wrap(access);
            return EXPAND_EXPR_NEW_EXPR;
    }

    Uast_symbol* sym = uast_symbol_unwrap(access->callee);
    if (callee_def->type == UAST_MOD_ALIAS) {
        Uname uname = uname_new(sym->name, access->member_name->name.base, access->member_name->name.gen_args, SCOPE_TOP_LEVEL);
        Name name = {0};
        if (!name_from_uname(&name, uname, access->pos)) {
            msg_todo("error message for this situation", access->pos);
            return EXPAND_EXPR_ERROR;
        }
        Uast_expr* new_rhs = rhs;
        if (is_rhs) {
            todo();
        }
        EXPAND_NAME_STATUS status = expand_def_symbol(
            new_lang_type,
            new_expr,
            uast_symbol_new(access->pos, name),
            is_rhs,
            new_rhs
        );
        switch (status) {
            case EXPAND_NAME_NORMAL:
                *new_expr = uast_member_access_wrap(access);
                return EXPAND_EXPR_NEW_EXPR;
            case EXPAND_NAME_NEW_EXPR:
                return EXPAND_EXPR_NEW_EXPR;
            case EXPAND_NAME_NEW_ULANG_TYPE:
                return EXPAND_EXPR_NEW_ULANG_TYPE;
            case EXPAND_NAME_ERROR:
                return EXPAND_EXPR_ERROR;
        }
        unreachable("");
    }

    *new_expr = uast_member_access_wrap(access);
    return EXPAND_EXPR_NEW_EXPR;
}

static bool expand_def_index(Uast_index* index, bool is_rhs, Uast_expr* rhs) {
    Uast_expr* index_rhs = rhs;
    Uast_expr* callee_rhs = rhs;

    if (is_rhs) {
        if (rhs->type == UAST_OPERATOR && uast_operator_unwrap(rhs)->type == UAST_BINARY) {
            index_rhs = uast_binary_unwrap(uast_operator_unwrap(rhs))->rhs;
            callee_rhs = uast_binary_unwrap(uast_operator_unwrap(rhs))->lhs;
        } else if (rhs->type == UAST_INDEX) {
            index_rhs = uast_index_unwrap(rhs)->index;
            callee_rhs = uast_index_unwrap(rhs)->callee;
        } else if (rhs->type == UAST_LITERAL) {
            is_rhs = false;
            rhs = NULL;
        } else if (rhs->type == UAST_SYMBOL) {
            is_rhs = false;
            rhs = NULL;
        } else {
            log(LOG_DEBUG, FMT"\n", uast_expr_print(rhs));
            todo();
            msg_soft_todo("", uast_expr_get_pos(rhs));
            msg_soft_todo("", index->pos);
            is_rhs = false;
            callee_rhs = NULL;
        }
    }

    bool status = expand_def_expr_not_ulang_type(&index->callee, index->callee, is_rhs, callee_rhs);
    return expand_def_expr_not_ulang_type(&index->index, index->index, is_rhs, index_rhs) && status;
}

bool expand_def_operator(Uast_operator* oper) {
    switch (oper->type) {
        case UAST_BINARY:
            return expand_def_binary(uast_binary_unwrap(oper));
        case UAST_UNARY:
            return expand_def_unary(uast_unary_unwrap(oper), false/*TODO*/, NULL);
    }
    unreachable("");
}

bool expand_def_underscore(Uast_expr** new_expr, Uast_underscore* underscore, bool is_rhs, Uast_expr* rhs) {
    if (is_rhs) {
        if (rhs->type != UAST_LITERAL || uast_literal_unwrap(rhs)->type != UAST_INT) {
            msg_todo("non-int type here", uast_expr_get_pos(rhs));
            return false;
        }
        *new_expr = rhs;
        return true;
    }

    *new_expr = uast_underscore_wrap(underscore);
    must_expand_again = true;
    return true;
}

static bool expand_def_array_literal(Uast_array_literal* lit) {
    return expand_def_expr_vec(&lit->members);
}

static bool expand_def_question_mark(Uast_question_mark* mark) {
    todo();
    //Ulang_type dummy = {0};
    //switch (expand_def_expr(&dummy, &mark->expr_to_unwrap, mark->expr_to_unwrap)) {
    //    case EXPAND_EXPR_ERROR:
    //        return false;
    //    case EXPAND_EXPR_NEW_EXPR:
    //        return true;
    //    case EXPAND_EXPR_NEW_ULANG_TYPE:
    //        msg_got_type_but_expected_expr(mark->pos);
    //        return false;
    //}
    unreachable("");
}

static bool expand_def_struct_def_base(Ustruct_def_base* base, Pos dest_pos) {
    if (!expand_def_generic_param_vec(&base->generics) || !expand_def_variable_def_vec(&base->members)) {
        return false;
    }
    Uast_expr* new_expr = NULL;
    Ulang_type new_lang_type = {0};
    switch (expand_def_name(&new_lang_type, &new_expr, &base->name, dest_pos)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            // TODO
            msg_todo("", dest_pos);
            return false;
        case EXPAND_NAME_ERROR:
            // TODO
            msg_todo("", dest_pos);
            return false;
        case EXPAND_NAME_NEW_ULANG_TYPE:
            // TODO
            msg_todo("", dest_pos);
            return false;
    }
    unreachable("");
}

static bool expand_def_struct_def(Uast_struct_def* def) {
    return expand_def_struct_def_base(&def->base, def->pos);
}

static bool expand_def_raw_union_def(Uast_raw_union_def* def) {
    return expand_def_struct_def_base(&def->base, def->pos);
}

static bool expand_def_enum_def(Uast_enum_def* def) {
    return expand_def_struct_def_base(&def->base, def->pos);
}

static bool expand_def_literal(Uast_literal* lit) {
    switch (lit->type) {
        case UAST_INT:
            return true;
        case UAST_FLOAT:
            return true;
        case UAST_STRING:
            return true;
        case UAST_VOID:
            return true;
    }
    unreachable("");
}

static EXPAND_NAME_STATUS expand_def_symbol(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Uast_symbol* sym,
    bool is_rhs,
    Uast_expr* rhs
) {
    Uast_def* def = NULL;
    if (expand_again_lookup(&def, sym->name)) {
        if (!expand_def_def(def, is_rhs, rhs)) {
            return false;
        }
    }
    return expand_def_name(new_lang_type, new_expr, &sym->name, sym->pos);
}

static bool expand_def_expr_not_ulang_type(Uast_expr** new_expr, Uast_expr* expr, bool is_rhs, Uast_expr* rhs) {
    Ulang_type dummy = {0};
    switch (expand_def_expr(&dummy, new_expr, expr, is_rhs, rhs)) {
        case EXPAND_EXPR_ERROR:
            return false;
        case EXPAND_EXPR_NEW_EXPR:
            return true;
        case EXPAND_EXPR_NEW_ULANG_TYPE:
            msg(DIAG_EXPECTED_EXPR_BUT_GOT_TYPE, uast_expr_get_pos(expr), "expected expression, but got type\n");
            return false;
    }
    unreachable("");
}

static EXPAND_EXPR_STATUS expand_def_expr(
    Ulang_type* new_lang_type,
    Uast_expr** new_expr,
    Uast_expr* expr,
    bool is_rhs,
    Uast_expr* rhs
) {
#   define a(expr) ((expr) ? EXPAND_EXPR_NEW_EXPR : EXPAND_EXPR_ERROR)

    switch (expr->type) {
        case UAST_BLOCK:
            *new_expr = expr;
            return a(expand_def_block(uast_block_unwrap(expr)));
        case UAST_IF_ELSE_CHAIN:
            *new_expr = expr;
            return a(expand_def_if_else_chain(uast_if_else_chain_unwrap(expr)));
        case UAST_SWITCH:
            *new_expr = expr;
            return a(expand_def_switch(uast_switch_unwrap(expr)));
        case UAST_UNKNOWN:
            *new_expr = expr;
            return a(true);
        case UAST_OPERATOR:
            *new_expr = expr;
            return a(expand_def_operator(uast_operator_unwrap(expr)));
        case UAST_UNDERSCORE:
            return a(expand_def_underscore(new_expr, uast_underscore_unwrap(expr), is_rhs, rhs));
        case UAST_SYMBOL: {
            switch (expand_def_symbol(new_lang_type, new_expr, uast_symbol_unwrap(expr), is_rhs, rhs)) {
                case EXPAND_NAME_NORMAL:
                    *new_expr = expr;
                    return EXPAND_EXPR_NEW_EXPR;
                case EXPAND_NAME_NEW_EXPR:
                    return EXPAND_EXPR_NEW_EXPR;
                case EXPAND_NAME_NEW_ULANG_TYPE:
                    return EXPAND_EXPR_NEW_ULANG_TYPE;
                case EXPAND_NAME_ERROR:
                    return EXPAND_EXPR_ERROR;
            }
            unreachable("");
        }
        case UAST_MEMBER_ACCESS:
            return expand_def_member_access(
                new_lang_type,
                new_expr,
                uast_member_access_unwrap(expr),
                is_rhs,
                rhs
            );
        case UAST_INDEX:
            *new_expr = expr;
            return a(expand_def_index(uast_index_unwrap(expr), is_rhs, rhs));
        case UAST_LITERAL:
            *new_expr = expr;
            return a(expand_def_literal(uast_literal_unwrap(expr)));
        case UAST_FUNCTION_CALL:
            *new_expr = expr;
            return a(expand_def_function_call(uast_function_call_unwrap(expr)));
        case UAST_STRUCT_LITERAL:
            *new_expr = expr;
            return a(expand_def_struct_literal(uast_struct_literal_unwrap(expr)));
        case UAST_ORELSE:
            *new_expr = expr;
            return a(expand_def_orelse(uast_orelse_unwrap(expr)));
        case UAST_ARRAY_LITERAL:
            *new_expr = expr;
            return a(expand_def_array_literal(uast_array_literal_unwrap(expr)));
        case UAST_TUPLE:
            msg_todo("def expansion in this situation", uast_expr_get_pos(expr));
            return EXPAND_EXPR_ERROR;
        case UAST_MACRO:
            *new_expr = expr;
            return EXPAND_EXPR_NEW_EXPR;
        case UAST_ENUM_ACCESS:
            msg_todo("def expansion in this situation", uast_expr_get_pos(expr));
            return EXPAND_EXPR_ERROR;
        case UAST_ENUM_GET_TAG:
            msg_todo("def expansion in this situation", uast_expr_get_pos(expr));
            return EXPAND_EXPR_ERROR;
        case UAST_EXPR_REMOVED:
            *new_expr = expr;
            return EXPAND_EXPR_NEW_EXPR;
        case UAST_QUESTION_MARK:
            *new_expr = expr;
            return a(expand_def_question_mark(uast_question_mark_unwrap(expr)));;
        case UAST_FN:
            *new_expr = expr;
            return a(expand_def_fn(uast_fn_unwrap(expr), is_rhs, rhs));
    }
    unreachable("");

#   undef a
}

static bool expand_def_return(Uast_return* rtn) {
    return expand_def_expr_not_ulang_type(&rtn->child, rtn->child, false, NULL);
}

static bool expand_def_defer(Uast_defer* lang_defer) {
    return expand_def_stmt(&lang_defer->child, lang_defer->child);
}

static bool expand_def_using(Uast_using* using) {
    Uast_expr* dummy_expr = NULL;
    Ulang_type dummy_lang_type = {0};
    switch (expand_def_name(&dummy_lang_type, &dummy_expr, &using->sym_name, using->pos)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            msg_todo("new expression substitution here", using->pos);
            return false;
        case EXPAND_NAME_NEW_ULANG_TYPE:
            msg_todo("", using->pos);
            return false;
        case EXPAND_NAME_ERROR:
            return false;
    }
    unreachable("");
}

static bool expand_def_yield(Uast_yield* yield) {
    return (!yield->do_yield_expr || expand_def_expr_not_ulang_type(&yield->yield_expr, yield->yield_expr, false, NULL));
    // TODO: does yield->break_out_of need to be expanded?
}

static bool expand_def_assignment(Uast_assignment* assign) {
    bool status = expand_def_expr_not_ulang_type(&assign->lhs, assign->lhs, true, assign->rhs);
    return expand_def_expr_not_ulang_type(&assign->rhs, assign->rhs, false, NULL) && status;
}

static bool expand_def_continue(Uast_continue* cont) {
    (void) cont;
    return true;
}

static bool expand_def_for_with_cond(Uast_for_with_cond* lang_for) {
    bool status = expand_def_condition(lang_for->condition);
    return expand_def_block(lang_for->body) && status;
}

static bool expand_def_stmt(Uast_stmt** new_stmt, Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Uast_expr* new_expr = NULL;
            if (!expand_def_expr_not_ulang_type(&new_expr, uast_expr_unwrap(stmt), false/*TODO*/, NULL)) {
                return false;
            }
            *new_stmt = uast_expr_wrap(new_expr);
            return true;
        }
        case UAST_DEF:
            return expand_def_def(uast_def_unwrap(stmt), false, NULL);
        case UAST_FOR_WITH_COND:
            return expand_def_for_with_cond(uast_for_with_cond_unwrap(stmt));
        case UAST_CONTINUE:
            return expand_def_continue(uast_continue_unwrap(stmt));
        case UAST_YIELD:
            return expand_def_yield(uast_yield_unwrap(stmt));
        case UAST_ASSIGNMENT:
            return expand_def_assignment(uast_assignment_unwrap(stmt));
        case UAST_RETURN:
            return expand_def_return(uast_return_unwrap(stmt));
        case UAST_DEFER:
            return expand_def_defer(uast_defer_unwrap(stmt));
        case UAST_STMT_REMOVED:
            return true;
        case UAST_USING:
            unreachable("using should have been removed in expand_using");
    }
    unreachable("");
}

static bool expand_def_lang_def(Uast_lang_def* def) {
    (void) def;
    // TODO: expand def->expr here to reduce the amount of duplicate work that is done?
    return true;
}

static bool expand_def_generic_param(Uast_generic_param* param) {
    (void) param;
    return true;
}

static bool expand_def_param(Uast_param* param) {
    bool status = true;
    
    if (!expand_def_variable_def(param->base, false, NULL)) {
        status = false;
    }
    if (param->is_optional && !expand_def_expr_not_ulang_type(&param->optional_default, param->optional_default, false, NULL)) {
        status = false;
    }

    return status;
}

static bool expand_def_generic_param_vec(Uast_generic_param_vec* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->info.count; idx++) {
        status = expand_def_generic_param(vec_at(*params, idx)) && status;
    }
    return status;
}

static bool expand_def_variable_def_vec(Uast_variable_def_vec* defs) {
    bool status = true;
    for (size_t idx = 0; idx < defs->info.count; idx++) {
        status = expand_def_variable_def(vec_at(*defs, idx), false, NULL) && status;
    }
    return status;
}

static bool expand_def_expr_vec(Uast_expr_vec* exprs) {
    bool status = true;
    for (size_t idx = 0; idx < exprs->info.count; idx++) {
        status = expand_def_expr_not_ulang_type(vec_at_ref(exprs, idx), vec_at(*exprs, idx), false/*TODO*/, NULL) && status;
    }
    return status;
}

static bool expand_def_case_vec(Uast_case_vec* cases) {
    bool status = true;
    for (size_t idx = 0; idx < cases->info.count; idx++) {
        if (!expand_def_case(vec_at(*cases, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_if_vec(Uast_if_vec* ifs) {
    bool status = true;
    for (size_t idx = 0; idx < ifs->info.count; idx++) {
        if (!expand_def_if(vec_at(*ifs, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_params(Uast_function_params* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        if (!expand_def_param(vec_at(params->params, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_decl(Uast_function_decl* def) {
    bool status = expand_def_generic_param_vec(&def->generics);
    status = expand_def_function_params(def->params) && status;
    status = expand_def_ulang_type(&def->return_type, def->pos, false, NULL) && status;
    return status;
}

static bool expand_def_function_def(Uast_function_def* def) {
    bool status = expand_def_function_decl(def->decl);
    status = expand_def_block(def->body) && status;
    return status;
}

static bool expand_def_mod_alias(Uast_mod_alias* alias) {
    (void) alias;
    //todo();
    //return expand_def_name(&alias->name) && expand_def_name(&alias->mod_path);
    // TODO
    return true;
}

static bool expand_def_import_path(Uast_import_path* path) {
    return expand_def_block(path->block);
}

static bool expand_def_def(Uast_def* def, bool is_rhs, Uast_expr* rhs) {
    bool status = true;
    bool old_must_expand_again = must_expand_again;

    switch (def->type) {
        case UAST_MOD_ALIAS:
            status = expand_def_mod_alias(uast_mod_alias_unwrap(def));
            break;
        case UAST_IMPORT_PATH:
            status = expand_def_import_path(uast_import_path_unwrap(def));
            break;
        case UAST_POISON_DEF:
            status = false;
            break;
        case UAST_GENERIC_PARAM:
            status = expand_def_generic_param(uast_generic_param_unwrap(def));
            break;
        case UAST_FUNCTION_DEF:
            status = expand_def_function_def(uast_function_def_unwrap(def));
            break;
        case UAST_VARIABLE_DEF:
            status = expand_def_variable_def(uast_variable_def_unwrap(def), is_rhs, rhs);
            break;
        case UAST_STRUCT_DEF:
            status = expand_def_struct_def(uast_struct_def_unwrap(def));
            break;
        case UAST_RAW_UNION_DEF:
            status = expand_def_raw_union_def(uast_raw_union_def_unwrap(def));
            break;
        case UAST_ENUM_DEF:
            status = expand_def_enum_def(uast_enum_def_unwrap(def));
            break;
        case UAST_PRIMITIVE_DEF:
            status = true;
            break;
        case UAST_FUNCTION_DECL:
            status = expand_def_function_decl(uast_function_decl_unwrap(def));
            break;
        case UAST_LANG_DEF:
            status = expand_def_lang_def(uast_lang_def_unwrap(def));
            break;
        case UAST_VOID_DEF:
            status = true;
            break;
        case UAST_LABEL:
            status = true;
            break;
        case UAST_BUILTIN_DEF:
            status = true;
            break;
    }

    if (must_expand_again) {
        expand_again_add(&a_pass, def);
    }
    must_expand_again = old_must_expand_again;

    return status;
}

static bool expand_def_switch(Uast_switch* lang_switch) {
    bool status = expand_def_expr_not_ulang_type(&lang_switch->operand, lang_switch->operand, false/*TODO*/, NULL);
    return expand_def_case_vec(&lang_switch->cases) && status;
}

static bool expand_def_if_else_chain(Uast_if_else_chain* if_else) {
    return expand_def_if_vec(&if_else->uasts);
}

static bool expand_def_block(Uast_block* block) {
    bool status = true;

    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        status = expand_def_def(curr, false/*TODO*/, NULL/*TODO*/) && status;
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        status = expand_def_stmt(vec_at_ref(&block->children, idx), vec_at(block->children, idx)) && status;
    }

    return status;
}

void expand_def(void) {
    Usymbol_iter iter = usym_tbl_iter_new(SCOPE_TOP_LEVEL);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        expand_def_def(curr, false/*TODO*/, NULL);
    }
}
