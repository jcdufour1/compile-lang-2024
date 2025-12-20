#ifndef DID_YOU_MEAN_H
#define DID_YOU_MEAN_H

#include <strv.h>
#include <strv_darr.h>
#include <name.h>

Strv did_you_mean_symbol_print_internal(Name sym_name);
#define did_you_mean_symbol_print(sym_name) strv_print(did_you_mean_symbol_print_internal(sym_name))

Strv did_you_mean_type_print_internal(Name sym_name);
#define did_you_mean_type_print(sym_name) strv_print(did_you_mean_type_print_internal(sym_name))

Strv did_you_mean_strv_choice_print_internal(Strv sym_strv, Strv_darr candidates);
#define did_you_mean_strv_choice_print(sym_strv, candidates) strv_print(did_you_mean_strv_choice_print_internal(sym_strv, candidates))

#endif // DID_YOU_MEAN_H
