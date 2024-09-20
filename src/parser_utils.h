#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "str_view.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"

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

const Node* get_lang_type_from_sym_definition(const Node* sym_def);

size_t sizeof_lang_type(Str_view lang_type);

size_t sizeof_item(const Node* item);

size_t sizeof_struct(const Node* struct_literal);

static inline const Node* struct_definition_from_node(const Node* node) {
    switch (node->type) {
        case NODE_SYMBOL:
            break;
        case NODE_STORE:
            break;
        case NODE_STRUCT_MEMBER_CALL:
            break;
        case NODE_LOAD_STRUCT_MEMBER:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node));
    }

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, node->name)) {
        unreachable("");
    }
    switch (var_def->type) {
        case NODE_STRUCT_ELEMENT_PTR_DEF:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(var_def));
    }

    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    return struct_def;
}

static inline const Node* struct_variable_definition_from_node(const Node* node) {
    switch (node->type) {
        case NODE_SYMBOL:
            break;
        case NODE_STORE:
            break;
        case NODE_STRUCT_MEMBER_ELEMENT_PTR_SYMBOL:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node));
    }

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, node->name)) {
        unreachable("");
    }
    switch (var_def->type) {
        case NODE_VARIABLE_DEFINITION:
            return var_def;
        default:
            unreachable(NODE_FMT"\n", node_print(var_def));
    }

    unreachable(NODE_FMT"\n", node_print(var_def));
}


static inline bool is_struct_variable_definition(const Node* var_def) {
    if (var_def->type == NODE_STRUCT_ELEMENT_PTR_DEF) {
        return false;
    }
    assert(var_def->type == NODE_VARIABLE_DEFINITION);

    Node* struct_def;
    return sym_tbl_lookup(&struct_def, var_def->lang_type);
}

static inline bool is_struct_symbol(const Node* symbol) {
    assert(symbol->type == NODE_SYMBOL);

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, symbol->lang_type)) {
        unreachable("");
    }
    return is_struct_variable_definition(var_def);
}

size_t sizeof_struct_definition(const Node* struct_def);

const Node* get_member_definition(const Node* struct_def, const Node* member_symbol);

const Node* get_store_member_symbol(const Node* node);

const Node* get_member_symbol_definition_auto(const Node* node);

const Node* get_prev_thing_member_ptr(const Node* load_member_value);

#endif // PARSER_UTIL_H
