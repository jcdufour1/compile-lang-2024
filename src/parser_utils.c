#include "str_view.h"
#include "string_vec.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include "error_msg.h"

Str_view literal_name_new(void) {
    static String_vec literal_strings = {0};
    static size_t count = 0;

    char var_name[20];
    sprintf(var_name, "str%zu", count);

    String symbol_name = {0};
    string_cpy_cstr_inplace(&a_main, &symbol_name, var_name);
    string_vec_append(&a_main, &literal_strings, symbol_name);

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
        assert(curr_node->parent == node_to_start->parent);
        if (is_matching(curr_node, var_call)) {
            *result = curr_node;
            return true;
        }
    }

    if (node_to_start->parent) {
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
    return (curr_node->type == NODE_LOAD_VARIABLE) && str_view_is_equal(curr_node->name, var_call->name);
}

static bool is_store(const Node* curr_node, const Node* var_call) {
    return curr_node->type == NODE_STORE_VARIABLE && 0 == str_view_cmp(curr_node->name, var_call->name);
}

static bool is_alloca(const Node* curr_node, const Node* var_call) {
    node_printf(curr_node);
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


Node* get_storage_location(Node* var_call) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, var_call->name)) {
        unreachable("symbol definition not found");
    }
    if (!sym_def->storage_location) {
        unreachable("no storage location associated with symbol definition");
    }
    return sym_def->storage_location;
}

Llvm_id get_store_dest_id(const Node* var_call) {
    Llvm_id llvm_id = get_alloca_const(var_call)->llvm_id;
    assert(llvm_id > 0);
    return llvm_id;
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
        case NODE_STORE_VARIABLE:
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

    nodes_remove(lhs, true);
    nodes_remove(rhs, true);

    Node* assignment = node_new(lhs->pos);
    assignment->type = NODE_ASSIGNMENT;
    nodes_append_child(assignment, lhs);
    nodes_append_child(assignment, rhs);

    set_assignment_operand_types(lhs, rhs);
    return assignment;
}

Node* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node* symbol = node_new(pos);
    symbol->type = NODE_LITERAL;
    symbol->name = literal_name_new();
    symbol->str_data = value;
    symbol->token_type = token_type;
    return symbol;
}

Node* symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node* symbol = node_new(pos);
    symbol->type = NODE_SYMBOL_UNTYPED;
    symbol->name = symbol_name;
    return symbol;
}

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

uint64_t sizeof_lang_type(Str_view lang_type) {
    if (str_view_cstr_is_equal(lang_type, "ptr")) {
        return 8;
    } else if (str_view_cstr_is_equal(lang_type, "i32")) {
        return 4;
    } else {
        unreachable("");
    }
}

