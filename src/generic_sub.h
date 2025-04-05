#ifndef GENERIC_SUB_H
#define GENERIC_SUB_H

#include <uast.h>
#include <ulang_type.h>

void generic_sub_param(Env* env, Uast_param* def, Name gen_param, Ulang_type gen_arg);

void generic_sub_variable_def(Env* env, Uast_variable_def* def, Name gen_param, Ulang_type gen_arg);

void generic_sub_stmt(Env* env, Uast_stmt* stmt, Name gen_param, Ulang_type gen_arg);

void generic_sub_block(Env* env, Uast_block* block, Name gen_param, Ulang_type gen_arg);

void generic_sub_struct_def_base(Env* env, Ustruct_def_base* base, Name gen_param, Ulang_type gen_arg);

void generic_sub_lang_type(
    Env* env,
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Name gen_param,
    Ulang_type gen_arg
);

void generic_sub_symbol(Env* env, Uast_symbol* sym, Name gen_param, Ulang_type gen_arg);

void generic_sub_expr(Env* env, Uast_expr* expr, Name gen_param, Ulang_type gen_arg);

void generic_sub_operator(Env* env, Uast_operator* operator, Name gen_param, Ulang_type gen_arg);

void generic_sub_binary(Env* env, Uast_binary* oper, Name gen_param, Ulang_type gen_arg);

void generic_sub_unary(Env* env, Uast_unary* unary, Name gen_param, Ulang_type gen_arg);

void generic_sub_assignment(Env* env, Uast_assignment* assign, Name gen_param, Ulang_type gen_arg);

void generic_sub_literal(Uast_literal* lit, Name gen_param, Ulang_type gen_arg);

void generic_sub_for_with_cond(Env* env, Uast_for_with_cond* lang_for, Name gen_param, Ulang_type gen_arg);

void generic_sub_condition(Env* env, Uast_condition* cond, Name gen_param, Ulang_type gen_arg);

void generic_sub_if_else_chain(Env* env, Uast_if_else_chain* if_else, Name gen_param, Ulang_type gen_arg);

void generic_sub_function_call(Env* env, Uast_function_call* fun_call, Name gen_param, Ulang_type gen_arg);

void generic_sub_switch(Env* env, Uast_switch* lang_switch, Name gen_param, Ulang_type gen_arg);

void generic_sub_case(Env* env, Uast_case* lang_case, Name gen_param, Ulang_type gen_arg);

void generic_sub_member_access(Env* env, Uast_member_access* access, Name gen_param, Ulang_type gen_arg);

#endif // GENERIC_SUB_H
