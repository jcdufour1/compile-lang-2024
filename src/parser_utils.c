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

Node* get_storage_location(Node* var_call) {
    Node* sym_def_;
    if (!sym_tbl_lookup(&sym_def_, var_call->name)) {
        unreachable("symbol definition not found");
    }
    Node_variable_def* sym_def = node_unwrap_variable_def(sym_def_);
    if (!sym_def->storage_location) {
        unreachable("no storage location associated with symbol definition");
    }
    return sym_def->storage_location;
}

Llvm_id get_store_dest_id(const Node* var_call) {
    Llvm_id llvm_id = get_llvm_id(get_storage_location_const(var_call));
    assert(llvm_id > 0);
    return llvm_id;
}

const Node_variable_def* get_normal_symbol_def_from_alloca(const Node* alloca) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, alloca->name)) {
        unreachable("alloca call to undefined variable:"NODE_FMT"\n", node_print(alloca));
    }
    return node_unwrap_variable_def(sym_def);
}

const Node_variable_def* get_member_def_from_alloca(const Node* store_struct) {
    const Node_variable_def* var_def = get_normal_symbol_def_from_alloca(store_struct);

    Node* struct_def;
    try(sym_tbl_lookup(&struct_def, var_def->lang_type.str));

    Lang_type member_type = get_lang_type(nodes_single_child_const(node_wrap(store_struct)));
    nodes_foreach_child(member_, struct_def) {
        Node_variable_def* member = node_unwrap_variable_def(member_);
        if (lang_type_is_equal(member->lang_type, member_type)) {
            return member;
        }
    }

    unreachable("");
}

const Node_variable_def* get_symbol_def_from_alloca(const Node* alloca) {
    switch (alloca->type) {
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

    Node* label_;
    if (!sym_tbl_lookup(&label_, symbol_call->name)) {
        unreachable("call to undefined label");
    }
    Node_label* label = node_unwrap_label(label_);
    return label->llvm_id;
}

Node_assignment* assignment_new(Node* lhs, Node* rhs) {
    assert(!lhs->prev);
    assert(!lhs->next);
    assert(!lhs->parent);
    assert(!rhs->prev);
    assert(!rhs->next);
    assert(!rhs->parent);

    nodes_remove(lhs, true);
    nodes_remove(rhs, true);

    Node_assignment* assignment = node_unwrap_assignment(node_new(lhs->pos, NODE_ASSIGNMENT));
    nodes_append_child(node_wrap(assignment), lhs);
    nodes_append_child(node_wrap(assignment), rhs);

    set_assignment_operand_types(assignment);
    return assignment;
}

Node_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* literal = node_unwrap_literal(node_new(pos, NODE_LITERAL));
    node_wrap(literal)->name = literal_name_new();
    literal->str_data = value;
    literal->token_type = token_type;
    return literal;
}

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node_symbol_untyped* symbol = node_unwrap_symbol_untyped(node_new(pos, NODE_SYMBOL_UNTYPED));
    node_wrap(symbol)->name = symbol_name;
    return symbol;
}

Node_operator* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node_operator* operation = node_unwrap_operator(node_new(lhs->pos, NODE_OPERATOR));
    operation->token_type = operation_type;
    nodes_append_child(node_wrap(operation), lhs);
    nodes_append_child(node_wrap(operation), rhs);

    try(try_set_operator_lang_type(operation));
    return operation;
}

const Node* get_lang_type_from_sym_definition(const Node* sym_def) {
    (void) sym_def;
    todo();
}

uint64_t sizeof_lang_type(Lang_type lang_type) {
    if (lang_type_is_equal(lang_type, lang_type_from_cstr("ptr", 0))) {
        return 8;
    } else if (lang_type_is_equal(lang_type, lang_type_from_cstr("i32", 0))) {
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
            return sizeof_lang_type(node_unwrap_literal_const(item)->lang_type);
        case NODE_VARIABLE_DEFINITION:
            return sizeof_lang_type(node_unwrap_variable_def_const(item)->lang_type);
        default:
            unreachable("");
    }
}

