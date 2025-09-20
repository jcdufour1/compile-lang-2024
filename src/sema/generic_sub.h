#ifndef GENERIC_SUB_H
#define GENERIC_SUB_H

#include <uast.h>
#include <ulang_type.h>

void generic_sub_param(Uast_param* def, Name gen_param, Ulang_type gen_arg);

void generic_sub_variable_def(Uast_variable_def* def, Name gen_param, Ulang_type gen_arg);

void generic_sub_stmt(Uast_stmt* stmt, Name gen_param, Ulang_type gen_arg);

void generic_sub_block(Uast_block* block, Name gen_param, Ulang_type gen_arg);

void generic_sub_struct_def_base(Ustruct_def_base* base, Name gen_param, Ulang_type gen_arg);

void generic_sub_lang_type(
    Ulang_type* new_lang_type,
    Ulang_type lang_type,
    Name gen_param,
    Ulang_type gen_arg
);

void generic_sub_lang_type_tuple(
    Ulang_type_tuple* lang_type,
    Name gen_param,
    Ulang_type gen_arg
);

void generic_sub_lang_type_fn(
    Ulang_type_fn* lang_type,
    Name gen_param,
    Ulang_type gen_arg
);

void generic_sub_symbol(Uast_symbol* sym, Name gen_param, Ulang_type gen_arg);

void generic_sub_expr(Uast_expr* expr, Name gen_param, Ulang_type gen_arg);

void generic_sub_operator(Uast_operator* operator, Name gen_param, Ulang_type gen_arg);

void generic_sub_binary(Uast_binary* oper, Name gen_param, Ulang_type gen_arg);

void generic_sub_unary(Uast_unary* unary, Name gen_param, Ulang_type gen_arg);

void generic_sub_assignment(Uast_assignment* assign, Name gen_param, Ulang_type gen_arg);

void generic_sub_literal(Uast_literal* lit, Name gen_param, Ulang_type gen_arg);

void generic_sub_for_with_cond(Uast_for_with_cond* lang_for, Name gen_param, Ulang_type gen_arg);

void generic_sub_condition(Uast_condition* cond, Name gen_param, Ulang_type gen_arg);

void generic_sub_if_else_chain(Uast_if_else_chain* if_else, Name gen_param, Ulang_type gen_arg);

void generic_sub_function_call(Uast_function_call* fun_call, Name gen_param, Ulang_type gen_arg);

void generic_sub_switch(Uast_switch* lang_switch, Name gen_param, Ulang_type gen_arg);

void generic_sub_case(Uast_case* lang_case, Name gen_param, Ulang_type gen_arg);

void generic_sub_member_access(Uast_member_access* access, Name gen_param, Ulang_type gen_arg);

void generic_sub_index(Uast_index* index, Name gen_param, Ulang_type gen_arg);

void generic_sub_name(Name* name, Name gen_param, Ulang_type gen_arg);

void generic_sub_generic_param(Uast_generic_param* def, Name gen_param, Ulang_type gen_arg);

#endif // GENERIC_SUB_H
