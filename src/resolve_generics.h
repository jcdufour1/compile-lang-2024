#ifndef RESOLVE_GENERICS_H
#define RESOLVE_GENERICS_H

#include <util.h>
#include <env.h>
#include <uast.h>

Ulang_type resolve_generics_ulang_type_reg_generic(Env* env, Ulang_type_reg_generic lang_type);

Ulang_type resolve_generics_ulang_type(Env* env, Ulang_type lang_type);

Ulang_type resolve_generics_ulang_type_regular(Env* env, Ulang_type_regular lang_type);

#endif // RESOLVE_GENERICS_H