uint64_t sizeof_struct_literal(const Node_struct_literal* struct_literal) {
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

uint64_t sizeof_struct_definition(const Node_struct_def* struct_def) {
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
            return sizeof_struct_definition(node_unwrap_struct_def_const(struct_literal_or_def));
        case NODE_STRUCT_LITERAL:
            return sizeof_struct_literal(node_unwrap_struct_literal_const(struct_literal_or_def));
        case NODE_FUNCTION_PARAM_SYM:
            return sizeof_struct_definition(get_struct_definition_const(struct_literal_or_def));
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
            if (!sym_tbl_lookup(&struct_def, get_lang_type(var_def).str)) {
                return false;
            }
            return true;
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        case NODE_LITERAL:
            return false;
        case NODE_STRUCT_MEMBER_SYM_TYPED:
            return true;
        default:
            log_tree(LOG_FETAL, node);
            todo();
    }
}

bool try_get_struct_definition(Node_struct_def** struct_def, Node* node) {
    switch (node->type) {
        case NODE_STRUCT_LITERAL:
            // fallthrough
        case NODE_VARIABLE_DEFINITION: {
            assert(get_lang_type(node).str.count > 0);
            Node* struct_def_;
            if (!sym_tbl_lookup(&struct_def_, get_lang_type(node).str)) {
                return false;
            }
            *struct_def = node_unwrap_struct_def(struct_def_);
            return true;
        }
        case NODE_STORE_VARIABLE:
            // fallthrough
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM_TYPED: {
            Node* var_def;
            assert(node->name.count > 0);
            if (!sym_tbl_lookup(&var_def, node->name)) {
                return false;
            }
            assert(get_lang_type(var_def).str.count > 0);
            Node* struct_def_;
            if (!sym_tbl_lookup(&struct_def_, get_lang_type(var_def).str)) {
                return false;
            }
            *struct_def = node_unwrap_struct_def(struct_def_);
            return true;
        }
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        default:
            unreachable("");
    }
}

static bool try_get_literal_lang_type(Lang_type* result, const Node_literal* literal) {
    switch (literal->token_type) {
        case TOKEN_STRING_LITERAL:
            *result = lang_type_from_cstr("ptr", 0);
            return true;
        case TOKEN_NUM_LITERAL:
            *result = lang_type_from_cstr("i32", 0);
            return true;
        default:
            unreachable(NODE_FMT, node_print(literal));
    }
}

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node_symbol_untyped* sym_untyped) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, node_wrap(sym_untyped)->name)) {
        msg_undefined_symbol(node_wrap(sym_untyped));
        return;
    }
    node_wrap(sym_untyped)->type = NODE_SYMBOL_TYPED;
    Node_symbol_typed* sym_typed = node_unwrap_symbol_typed(node_wrap(sym_untyped));
    sym_typed->lang_type = node_unwrap_variable_def_const(sym_def)->lang_type;
}

// returns false if unsuccessful
bool try_set_operator_lang_type(Node_operator* operator) {
    Node* lhs = nodes_get_child(node_wrap(operator), 0);
    Node* rhs = nodes_get_child(node_wrap(operator), 1);

    Lang_type dummy;
    if (!try_set_operator_operand_lang_type(&dummy, lhs)) {
        return false;
    }
    return try_set_operator_operand_lang_type(&operator->lang_type, rhs);
}

// returns false if unsuccessful
bool try_set_operator_operand_lang_type(Lang_type* result, Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL: {
            if (!try_get_literal_lang_type(result, node_unwrap_literal(operand))) {
                todo();
            }
            return true;
        }
        case NODE_OPERATOR:
            if (!try_set_operator_lang_type(node_unwrap_operator(operand))) {
                return false;
            }
            *result = node_unwrap_operator(operand)->lang_type;
            return true;
        case NODE_SYMBOL_UNTYPED: {
            set_symbol_type(node_unwrap_symbol_untyped(operand));
            if (is_corresponding_to_a_struct(operand)) {
                todo();
            }
            *result = node_unwrap_symbol_typed(operand)->lang_type;
            return true;
        }
        case NODE_SYMBOL_TYPED:
            *result = node_unwrap_symbol_typed(operand)->lang_type;
            return true;
        case NODE_FUNCTION_CALL:
            *result = node_unwrap_function_call(operand)->lang_type;
            set_function_call_types(node_unwrap_function_call(operand));
            return true;
        default:
            unreachable(NODE_FMT, node_print(operand));
    }
    unreachable("");
}