uint64_t sizeof_item(const Node* item) {
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

uint64_t sizeof_struct_literal(const Node* struct_literal) {
    assert(struct_literal->type == NODE_STRUCT_LITERAL);
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    nodes_foreach_child(child, struct_literal) {
        const Node* member = nodes_single_child_const(child);
        uint64_t sizeof_curr_item = sizeof_item(member);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct_definition(const Node* struct_def) {
    assert(struct_def->type == NODE_STRUCT_DEFINITION);
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    nodes_foreach_child(member_def, struct_def) {
        uint64_t sizeof_curr_item = sizeof_item(member_def);
        if (total%required_alignment + sizeof_curr_item > required_alignment) {
            total += required_alignment - total%required_alignment;
        }
        total += sizeof_curr_item;
    }
    return total;
}

uint64_t sizeof_struct(const Node* struct_literal_or_def) {
    assert(is_corresponding_to_a_struct(struct_literal_or_def));

    switch (struct_literal_or_def->type) {
        case NODE_STRUCT_DEFINITION:
            return sizeof_struct_definition(struct_literal_or_def);
        case NODE_STRUCT_LITERAL:
            return sizeof_struct_literal(struct_literal_or_def);
        case NODE_FUNCTION_PARAM_SYM:
            return sizeof_struct_literal(get_struct_definition_const(struct_literal_or_def));
        default:
            node_printf(struct_literal_or_def);
            todo();
    }
}

bool is_corresponding_to_a_struct(const Node* node) {
    Node* var_def;
    Node* struct_def;
    switch (node->type) {
        case NODE_STRUCT_LITERAL:
            return true;
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_STORE_VARIABLE:
            // fallthrough
        case NODE_STORE_ANOTHER_NODE:
            // fallthrough
        case NODE_FUNCTION_PARAM_SYM:
            // fallthrough
            assert(node->name.count > 0);
            if (!sym_tbl_lookup(&var_def, node->name)) {
                return false;
            }
            if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
                return false;
            }
            return true;
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        case NODE_LITERAL:
            return false;
        default:
            log_tree(LOG_DEBUG, node);
            todo();
    }
}

bool try_get_struct_definition(Node** struct_def, Node* node) {
    switch (node->type) {
        case NODE_STRUCT_LITERAL: {
            // fallthrough
        case NODE_VARIABLE_DEFINITION:
            assert(node->lang_type.count > 0);
            if (!sym_tbl_lookup(struct_def, node->lang_type)) {
                return false;
            }
            return true;
        }
        case NODE_STORE_VARIABLE:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM: {
            Node* var_def;
            assert(node->name.count > 0);
            if (!sym_tbl_lookup(&var_def, node->name)) {
                return false;
            }
            assert(var_def->lang_type.count > 0);
            if (!sym_tbl_lookup(struct_def, var_def->lang_type)) {
                return false;
            }
            return true;
        }
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        default:
            unreachable("");
    }
}

static bool try_get_literal_lang_type(Str_view* result, const Node* literal) {
    assert(literal->type == NODE_LITERAL);

    switch (literal->token_type) {
        case TOKEN_STRING_LITERAL:
            *result = str_view_from_cstr("ptr");
            return true;
        case TOKEN_NUM_LITERAL:
            *result = str_view_from_cstr("i32");
            return true;
        default:
            unreachable(NODE_FMT, node_print(literal));
    }
}

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node* sym_untyped) {
    assert(sym_untyped->type == NODE_SYMBOL_UNTYPED);

    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, sym_untyped->name)) {
        msg_undefined_symbol(sym_untyped);
        return;
    }

    sym_untyped->type = NODE_SYMBOL_TYPED;
    sym_untyped->lang_type = sym_def->lang_type;
}

// returns false if unsuccessful
bool try_set_operator_lang_type(Str_view* result, Node* operator) {
    assert(operator->type == NODE_OPERATOR);

    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

    if (!try_set_operator_operand_lang_type(result, lhs)) {
        return false;
    }
    return try_set_operator_operand_lang_type(result, rhs);
}

// returns false if unsuccessful
bool try_set_operator_operand_lang_type(Str_view* result, Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL: {
            if (!try_get_literal_lang_type(result, operand)) {
                todo();
            }
            return true;
        }
        case NODE_OPERATOR:
            return try_set_operator_lang_type(result, operand);
        case NODE_SYMBOL_UNTYPED: {
            set_symbol_type(operand);
            if (is_corresponding_to_a_struct(operand)) {
                todo();
            }
            *result = operand->lang_type;
            return true;
        }
        case NODE_SYMBOL_TYPED:
            return true;
        default:
            unreachable(NODE_FMT, node_print(operand));
    }
    unreachable("");
}

void set_struct_literal_assignment_types(Node* lhs, Node* struct_literal) {
    assert(struct_literal->type == NODE_STRUCT_LITERAL);

    if (!is_corresponding_to_a_struct(lhs)) {
        todo(); // non_struct assigned struct literal
    }
    Node* lhs_var_def;
    try(sym_tbl_lookup(&lhs_var_def, lhs->name));
    Node* struct_def;
    try(sym_tbl_lookup(&struct_def, lhs_var_def->lang_type));
    struct_literal->lang_type = struct_def->name;
    size_t idx = 0;
    nodes_foreach_child(memb_sym_def, struct_def) {
        Node* assign_memb_sym = nodes_get_child(struct_literal, idx);
        Node* memb_sym = nodes_get_child(assign_memb_sym, 0);
        Node* assign_memb_sym_rhs = nodes_get_child(assign_memb_sym, 1);
        if (assign_memb_sym_rhs->type != NODE_LITERAL) {
            todo();
        }
        memb_sym->type = NODE_SYMBOL_TYPED;
        try(try_get_literal_lang_type(&memb_sym->lang_type, assign_memb_sym_rhs));
        if (!str_view_is_equal(memb_sym_def->name, memb_sym->name)) {
            msg_invalid_struct_member_assignment_in_literal(
                lhs_var_def,
                memb_sym_def,
                memb_sym
            );
        }

        idx++;
    }

    assert(struct_literal->lang_type.count > 0);
}

