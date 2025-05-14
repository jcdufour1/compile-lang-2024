#include <symbol_iter.h>
#include <uast.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <ulang_type_clone.h>
#include <expand_lang_def.h>
#include <uast_clone.h>
#include <type_checking.h>
#include <msg_todo.h>

// TODO: consider if def definition has pointer_depth > 0

static bool expand_def_ulang_type_regular(
    Ulang_type_regular* new_lang_type,
    Ulang_type_regular lang_type
) {
    (void) new_lang_type;
    Uast_expr* new_expr = NULL;
    switch (expand_def_uname(&new_expr, &lang_type.atom.str)) {
        case EXPAND_NAME_ERROR:
            todo();
        case EXPAND_NAME_NORMAL:
            *new_lang_type = lang_type;
            return true;
        case EXPAND_NAME_NEW_EXPR:
            break;
        default:
            todo();
    }

    switch (new_expr->type) {
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(new_expr);
            if (access->callee->type != UAST_SYMBOL) {
                msg_todo("", uast_expr_get_pos(access->callee));
                return false;
            }
            Uname new_uname = {.mod_alias = uast_symbol_unwrap(access->callee)->name, .base = access->member_name->name.base, .gen_args = lang_type.atom.str.gen_args};
            *new_lang_type = ulang_type_regular_new(ulang_type_atom_new(new_uname, lang_type.atom.pointer_depth), lang_type.pos);
            return true;
        }
        case UAST_SYMBOL:
            todo();
        default:
            msg_todo("", uast_expr_get_pos(new_expr));
            return false;
    }

    todo();
}

static bool expand_def_ulang_type_fn(
    Ulang_type_fn* new_lang_type,
    Ulang_type_fn lang_type
) {
    bool status = true;

    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        if (!expand_def_ulang_type(vec_at_ref(&lang_type.params.ulang_types, idx))) {
            status = false;
        }
    }

    if (!expand_def_ulang_type(lang_type.return_type)) {
        status = false;
    }

    *new_lang_type = lang_type;
    return status;
}

static bool expand_def_ulang_type_tuple(
    Ulang_type_tuple* new_lang_type,
    Ulang_type_tuple lang_type
) {
    bool status = true;

    for (size_t idx = 0; idx < lang_type.ulang_types.info.count; idx++) {
        if (!expand_def_ulang_type(vec_at_ref(&lang_type.ulang_types, idx))) {
            status = false;
        }
    }

    *new_lang_type = lang_type;
    return status;
}

bool expand_def_ulang_type(Ulang_type* lang_type) {
    switch (lang_type->type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular new_lang_type = {0};
            if (!expand_def_ulang_type_regular(&new_lang_type, ulang_type_regular_const_unwrap(*lang_type))) {
                return false;
            }
            *lang_type = ulang_type_regular_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_FN: {
            Ulang_type_fn new_lang_type = {0};
            if (!expand_def_ulang_type_fn(&new_lang_type, ulang_type_fn_const_unwrap(*lang_type))) {
                return false;
            }
            *lang_type = ulang_type_fn_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_TUPLE: {
            Ulang_type_tuple new_lang_type = {0};
            if (!expand_def_ulang_type_tuple(&new_lang_type, ulang_type_tuple_const_unwrap(*lang_type))) {
                return false;
            }
            *lang_type = ulang_type_tuple_const_wrap(new_lang_type);
            return true;
        }
    }
    unreachable("");
}

static EXPAND_NAME_STATUS expand_def_name_internal(Uast_expr** new_expr, Name* new_name, Name name) {
    Uast_def* def = NULL;
    *new_name = name;
    memset(&new_name->gen_args, 0, sizeof(new_name->gen_args));
    if (!usymbol_lookup(&def, *new_name)) {
        new_name->gen_args = name.gen_args;
        return EXPAND_NAME_NORMAL;
    }
    new_name->gen_args = name.gen_args;

    switch (def->type) {
        case UAST_POISON_DEF:
            return EXPAND_NAME_ERROR;
        case UAST_LANG_DEF:
            break;
        case UAST_IMPORT_PATH:
            return EXPAND_NAME_NORMAL;
        case UAST_MOD_ALIAS:
            // TODO: expand mod_alias here?
            return EXPAND_NAME_NORMAL;
        case UAST_GENERIC_PARAM:
            return EXPAND_NAME_NORMAL;
        case UAST_FUNCTION_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_VARIABLE_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_STRUCT_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_RAW_UNION_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_SUM_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_PRIMITIVE_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_FUNCTION_DECL:
            return EXPAND_NAME_NORMAL;
    }

    Uast_expr* expr = uast_expr_clone(uast_lang_def_unwrap(def)->expr, name.scope_id);
    switch (expr->type) {
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(expr);
            unwrap(access->member_name->name.gen_args.info.count == 0 && "not implemented");
            new_name->gen_args = name.gen_args;
            *new_expr = uast_member_access_wrap(access);
            return EXPAND_NAME_NEW_EXPR;
        }
        case UAST_SYMBOL: {
            Uast_symbol* sym = uast_symbol_unwrap(expr);
            *new_name = sym->name;
            unwrap(new_name->gen_args.info.count == 0 && "not implemented");
            new_name->gen_args = name.gen_args;
            return EXPAND_NAME_NORMAL;
        }
        case UAST_IF_ELSE_CHAIN:
            todo();
        case UAST_SWITCH:
            todo();
        case UAST_UNKNOWN:
            todo();
        case UAST_OPERATOR:
            todo();
        case UAST_INDEX:
            todo();
        case UAST_LITERAL:
            todo();
        case UAST_FUNCTION_CALL:
            todo();
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_ARRAY_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_SUM_ACCESS:
            todo();
        case UAST_SUM_GET_TAG:
            todo();
    }
    unreachable("");
}

