#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "node_utils.h"

Str_view literal_name_new(void);

Llvm_id get_prev_load_id(const Node* var_call);

Node* get_storage_location(Str_view sym_name);

Llvm_id get_store_dest_id(const Node* var_call);

const Node_variable_def* get_symbol_def_from_alloca(const Node* alloca);

Llvm_id get_matching_label_id(const Node* symbol_call);

// lhs and rhs should not be used for other tasks after this
Node_assignment* assignment_new(Node* lhs, Node* rhs);

Node_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos);

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos);

Node_operator* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type);

Llvm_id get_matching_fun_param_load_id(const Node* src);

const Node* get_lang_type_from_sym_definition(const Node* sym_def);

uint64_t sizeof_lang_type(Lang_type lang_type);

uint64_t sizeof_item(const Node* item);

uint64_t sizeof_struct(const Node* struct_literal);

uint64_t sizeof_struct_definition(const Node_struct_def* struct_def);

uint64_t sizeof_struct_literal(const Node_struct_literal* struct_literal);

static inline bool is_struct_variable_definition(const Node_variable_def* var_def) {
    Node* struct_def;
    return sym_tbl_lookup(&struct_def, var_def->lang_type.str);
}

static inline bool is_struct_symbol(const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL_TYPED || symbol->type == NODE_STRUCT_MEMBER_SYM_TYPED);

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, get_node_name(symbol))) {
        unreachable("");
    }
    return is_struct_variable_definition(node_unwrap_variable_def(var_def));
}

bool is_corresponding_to_a_struct(const Node* node);

static inline size_t get_member_index(const Node_struct_def* struct_def, const Node_struct_member_sym_piece_typed* member_symbol) {
    size_t idx = 0;
    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(get_node_name(node_wrap(curr_member)), get_node_name(node_wrap(member_symbol)))) {
            return idx;
        }
        idx++;
    }
    unreachable("member not found");
}

static inline bool try_get_member_def(Node_variable_def** member_def, const Node_struct_def* struct_def, const Node* member_symbol) {
    assert(
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED ||
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED
    );

    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(get_node_name(curr_member), get_node_name(member_symbol))) {
            assert(get_lang_type(curr_member).str.count > 0);
            *member_def = (Node_variable_def*)curr_member;
            return true;
        }
    }
    return false;
}

static inline const Node_variable_def* get_member_def(const Node_struct_def* struct_def, const Node* member_symbol) {
    Node_variable_def* member_def;
    if (!try_get_member_def(&member_def, struct_def, member_symbol)) {
        unreachable("could not find member definition");
    }
    return member_def;
}

bool try_get_struct_definition(Node_struct_def** struct_def, Node* node);

static inline Node_struct_def* get_struct_definition(Node* node) {
    Node_struct_def* struct_def;
    if (!try_get_struct_definition(&struct_def, node)) {
        unreachable("could not find struct definition for "NODE_FMT"\n", node_print(node));
    }
    return struct_def;
}

static inline const Node_struct_def* get_struct_definition_const(const Node* node) {
    return get_struct_definition((Node*)node);
}

bool set_assignment_operand_types(Node_assignment* assignment);

// returns false if unsuccessful
bool try_set_operator_lang_type(Node_operator* operator);

// returns false if unsuccessful
bool try_set_operator_operand_lang_type(Lang_type* result, Node* operand);

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node_symbol_untyped* sym_untyped);

void set_function_call_types(Node_function_call* fun_call);

void set_struct_member_symbol_types(Node_struct_member_sym_untyped* struct_memb_sym);

void set_return_statement_types(Node* rtn_statement);

#endif // PARSER_UTIL_H
