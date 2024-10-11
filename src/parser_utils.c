#include "str_view.h"
#include "string_vec.h"
#include "node_ptr_vec.h"
#include "node.h"
#include "nodes.h"
#include "symbol_table.h"
#include "parser_utils.h"
#include "error_msg.h"

static void set_literal_lang_type(Node_literal* literal, TOKEN_TYPE token_type);

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

Node* get_storage_location(Str_view sym_name) {
    Node* sym_def_;
    if (!sym_tbl_lookup(&sym_def_, sym_name)) {
        unreachable("symbol definition for symbol "STR_VIEW_FMT" not found\n", str_view_print(sym_name));
    }
    Node_variable_def* sym_def = node_unwrap_variable_def(sym_def_);
    if (!sym_def->storage_location) {
        unreachable("no storage location associated with symbol definition");
    }
    return sym_def->storage_location;
}

Llvm_id get_store_dest_id(const Node* var_call) {
    Llvm_id llvm_id = get_llvm_id(get_storage_location(get_node_name(var_call)));
    assert(llvm_id > 0);
    return llvm_id;
}

const Node_variable_def* get_normal_symbol_def_from_alloca(const Node* node) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, get_node_name(node))) {
        unreachable("node call to undefined variable:"NODE_FMT"\n", node_print(node));
    }
    return node_unwrap_variable_def(sym_def);
}

const Node_variable_def* get_symbol_def_from_alloca(const Node* alloca) {
    switch (alloca->type) {
        case NODE_ALLOCA:
            return get_normal_symbol_def_from_alloca(alloca);
        default:
            unreachable(NODE_FMT"\n", node_print(alloca));
    }
}

Llvm_id get_matching_label_id(Str_view name) {
    Node* label_;
    if (!sym_tbl_lookup(&label_, name)) {
        unreachable("call to undefined label");
    }
    Node_label* label = node_unwrap_label(label_);
    return label->llvm_id;
}

Node_assignment* assignment_new(Node* lhs, Node* rhs) {
    Node_assignment* assignment = node_unwrap_assignment(node_new(lhs->pos, NODE_ASSIGNMENT));
    assignment->lhs = lhs;
    assignment->rhs = rhs;

    set_assignment_operand_types(assignment);
    return assignment;
}

Node_literal* literal_new(Str_view value, TOKEN_TYPE token_type, Pos pos) {
    Node_literal* literal = node_unwrap_literal(node_new(pos, NODE_LITERAL));
    literal->name = literal_name_new();
    literal->str_data = value;
    set_literal_lang_type(literal, token_type);
    return literal;
}

Node_symbol_untyped* symbol_new(Str_view symbol_name, Pos pos) {
    assert(symbol_name.count > 0);

    Node_symbol_untyped* symbol = node_unwrap_symbol_untyped(node_new(pos, NODE_SYMBOL_UNTYPED));
    symbol->name = symbol_name;
    return symbol;
}

Node_binary* operation_new(Node* lhs, Node* rhs, TOKEN_TYPE operation_type) {
    // TODO: check if lhs or rhs were already appended to the tree
    Node_binary* operator = node_unwrap_binary(node_new(lhs->pos, NODE_BINARY));
    operator->token_type = operation_type;
    operator->lhs = lhs;
    operator->rhs = rhs;

    try(try_set_operator_lang_type(operator));
    return operator;
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
    return sizeof_struct_definition(get_struct_definition_const(node_wrap(struct_literal)));
}

