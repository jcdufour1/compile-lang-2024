#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

bool resolve_generics_ulang_type(Ulang_type* result, Ulang_type lang_type);

bool resolve_generics_ulang_type_regular(Ulang_type* result, Ulang_type_regular lang_type);

bool resolve_generics_function_def_call(
    Uast_function_def** new_def,
    Uast_function_def* def,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
);

bool resolve_generics_function_def_implementation(Name name);

#endif // RESOLVE_GENERICS_H