void set_struct_literal_assignment_types(Node* lhs, Node_struct_literal* struct_literal) {
    if (!is_corresponding_to_a_struct(lhs)) {
        todo(); // non_struct assigned struct literal
    }
    Node* lhs_var_def_;
    try(sym_tbl_lookup(&lhs_var_def_, lhs->name));
    Node_variable_def* lhs_var_def = node_unwrap_variable_def(lhs_var_def_);
    Node* struct_def;
    try(sym_tbl_lookup(&struct_def, lhs_var_def->lang_type.str));
    Lang_type lang_type = {.str = struct_def->name, .pointer_depth = 0};
    struct_literal->lang_type = lang_type;
    size_t idx = 0;
    nodes_foreach_child(memb_sym_def_, struct_def) {
        Node_variable_def* memb_sym_def = node_unwrap_variable_def(memb_sym_def_);
        Node_assignment* assign_memb_sym = node_unwrap_assignment(nodes_get_child(node_wrap(struct_literal), idx));
        Node* memb_sym = nodes_get_child(node_wrap(assign_memb_sym), 0);
        Node* assign_memb_sym_rhs = nodes_get_child(node_wrap(assign_memb_sym), 1);
        if (assign_memb_sym_rhs->type != NODE_LITERAL) {
            todo();
        }
        memb_sym->type = NODE_SYMBOL_TYPED;
        try(try_get_literal_lang_type(&node_unwrap_symbol_typed(memb_sym)->lang_type, node_unwrap_literal(assign_memb_sym_rhs)));
        if (!str_view_is_equal(node_wrap(memb_sym_def)->name, memb_sym->name)) {
            msg_invalid_struct_member_assignment_in_literal(
                lhs_var_def,
                memb_sym_def,
                memb_sym
            );
        }

        idx++;
    }

    assert(struct_literal->lang_type.str.count > 0);
}

bool set_assignment_operand_types(Node_assignment* assignment) {
    Node* lhs = nodes_get_child(node_wrap(assignment), 0);
    Node* rhs = nodes_get_child(node_wrap(assignment), 1);

    switch (lhs->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(node_unwrap_symbol_untyped(lhs));
            break;
        case NODE_VARIABLE_DEFINITION:
            break;
        case NODE_LITERAL:
            break;
        case NODE_FUNCTION_CALL:
            unreachable("");
            break;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            set_struct_member_symbol_types(node_unwrap_struct_member_sym_untyped(lhs));
            if (rhs->type == NODE_SYMBOL_UNTYPED) {
                set_symbol_type(node_unwrap_symbol_untyped(rhs));
            }
            return true; // TODO: do not return here
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

            Lang_type rhs_lang_type;
            if (!try_get_literal_lang_type(&rhs_lang_type, node_unwrap_literal(rhs))) {
                msg_invalid_assignment_to_literal(lhs, rhs);
                are_compatible = false;
            }
            if (!lang_type_is_equal(get_lang_type(lhs), rhs_lang_type)) {
                msg_invalid_assignment_to_literal(lhs, rhs);
                are_compatible = false;
            }
            break;
        }
        case NODE_STRUCT_LITERAL: {
            set_struct_literal_assignment_types(lhs, node_unwrap_struct_literal(rhs));
            break;
        }
        case NODE_SYMBOL_UNTYPED: {
            set_symbol_type(node_unwrap_symbol_untyped(rhs));
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            Node* rhs_var_def;
            try(sym_tbl_lookup(&rhs_var_def, rhs->name));
            if (!lang_type_is_equal(node_unwrap_variable_def(lhs_var_def)->lang_type, node_unwrap_lang_type(rhs_var_def)->lang_type)) {
                todo();
            }
            break;
        }
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_OPERATOR: {
            if (!try_set_operator_lang_type(node_unwrap_operator(rhs))) {
                break;
            }
            Node_operator* rhs_oper = node_unwrap_operator(rhs);
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, lhs->name));
            if (!lang_type_is_equal(rhs_oper->lang_type, node_unwrap_variable_def(lhs_var_def)->lang_type)) {
                msg_invalid_assignment_to_operation(lhs, rhs_oper);
                are_compatible = false;
            }
            break;
        }
        case NODE_FUNCTION_CALL: {
            set_function_call_types(node_unwrap_function_call(rhs));
            break;
        }
        default:
            unreachable("rhs: "NODE_FMT"\n", node_print(rhs));
    }

    return are_compatible;
}

