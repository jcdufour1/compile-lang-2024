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

static bool is_load_struct_member(const Node* curr_node, const Node* var_call) {
    if (curr_node->type != NODE_LOAD_STRUCT_MEMBER) {
        return false;
    }
    node_printf(curr_node);
    node_printf(var_call);
    if (!str_view_is_equal(curr_node->name, var_call->name)) {
        return false;
    }

    const Node* curr_member = nodes_single_child_const(curr_node);
    node_printf(curr_member);
    const Node* member_to_find = nodes_single_child_const(var_call);
    node_printf(member_to_find);
    return str_view_is_equal(curr_member->name, member_to_find->name);
}

static bool is_load(const Node* curr_node, const Node* var_call) {
    return (curr_node->type == NODE_LOAD) && str_view_is_equal(curr_node->name, var_call->name);
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

static bool is_function_call(const Node* curr_node, const Node* var_call) {
    (void) var_call;
    return curr_node->type == NODE_FUNCTION_CALL;
}

static bool is_operator(const Node* curr_node, const Node* var_call) {
    (void) var_call;
    return curr_node->type == NODE_OPERATOR;
}

Llvm_id get_prev_load_id(const Node* var_call) {
    node_printf(var_call);
    const Node* load;
    if (var_call->type == NODE_STRUCT_MEMBER_CALL) {
        if (!get_prev_matching_node(&load, var_call, var_call, is_load_struct_member)) {
            unreachable("no struct load node found before symbol call:"NODE_FMT"\n", node_print(var_call));
        }
    } else {
        if (!get_prev_matching_node(&load, var_call, var_call, is_load)) {
            unreachable("no struct load node found before symbol call:"NODE_FMT"\n", node_print(var_call));
        }
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

Llvm_id get_prev_function_call_id(const Node* node) {
    const Node* fun_call;
    if (!get_prev_matching_node(&fun_call, node, node, is_function_call)) {
        unreachable("no function call found before:"NODE_FMT"\n", node_print(node));
    }
    return fun_call->llvm_id;
}

Llvm_id get_prev_operator_id(const Node* node) {
    const Node* operator;
    if (!get_prev_matching_node(&operator, node, node, is_operator)) {
        unreachable("no operator found before:"NODE_FMT"\n", node_print(node));
    }
    return operator->llvm_id;
}

const Node* get_normal_symbol_def_from_alloca(const Node* alloca) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, alloca->name)) {
        unreachable("alloca call to undefined variable:"NODE_FMT"\n", node_print(alloca));
    }
    return sym_def;
}

const Node* get_member_def_from_alloca(const Node* store_struct) {
    const Node* var_def = get_normal_symbol_def_from_alloca(store_struct);

    Node* struct_def;
    try(sym_tbl_lookup(&struct_def, var_def->lang_type));

    Str_view member_type = nodes_single_child_const(store_struct)->lang_type;
    nodes_foreach_child(member, struct_def) {
        node_printf(member);
        if (str_view_is_equal(member->lang_type, member_type)) {
            return member;
        }
    }

    unreachable("");
}

const Node* get_symbol_def_from_alloca(const Node* alloca) {
    switch (alloca->type) {
        case NODE_STORE_STRUCT_MEMBER:
            return get_member_def_from_alloca(alloca);
        case NODE_ALLOCA:
            // fallthrough
        case NODE_STORE:
            return get_normal_symbol_def_from_alloca(alloca);
        default:
            unreachable(NODE_FMT"\n", node_print(alloca));
    }
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
    assert(fun_param_call->type == NODE_FUNCTION_PARAM_SYM);

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

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

size_t sizeof_lang_type(Str_view lang_type) {
    if (str_view_cstr_is_equal(lang_type, "ptr")) {
        return 8;
     } else if (str_view_cstr_is_equal(lang_type, "i32")) {
        return 4;
     } else {
        unreachable("");
     }
}

size_t sizeof_item(const Node* item) {
    switch (item->type) {
        case NODE_STRUCT_LITERAL:
            todo();
        case NODE_LITERAL:
            return sizeof_lang_type(item->lang_type);
        case NODE_VARIABLE_DEFINITION:
            return sizeof_lang_type(item->lang_type);
        default:
            node_printf(item);
            unreachable("");
    }
}

size_t sizeof_struct_literal(const Node* struct_literal_or_def) {
    assert(struct_literal_or_def->type == NODE_STRUCT_LITERAL || struct_literal_or_def->type == NODE_STRUCT_DEFINITION);

    size_t size = 0;
    nodes_foreach_child(child, struct_literal_or_def) {
        const Node* member = nodes_single_child_const(child);
        size += sizeof_item(member);
    }
    return size;
}

size_t sizeof_struct_definition(const Node* struct_def) {
    assert(struct_def->type == NODE_STRUCT_LITERAL || struct_def->type == NODE_STRUCT_DEFINITION);

    size_t size = 0;
    nodes_foreach_child(member_def, struct_def) {
        size += sizeof_item(member_def);
    }
    return size;
}

size_t sizeof_struct(const Node* struct_literal_or_def) {
    switch (struct_literal_or_def->type) {
        case NODE_STRUCT_DEFINITION:
            return sizeof_struct_definition(struct_literal_or_def);
        case NODE_STRUCT_LITERAL:
            return sizeof_struct_literal(struct_literal_or_def);
        default:
            node_printf(struct_literal_or_def);
            todo();
    }
}

