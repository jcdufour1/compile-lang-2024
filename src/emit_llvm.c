
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"
#include "parameters.h"
#include "file.h"
#include "parser_utils.h"

static void emit_block(String* output, const Node* fun_block);

// \n excapes are actually stored as is in tokens and nodes, but should be printed as \0a
static void string_extend_strv_eval_escapes(String* string, Str_view str_view) {
    while (str_view.count > 0) {
        char front_char = str_view_chop_front(&str_view);
        if (front_char == '\\') {
            string_append(string, '\\');
            switch (str_view_chop_front(&str_view)) {
                case 'n':
                    string_extend_hex_2_digits(string, 0x0a);
                    break;
                default:
                    unreachable("");
            }
        } else {
            string_append(string, front_char);
        }
    }
}

static size_t get_count_excape_seq(Str_view str_view) {
    size_t count_excapes = 0;
    while (str_view.count > 0) {
        if (str_view_chop_front(&str_view) == '\\') {
            if (str_view.count < 1) {
                unreachable("invalid excape sequence");
            }

            str_view_chop_front(&str_view); // important in case of // excape sequence
            count_excapes++;
        }
    }
    return count_excapes;
}

static void extend_type_call_str(String* output, const Node* sym_def) {
    node_printf(sym_def);
    Node* struct_def;
    if (sym_tbl_lookup(&struct_def, sym_def->lang_type)) {
        string_extend_cstr(output, "%struct.");
    }
    string_extend_strv(output, sym_def->lang_type);
}

static void extend_type_decl_str(String* output, const Node* variable_def) {
    if (variable_def->is_variadic) {
        string_extend_cstr(output, "...");
        return;
    }

    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, " noundef");
}

static void extend_literal_decl_prefix(String* output, const Node* var_decl_or_def) {
    Str_view lang_type = var_decl_or_def->lang_type;
    if (str_view_cstr_is_equal(lang_type, "ptr")) {
        string_extend_cstr(output, " @.");
        string_extend_strv(output, var_decl_or_def->name);
    } else if (str_view_cstr_is_equal(lang_type, "i32")) {
        string_append(output, ' ');
        string_extend_strv(output, var_decl_or_def->str_data);
    } else {
        todo();
    }
}

static void extend_literal_decl(String* output, const Node* var_decl_or_def) {
    extend_type_decl_str(output, var_decl_or_def);
    extend_literal_decl_prefix(output, var_decl_or_def);
}

static size_t get_member_index(const Node* struct_def, const Node* member_symbol) {
    size_t idx = 0;
    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(curr_member->name, member_symbol->name)) {
            return idx;
        }
        idx++;
    }
    unreachable("member not found");
}

static const Node* get_member_def(const Node* struct_def, const Node* member_symbol) {
    assert(struct_def->type == NODE_STRUCT_DEFINITION);
    assert(member_symbol->type == NODE_SYMBOL);

    nodes_foreach_child(curr_member, struct_def) {
        if (str_view_is_equal(curr_member->name, member_symbol->name)) {
            return curr_member;
        }
    }
    unreachable("member not found");
}

static Node* return_type_from_function_definition(const Node* fun_def) {
    const Node* return_types = nodes_get_child_of_type_const(fun_def, NODE_FUNCTION_RETURN_TYPES);
    assert(nodes_count_children(return_types) < 2);
    if (nodes_count_children(return_types) > 0) {
        return return_types->left_child;
    }
    unreachable("");
}

static void emit_function_params(String* output, const Node* fun_params) {
    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        if (is_struct_variable_definition(param)) {
            string_extend_cstr(output, "ptr noundef byval(");
            extend_type_call_str(output, param);
            string_extend_cstr(output, ")");
        } else {
            extend_type_decl_str(output, param);
        }
        if (param->is_variadic) {
            return;
        }
        string_extend_cstr(output, " %");
        string_extend_size_t(output, param->llvm_id);
    }
}

