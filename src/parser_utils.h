#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"

Str_view literal_name_new(void);

Llvm_id get_block_return_id(const Node* fun_call);

Llvm_id get_prev_load_id(const Node* var_call);

Llvm_id get_store_dest_id(const Node* var_call);

const Node* get_symbol_def_from_alloca(const Node* alloca);

Llvm_id get_matching_label_id(const Node* symbol_call);

// lhs and rhs should not be used for other tasks after this
Node* assignment_new(Node* lhs, Node* rhs);

Node* literal_new(Str_view value);

Node* symbol_new(Str_view symbol_name);

Llvm_id get_matching_fun_param_load_id(const Node* src);

#endif // PARSER_UTIL_H
