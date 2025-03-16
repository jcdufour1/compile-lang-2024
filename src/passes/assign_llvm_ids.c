#include <tast.h>
#include <tasts.h>
#include <llvm.h>
#include <llvms.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <llvms.h>
#include "passes.h"

static Llvm_expr* id_expr(Llvm_expr* expr);
static Llvm_block* id_block(Env* env, Llvm_block* block);

static Llvm_id llvm_id_new(void) {
    static Llvm_id llvm_id_for_next_var = 1;
    Llvm_id id_to_rtn = llvm_id_for_next_var;
    llvm_id_for_next_var += 2;
    return id_to_rtn;
}

static Llvm_unary* id_unary(Llvm_unary* unary) {
    unary->llvm_id = llvm_id_new();
    return unary;
}
   

static Llvm_binary* id_binary(Llvm_binary* binary) {
    binary->llvm_id = llvm_id_new();
    return binary;
}

static Llvm_operator* id_operator(Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_UNARY:
            return llvm_unary_wrap(id_unary(llvm_unary_unwrap(operator)));
        case LLVM_BINARY:
            return llvm_binary_wrap(id_binary(llvm_binary_unwrap(operator)));
    }
    unreachable("");
}

static Llvm_function_call* id_function_call(Llvm_function_call* fun_call) {
    fun_call->llvm_id = llvm_id_new();
    return fun_call;
}

static Llvm_expr* id_expr(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_SYMBOL:
            return expr;
        case LLVM_LITERAL:
            return expr;
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_wrap(id_function_call(llvm_function_call_unwrap(expr)));
        case LLVM_OPERATOR:
            return llvm_operator_wrap(id_operator(llvm_operator_unwrap(expr)));
        case LLVM_LLVM_PLACEHOLDER:
            return expr;
    }
    unreachable("");
}

static Llvm_variable_def* id_variable_def(Llvm_variable_def* def) {
    def->llvm_id = llvm_id_new();
    return def;
}

static Llvm_function_params* id_function_params(Llvm_function_params* params) {
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        Llvm_variable_def** curr = vec_at_ref(&params->params, idx);
        *curr = id_variable_def(*curr);
    }

    return params;
}

static Llvm_function_decl* id_function_decl(Llvm_function_decl* fun_decl) {
    fun_decl->params = id_function_params(fun_decl->params);
    return fun_decl;
}

static Llvm_function_def* id_function_def(Env* env, Llvm_function_def* fun_def) {
    fun_def->llvm_id = llvm_id_new();
    fun_def->decl = id_function_decl(fun_def->decl);
    fun_def->body = id_block(env, fun_def->body);
    return fun_def;
}

static Llvm_def* id_def(Env* env, Llvm_def* def) {
    switch (def->type) {
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_wrap(id_variable_def(llvm_variable_def_unwrap(def)));
        case LLVM_LABEL:
            llvm_label_unwrap(def)->llvm_id = llvm_id_new();
            return def;
        case LLVM_STRUCT_DEF:
            return def;
        case LLVM_RAW_UNION_DEF:
            return def;
        case LLVM_ENUM_DEF:
            return def;
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_wrap(id_function_decl(llvm_function_decl_unwrap(def)));
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_wrap(id_function_def(env, llvm_function_def_unwrap(def)));
        case LLVM_PRIMITIVE_DEF:
            return def;
        case LLVM_LITERAL_DEF:
            return def;
    }
    unreachable("");
}

static Llvm* id_llvm(Env* env, Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_EXPR:
            return llvm_expr_wrap(id_expr(llvm_expr_unwrap(llvm)));
        case LLVM_DEF:
            return llvm_def_wrap(id_def(env, llvm_def_unwrap(llvm)));
        case LLVM_FUNCTION_PARAMS:
            return llvm;
        case LLVM_BLOCK:
            return llvm_block_wrap(id_block(env, llvm_block_unwrap(llvm)));
        case LLVM_RETURN:
            return llvm;
        case LLVM_LOAD_ELEMENT_PTR:
            llvm_load_element_ptr_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_load_another_llvm_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_store_another_llvm_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_ALLOCA:
            llvm_alloca_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_GOTO:
            llvm_goto_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_COND_GOTO:
            llvm_cond_goto_unwrap(llvm)->llvm_id = llvm_id_new();
            return llvm;
    }
    unreachable("");
}

// only for alloca_table, etc.
static Llvm_def* id_def_sometimes(Env* env, Llvm_def* def) {
    switch (def->type) {
        case LLVM_VARIABLE_DEF:
            return def;
        case LLVM_LABEL:
            return def;
        case LLVM_STRUCT_DEF:
            return def;
        case LLVM_RAW_UNION_DEF:
            return def;
        case LLVM_ENUM_DEF:
            return def;
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_wrap(id_function_decl(llvm_function_decl_unwrap(def)));
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_wrap(id_function_def(env, llvm_function_def_unwrap(def)));
        case LLVM_PRIMITIVE_DEF:
            return def;
        case LLVM_LITERAL_DEF:
            return def;
    }
    unreachable("");
}

// only for alloca_table, etc.
static Llvm* id_llvm_sometimes(Env* env, Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_EXPR:
            return llvm;
        case LLVM_DEF:
            return llvm_def_wrap(id_def_sometimes(env, llvm_def_unwrap(llvm)));
        case LLVM_FUNCTION_PARAMS:
            return llvm;
        case LLVM_BLOCK:
            return llvm;
        case LLVM_RETURN:
            return llvm;
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm;
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm;
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm;
        case LLVM_ALLOCA:
            return llvm;
        case LLVM_GOTO:
            return llvm;
        case LLVM_COND_GOTO:
            return llvm;
    }
    unreachable("");
}

static Llvm_block* id_block(Env* env, Llvm_block* block) {
    vec_append(&a_main, &env->ancesters, &block->symbol_collection);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Llvm** curr = vec_at_ref(&block->children, idx);
        *curr = id_llvm(env, *curr);
    }

    Alloca_table table = vec_top(&env->ancesters)->alloca_table;
    for (size_t idx = 0; idx < table.capacity; idx++) {
        if (table.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }

        // TODO: come up with a better way to only id correct things
        Llvm* new_llvm = id_llvm_sometimes(env, table.table_tasts[idx].tast);
        alloca_update(env, new_llvm);
    }

    vec_rem_last(&env->ancesters);
    return block;
}

Llvm_block* assign_llvm_ids(Env* env, Llvm_block* root) {
    return id_block(env, root);
}
