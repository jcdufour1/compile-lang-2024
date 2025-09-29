#include <symbol_iter.h>
#include <uast.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <ulang_type_clone.h>
#include <expand_lang_def.h>
#include <uast_clone.h>
#include <type_checking.h>
#include <msg_todo.h>
#include <lang_type_from_ulang_type.h>
#include <msg_undefined_symbol.h>

// TODO: consider if def definition has pointer_depth > 0

bool expand_def_ulang_type_regular(
    Ulang_type_regular* new_lang_type,
    Ulang_type_regular lang_type,
    Pos dest_pos
) {

    (void) new_lang_type;
    Uast_expr* new_expr = NULL;
    switch (expand_def_uname(&new_expr, &lang_type.atom.str, lang_type.pos, dest_pos)) {
        case EXPAND_NAME_ERROR:
            return false;
        case EXPAND_NAME_NORMAL:
            *new_lang_type = lang_type;
            return true;
        case EXPAND_NAME_NEW_EXPR:
            break;
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
            // TODO: use uname_new function
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
    unreachable("");
}

static bool expand_def_ulang_type_fn(
    Ulang_type_fn* new_lang_type,
    Ulang_type_fn lang_type,
    Pos dest_pos
) {
    bool status = true;

    for (size_t idx = 0; idx < lang_type.params.ulang_types.info.count; idx++) {
        if (!expand_def_ulang_type(vec_at_ref(&lang_type.params.ulang_types, idx), dest_pos)) {
            status = false;
        }
    }

    if (!expand_def_ulang_type(lang_type.return_type, dest_pos)) {
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
        if (!expand_def_ulang_type(vec_at_ref(&lang_type.ulang_types, idx), dest_pos)) {
            status = false;
        }
    }

    *new_lang_type = lang_type;
    return status;
}

bool expand_def_ulang_type(Ulang_type* lang_type, Pos dest_pos) {
    switch (lang_type->type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular new_lang_type = {0};
            if (!expand_def_ulang_type_regular(&new_lang_type, ulang_type_regular_const_unwrap(*lang_type), dest_pos)) {
                return false;
            }
            *lang_type = ulang_type_regular_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_FN: {
            Ulang_type_fn new_lang_type = {0};
            if (!expand_def_ulang_type_fn(&new_lang_type, ulang_type_fn_const_unwrap(*lang_type), dest_pos)) {
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
        case ULANG_TYPE_GEN_PARAM: {
            // TODO: actually implement this if needed
            return true;
        }
    }
    unreachable("");
}

static EXPAND_NAME_STATUS expand_def_name_internal(Uast_expr** new_expr, Name* new_name, Name name, Pos dest_pos) {
    if (strv_is_equal(name.base, sv("Token"))) {
        log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, name));
    }

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
            if (strv_is_equal(name.base, sv("Token"))) {
                todo();
            }
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
        case UAST_ENUM_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_PRIMITIVE_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_FUNCTION_DECL:
            return EXPAND_NAME_NORMAL;
        case UAST_VOID_DEF:
            return EXPAND_NAME_NORMAL;
        case UAST_LABEL:
            todo();
    }

    // TODO: this clone is expensive I think
    Uast_expr* expr = uast_expr_clone(uast_lang_def_unwrap(def)->expr, true, name.scope_id, dest_pos);
    switch (expr->type) {
        case UAST_BLOCK:
            todo();
        case UAST_MEMBER_ACCESS: {
            Uast_member_access* access = uast_member_access_unwrap(expr);
            unwrap(access->member_name->name.gen_args.info.count == 0 && "not implemented");
            new_name->gen_args = name.gen_args;

            log_temp(LOG_DEBUG, FMT"\n", uast_def_print(def));
            log_temp(LOG_DEBUG, FMT"\n", uast_member_access_print(access));
            Uast_def* result = NULL;
            if (access->callee->type == UAST_SYMBOL) {
                if (!usymbol_lookup(&result, uast_symbol_unwrap(access->callee)->name)) {
                    msg_undefined_symbol(uast_symbol_unwrap(access->callee)->name, uast_symbol_unwrap(access->callee)->pos);
                }
                log_temp(LOG_DEBUG, FMT"\n", uast_def_print(result));
                if (result->type == UAST_MOD_ALIAS) {
                    new_name->mod_path = uast_mod_alias_unwrap(result)->mod_path;
                    new_name->base = access->member_name->name.base;
                    log(LOG_DEBUG, FMT"\n", name_print(NAME_LOG, *new_name));
                    return expand_def_name_internal(new_expr, new_name, *new_name, dest_pos);
                }
                todo();
            }
            todo();

            log(LOG_DEBUG, FMT"\n", strv_print(name.mod_path));
            log(LOG_DEBUG, FMT"\n", strv_print(name.base));
            *new_expr = uast_member_access_wrap(access);
            return EXPAND_NAME_NEW_EXPR;
        }
        case UAST_SYMBOL: {
            Uast_symbol* sym = uast_symbol_unwrap(expr);
            *new_name = sym->name;
            if (new_name->gen_args.info.count > 0) {
                if (name.gen_args.info.count > 0) {
                    // TODO: expected failure case?
                    todo();
                }
            } else {
                new_name->gen_args = name.gen_args;
            }
            return expand_def_name_internal(new_expr, new_name, *new_name, dest_pos);
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
        case UAST_MACRO:
            todo();
        case UAST_ENUM_ACCESS:
            todo();
        case UAST_ENUM_GET_TAG:
            todo();
    }
    unreachable("");
}

EXPAND_NAME_STATUS expand_def_uname(Uast_expr** new_expr, Uname* name, Pos pos, Pos dest_pos) {
    Name actual = {0};
    if (!name_from_uname(&actual, *name, pos)) {
        return EXPAND_NAME_ERROR;
    }

    Name new_name = {0};
    switch (expand_def_name_internal(new_expr, &new_name, actual, dest_pos)) {
        case EXPAND_NAME_NORMAL:
            *name = name_to_uname(new_name);
            //log(LOG_DEBUG, FMT"\n", name_print(
            //unwrap(strv_is_equal(actual.mod_path, new_name.mod_path) && "not implemented");
            //name->gen_args = new_name.gen_args;
            //name->base = new_name.base;
            return EXPAND_NAME_NORMAL;
        case EXPAND_NAME_NEW_EXPR:
            unwrap(strv_is_equal(actual.mod_path, new_name.mod_path) && "not implemented");
            unwrap(ulang_type_vec_is_equal(actual.gen_args, new_name.gen_args) && "not implemented");
            return EXPAND_NAME_NEW_EXPR;
        case EXPAND_NAME_ERROR:
            return EXPAND_NAME_ERROR;
    }
    unreachable("");
}

// TODO: expected failure case for having generic parameters in def definition
static EXPAND_NAME_STATUS expand_def_name(Uast_expr** new_expr, Name* name, Pos dest_pos) {
    Name new_name = {0};
    switch (expand_def_name_internal(new_expr, &new_name, *name, dest_pos)) {
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
    if (!expand_def_ulang_type(&def->lang_type, def->pos /* TODO */)) {
        return false;
    }
    Uast_expr* new_expr = NULL;
    switch (expand_def_name(&new_expr, &def->name, def->pos /* TODO */)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            // TODO
            todo();
        case EXPAND_NAME_ERROR:
            todo();
    }
    unreachable("");
}

static bool expand_def_case(Uast_case* lang_case) {
    return expand_def_expr(&lang_case->expr, lang_case->expr) && expand_def_stmt(&lang_case->if_true, lang_case->if_true);
}

static bool expand_def_function_call(Uast_function_call* call) {
    return expand_def_expr_vec(&call->args) && expand_def_expr(&call->callee, call->callee);
}

static bool expand_def_struct_literal(Uast_struct_literal* lit) {
    return expand_def_expr_vec(&lit->members);
}

static bool expand_def_binary(Uast_binary* bin) {
    return expand_def_expr(&bin->lhs, bin->lhs) && expand_def_expr(&bin->rhs, bin->rhs);
}

static bool expand_def_unary(Uast_unary* unary) {
    return expand_def_expr(&unary->child, unary->child) && expand_def_ulang_type(&unary->lang_type, unary->pos /* TODO */);
}

static bool expand_def_condition(Uast_condition* cond) {
    return expand_def_operator(cond->child);
}

static bool expand_def_if(Uast_if* lang_if) {
    return expand_def_condition(lang_if->condition) && expand_def_block(lang_if->body);
}

static bool expand_def_member_access(Uast_member_access* access) {
    if (!expand_def_expr(&access->callee, access->callee)) {
        return false;
    }

    Uast_expr* dummy = NULL;
    switch (expand_def_symbol(&dummy, access->member_name)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            // TODO: this may not be right, but this switch will be refactored in the 
            //   future anyway when changing access->member_name type
            return true;
        case EXPAND_NAME_ERROR:
            return false;
    }
    unreachable("");
}

static bool expand_def_index(Uast_index* index) {
    bool status = expand_def_expr(&index->callee, index->callee);
    return expand_def_expr(&index->index, index->index) && status;
}

bool expand_def_operator(Uast_operator* oper) {
    switch (oper->type) {
        case UAST_BINARY:
            return expand_def_binary(uast_binary_unwrap(oper));
        case UAST_UNARY:
            return expand_def_unary(uast_unary_unwrap(oper));
    }
    unreachable("");
}

static bool expand_def_array_literal(Uast_array_literal* lit) {
    return expand_def_expr_vec(&lit->members);
}

static bool expand_def_struct_def_base(Ustruct_def_base* base, Pos dest_pos) {
    if (!expand_def_generic_param_vec(&base->generics) || !expand_def_variable_def_vec(&base->members)) {
        return false;
    }
    Uast_expr* new_expr = NULL;
    switch (expand_def_name(&new_expr, &base->name, dest_pos)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            todo();
        case EXPAND_NAME_ERROR:
            todo();
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
        case UAST_CHAR:
            return true;
    }
    unreachable("");
}

EXPAND_NAME_STATUS expand_def_symbol(Uast_expr** new_expr, Uast_symbol* sym) {
    return expand_def_name(new_expr, &sym->name, sym->pos);
}

bool expand_def_expr(Uast_expr** new_expr, Uast_expr* expr) {
    switch (expr->type) {
        case UAST_BLOCK:
            *new_expr = expr;
            return expand_def_block(uast_block_unwrap(expr));
        case UAST_IF_ELSE_CHAIN:
            *new_expr = expr;
            return expand_def_if_else_chain(uast_if_else_chain_unwrap(expr));
        case UAST_SWITCH:
            *new_expr = expr;
            return expand_def_switch(uast_switch_unwrap(expr));
        case UAST_UNKNOWN:
            *new_expr = expr;
            return true;
        case UAST_OPERATOR:
            *new_expr = expr;
            return expand_def_operator(uast_operator_unwrap(expr));
        case UAST_SYMBOL: {
            switch (expand_def_symbol(new_expr, uast_symbol_unwrap(expr))) {
                case EXPAND_NAME_NORMAL:
                    *new_expr = expr;
                    return true;
                case EXPAND_NAME_NEW_EXPR:
                    return true;
                case EXPAND_NAME_ERROR:
                    return false;
                default:
                    unreachable("");
            }
        }
        case UAST_MEMBER_ACCESS:
            *new_expr = expr;
            return expand_def_member_access(uast_member_access_unwrap(expr));
        case UAST_INDEX:
            *new_expr = expr;
            return expand_def_index(uast_index_unwrap(expr));
        case UAST_LITERAL:
            *new_expr = expr;
            return expand_def_literal(uast_literal_unwrap(expr));
        case UAST_FUNCTION_CALL:
            *new_expr = expr;
            return expand_def_function_call(uast_function_call_unwrap(expr));
        case UAST_STRUCT_LITERAL:
            *new_expr = expr;
            return expand_def_struct_literal(uast_struct_literal_unwrap(expr));
        case UAST_ARRAY_LITERAL:
            *new_expr = expr;
            return expand_def_array_literal(uast_array_literal_unwrap(expr));
        case UAST_TUPLE:
            todo();
        case UAST_MACRO:
            // TODO
            return true;
        case UAST_ENUM_ACCESS:
            todo();
        case UAST_ENUM_GET_TAG:
            todo();
    }
    unreachable("");
}

static bool expand_def_return(Uast_return* rtn) {
    return expand_def_expr(&rtn->child, rtn->child);
}

bool expand_def_defer(Uast_defer* lang_defer) {
    return expand_def_stmt(&lang_defer->child, lang_defer->child);
}

static bool expand_def_yield(Uast_yield* yield) {
    return (!yield->do_yield_expr || expand_def_expr(&yield->yield_expr, yield->yield_expr));
    // TODO: does yield->break_out_of need to be expanded?
}

static bool expand_def_assignment(Uast_assignment* assign) {
    bool status = expand_def_expr(&assign->lhs, assign->lhs);
    return expand_def_expr(&assign->rhs, assign->rhs) && status;
}

static bool expand_def_continue(Uast_continue* cont) {
    (void) cont;
    return true;
}

static bool expand_def_for_with_cond(Uast_for_with_cond* lang_for) {
    bool status = expand_def_condition(lang_for->condition);
    return expand_def_block(lang_for->body) && status;
}

bool expand_def_stmt(Uast_stmt** new_stmt, Uast_stmt* stmt) {
    switch (stmt->type) {
        case UAST_EXPR: {
            Uast_expr* new_expr = NULL;
            if (!expand_def_expr(&new_expr, uast_expr_unwrap(stmt))) {
                return false;
            }
            *new_stmt = uast_expr_wrap(new_expr);
            return true;
        }
        case UAST_DEF:
            return expand_def_def(uast_def_unwrap(stmt));
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
        case UAST_USING:
            todo();
    }
    unreachable("");
}

static bool expand_def_lang_def(Uast_lang_def* def) {
    log(LOG_DEBUG, FMT"\n", uast_lang_def_print(def));
    (void) def;
    // TODO
    return true;
}

static bool expand_def_generic_param(Uast_generic_param* param) {
    (void) param;
    return true;
}

static bool expand_def_param(Uast_param* param) {
    bool status = true;
    
    if (!expand_def_variable_def(param->base)) {
        status = false;
    }
    if (param->is_optional && !expand_def_expr(&param->optional_default, param->optional_default)) {
        status = false;
    }

    return status;
}

bool expand_def_generic_param_vec(Uast_generic_param_vec* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->info.count; idx++) {
        status = expand_def_generic_param(vec_at(params, idx)) && status;
    }
    return status;
}

bool expand_def_variable_def_vec(Uast_variable_def_vec* defs) {
    bool status = true;
    for (size_t idx = 0; idx < defs->info.count; idx++) {
        status = expand_def_variable_def(vec_at(defs, idx)) && status;
    }
    return status;
}

bool expand_def_expr_vec(Uast_expr_vec* exprs) {
    bool status = true;
    for (size_t idx = 0; idx < exprs->info.count; idx++) {
        status = expand_def_expr(vec_at_ref(exprs, idx), vec_at(exprs, idx)) && status;
    }
    return status;
}

bool expand_def_case_vec(Uast_case_vec* cases) {
    bool status = true;
    for (size_t idx = 0; idx < cases->info.count; idx++) {
        if (!expand_def_case(vec_at(cases, idx))) {
            status = false;
        }
    }
    return status;
}

bool expand_def_if_vec(Uast_if_vec* ifs) {
    bool status = true;
    for (size_t idx = 0; idx < ifs->info.count; idx++) {
        if (!expand_def_if(vec_at(ifs, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_params(Uast_function_params* params) {
    bool status = true;
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        if (!expand_def_param(vec_at(&params->params, idx))) {
            status = false;
        }
    }
    return status;
}

static bool expand_def_function_decl(Uast_function_decl* def) {
    if ( 
        !expand_def_generic_param_vec(&def->generics) ||
        !expand_def_function_params(def->params) ||
        !expand_def_ulang_type(&def->return_type, def->pos)
    ) {
        return false;
    }

    Uast_expr* new_expr = NULL;
    switch (expand_def_name(&new_expr, &def->name, def->pos)) {
        case EXPAND_NAME_NORMAL:
            return true;
        case EXPAND_NAME_NEW_EXPR:
            todo();
        case EXPAND_NAME_ERROR:
            todo();
    }
    unreachable("");
}

bool expand_def_function_def(Uast_function_def* def) {
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
            return expand_def_generic_param(uast_generic_param_unwrap(def));
        case UAST_FUNCTION_DEF:
            return expand_def_function_def(uast_function_def_unwrap(def));
        case UAST_VARIABLE_DEF:
            return expand_def_variable_def(uast_variable_def_unwrap(def));
        case UAST_STRUCT_DEF:
            return expand_def_struct_def(uast_struct_def_unwrap(def));
        case UAST_RAW_UNION_DEF:
            return expand_def_raw_union_def(uast_raw_union_def_unwrap(def));
        case UAST_ENUM_DEF:
            return expand_def_enum_def(uast_enum_def_unwrap(def));
        case UAST_PRIMITIVE_DEF:
            // TODO
            return true;
        case UAST_FUNCTION_DECL:
            return expand_def_function_decl(uast_function_decl_unwrap(def));
        case UAST_LANG_DEF:
            return expand_def_lang_def(uast_lang_def_unwrap(def));
        case UAST_VOID_DEF:
            return true;
        case UAST_LABEL:
            return true;
    }
    unreachable("");
}

bool expand_def_switch(Uast_switch* lang_switch) {
    bool status = expand_def_expr(&lang_switch->operand, lang_switch->operand);
    return expand_def_case_vec(&lang_switch->cases) && status;
}

bool expand_def_if_else_chain(Uast_if_else_chain* if_else) {
    return expand_def_if_vec(&if_else->uasts);
}

bool expand_def_block(Uast_block* block) {
    bool status = true;

    Usymbol_iter iter = usym_tbl_iter_new(block->scope_id);
    Uast_def* curr = NULL;
    while (usym_tbl_iter_next(&curr, &iter)) {
        status = expand_def_def(curr) && status;
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        status = expand_def_stmt(vec_at_ref(&block->children, idx), vec_at(&block->children, idx)) && status;
    }

    return status;
}

