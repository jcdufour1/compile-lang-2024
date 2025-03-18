#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

bool resolve_generics_ulang_type_reg_generic(Ulang_type* result, Env* env, Ulang_type_reg_generic lang_type);

bool resolve_generics_ulang_type(Ulang_type* result, Env* env, Ulang_type lang_type);

bool resolve_generics_ulang_type_regular(Ulang_type* result, Env* env, Ulang_type_regular lang_type);

bool resolve_generics_function_def(
    Uast_function_def** new_def,
    Env* env,
    Uast_function_def* def,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
);

bool function_decl_generics_are_present(const Uast_function_decl* decl);

bool variable_def_generics_are_present(const Uast_variable_def* def);

#endif // RESOLVE_GENERICS_H