bool set_assignment_operand_types(Node* lhs, Node* rhs) {
    switch (lhs->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(lhs);
            break;
        case NODE_VARIABLE_DEFINITION:
            break;
        case NODE_LITERAL:
            break;
        default:
            unreachable(NODE_FMT, node_print(lhs));
    }

    bool are_compatible = true;

    switch (rhs->type) {
        case NODE_LITERAL: {
            if (is_corresponding_to_a_struct(lhs)) {
                // struct assigned literal of invalid type
                meg_struct_assigned_to_invalid_literal(lhs, rhs);
                todo();
                are_compatible = false;
                break;
            }

            Str_view rhs_lang_type;
            if (!try_get_literal_lang_type(&rhs_lang_type, rhs)) {
                msg_invalid_assignment_to_literal(lhs, rhs);
                are_compatible = false;
            }
            if (!str_view_is_equal(lhs->lang_type, rhs_lang_type)) {
                msg_invalid_assignment_to_literal(lhs, rhs);
                are_compatible = false;
            }
            break;
        }
        case NODE_STRUCT_LITERAL: {
            set_struct_literal_assignment_types(lhs, rhs);
            break;
        }
        case NODE_SYMBOL_UNTYPED: {
            set_symbol_type(rhs);
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            Node* rhs_var_def;
            try(sym_tbl_lookup(&rhs_var_def, rhs->name));
            if (!str_view_is_equal(lhs_var_def->lang_type, rhs_var_def->lang_type)) {
                todo();
            }
            break;
        }
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_OPERATOR: {
            Str_view rhs_lang_type;
            if (!try_set_operator_lang_type(&rhs_lang_type, rhs)) {
                break;
            }
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            if (!str_view_is_equal(rhs_lang_type, lhs_var_def->lang_type)) {
                msg_invalid_assignment_to_operation(lhs, rhs, rhs_lang_type);
                are_compatible = false;
            }
            break;
        }
        case NODE_FUNCTION_CALL: {
            set_function_call_types(rhs);
            break;
        }
        default:
            unreachable("rhs: "NODE_FMT"\n", node_print(rhs));
    }

    return are_compatible;
}

void set_function_call_types(Node* fun_call) {
    Node* fun_def;
    if (!sym_tbl_lookup(&fun_def, fun_call->name)) {
        msg_undefined_function(fun_call);
        return;
    }
    Node* params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    size_t params_idx = 0;

    nodes_foreach_child(argument, fun_call) {
        switch (argument->type) {
            case NODE_LITERAL:
                break;
            case NODE_SYMBOL_UNTYPED:
                set_symbol_type(argument);
                break;
            case NODE_STRUCT_MEMBER_SYM:
                set_struct_member_symbol_types(argument);
                break;
            default:
                unreachable(NODE_FMT, node_print(argument));
        }

        Node* corresponding_param = nodes_get_child(params, params_idx);
        switch (corresponding_param->pointer_depth) {
            case 0:
                break;
            case 1:
                argument->pointer_depth = 1;
                break;
            default:
                todo();
        }

        if (!corresponding_param->is_variadic) {
            params_idx++;
        }
    }
}

void set_struct_member_symbol_types(const Node* struct_memb_sym) {
    Node* curr_memb_def = NULL;
    Node* struct_def;
    Node* struct_var;
    if (!sym_tbl_lookup(&struct_var, struct_memb_sym->name)) {
        msg_undefined_symbol(struct_memb_sym);
        return;
    }
    if (!sym_tbl_lookup(&struct_def, struct_var->lang_type)) {
        todo(); // this should possibly never happen
    }
    bool is_struct = true;
    nodes_foreach_child(memb_sym, struct_memb_sym) {
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, struct_def, memb_sym)) {
            msg_invalid_struct_member(memb_sym);
            return;
        }
        if (!try_get_struct_definition(&struct_def, curr_memb_def)) {
            is_struct = false;
        }
    }
}

void set_return_statement_types(Node* rtn_statement) {
    assert(rtn_statement->type == NODE_RETURN_STATEMENT);

    Node* child = nodes_single_child(rtn_statement);
    switch (child->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(child);
            break;
        case NODE_STRUCT_MEMBER_SYM:
            set_struct_member_symbol_types(child);
            break;
        case NODE_LITERAL:
            // TODO: check type of this literal with function return type
            break;
        case NODE_OPERATOR: {
            Str_view dummy;
            try_set_operator_lang_type(&dummy, child);
            break;
        }
        default:
            unreachable(NODE_FMT"\n", node_print(child));
    }
}

