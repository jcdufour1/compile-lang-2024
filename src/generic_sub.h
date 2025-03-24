#ifndef GENERIC_SUB_H
#define GENERIC_SUB_H

#include <uast.h>
#include <ulang_type.h>

void generic_sub_param(Env* env, Uast_param* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_variable_def(Env* env, Uast_variable_def* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_stmt(Env* env, Uast_stmt* stmt, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_block(Env* env, Uast_block* block, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_struct_def_base(Env* env, Ustruct_def_base* base, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_lang_type(
    Env* env,
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Str_view gen_param,
    Ulang_type gen_arg
);

void generic_sub_symbol(Env* env, Uast_symbol* sym, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_expr(Env* env, Uast_expr* expr, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_operator(Env* env, Uast_operator* operator, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_binary(Env* env, Uast_binary* oper, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_unary(Env* env, Uast_unary* unary, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_assignment(Env* env, Uast_assignment* assign, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_literal(Uast_literal* lit, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_for_with_cond(Env* env, Uast_for_with_cond* lang_for, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_condition(Env* env, Uast_condition* cond, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_if_else_chain(Env* env, Uast_if_else_chain* if_else, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_function_call(Env* env, Uast_function_call* fun_call, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_switch(Env* env, Uast_switch* lang_switch, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_case(Env* env, Uast_case* lang_case, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_member_access(Env* env, Uast_member_access* access, Str_view gen_param, Ulang_type gen_arg);

#endif // GENERIC_SUB_H
