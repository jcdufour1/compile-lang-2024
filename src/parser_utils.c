
#include "str_view.h"
#include "string_vec.h"
#include "node.h"
#include "nodes.h"

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

Node_id get_block_return_id(Node_id fun_call) {
    (void) fun_call;
    todo();
}

Llvm_id get_prev_load_id(Node_id var_call) {
    nodes_foreach_from_curr_rev(curr_node, var_call) {
        if (nodes_at(curr_node)->type == NODE_LOAD && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name)) {
            assert(nodes_at(curr_node)->llvm_id > 0);
            return nodes_at(curr_node)->llvm_id;
        }
    }

    Node_id parent_var_call = nodes_at(var_call)->parent;
    switch (nodes_at(parent_var_call)->type) {
        case NODE_FUNCTION_CALL:
            break;
        default:
            unreachable();
    }
    
    nodes_foreach_from_curr_rev(curr_node, parent_var_call) {
        if (nodes_at(curr_node)->type == NODE_LOAD && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name)) {
            assert(nodes_at(curr_node)->llvm_id > 0);
            return nodes_at(curr_node)->llvm_id;
        }
    }

    unreachable();
}

Llvm_id get_store_dest_id(Node_id var_call) {
    nodes_foreach_from_curr_rev(curr_node, var_call) {
        log(LOG_DEBUG, NODE_FMT"\n"NODE_FMT"\n", node_print(curr_node), node_print(var_call));
        if (nodes_at(curr_node)->type == NODE_ALLOCA && 0 == str_view_cmp(nodes_at(curr_node)->name, nodes_at(var_call)->name)) {
            assert(nodes_at(curr_node)->llvm_id > 0);
            return nodes_at(curr_node)->llvm_id;
        }
    }

    unreachable();
}

Node_id get_symbol_def_from_alloca(Node_id alloca) {
    nodes_foreach_from_curr_rev(curr_node, alloca) {
        if (nodes_at(curr_node)->type == NODE_VARIABLE_DEFINITION && 0 == str_view_cmp(nodes_at(alloca)->name, nodes_at(alloca)->name)) {
            return curr_node;
        }
    }

    unreachable();
}
