#ifndef GENERIC_SUB_H
#define GENERIC_SUB_H

#include <uast.h>
#include <ulang_type.h>

void generic_sub_param(Uast_param* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_variable_def(Uast_variable_def* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_stmt(Env* env, Uast_stmt* stmt, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_block(Env* env, Uast_block* block, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_struct_def_base(Ustruct_def_base* base, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_lang_type(
    Env* env,
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
);

void generic_sub_expr(Uast_expr* expr, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_operator(Uast_operator* operator, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_binary(Uast_binary* oper, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_unary(Uast_unary* unary, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_assignment(Uast_assignment* assign, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_literal(Uast_literal* lit, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_for_with_cond(Env* env, Uast_for_with_cond* lang_for, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_condition(Uast_condition* cond, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_if_else_chain(Env* env, Uast_if_else_chain* if_else, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_function_call(Uast_function_call* fun_call, Str_view gen_param, Ulang_type gen_arg);

#endif // GENERIC_SUB_H
