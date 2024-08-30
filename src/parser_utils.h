#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"

Str_view literal_name_new(void);

Llvm_id get_block_return_id(Node_id fun_call);

Llvm_id get_prev_load_id(Node_id var_call);

Llvm_id get_store_dest_id(Node_id var_call);

Node_id get_symbol_def_from_alloca(Node_id alloca);

#endif // PARSER_UTIL_H
