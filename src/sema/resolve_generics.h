#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

#define msg_invalid_count_generic_args(pos_def, pos_a_genrgs, a_genrgs_count, min_args, max_args) \
    msg_invalid_count_generic_args_internal(__FILE__, __LINE__,  pos_def, pos_a_genrgs, a_genrgs_count, min_args, max_args)

void msg_invalid_count_generic_args_internal(
    const char* file,
    int line,
    Pos pos_def,
    Pos pos_a_genrgs,
    size_t a_genrgs_count,
    size_t min_args,
    size_t max_args
);

bool resolve_generics_ulang_type_regular(LANG_TYPE_TYPE* type, Ulang_type* result, Ulang_type_regular lang_type);

bool resolve_generics_struct_like_def_implementation(Name name);

bool resolve_generics_function_def_call(
    Lang_type_fn* rtn_type,
    Name* new_name,
    Uast_function_def* def,
    Ulang_type_vec a_genrgs,
    Pos pos_a_genrgs
);

bool resolve_generics_function_def_implementation(Name name);

#endif // RESOLVE_GENERICS_H
