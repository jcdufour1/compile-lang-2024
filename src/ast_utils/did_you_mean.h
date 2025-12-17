#ifndef DID_YOU_MEAN_H
#define DID_YOU_MEAN_H

#include <strv.h>
#include <name.h>

Strv did_you_mean_symbol_print_internal(Name sym_name);

#define did_you_mean_symbol_print(sym_name) strv_print(did_you_mean_symbol_print_internal(sym_name))

#endif // DID_YOU_MEAN_H
