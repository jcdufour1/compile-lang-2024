#include "str_view.h"
#include "string_vec.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"

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

Llvm_id get_block_return_id(const Node* fun_call) {
    (void) fun_call;
    todo();
}

// return true if successful, false otherwise
static bool get_prev_matching_node(const Node** result, const Node* node_to_start, const Node* var_call, bool is_matching(const Node* curr_node, const Node* var_call)) {
    nodes_foreach_from_curr_rev_const(curr_node, node_to_start) {
        //node_printf(curr_node);
        assert(curr_node->parent == node_to_start->parent);
        if (is_matching(curr_node, var_call)) {
            *result = curr_node;
            return true;
        }
    }

    if (node_to_start->parent) {
        //log(LOG_DEBUG, "thing 87\n");
        return get_prev_matching_node(result, node_to_start->parent, var_call, is_matching);
    }

    return false;
}

static bool is_load(const Node* curr_node, const Node* var_call) {
    return curr_node->type == NODE_LOAD && 0 == str_view_cmp(curr_node->name, var_call->name);
}

static bool is_store(const Node* curr_node, const Node* var_call) {
    return curr_node->type == NODE_STORE && 0 == str_view_cmp(curr_node->name, var_call->name);
}

static bool is_alloca(const Node* curr_node, const Node* var_call) {
    return curr_node->type == NODE_ALLOCA && 0 == str_view_cmp(curr_node->name, var_call->name);
}

static bool is_variable_def(const Node* curr_node, const Node* var_call) {
    return curr_node->type == NODE_VARIABLE_DEFINITION && 0 == str_view_cmp(curr_node->name, var_call->name);
}

Llvm_id get_prev_load_id(const Node* var_call) {
    const Node* load;
    if (!get_prev_matching_node(&load, var_call, var_call, is_load)) {
        unreachable("no load node found before symbol call:"NODE_FMT"\n", node_print(var_call));
    }
    Llvm_id llvm_id = load->llvm_id;
    assert(llvm_id > 0);
    return llvm_id;
}

Llvm_id get_store_dest_id(const Node* var_call) {
    const Node* store;
    if (!get_prev_matching_node(&store, var_call, var_call, is_alloca)) {
        unreachable("no alloca node found before symbol call:"NODE_FMT"\n", node_print(var_call));
    }
    Llvm_id llvm_id = store->llvm_id;
    assert(llvm_id > 0);
    return llvm_id;
}

const Node* get_symbol_def_from_alloca(const Node* alloca) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, alloca->name)) {
        unreachable("alloca call to undefined variable:"NODE_FMT"\n", node_print(alloca));
    }

    return sym_def;
}

Llvm_id get_matching_label_id(const Node* symbol_call) {
    assert(symbol_call->name.count > 0);

    Node* label;
    if (!sym_tbl_lookup(&label, symbol_call->name)) {
        unreachable("call to undefined label");
    }
    assert(label->type == NODE_LABEL);

    return label->llvm_id;
}

Node* assignment_new(Node* lhs, Node* rhs) {
    assert(!lhs->prev);
    assert(!lhs->next);
    assert(!lhs->parent);
    assert(!rhs->prev);
    assert(!rhs->next);
    assert(!rhs->parent);

    nodes_remove_siblings_and_parent(lhs);
    nodes_remove_siblings_and_parent(rhs);

    Node* assignment = node_new();
    assignment->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);
    return assignment;
}

Node* literal_new(Str_view value) {
    Node* symbol = node_new();
    symbol->type = NODE_LITERAL;
    symbol->name = literal_name_new();
    symbol->str_data = value;
    return symbol;
}

Node* symbol_new(Str_view symbol_name) {
    assert(symbol_name.count > 0);

    Node* symbol = node_new();
    symbol->type = NODE_SYMBOL;
    symbol->name = symbol_name;
    return symbol;
}

Llvm_id get_matching_fun_param_load_id(const Node* fun_param_call) {
    assert(fun_param_call->type == NODE_FUNCTION_PARAM_CALL);

    const Node* fun_def = fun_param_call;
    while (fun_def->type != NODE_FUNCTION_DEFINITION) {
        fun_def = fun_def->parent;
        assert(fun_def);
    }

    const Node* fun_params = nodes_get_child_of_type_const(fun_def, NODE_FUNCTION_PARAMETERS);
    nodes_foreach_child(param, fun_params) {
        if (0 == str_view_cmp(param->name, fun_param_call->name)) {
            assert(param->llvm_id > 0);
            return param->llvm_id;
        }
    }

    unreachable("fun_param matching "NODE_FMT" not found", node_print(fun_param_call));
}