void set_function_call_types(Node_function_call* fun_call) {
    Node* fun_def;
    if (!sym_tbl_lookup(&fun_def, node_wrap(fun_call)->name)) {
        msg_undefined_function(fun_call);
        return;
    }
    Node_function_return_types* fun_rtn_types = node_unwrap_function_return_types(nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES));
    Node_lang_type* fun_rtn_type = node_unwrap_lang_type(nodes_single_child(node_wrap(fun_rtn_types)));
    fun_call->lang_type = fun_rtn_type->lang_type;
    assert(fun_call->lang_type.str.count > 0);
    Node* params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    size_t params_idx = 0;

    nodes_foreach_child(argument, fun_call) {
        switch (argument->type) {
            case NODE_LITERAL:
                break;
            case NODE_SYMBOL_UNTYPED:
                set_symbol_type(node_unwrap_symbol_untyped(argument));
                break;
            case NODE_STRUCT_MEMBER_SYM_UNTYPED:
                set_struct_member_symbol_types(node_unwrap_struct_member_sym_untyped(argument));
                break;
            default:
                unreachable(NODE_FMT, node_print(argument));
        }

        Node_variable_def* corresponding_param = node_unwrap_variable_def(nodes_get_child(params, params_idx));
        switch (corresponding_param->lang_type.pointer_depth) {
            case 0:
                break;
            case 1:
                todo();
                //argument.pointer_depth = 1;
                break;
            default:
                todo();
        }

        if (!corresponding_param->is_variadic) {
            params_idx++;
        }
    }
}

void set_struct_member_symbol_types(Node_struct_member_sym_untyped* struct_memb_sym_untyped) {
    Node_variable_def* curr_memb_def = NULL;
    Node* struct_def_;
    Node* struct_var;
    if (!sym_tbl_lookup(&struct_var, node_wrap(struct_memb_sym_untyped)->name)) {
        msg_undefined_symbol(node_wrap(struct_memb_sym_untyped));
        return;
    }
    if (!sym_tbl_lookup(&struct_def_, node_unwrap_variable_def(struct_var)->lang_type.str)) {
        todo(); // this should possibly never happen
    }
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    node_wrap(struct_memb_sym_untyped)->type = NODE_STRUCT_MEMBER_SYM_TYPED;
    Node_struct_member_sym_typed* struct_memb_sym_typed = node_unwrap_struct_member_sym_typed(node_wrap(struct_memb_sym_untyped));
    struct_memb_sym_typed->lang_type.str = node_wrap(struct_def)->name;
    struct_memb_sym_typed->lang_type.pointer_depth = 0;
    bool is_struct = true;
    Node_struct_def* prev_struct_def = struct_def;
    nodes_foreach_child(memb_sym_untyped_, struct_memb_sym_typed) {
        Node_struct_member_sym_piece_untyped* memb_sym_untyped = node_unwrap_struct_member_sym_piece_untyped(memb_sym_untyped_);
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, struct_def, node_wrap(memb_sym_untyped))) {
            msg_invalid_struct_member(node_wrap(memb_sym_untyped));
            return;
        }
        if (!try_get_struct_definition(&struct_def, node_wrap(curr_memb_def))) {
            is_struct = false;
        }

        node_wrap(memb_sym_untyped)->type = NODE_STRUCT_MEMBER_SYM_PIECE_TYPED;
        Node_struct_member_sym_piece_typed* memb_sym_typed = node_unwrap_struct_member_sym_piece_typed(node_wrap(memb_sym_untyped));
        memb_sym_typed->lang_type = curr_memb_def->lang_type;
        node_printf(struct_def);
        memb_sym_typed->struct_index = get_member_index(prev_struct_def, memb_sym_typed);

        prev_struct_def = struct_def;
    }
}

void set_return_statement_types(Node* rtn_statement) {
    assert(rtn_statement->type == NODE_RETURN_STATEMENT);

    Node* child = nodes_single_child(rtn_statement);
    switch (child->type) {
        case NODE_SYMBOL_UNTYPED:
            set_symbol_type(node_unwrap_symbol_untyped(child));
            break;
        case NODE_STRUCT_MEMBER_SYM_UNTYPED:
            set_struct_member_symbol_types(node_unwrap_struct_member_sym_untyped(child));
            break;
        case NODE_LITERAL:
            // TODO: check type of this literal with function return type
            break;
        case NODE_OPERATOR: {
            try_set_operator_lang_type(node_unwrap_operator(child));
            break;
        }
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(child));
    }
}