uint64_t sizeof_struct_definition(const Node_struct_def* struct_def) {
    uint64_t required_alignment = 8; // TODO: do not hardcode this

    uint64_t total = 0;
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        const Node* memb_def = node_ptr_vec_at_const(&struct_def->members, idx);
        uint64_t sizeof_curr_item = sizeof_item(memb_def);
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
        case NODE_STORE_ANOTHER_NODE:
            // fallthrough
            assert(get_node_name(node).count > 0);
            if (!sym_tbl_lookup(&var_def, get_node_name(node))) {
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
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_STRUCT_MEMBER_SYM_TYPED: {
            Node* var_def;
            assert(get_node_name(node).count > 0);
            if (!sym_tbl_lookup(&var_def, get_node_name(node))) {
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

static void set_literal_lang_type(Node_literal* literal, TOKEN_TYPE token_type) {
    switch (token_type) {
        case TOKEN_STRING_LITERAL:
            literal->lang_type = lang_type_from_cstr("ptr", 0);
            break;
        case TOKEN_NUM_LITERAL:
            literal->lang_type = lang_type_from_cstr("i32", 0);
            break;
        default:
            unreachable(NODE_FMT, node_print(literal));
    }
}

// set symbol lang_type, and report error if symbol is undefined
void set_symbol_type(Node_symbol_untyped* sym_untyped) {
    Node* sym_def;
    if (!sym_tbl_lookup(&sym_def, sym_untyped->name)) {
        msg_undefined_symbol(node_wrap(sym_untyped));
        return;
    }
    Node_symbol_untyped temp = *sym_untyped;
    node_wrap(sym_untyped)->type = NODE_SYMBOL_TYPED;
    Node_symbol_typed* sym_typed = node_unwrap_symbol_typed(node_wrap(sym_untyped));
    sym_typed->name = temp.name;
    sym_typed->lang_type = node_unwrap_variable_def_const(sym_def)->lang_type;
}

// returns false if unsuccessful
bool try_set_operator_lang_type(Node_binary* operator) {
    Lang_type dummy;
    if (!try_set_operator_operand_lang_type(&dummy, operator->lhs)) {
        return false;
    }
    return try_set_operator_operand_lang_type(&operator->lang_type, operator->rhs);
}

// returns false if unsuccessful
bool try_set_operator_operand_lang_type(Lang_type* result, Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL: {
            *result = node_unwrap_literal(operand)->lang_type;
            return true;
        }
        case NODE_BINARY:
            if (!try_set_operator_lang_type(node_unwrap_binary(operand))) {
                return false;
            }
            *result = node_unwrap_binary(operand)->lang_type;
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
    try(sym_tbl_lookup(&lhs_var_def_, get_node_name(lhs)));
    Node_variable_def* lhs_var_def = node_unwrap_variable_def(lhs_var_def_);
    Node* struct_def_;
    try(sym_tbl_lookup(&struct_def_, lhs_var_def->lang_type.str));
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    Lang_type lang_type = {.str = struct_def->name, .pointer_depth = 0};
    struct_literal->lang_type = lang_type;
    
    Node_ptr_vec new_literal_members = {0};
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        Node* memb_sym_def_ = node_ptr_vec_at(&struct_def->members, idx);
        if (memb_sym_def_->type == NODE_LITERAL) {
            break;
        }

        Node_variable_def* memb_sym_def = node_unwrap_variable_def(memb_sym_def_);
        Node_assignment* assign_memb_sym = node_unwrap_assignment(node_ptr_vec_at(&struct_literal->members, idx));
        Node_symbol_untyped* memb_sym_untyped = node_unwrap_symbol_untyped(assign_memb_sym->lhs);
        Node_literal* assign_memb_sym_rhs = node_unwrap_literal(assign_memb_sym->rhs);
        if (!str_view_is_equal(memb_sym_def->name, memb_sym_untyped->name)) {
            todo();
            msg_invalid_struct_member_assignment_in_literal(
                lhs_var_def,
                memb_sym_def,
                memb_sym_untyped
            );
        }

        node_ptr_vec_append(&new_literal_members, node_wrap(assign_memb_sym_rhs));
    }

    struct_literal->members = new_literal_members;

    log_tree(LOG_DEBUG, node_wrap(struct_literal));

    assert(struct_literal->lang_type.str.count > 0);
}

bool set_assignment_operand_types(Node_assignment* assignment) {
    Node* lhs = assignment->lhs;
    Node* rhs = assignment->rhs;

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

            if (!lang_type_is_equal(get_lang_type(lhs), node_unwrap_literal(rhs)->lang_type)) {
                msg_invalid_assignment_to_literal(node_unwrap_symbol_typed(lhs), node_unwrap_literal(rhs));
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
            try(sym_tbl_lookup(&lhs_var_def, get_node_name(lhs)));
            Node* rhs_var_def;
            try(sym_tbl_lookup(&rhs_var_def, get_node_name(rhs)));
            if (!lang_type_is_equal(node_unwrap_variable_def(lhs_var_def)->lang_type, node_unwrap_variable_def(rhs_var_def)->lang_type)) {
                todo();
            }
            break;
        }
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_BINARY: {
            if (!try_set_operator_lang_type(node_unwrap_binary(rhs))) {
                break;
            }
            Node_binary* rhs_oper = node_unwrap_binary(rhs);
            Node* lhs_var_def;
            try(sym_tbl_lookup(&lhs_var_def, get_node_name(lhs)));
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
    if (!sym_tbl_lookup(&fun_def, fun_call->name)) {
        msg_undefined_function(fun_call);
        return;
    }
    Node_function_declaration* fun_decl;
    if (fun_def->type == NODE_FUNCTION_DEFINITION) {
        fun_decl = node_unwrap_function_definition(fun_def)->declaration;
    } else {
        fun_decl = node_unwrap_function_declaration(fun_def);
    }
    Node_function_return_types* fun_rtn_types = fun_decl->return_types;
    Node_lang_type* fun_rtn_type = fun_rtn_types->child;
    fun_call->lang_type = fun_rtn_type->lang_type;
    assert(fun_call->lang_type.str.count > 0);
    Node_function_params* params = fun_decl->parameters;
    size_t params_idx = 0;

    for (size_t arg_idx = 0; arg_idx < fun_call->args.info.count; arg_idx++) {
        Node* argument = node_ptr_vec_at(&fun_call->args, arg_idx);
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

        Node_variable_def* corresponding_param = node_unwrap_variable_def(node_ptr_vec_at(&params->params, params_idx));
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
    if (!sym_tbl_lookup(&struct_var, struct_memb_sym_untyped->name)) {
        msg_undefined_symbol(node_wrap(struct_memb_sym_untyped));
        return;
    }
    if (!sym_tbl_lookup(&struct_def_, node_unwrap_variable_def(struct_var)->lang_type.str)) {
        todo(); // this should possibly never happen
    }
    Node_struct_member_sym_untyped temp = *struct_memb_sym_untyped;
    Node_struct_def* struct_def = node_unwrap_struct_def(struct_def_);
    node_wrap(struct_memb_sym_untyped)->type = NODE_STRUCT_MEMBER_SYM_TYPED;
    Node_struct_member_sym_typed* struct_memb_sym_typed = node_unwrap_struct_member_sym_typed(node_wrap(struct_memb_sym_untyped));
    struct_memb_sym_typed->name = temp.name;
    assert(struct_memb_sym_typed->name.count > 0);
    struct_memb_sym_typed->lang_type.str = struct_def->name;
    struct_memb_sym_typed->lang_type.pointer_depth = 0;
    bool is_struct = true;
    Node_struct_def* prev_struct_def = struct_def;
    for (size_t idx = 0; idx < struct_memb_sym_typed->children.info.count; idx++) {
        Node* memb_sym_untyped_ = node_ptr_vec_at(&struct_memb_sym_typed->children, idx);
        Node_struct_member_sym_piece_untyped* memb_sym_untyped = node_unwrap_struct_member_sym_piece_untyped(memb_sym_untyped_);
        if (!is_struct) {
            todo();
        }
        if (!try_get_member_def(&curr_memb_def, struct_def, node_wrap(memb_sym_untyped))) {
            msg_invalid_struct_member(node_wrap(struct_memb_sym_typed), node_wrap(memb_sym_untyped));
            return;
        }
        if (!try_get_struct_definition(&struct_def, node_wrap(curr_memb_def))) {
            is_struct = false;
        }

        Node_struct_member_sym_piece_untyped temp_piece = *memb_sym_untyped;
        node_wrap(memb_sym_untyped)->type = NODE_STRUCT_MEMBER_SYM_PIECE_TYPED;
        Node_struct_member_sym_piece_typed* memb_sym_typed = node_unwrap_struct_member_sym_piece_typed(node_wrap(memb_sym_untyped));
        memb_sym_typed->name = temp_piece.name;
        memb_sym_typed->lang_type = curr_memb_def->lang_type;
        memb_sym_typed->struct_index = get_member_index(prev_struct_def, memb_sym_typed);

        prev_struct_def = struct_def;
    }
}

void set_return_statement_types(Node_return_statement* rtn_statement) {
    Node* child = rtn_statement->child;
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
        case NODE_BINARY: {
            try_set_operator_lang_type(node_unwrap_binary(child));
            break;
        }
        case NODE_LLVM_REGISTER_SYM:
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(child));
    }
}
