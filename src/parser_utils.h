#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"

Str_view literal_name_new(void);

Llvm_id get_prev_load_id(const Node* var_call);

Node* get_storage_location(Node* var_call);

static inline const Node* get_storage_location_const(const Node* var_call) {
    return get_storage_location((Node*)var_call);
}

Llvm_id get_store_dest_id(const Node* var_call);

const Node* get_symbol_def_from_alloca(const Node* alloca);

Llvm_id get_matching_label_id(const Node* symbol_call);

// lhs and rhs should not be used for other tasks after this
Node* assignment_new(Node* lhs, Node* rhs);

Node* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos);

Node* symbol_new(Str_view symbol_name, Pos pos);

Node* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type);

Llvm_id get_matching_fun_param_load_id(const Node* src);

const Node* get_lang_type_from_sym_definition(const Node* sym_def);

uint64_t sizeof_lang_type(Str_view lang_type);

uint64_t sizeof_item(const Node* item);

uint64_t sizeof_struct(const Node* struct_literal);

uint64_t sizeof_struct_definition(const Node* struct_def);

static inline bool is_struct_variable_definition(const Node* var_def) {
    assert(var_def->type == NODE_VARIABLE_DEFINITION);

    Node* struct_def;
    return sym_tbl_lookup(&struct_def, var_def->lang_type);
}

static inline bool is_struct_symbol(const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL_TYPED || symbol->type == NODE_STRUCT_MEMBER_SYM_TYPED);

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, symbol->name)) {
        unreachable("");
    }
    return is_struct_variable_definition(var_def);
}

bool is_corresponding_to_a_struct(const Node* node);

static inline size_t get_member_index(const Node* struct_def, const Node* member_symbol) {
    assert(struct_def->type == NODE_STRUCT_DEFINITION);
    size_t idx = 0;
    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(curr_member->name, member_symbol->name)) {
            return idx;
        }
        idx++;
    }
    unreachable("member not found");
}

static inline bool try_get_member_def(Node** member_def, const Node* struct_def, const Node* member_symbol) {
    assert(struct_def->type == NODE_STRUCT_DEFINITION);
    assert(
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_TYPED ||
        member_symbol->type == NODE_STRUCT_MEMBER_SYM_PIECE_UNTYPED
    );

    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(curr_member->name, member_symbol->name)) {
            assert(curr_member->lang_type.count > 0);
            *member_def = curr_member;
            return true;
        }
    }
    return false;
}

static inline const Node* get_member_def(const Node* struct_def, const Node* member_symbol) {
    Node* member_def;
    if (!try_get_member_def(&member_def, struct_def, member_symbol)) {
        unreachable("could not find member definition");
    }
    return member_def;
}

bool try_get_struct_definition(Node** struct_def, Node* node);

static inline Node* get_struct_definition(Node* node) {
    Node* struct_def;
    if (!try_get_struct_definition(&struct_def, node)) {
        unreachable("could not find struct definition for "NODE_FMT"\n", node_print(node));
    }
    return struct_def;
}

static inline const Node* get_struct_definition_const(const Node* node) {
    return get_struct_definition((Node*)node);
}

bool set_assignment_operand_types(Node* assignment);

// returns false if unsuccessful
bool try_set_operator_lang_type(Node* operator);

// returns false if unsuccessful
bool try_set_operator_operand_lang_type(Str_view* result, Node* operand);

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node* sym_untyped);

void set_function_call_types(Node* fun_call);

void set_struct_member_symbol_types(Node* struct_memb_sym);

void set_return_statement_types(Node* rtn_statement);

#endif // PARSER_UTIL_H