static void emit_fun_arg_struct_member_call(String* output, const Node* member_call) {
    Node* var_def;
    if (!sym_tbl_lookup(&var_def, member_call->name)) {
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    const Node* member_def = get_member_def(struct_def, nodes_single_child_const(member_call));
    if (is_struct_variable_definition(member_def)) {
        string_extend_cstr(output, "ptr noundef byval(");
        extend_type_call_str(output, member_def);
        string_extend_cstr(output, ")");
    } else {
        extend_type_call_str(output, member_def);
    }
    string_extend_cstr(output, " %");
    string_extend_size_t(output, get_prev_load_id(member_call));
}

static void emit_function_call_arguments(String* output, const Node* fun_call) {
    size_t idx = 0;
    nodes_foreach_child(argument, fun_call) {
        Node* var_decl_or_def;
        if (!sym_tbl_lookup(&var_decl_or_def, argument->name)) {
            msg(
                LOG_WARNING,
                params.input_file_name,
                argument->line_num,
                "unknown variable: "STR_VIEW_FMT"\n",
                str_view_print(argument->name)
            );
            break;
        }
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }
        switch (argument->type) {
            case NODE_LITERAL: {
                extend_literal_decl(output, var_decl_or_def);
                break;
            }
            case NODE_STRUCT_MEMBER_CALL: {
                emit_fun_arg_struct_member_call(output, argument);
                break;
            }
            case NODE_SYMBOL: {
                node_printf(var_decl_or_def);
                if (is_struct_variable_definition(var_decl_or_def)) {
                    string_extend_cstr(output, "ptr noundef byval(");
                    extend_type_call_str(output, var_decl_or_def);
                    string_extend_cstr(output, ")");
                } else {
                    extend_type_call_str(output, var_decl_or_def);
                }
                string_extend_cstr(output, " %");
                node_printf(argument);
                log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
                if (is_struct_variable_definition(var_decl_or_def)) {
                    string_extend_size_t(output, get_store_dest_id(argument));
                } else {
                    string_extend_size_t(output, get_prev_load_id(argument));
                }
                break;
            }
            case NODE_FUNCTION_CALL:
                unreachable(""); // this function call should be changed to assign to a variable 
                               // before reaching emit_llvm stage, then assign that variable here. 
            default:
                todo();
        }
    }
}

static void emit_function_call(String* output, const Node* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // load arguments
    nodes_foreach_child(argument, fun_call) {
        if (argument->type == NODE_SYMBOL) {
            //emit_load_variable(output, argument);
        } else if (argument->type == NODE_LITERAL) {
        } else if (argument->type == NODE_STRUCT_MEMBER_CALL) {
        } else {
            node_printf(argument);
            todo();
        }
    }

    Node* fun_def;
    if (!sym_tbl_lookup(&fun_def, fun_call->name)) {
        unreachable(NODE_FMT"\n", node_print(fun_call));
    }

    // start of actual function call
    string_extend_cstr(output, "    %");
    string_extend_size_t(output, fun_call->llvm_id);
    string_extend_cstr(output, " = call ");
    extend_type_call_str(output, return_type_from_function_definition(fun_def));
    string_extend_cstr(output, " @");
    string_extend_strv(output, fun_call->name);

    // arguments
    string_extend_cstr(output, "(");
    emit_function_call_arguments(output, fun_call);
    string_extend_cstr(output, ")");

    string_extend_cstr(output, "\n");
}

static void emit_alloca(String* output, const Node* alloca) {
    assert(alloca->type == NODE_ALLOCA);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, alloca->llvm_id);
    string_extend_cstr(output, " = alloca ");
    extend_type_call_str(output, get_symbol_def_from_alloca(alloca));
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_operator_type(String* output, const Node* operator) {
    // TODO: do signed and unsigned operations correctly
    assert(operator->type == NODE_OPERATOR);
    
    switch (operator->token_type) {
        case TOKEN_SINGLE_MINUS:
            string_extend_cstr(output, "sub nsw i32 ");
            break;
        case TOKEN_SINGLE_PLUS:
            string_extend_cstr(output, "add nsw i32 ");
            break;
        case TOKEN_ASTERISK:
            string_extend_cstr(output, "mul nsw i32 ");
            break;
        case TOKEN_SLASH:
            string_extend_cstr(output, "sdiv i32 ");
            break;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(operator->token_type));
    }
}

