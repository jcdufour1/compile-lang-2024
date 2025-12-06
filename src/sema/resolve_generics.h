#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

#define msg_invalid_count_generic_args(pos_def, pos_gen_args, gen_args_count, min_args, max_args) \
    msg_invalid_count_generic_args_internal(__FILE__, __LINE__,  pos_def, pos_gen_args, gen_args_count, min_args, max_args)

static void msg_invalid_count_generic_args_internal(
    const char* file,
    int line,
    Pos pos_def,
    Pos pos_gen_args,
    Ulang_type_vec gen_args,
    size_t min_args,
    size_t max_args
);

bool resolve_generics_ulang_type_regular(LANG_TYPE_TYPE* type, Ulang_type* result, Ulang_type_regular lang_type);

bool resolve_generics_struct_like_def_implementation(Name name);

bool resolve_generics_function_def_call(
    Lang_type_fn* rtn_type,
    Name* new_name,
    Uast_function_def* def,
    Ulang_type_vec gen_args,
    Pos pos_gen_args
);

bool resolve_generics_function_def_implementation(Name name);

#endif // RESOLVE_GENERICS_H
