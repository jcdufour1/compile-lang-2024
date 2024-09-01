
#include "str_view.h"
#include "string_vec.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"

Str_view literal_name_new(void) {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    char var_name[20];
    sprintf(var_name, "str%zu", count);

    String symbol_name = {0};
    string_cpy_cstr_inplace(&symbol_name, var_name);
    string_vec_append(&literal_strings, symbol_name);

    count++;

    String symbol_in_vec = literal_strings.buf[literal_strings.info.count - 1];
    Str_view str_view = {.str = symbol_in_vec.buf, .count = symbol_in_vec.info.count};
    return str_view;
}

Llvm_id get_block_return_id(Node_id fun_call) {
    (void) fun_call;
    todo();
}

// return true if successful, false otherwise
static bool get_prev_matching_node(Node_id* result, Node_id node_to_start, Node_id var_call, bool is_matching(Node_id curr_node, Node_id var_call)) {
    nodes_foreach_from_curr_rev(curr_node, node_to_start) {
        node_printf(curr_node);
        assert(node_ids_equal(nodes_parent(curr_node), nodes_parent(node_to_start)));
        if (is_matching(curr_node, var_call)) {
            *result = curr_node;
            return true;
        }
    }

    if (!node_is_null(nodes_parent(node_to_start))) {
        log(LOG_DEBUG, "thing 87\n");
        return get_prev_matching_node(result, nodes_at(node_to_start)->parent, var_call, is_matching);
    }

    return false;
}

static bool is_load(Node_id curr_node, Node_id var_call) {
    return nodes_at(curr_node)->type == NODE_LOAD && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name);
}

static bool is_store(Node_id curr_node, Node_id var_call) {
    return nodes_at(curr_node)->type == NODE_STORE && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name);
}

static bool is_alloca(Node_id curr_node, Node_id var_call) {
    return nodes_at(curr_node)->type == NODE_ALLOCA && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name);
}

static bool is_variable_def(Node_id curr_node, Node_id var_call) {
    return nodes_at(curr_node)->type == NODE_VARIABLE_DEFINITION && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name);
}

Llvm_id get_prev_load_id(Node_id var_call) {
    Node_id load;
    if (!get_prev_matching_node(&load, var_call, var_call, is_load)) {
        unreachable("no load node found before symbol call:"NODE_FMT"\n", node_print(var_call));
    }
    Llvm_id llvm_id = nodes_at(load)->llvm_id;
    assert(llvm_id > 0);
    return llvm_id;
}

Llvm_id get_store_dest_id(Node_id var_call) {
    Node_id store;
    if (!get_prev_matching_node(&store, var_call, var_call, is_alloca)) {
        unreachable("no store node found before symbol call:"NODE_FMT"\n", node_print(var_call));
    }
    Llvm_id llvm_id = nodes_at(store)->llvm_id;
    assert(llvm_id > 0);
    return llvm_id;
}

Node_id get_symbol_def_from_alloca(Node_id alloca) {
    Node_id sym_def;
    if (!sym_tbl_lookup(&sym_def, nodes_at(alloca)->name)) {
        unreachable("alloca call to undefined variable:"NODE_FMT"\n", node_print(alloca));
    }

    return sym_def;
}

Llvm_id get_matching_label_id(Node_id symbol_call) {
    assert(nodes_at(symbol_call)->name.count > 0);

    Node_id label;
    if (!sym_tbl_lookup(&label, nodes_at(symbol_call)->name)) {
        unreachable("call to undefined label");
    }
    assert(nodes_at(label)->type == NODE_LABEL);

    return nodes_at(label)->llvm_id;
}