EXPAND_NAME_STATUS expand_def_uname(Uast_expr** new_expr, Uname* name) {
    Name actual = {0};
    if (!name_from_uname(&actual, *name)) {
        todo();
        return false;
    }

    Name new_name = {0};
    switch (expand_def_name_internal(new_expr, &new_name, actual)) {
        case EXPAND_NAME_NORMAL:
            unwrap(str_view_is_equal(actual.mod_path, new_name.mod_path) && "not implemented");
            unwrap(ulang_type_vec_is_equal(actual.gen_args, new_name.gen_args) && "not implemented");
            name->base = new_name.base;
            return EXPAND_NAME_NORMAL;
        case EXPAND_NAME_NEW_EXPR:
            unwrap(str_view_is_equal(actual.mod_path, new_name.mod_path) && "not implemented");
            unwrap(ulang_type_vec_is_equal(actual.gen_args, new_name.gen_args) && "not implemented");
            return EXPAND_NAME_NEW_EXPR;
        case EXPAND_NAME_ERROR:
            todo();
    }
    unreachable("");
}

// TODO: expected failure case for having generic parameters in def definition
static EXPAND_NAME_STATUS expand_def_name(Uast_expr** new_expr, Name* name) {
    Name new_name = {0};
    switch (expand_def_name_internal(new_expr, &new_name, *name)) {
        case EXPAND_NAME_NORMAL:
            *name = new_name;
            return EXPAND_NAME_NORMAL;
        case EXPAND_NAME_NEW_EXPR:
            return EXPAND_NAME_NEW_EXPR;
        case EXPAND_NAME_ERROR:
            return EXPAND_NAME_ERROR;
    }
    unreachable("");
}

static bool expand_def_variable_def(Uast_variable_def* def) {
    (void) def;
    todo();
    //if (!expand_def_ulang_type(&def->lang_type)) {
    //    return false;
    //}
    //Uast_expr* new_expr = NULL;
    //switch (expand_def_name(&new_expr, &def->name)) {
    //    case EXPAND_NAME_NORMAL:
    //        todo();
    //    case EXPAND_NAME_NEW_EXPR:
    //        todo();
    //    case EXPAND_NAME_ERROR:
    //        todo();
    //}
    unreachable("");
}

static bool expand_def_function_call(Uast_function_call* call) {
    (void) call;
    todo();
    //return expand_def_expr_vec(&call->args) && expand_def_expr(call->callee);
}

static bool expand_def_struct_def_base(Ustruct_def_base* base) {
    if (!expand_def_generic_param_vec(&base->generics) || !expand_def_variable_def_vec(&base->members)) {
        return false;
    }
    Uast_expr* new_expr = NULL;
    switch (expand_def_name(&new_expr, &base->name)) {
        case EXPAND_NAME_NORMAL:
            todo();
        case EXPAND_NAME_NEW_EXPR:
            todo();
        case EXPAND_NAME_ERROR:
            todo();
    }
    unreachable("");
}

static bool expand_def_struct_def(Uast_struct_def* def) {
    return expand_def_struct_def_base(&def->base);
}

static bool expand_def_raw_union_def(Uast_raw_union_def* def) {
    return expand_def_struct_def_base(&def->base);
}

static bool expand_def_sum_def(Uast_sum_def* def) {
    return expand_def_struct_def_base(&def->base);
}

static bool expand_def_literal(Uast_literal* lit) {
    switch (lit->type) {
        case UAST_NUMBER:
            return true;
        case UAST_STRING:
            return true;
        case UAST_VOID:
            return true;
        case UAST_CHAR:
            return true;
    }
    unreachable("");
}

EXPAND_NAME_STATUS expand_def_symbol(Uast_expr** new_expr, Uast_symbol* sym) {
    return expand_def_name(new_expr, &sym->name);
}

bool expand_def_expr(Uast_expr* expr) {
    switch (expr->type) {
        case UAST_IF_ELSE_CHAIN:
            todo();
        case UAST_SWITCH:
            todo();
        case UAST_UNKNOWN:
            todo();
        case UAST_OPERATOR:
            todo();
        case UAST_SYMBOL:
            todo();
            //return expand_def_symbol(uast_symbol_unwrap(expr));
        case UAST_MEMBER_ACCESS:
            todo();
        case UAST_INDEX:
            todo();
        case UAST_LITERAL:
            return expand_def_literal(uast_literal_unwrap(expr));
        case UAST_FUNCTION_CALL:
            return expand_def_function_call(uast_function_call_unwrap(expr));
        case UAST_STRUCT_LITERAL:
            todo();
        case UAST_ARRAY_LITERAL:
            todo();
        case UAST_TUPLE:
            todo();
        case UAST_SUM_ACCESS:
            todo();
        case UAST_SUM_GET_TAG:
            todo();
    }
    unreachable("");
}