static void emit_operator_operand(String* output, const Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL:
            string_extend_strv(output, operand->str_data);
            break;
        case NODE_SYMBOL:
            string_extend_cstr(output, "%");
            string_extend_size_t(output, get_prev_load_id(operand));
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static void emit_operator(String* output, const Node* operator) {
    const Node* lhs = nodes_get_child_const(operator, 0);
    const Node* rhs = nodes_get_child_const(operator, 1);

    // emit prereq function calls (may not be needed), etc. before actual operation
    switch (lhs->type) {
        case NODE_SYMBOL:
            break;
        case NODE_LITERAL:
            break;
        case NODE_FUNCTION_CALL:
            todo();
        case NODE_OPERATOR:
            todo(); // this possibly should not ever happen
        default:
            todo();
    }

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, operator->llvm_id);

    string_extend_cstr(output, " = ");
    emit_operator_type(output, operator);

    emit_operator_operand(output, lhs);
    string_extend_cstr(output, ", ");
    emit_operator_operand(output, rhs);

    string_extend_cstr(output, "\n");
}

static void emit_src(String* output, const Node* src) {
    //log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
    switch (src->type) {
        case NODE_LITERAL:
            extend_literal_decl_prefix(output, src);
            break;
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_OPERATOR:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, src->llvm_id);
            break;
        case NODE_SYMBOL:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_prev_load_id(src));
            break;
        case NODE_FUNCTION_PARAM_CALL:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_matching_fun_param_load_id(src));
            break;
        default:
            node_printf(src);
            todo();
    }
}

static void emit_load_variable(String* output, const Node* variable_call) {
    Node* variable_def;
    node_printf(variable_call);
    if (!sym_tbl_lookup(&variable_def, variable_call->name)) {
        unreachable("attempt to load a symbol that is undefined:"NODE_FMT"\n", node_print(variable_call));
    }
    //log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
    //log_tree(LOG_DEBUG, variable_call);
    assert(get_store_dest_id(variable_call) > 0);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, variable_call->llvm_id);
    string_extend_cstr(output, " = load ");
    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, ", ");
    string_extend_cstr(output, "ptr");
    string_extend_cstr(output, " %");
    string_extend_size_t(output, get_store_dest_id(variable_call));
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_store_struct_literal(String* output, const Node* store) {
    size_t alloca_dest_id = get_store_dest_id(store);
    string_extend_cstr(output, "    call void @llvm.memcpy.p0.p0.i64(ptr align 4 %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", ptr align 4 @__const.main.");
    string_extend_strv(output, nodes_single_child_const(store)->name);
    string_extend_cstr(output, ", i64 ");
    string_extend_size_t(output, sizeof_struct(nodes_single_child_const(store)));
    string_extend_cstr(output, ", i1 false)\n");
}

static void emit_store_struct_fun_param(String* output, const Node* store) {
    size_t alloca_dest_id = get_store_dest_id(store);

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, store->name)) {
        node_printf(store);
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }

    const Node* src = nodes_single_child_const(store);
    string_extend_cstr(output, "    call void @llvm.memcpy.p0.p0.i64(ptr align 4 %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", ptr align 4 %");
    string_extend_size_t(output, get_matching_fun_param_load_id(src));
    string_extend_cstr(output, ", i64 ");
    string_extend_size_t(output, sizeof_struct_definition(struct_def));
    string_extend_cstr(output, ", i1 false)\n");
}

