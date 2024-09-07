#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"

Str_view literal_name_new(void);

Llvm_id get_block_return_id(Node* fun_call);

Llvm_id get_prev_load_id(Node* var_call);

Llvm_id get_store_dest_id(Node* var_call);

Node* get_symbol_def_from_alloca(Node* alloca);

Llvm_id get_matching_label_id(Node* symbol_call);

#endif // PARSER_UTIL_H
