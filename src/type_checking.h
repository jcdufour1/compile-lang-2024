#ifndef TYPE_CHECKING_H
#define TYPE_CHECKING_H

#include <stddef.h>
#include <util.h>
#include <parser_utils.h>

bool try_set_assignment_types(Env* env, Node_assignment* assignment);

// returns false if unsuccessful
bool try_set_expr_types(Env* env, Node_expr** new_node, Node_expr* expr);

// returns false if unsuccessful
bool try_set_binary_types(Env* env, Node_expr** new_node, Node_binary* operator);

bool try_set_block_types(Env* env, Node_block** new_node, Node_block* node, bool is_directly_in_fun_def);

bool try_set_node_types(Env* env, Node** new_node, Node* node);

// returns false if unsuccessful
bool try_set_binary_operand_types(Node_expr* operand);

bool try_set_unary_types(Env* env, Node_expr** new_node, Node_unary* unary);

bool try_set_operation_types(const Env* env, Node** new_node, Node_operator* operator);

void try_set_literal_types(Node_literal* literal, TOKEN_TYPE token_type);

bool try_set_function_call_types(Env* env, Node_expr** new_node, Node_function_call* fun_call);

bool try_set_member_access_types(Env* env, Node** new_node, Node_member_access_untyped* access);

bool try_set_function_def_types(
    Env* env,
    Node_function_def** new_node,
    Node_function_def* old_decl
);

bool try_set_function_decl_types(
    Env* env,
    Node_function_decl** new_node,
    Node_function_decl* old_decl
);

bool try_set_index_untyped_types(Env* env, Node** new_node, Node_index_untyped* index);

bool try_set_function_params_types(
    Env* env,
    Node_function_params** new_node,
    Node_function_params* old_def
);

bool try_set_lang_type_types(
    Env* env,
    Node_lang_type** new_node,
    Node_lang_type* old_def
);

#endif // TYPE_CHECKING_H
