#ifndef ERROR_MSG_H
#define ERROR_MSG_H

#include "node.h"

void msg_redefinition_of_symbol(const Node* new_sym_def);

void msg_undefined_symbol(const Node* sym_call);

void msg_invalid_struct_member(const Node* node);

void msg_undefined_function(const Node* fun_call);

void msg_invalid_struct_member_assignment_in_literal(
    const Node* struct_var_def,
    const Node* memb_sym_def,
    const Node* memb_sym
);

void meg_struct_assigned_to_invalid_literal(const Node* lhs, const Node* rhs);

void msg_invalid_assignment_to_literal(const Node* lhs, const Node* rhs);

#endif // ERROR_MSG_H