static void emit_normal_store(String* output, const Node* store) {
    size_t alloca_dest_id = get_store_dest_id(store);
    const Node* var_or_member_def;
    if (store->type == NODE_STORE_STRUCT_MEMBER) {
        var_or_member_def = get_symbol_def_from_alloca(store);
    } else {
        var_or_member_def = get_symbol_def_from_alloca(store);
    }
    assert(alloca_dest_id > 0);

    bool is_fun_param_call = false;
    // emit prerequisite call, etc before actually storing
    const Node* src = nodes_single_child_const(store);
    switch (src->type) {
        case NODE_FUNCTION_CALL:
            emit_function_call(output, src);
            break;
        case NODE_OPERATOR:
            emit_operator(output, src);
            break;
        case NODE_LITERAL:
            break;
        case NODE_SYMBOL:
            //emit_load_variable(output, src);
            break;
        case NODE_FUNCTION_PARAM_CALL:
            is_fun_param_call = true;
            //emit_load_variable(output, src);
            break;
        default:
            log_tree(LOG_DEBUG, store);
            unreachable(NODE_FMT"\n", node_print(src));
    }

    string_extend_cstr(output, "    store ");
    if (is_struct_variable_definition(var_or_member_def) && is_fun_param_call) {
        node_printf(store);
        node_printf(var_or_member_def);
        node_printf(src);
        unreachable("");
    } else {
        extend_type_call_str(output, var_or_member_def);
    }

    emit_src(output, src);

    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_store_struct_member(String* output, const Node* store_struct) {
    Node* var_def;
    if (!sym_tbl_lookup(&var_def, store_struct->name)) {
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    size_t member_idx = get_member_index(struct_def, nodes_get_child_const(store_struct, 0));
    string_extend_cstr(output, "    %"); 
    string_extend_size_t(output, store_struct->llvm_id - 1);
    string_extend_cstr(output, " = getelementptr inbounds %struct.");
    string_extend_strv(output, var_def->lang_type);
    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, get_store_dest_id(store_struct));
    string_extend_cstr(output, ", i32 0");
    string_extend_cstr(output, ", i32 ");
    string_extend_size_t(output, member_idx);
    string_append(output, '\n');

    const Node* member_def = get_member_def(struct_def, nodes_get_child_const(store_struct, 0));
    string_extend_cstr(output, "    store ");
    extend_type_call_str(output, member_def);
    emit_src(output, nodes_get_child_const(store_struct, 1));
    string_extend_cstr(output, ", ");
    string_extend_cstr(output, "ptr");
    string_extend_cstr(output, " %");
    string_extend_size_t(output, store_struct->llvm_id - 1);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_store(String* output, const Node* store) {
    const Node* src = nodes_single_child_const(store);
    bool is_struct_def = is_struct_variable_definition(get_symbol_def_from_alloca(store));
    if (store->left_child->type == NODE_STRUCT_LITERAL) {
        emit_store_struct_literal(output, store);
    } else if (store->left_child->type == NODE_STORE_STRUCT_MEMBER) {
        emit_store_struct_member(output, store);
    } else if (is_struct_def && src->type == NODE_FUNCTION_PARAM_CALL) {
        emit_store_struct_fun_param(output, store);
    } else {
        emit_normal_store(output, store);
    }
}

static void emit_function_definition(String* output, const Node* fun_def) {
    string_extend_cstr(output, "define dso_local ");

    extend_type_call_str(output, return_type_from_function_definition(fun_def));

    string_extend_cstr(output, " @");
    string_extend_strv(output, fun_def->name);

    const Node* params = nodes_get_child_of_type_const(fun_def, NODE_FUNCTION_PARAMETERS);
    string_append(output, '(');
    emit_function_params(output, params);
    string_append(output, ')');

    string_extend_cstr(output, " {\n");

    const Node* block = nodes_get_child_of_type_const(fun_def, NODE_BLOCK);
    emit_block(output, block);

    string_extend_cstr(output, "}\n");
}

static void emit_function_return_statement(String* output, const Node* fun_return) {
    Node* sym_to_return = fun_return->left_child;
    Node* sym_to_rtn_def;
    if (!sym_tbl_lookup(&sym_to_rtn_def, sym_to_return->name)) {
        unreachable("");
    }

    switch (sym_to_return->type) {
        case NODE_LITERAL:
            if (sym_to_return->token_type != TOKEN_NUM_LITERAL) {
                todo();
            }
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " ");
            string_extend_strv(output, sym_to_return->str_data);
            string_extend_cstr(output, "\n");
            break;
        case NODE_FUNCTION_CALL: {
            emit_function_call(output, sym_to_return);
            string_extend_cstr(output, "    ret ");

            Node* return_types = nodes_get_child_of_type(sym_to_rtn_def, NODE_FUNCTION_RETURN_TYPES);
            assert(nodes_count_children(return_types) == 1 && "not implemented");
            Node* return_type = return_types->left_child;

            extend_type_call_str(output, return_type);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, sym_to_return->llvm_id);
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_BLOCK: {
            emit_block(output, sym_to_return);
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_block_return_id(sym_to_return));
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_SYMBOL:
            if (sym_to_return->parent && sym_to_return->parent->type == NODE_STRUCT_MEMBER_CALL) {
                break;
            }
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_prev_load_id(sym_to_return));
            string_extend_cstr(output, "\n");
            break;
        case NODE_STRUCT_MEMBER_CALL: {
            Node* struct_def;
            if (!sym_tbl_lookup(&struct_def, sym_to_rtn_def->lang_type)) {
                unreachable("");
            }
            const Node* member_def = get_member_def(
                struct_def, nodes_single_child_const(sym_to_return)
            );
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, member_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_prev_load_id(sym_to_return));
            string_extend_cstr(output, "\n");
            break;
        }
        default:
            todo();
    }
}

