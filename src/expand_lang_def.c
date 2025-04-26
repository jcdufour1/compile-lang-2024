#include <symbol_iter.h>
#include <uast.h>
#include <ulang_type.h>
#include <lang_type.h>
#include <ulang_type_clone.h>
#include <extend_name.h>
#include <serialize_module_symbol_name.h>
#include <expand_lang_def.h>

static bool expand_def_ulang_type_regular(Ulang_type_regular* new_lang_type, Ulang_type_regular lang_type) {
    bool status = false;
    *new_lang_type = ulang_type_regular_clone(lang_type);
    status = expand_def_uname(&new_lang_type->atom.str);
    *new_lang_type = lang_type;
    return status;
}

static bool expand_def_ulang_type(Ulang_type* lang_type) {
    switch (lang_type->type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_regular new_lang_type = {0};
            if (!expand_def_ulang_type_regular(&new_lang_type, ulang_type_regular_const_unwrap(*lang_type))) {
                todo();
                return false;
            }
            *lang_type = ulang_type_regular_const_wrap(new_lang_type);
            return true;
        }
        case ULANG_TYPE_FN:
            todo();
        case ULANG_TYPE_TUPLE:
            todo();
    }
    unreachable("");
}

bool expand_def_uname(Uname* name) {
    Name actual = {0};
    if (!name_from_uname(&actual, *name)) {
        todo();
        return false;
    }
    Uast_def* result = NULL;
    if (!usymbol_lookup(&result, actual)) {
        return true;
    }

    switch (result->type) {
        case UAST_POISON_DEF:
            todo();
            return false;
        case UAST_LANG_DEF:
            log(LOG_DEBUG, TAST_FMT"\n", uname_print(*name));
            log(LOG_DEBUG, TAST_FMT"\n", name_print(actual));
            todo();
        case UAST_IMPORT_PATH:
            return true;
        case UAST_MOD_ALIAS:
            // TODO: expand mod_alias here?
            return true;
        case UAST_GENERIC_PARAM:
            return true;
        case UAST_FUNCTION_DEF:
            return true;
        case UAST_VARIABLE_DEF:
            return true;
        case UAST_STRUCT_DEF:
            return true;
        case UAST_RAW_UNION_DEF:
            return true;
        case UAST_ENUM_DEF:
            return true;
        case UAST_SUM_DEF:
            return true;
        case UAST_PRIMITIVE_DEF:
            return true;
        case UAST_LITERAL_DEF:
            return true;
        case UAST_FUNCTION_DECL:
            return true;
    }
    unreachable("");
}

// TODO: expected failure case for having generic parameters in def definition
static bool expand_def_name(Name* name) {
    Uast_def* def = NULL;
    Ulang_type_vec gen_args = name->gen_args;
    memset(&name->gen_args, 0, sizeof(name->gen_args));
    if (!usymbol_lookup(&def, *name)) {
        return true;
    }

    switch (def->type) {
        case UAST_POISON_DEF:
            return false;
        case UAST_LANG_DEF: {
            Uast_symbol* sym = uast_symbol_unwrap(uast_lang_def_unwrap(def)->expr);
            *name = sym->name;
            unwrap(name->gen_args.info.count == 0 && "not implemented");
            name->gen_args = gen_args;
            return true;
        }
        case UAST_IMPORT_PATH:
            return true;
        case UAST_MOD_ALIAS:
            // TODO: expand mod_alias here?
            return true;
        case UAST_GENERIC_PARAM:
            return true;
        case UAST_FUNCTION_DEF:
            return true;
        case UAST_VARIABLE_DEF:
            return true;
        case UAST_STRUCT_DEF:
            return true;
        case UAST_RAW_UNION_DEF:
            return true;
        case UAST_ENUM_DEF:
            return true;
        case UAST_SUM_DEF:
            return true;
        case UAST_PRIMITIVE_DEF:
            return true;
        case UAST_LITERAL_DEF:
            return true;
        case UAST_FUNCTION_DECL:
            return true;
    }
    unreachable("");
}

static bool expand_def_variable_def(Uast_variable_def* def) {
    return expand_def_ulang_type(&def->lang_type) && expand_def_name(&def->name);
}

static bool expand_def_function_call(Uast_function_call* call) {
    return expand_def_expr_vec(&call->args) && expand_def_expr(call->callee);
}

static bool expand_def_struct_def_base(Ustruct_def_base* base) {
    return 
        expand_def_generic_param_vec(&base->generics) &&
        expand_def_variable_def_vec(&base->members) &&
        expand_def_name(&base->name);
    todo();
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

static bool expand_def_enum_def(Uast_enum_def* def) {
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
        // TODO: remove UAST_ENUM_LIT
        case UAST_ENUM_LIT:
            todo();
        case UAST_CHAR:
            return true;
    }
    unreachable("");
}

bool expand_def_symbol(Uast_symbol* sym) {
    bool status = true;
    status = expand_def_name(&sym->name);
    return status;
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
            return expand_def_symbol(uast_symbol_unwrap(expr));
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
    return expand_def_symbol(param->child);
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
    return 
        expand_def_generic_param_vec(&def->generics) &&
        expand_def_function_params(def->params) &&
        expand_def_name(&def->name) &&
        expand_def_ulang_type(&def->return_type);
}

static bool expand_def_function_def(Uast_function_def* def) {
    return expand_def_function_decl(def->decl) && expand_def_block(def->body);
}

static bool expand_def_mod_alias(Uast_mod_alias* alias) {
    return expand_def_name(&alias->name) && expand_def_name(&alias->mod_path);
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
        case UAST_ENUM_DEF:
            return expand_def_enum_def(uast_enum_def_unwrap(def));
        case UAST_SUM_DEF:
            return expand_def_sum_def(uast_sum_def_unwrap(def));
        case UAST_PRIMITIVE_DEF:
            todo();
        case UAST_FUNCTION_DECL:
            return expand_def_function_decl(uast_function_decl_unwrap(def));
        case UAST_LITERAL_DEF:
            todo();
        case UAST_LANG_DEF:
            return expand_def_lang_def(uast_lang_def_unwrap(def));
    }
    unreachable("");
}

bool expand_def_block(Uast_block* block) {
    bool status = true;

    Usymbol_iter iter = usym_tbl_iter_new(block->symbol_collection.usymbol_table);
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

