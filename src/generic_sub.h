#ifndef GENERIC_SUB_H
#define GENERIC_SUB_H

#include <uast.h>
#include <ulang_type.h>

void generic_sub_param(Uast_param* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_variable_def(Uast_variable_def* def, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_stmt(Uast_stmt* stmt, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_block(Uast_block* block, Str_view gen_param, Ulang_type gen_arg);

void generic_sub_struct_def_base(Ustruct_def_base* base, Str_view gen_param, Ulang_type gen_arg);

#endif // GENERIC_SUB_H