static void emit_function_declaration(String* output, const Node* fun_decl) {
    string_extend_cstr(output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(output, " @");
    string_extend_strv(output, fun_decl->name);
    string_append(output, '(');
    emit_function_params(output, nodes_get_child_of_type_const(fun_decl, NODE_FUNCTION_PARAMETERS));
    string_append(output, ')');
    string_extend_cstr(output, "\n");
}

static void emit_label(String* output, const Node* label) {
    string_extend_cstr(output, "\n");
    string_extend_size_t(output, label->llvm_id);
    string_extend_cstr(output, ":\n");
}

static void emit_compare(String* output, const Node* operator) {
    assert(operator->type == NODE_OPERATOR);
    const Node* lhs = nodes_get_child_const(operator, 0);
    const Node* rhs = nodes_get_child_const(operator, 1);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, operator->llvm_id);
    string_extend_cstr(output, " = icmp ");

    switch (operator->token_type) {
        case TOKEN_LESS_THAN:
            string_extend_cstr(output, "slt");
            break;
        case TOKEN_GREATER_THAN:
            string_extend_cstr(output, "sgt");
            break;
        default:
            unreachable("");
    }

    string_extend_cstr(output, " i32 ");
    emit_operator_operand(output, lhs);
    string_extend_cstr(output, ", ");
    emit_operator_operand(output, rhs);
    string_extend_cstr(output, "\n");
}

static void emit_goto(String* output, const Node* lang_goto) {
    string_extend_cstr(output, "    br label %");
    string_extend_size_t(output, get_matching_label_id(lang_goto));
    string_append(output, '\n');
}

static void emit_cond_goto(String* output, const Node* cond_goto) {
    const Node* label_if_true = nodes_get_child_const(cond_goto, 1);
    const Node* label_if_false = nodes_get_child_const(cond_goto, 2);

    const Node* operator = nodes_get_child_of_type_const(cond_goto, NODE_OPERATOR);
    switch (operator->token_type) {
        case TOKEN_LESS_THAN:
            // fallthrough
        case TOKEN_GREATER_THAN:
            break;
        default:
            unreachable("");
    }
    assert(nodes_count_children(operator) == 2);
    size_t llvm_cmp_dest = operator->llvm_id;

    emit_compare(output, operator);

    string_extend_cstr(output, "    br i1 %");
    string_extend_size_t(output, llvm_cmp_dest);
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_true));
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_false));
    string_append(output, '\n');
}

static void emit_struct_definition(String* output, const Node* statement) {
    string_extend_cstr(output, "%struct.");
    string_extend_strv(output, statement->name);
    string_extend_cstr(output, " = type { ");
    bool is_first = true;
    nodes_foreach_child(member, statement) {
        if (!is_first) {
            string_extend_cstr(output, ", ");
        }
        string_extend_strv(output, member->lang_type);
        is_first = false;
    }
    string_extend_cstr(output, " }\n");
}

