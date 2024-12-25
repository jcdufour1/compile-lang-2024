#include <node.h>
#include <nodes.h>
#include <llvm.h>
#include <llvms.h>
#include <symbol_table.h>
#include <parser_utils.h>
#include <llvms.h>
#include "passes.h"

static Llvm_expr* id_expr(Llvm_expr* expr);
static Llvm_block* id_block(Llvm_block* block);

static Llvm_id llvm_id_new(void) {
    static Llvm_id llvm_id_for_next_var = 1;
    Llvm_id id_to_rtn = llvm_id_for_next_var;
    llvm_id_for_next_var += 2;
    return id_to_rtn;
}

static Llvm_unary* id_unary(Llvm_unary* unary) {
    unary->child = id_expr(unary->child);
    unary->llvm_id = llvm_id_new();
    return unary;
}
   

static Llvm_binary* id_binary(Llvm_binary* binary) {
    binary->lhs = id_expr(binary->lhs);
    binary->rhs = id_expr(binary->rhs);
    binary->llvm_id = llvm_id_new();
    return binary;
}

static Llvm_operator* id_operator(Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_UNARY:
            return llvm_wrap_unary(id_unary(llvm_unwrap_unary(operator)));
        case LLVM_BINARY:
            return llvm_wrap_binary(id_binary(llvm_unwrap_binary(operator)));
    }
    unreachable("");
}

static Llvm_function_call* id_function_call(Llvm_function_call* fun_call) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Llvm_expr** expr = vec_at_ref(&fun_call->args, idx);
        *expr = id_expr(*expr);
    }

    fun_call->llvm_id = llvm_id_new();

    return fun_call;
}

static Llvm_expr* id_expr(Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_MEMBER_ACCESS_TYPED:
            llvm_unwrap_member_access_typed(expr)->llvm_id = llvm_id_new();
            return expr;
        case LLVM_INDEX_TYPED:
            llvm_unwrap_index_typed(expr)->llvm_id = llvm_id_new();
            return expr;
        case LLVM_STRUCT_LITERAL:
            return expr;
        case LLVM_SYMBOL_TYPED:
            return expr;
        case LLVM_LITERAL:
            return expr;
        case LLVM_FUNCTION_CALL:
            return llvm_wrap_function_call(id_function_call(llvm_unwrap_function_call(expr)));
        case LLVM_OPERATOR:
            return llvm_wrap_operator(id_operator(llvm_unwrap_operator(expr)));
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
    fun_decl->parameters = id_function_params(fun_decl->parameters);
    return fun_decl;
}

static Llvm_function_def* id_function_def(Llvm_function_def* fun_def) {
    fun_def->llvm_id = llvm_id_new();
    fun_def->declaration = id_function_decl(fun_def->declaration);
    fun_def->body = id_block(fun_def->body);
    return fun_def;
}

static Llvm_def* id_def(Llvm_def* def) {
    switch (def->type) {
        case LLVM_VARIABLE_DEF:
            return llvm_wrap_variable_def(id_variable_def(llvm_unwrap_variable_def(def)));
        case LLVM_LABEL:
            llvm_unwrap_label(def)->llvm_id = llvm_id_new();
            return def;
        case LLVM_STRUCT_DEF:
            return def;
        case LLVM_RAW_UNION_DEF:
            return def;
        case LLVM_ENUM_DEF:
            return def;
        case LLVM_FUNCTION_DECL:
            return def;
        case LLVM_FUNCTION_DEF:
            return llvm_wrap_function_def(id_function_def(llvm_unwrap_function_def(def)));
        case LLVM_PRIMITIVE_DEF:
            return def;
        case LLVM_LITERAL_DEF:
            return def;
    }
    unreachable("");
}

static Llvm* id_llvm(Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_EXPR:
            return llvm_wrap_expr(id_expr(llvm_unwrap_expr(llvm)));
        case LLVM_DEF:
            return llvm_wrap_def(id_def(llvm_unwrap_def(llvm)));
        case LLVM_FUNCTION_PARAMS:
            return llvm;
        case LLVM_BLOCK:
            return llvm_wrap_block(id_block(llvm_unwrap_block(llvm)));
        case LLVM_RETURN:
            return llvm;
        case LLVM_LANG_TYPE:
            return llvm;
        case LLVM_LOAD_ELEMENT_PTR:
            llvm_unwrap_load_element_ptr(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_unwrap_load_another_llvm(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_unwrap_store_another_llvm(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_ALLOCA:
            llvm_unwrap_alloca(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_GOTO:
            llvm_unwrap_goto(llvm)->llvm_id = llvm_id_new();
            return llvm;
        case LLVM_COND_GOTO:
            llvm_unwrap_cond_goto(llvm)->llvm_id = llvm_id_new();
            return llvm;
    }
    unreachable("");
}

static Llvm_block* id_block(Llvm_block* block) {
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Llvm** curr = vec_at_ref(&block->children, idx);
        *curr = id_llvm(*curr);
    }

    return block;
}

Llvm_block* assign_llvm_ids(Env* env, Llvm_block* root) {
    (void) env;
    return id_block(root);
}
