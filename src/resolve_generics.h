#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

bool resolve_generics_ulang_type_reg_generic(Ulang_type* result, Env* env, Ulang_type_reg_generic lang_type);

bool resolve_generics_ulang_type(Ulang_type* result, Env* env, Ulang_type lang_type, Pos pos);

bool resolve_generics_ulang_type_regular(Ulang_type* result, Env* env, Ulang_type_regular lang_type, Pos pos);

#endif // RESOLVE_GENERICS_H