static void emit_load_struct_member(String* output, const Node* load_member) {
    Node* var_def;
    if (!sym_tbl_lookup(&var_def, load_member->name)) {
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    size_t member_idx = get_member_index(struct_def, nodes_single_child_const(load_member));
    const Node* member_def = get_member_def(struct_def, nodes_single_child_const(load_member));
    string_extend_cstr(output, "    %"); 
    string_extend_size_t(output, load_member->llvm_id - 1);
    string_extend_cstr(output, " = getelementptr inbounds %struct.");
    string_extend_strv(output, var_def->lang_type);
    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, get_store_dest_id(load_member));
    string_extend_cstr(output, ", i32 0");
    string_extend_cstr(output, ", i32 ");
    string_extend_size_t(output, member_idx);
    string_append(output, '\n');

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, load_member->llvm_id);
    string_extend_cstr(output, " = load ");
    extend_type_call_str(output, member_def);
    string_extend_cstr(output, ", ");
    string_extend_cstr(output, "ptr");
    string_extend_cstr(output, " %");
    string_extend_size_t(output, load_member->llvm_id - 1);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_block(String* output, const Node* block) {
    assert(block->type == NODE_BLOCK);

    nodes_foreach_child(statement, block) {
        switch (statement->type) {
            case NODE_FUNCTION_DEFINITION:
                emit_function_definition(output, statement);
                break;
            case NODE_FUNCTION_CALL:
                emit_function_call(output, statement);
                break;
            case NODE_RETURN_STATEMENT:
                emit_function_return_statement(output, statement);
                break;
            case NODE_VARIABLE_DEFINITION:
                break;
            case NODE_FUNCTION_DECLARATION:
                emit_function_declaration(output, statement);
                break;
            case NODE_ASSIGNMENT:
                // TODO: remove this case
                break;
            case NODE_BLOCK:
                emit_block(output, statement);
                break;
            case NODE_LABEL:
                emit_label(output, statement);
                break;
            case NODE_COND_GOTO:
                emit_cond_goto(output, statement);
                break;
            case NODE_GOTO:
                emit_goto(output, statement);
                break;
            case NODE_ALLOCA:
                emit_alloca(output, statement);
                break;
            case NODE_STORE:
                emit_store(output, statement);
                break;
            case NODE_LOAD:
                emit_load_variable(output, statement);
                break;
            case NODE_STRUCT_DEFINITION:
                emit_struct_definition(output, statement);
                break;
            case NODE_LOAD_STRUCT_MEMBER:
                emit_load_struct_member(output, statement);
                break;
            case NODE_STORE_STRUCT_MEMBER:
                emit_store_struct_member(output, statement);
                break;
            case NODE_FOR_LOOP:
                unreachable("for loop should not still be present at this point\n");
            default:
                //log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
                node_printf(statement);
                todo();
        }
    }
    //get_block_return_id(fun_block) = get_block_return_id(fun_block->left_child);
}

static void emit_symbol(String* output, const Symbol_table_node node) {
    size_t literal_width = node.node->str_data.count + 1 -
                           get_count_excape_seq(node.node->str_data);

    string_extend_cstr(output, "@.");
    string_extend_strv(output, node.key);
    string_extend_cstr(output, " = private unnamed_addr constant [ ");
    string_extend_size_t(output, literal_width);
    string_extend_cstr(output, " x i8] c\"");
    string_extend_strv_eval_escapes(output, node.node->str_data);
    string_extend_cstr(output, "\\00\", align 1");
    string_extend_cstr(output, "\n");
}

static void emit_struct_literal(String* output, const Node* literal) {
    string_extend_cstr(output, "@__const.main.");
    string_extend_strv(output, literal->name);
    string_extend_cstr(output, " = private unnamed_addr constant %struct.Div {");

    size_t is_first = true;
    nodes_foreach_child(member_store, literal) {
        if (!is_first) {
            string_append(output, ',');
        }
        assert(member_store->type == NODE_STORE);
        const Node* member = nodes_single_child_const(member_store);
        string_extend_strv(output, member->lang_type);
        string_append(output, ' ');
        string_extend_strv(output, member->str_data);
        is_first = false;
    }

    string_extend_cstr(output, "} , align 4\n");
}

static void emit_symbols(String* output) {
    for (size_t idx = 0; idx < symbol_table.capacity; idx++) {
        const Symbol_table_node curr_node = symbol_table.table_nodes[idx];
        if (curr_node.status != SYM_TBL_OCCUPIED) {
            continue;
        }
        switch (curr_node.node->type) {
            case NODE_LITERAL:
                emit_symbol(output, curr_node);
                break;
            case NODE_STRUCT_LITERAL:
                emit_struct_literal(output, curr_node.node);
                break;
            case NODE_VARIABLE_DEFINITION:
                // fallthrough
            case NODE_LANG_TYPE:
                // fallthrough
            case NODE_FUNCTION_CALL:
                // fallthrough
            case NODE_FUNCTION_DECLARATION:
                // fallthrough
            case NODE_LABEL:
                // fallthrough
            case NODE_FUNCTION_DEFINITION:
                // fallthrough
            case NODE_STRUCT_DEFINITION:
                break;
            default:
                log(LOG_FETAL, NODE_FMT"\n", node_print(curr_node.node));
                todo();
        }
    }
}

void emit_llvm_from_tree(const Node* root) {
    String output = {0};

    emit_block(&output, root);

    emit_symbols(&output);

    log(LOG_NOTE, "\n"STRING_FMT"\n", string_print(output));

    Str_view final_output = {.str = output.buf, .count = output.info.count};
    write_file("test.ll", final_output);
}

