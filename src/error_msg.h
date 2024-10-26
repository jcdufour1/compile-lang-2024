#ifndef ERROR_MSG_H
#define ERROR_MSG_H

#include "node.h"
#include "env.h"
#include "token.h"

void msg_redefinition_of_symbol(const Env* env, const Node* new_sym_def);

void msg_parser_expected_internal(Token got, int count_expected, ...);

#define msg_parser_expected(got, ...) \
    do { \
        msg_parser_expected_internal(got, sizeof((TOKEN_TYPE[]){__VA_ARGS__})/sizeof(TOKEN_TYPE), __VA_ARGS__); \
    } while(0)

void msg_undefined_symbol(const Node* sym_call);

void msg_invalid_struct_member(const Env* env, const Node* parent, const Node* node);

void msg_undefined_function(const Node_function_call* fun_call);

void msg_invalid_struct_member_assignment_in_literal(
    const Node_variable_def* struct_var_def,
    const Node_variable_def* memb_sym_def,
    const Node_symbol_untyped* memb_sym
);

void meg_struct_assigned_to_invalid_literal(const Env* env, const Node* lhs, const Node* rhs);

void msg_invalid_assignment_to_literal(const Env* env, const Node_symbol_typed* lhs, const Node_literal* rhs);

void msg_invalid_assignment_to_operation(const Env* env, const Node* lhs, const Node_operator* operation);

void msg_mismatched_return_types(
    const Node_return_statement* rtn_statement,
    const Node_function_definition* fun_def
);

#endif // ERROR_MSG_H