static bool expand_def_return(Uast_return* rtn) {
    return expand_def_expr(rtn->child);
}

static bool expand_def_stmt(Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_BLOCK:
            todo();
        case UAST_EXPR:
            return expand_def_expr(uast_expr_unwrap(stmt));
        case UAST_DEF:
            return expand_def_def(uast_def_unwrap(stmt));
        case UAST_FOR_WITH_COND:
            todo();
        case UAST_BREAK:
            todo();
        case UAST_CONTINUE:
            todo();
        case UAST_ASSIGNMENT:
            todo();
        case UAST_RETURN:
            return expand_def_return(uast_return_unwrap(stmt));
        case UAST_LABEL:
            todo();
    }
    unreachable("");
}

static bool expand_def_lang_def(Uast_lang_def* def) {
    (void) def;
    // TODO
    return true;
}

static bool expand_def_generic_param(Uast_generic_param* param) {
    (void) param;
    todo();
    //return expand_def_symbol(param->child);
}

static bool expand_def_param(Uast_param* param) {
    bool status = true;
    
    if (!expand_def_variable_def(param->base)) {
        status = false;
    }
    if (param->is_optional && !expand_def_expr(param->optional_default)) {
        status = false;
    }

    return status;
}

bool expand_def_generic_param_vec(Uast_generic_param_vec* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->info.count; idx++) {
        if (!expand_def_generic_param(vec_at(params, idx))) {
            status = false;
        }
    }
    return status;
}

bool expand_def_variable_def_vec(Uast_variable_def_vec* defs) {
    bool status = true;
    for (size_t idx = 0; idx < defs->info.count; idx++) {
        if (!expand_def_variable_def(vec_at(defs, idx))) {
            status = false;
        }
    }
    return status;
}

bool expand_def_expr_vec(Uast_expr_vec* exprs) {
    bool status = true;
    for (size_t idx = 0; idx < exprs->info.count; idx++) {
        if (!expand_def_expr(vec_at(exprs, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_params(Uast_function_params* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        if (!expand_def_param(vec_at(&params->params, idx))) {
            todo();
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_decl(Uast_function_decl* def) {
    if ( 
        !expand_def_generic_param_vec(&def->generics) ||
        !expand_def_function_params(def->params) ||
        !expand_def_ulang_type(&def->return_type)
    ) {
        return false;
    }

    Uast_expr* new_expr = NULL;
    switch (expand_def_name(&new_expr, &def->name)) {
        case EXPAND_NAME_NORMAL:
            todo();
        case EXPAND_NAME_NEW_EXPR:
            todo();
        case EXPAND_NAME_ERROR:
            todo();
    }
    unreachable("");
}

static bool expand_def_function_def(Uast_function_def* def) {
    return expand_def_function_decl(def->decl) && expand_def_block(def->body);
}

static bool expand_def_mod_alias(Uast_mod_alias* alias) {
    (void) alias;
    todo();
    //return expand_def_name(&alias->name) && expand_def_name(&alias->mod_path);
}

static bool expand_def_import_path(Uast_import_path* path) {
    return expand_def_block(path->block);
}

bool expand_def_def(Uast_def* def) {
    switch (def->type) {
        case UAST_MOD_ALIAS:
            return expand_def_mod_alias(uast_mod_alias_unwrap(def));
        case UAST_IMPORT_PATH:
            return expand_def_import_path(uast_import_path_unwrap(def));
        case UAST_POISON_DEF:
            todo();
        case UAST_GENERIC_PARAM:
            todo();
        case UAST_FUNCTION_DEF:
            return expand_def_function_def(uast_function_def_unwrap(def));
        case UAST_VARIABLE_DEF:
            return expand_def_variable_def(uast_variable_def_unwrap(def));
        case UAST_STRUCT_DEF:
            return expand_def_struct_def(uast_struct_def_unwrap(def));
        case UAST_RAW_UNION_DEF:
            return expand_def_raw_union_def(uast_raw_union_def_unwrap(def));
        case UAST_SUM_DEF:
            return expand_def_sum_def(uast_sum_def_unwrap(def));
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            return expand_def_function_decl(uast_function_decl_unwrap(def));
        case UAST_LANG_DEF:
            return expand_def_lang_def(uast_lang_def_unwrap(def));
    }
    unreachable("");
}

bool expand_def_block(Uast_block* block) {
    bool status = true;

    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        if (!expand_def_def(curr)) {
    todo();
            status = false;
        }
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        if (!expand_def_stmt(vec_at(&block->children, idx))) {
            log(LOG_DEBUG, TAST_FMT"\n", uast_stmt_print(vec_at(&block->children, idx)));
    todo();
            status = false;
        }
    }

    return status;
}

