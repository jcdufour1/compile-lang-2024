
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
static void string_extend_strv_eval_escapes(Arena* arena, String* string, Str_view str_view) {
    while (str_view.count > 0) {
        char front_char = str_view_consume(&str_view);
        if (front_char == '\\') {
            string_append(arena, string, '\\');
            switch (str_view_consume(&str_view)) {
                case 'n':
                    string_extend_hex_2_digits(arena, string, 0x0a);
                    break;
                default:
                    unreachable("");
            }
        } else {
            string_append(arena, string, front_char);
        }
    }
}

static size_t get_count_excape_seq(Str_view str_view) {
    size_t count_excapes = 0;
    while (str_view.count > 0) {
        if (str_view_consume(&str_view) == '\\') {
            if (str_view.count < 1) {
                unreachable("invalid excape sequence");
            }

            str_view_consume(&str_view); // important in case of // excape sequence
            count_excapes++;
        }
    }
    return count_excapes;
}

// todo: take lang_type instead of Node*
static void extend_type_call_str(String* output, Str_view lang_type) {
    assert(lang_type.count > 0);
    Node* struct_def;
    if (sym_tbl_lookup(&struct_def, lang_type)) {
        string_extend_cstr(&a_main, output, "%struct.");
    }
    string_extend_strv(&a_main, output, lang_type);
}

static void extend_type_decl_str(String* output, const Node* variable_def, bool noundef) {
    assert(variable_def->lang_type.count > 0);

    if (variable_def->is_variadic) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    extend_type_call_str(output, variable_def->lang_type);
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void extend_literal_decl_prefix(String* output, const Node* var_decl_or_def) {
    Str_view lang_type = var_decl_or_def->lang_type;
    assert(lang_type.count > 0);
    if (str_view_cstr_is_equal(lang_type, "ptr")) {
        string_extend_cstr(&a_main, output, " @.");
        string_extend_strv(&a_main, output, var_decl_or_def->name);
    } else if (str_view_cstr_is_equal(lang_type, "i32")) {
        string_append(&a_main, output, ' ');
        string_extend_strv(&a_main, output, var_decl_or_def->str_data);
    } else {
        log(LOG_ERROR, NODE_FMT"\n", node_print(var_decl_or_def));
        log(LOG_ERROR, STR_VIEW_FMT"\n", str_view_print(lang_type));
        todo();
    }
}

static void extend_literal_decl(String* output, const Node* var_decl_or_def, bool noundef) {
    extend_type_decl_str(output, var_decl_or_def, noundef);
    extend_literal_decl_prefix(output, var_decl_or_def);
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
            string_extend_cstr(&a_main, output, ", ");
        }

        if (is_struct_variable_definition(param)) {
            string_extend_cstr(&a_main, output, "ptr noundef byval(");
            extend_type_call_str(output, param->lang_type);
            string_extend_cstr(&a_main, output, ")");
        } else {
            extend_type_decl_str(output, param, true);
        }
        if (param->is_variadic) {
            return;
        }
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, param->llvm_id);
    }
}

static void emit_fun_arg_struct_member_call(String* output, const Node* member_call) {
    assert(member_call->lang_type.count > 0);

    Node* var_def;
    if (!sym_tbl_lookup(&var_def, member_call->name)) {
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    extend_type_call_str(output, member_call->lang_type);
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, member_call->node_src->llvm_id);
}

static void emit_function_call_arguments(String* output, const Node* fun_call) {
    size_t idx = 0;
    nodes_foreach_child(argument, fun_call) {
        Node* var_decl_or_def;
        if (!sym_tbl_lookup(&var_decl_or_def, argument->name)) {
            msg(
                LOG_WARNING,
                argument->pos,
                "unknown variable: "STR_VIEW_FMT"\n",
                str_view_print(argument->name)
            );
            break;
        }
        if (idx++ > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }
        switch (argument->type) {
            case NODE_LITERAL: {
                extend_literal_decl(output, var_decl_or_def, true);
                break;
            }
            case NODE_STRUCT_MEMBER_SYM:
                emit_fun_arg_struct_member_call(output, argument);
                break;
            case NODE_STRUCT_LITERAL:
                todo();
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_SYMBOL_TYPED: {
                node_printf(var_decl_or_def);
                node_printf(argument);
                if (is_struct_variable_definition(var_decl_or_def)) {
                    string_extend_cstr(&a_main, output, "ptr noundef byval(");
                    node_printf(argument);
                    extend_type_call_str(output, var_decl_or_def->lang_type);
                    string_extend_cstr(&a_main, output, ")");
                } else {
                    extend_type_call_str(output, var_decl_or_def->lang_type);
                }
                string_extend_cstr(&a_main, output, " %");
                node_printf(argument);
                if (is_struct_variable_definition(var_decl_or_def)) {
                    string_extend_size_t(&a_main, output, get_store_dest_id(argument));
                } else {
                    string_extend_size_t(&a_main, output, argument->node_src->llvm_id);
                }
                break;
            }
            case NODE_FUNCTION_CALL:
                unreachable(""); // this function call should be changed to assign to a variable 
                               // before reaching emit_llvm stage, then assign that variable here. 
            default:
                unreachable(NODE_FMT"\n", node_print(argument));
        }
    }
}

static void emit_function_call(String* output, const Node* fun_call) {
    //assert(fun_call->llvm_id == 0);

    Node* fun_def;
    if (!sym_tbl_lookup(&fun_def, fun_call->name)) {
        unreachable(NODE_FMT"\n", node_print(fun_call));
    }

    // start of actual function call
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, fun_call->llvm_id);
    string_extend_cstr(&a_main, output, " = call ");
    extend_type_call_str(output, return_type_from_function_definition(fun_def)->lang_type);
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_call->name);

    // arguments
    string_extend_cstr(&a_main, output, "(");
    emit_function_call_arguments(output, fun_call);
    string_extend_cstr(&a_main, output, ")");

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_alloca(String* output, const Node* alloca) {
    assert(alloca->type == NODE_ALLOCA);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, alloca->llvm_id);
    string_extend_cstr(&a_main, output, " = alloca ");
    extend_type_call_str(output, get_symbol_def_from_alloca(alloca)->lang_type);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_operator_type(String* output, const Node* operator) {
    // TODO: do signed and unsigned operations correctly
    assert(operator->type == NODE_OPERATOR);
    
    switch (operator->token_type) {
        case TOKEN_SINGLE_MINUS:
            string_extend_cstr(&a_main, output, "sub nsw i32 ");
            break;
        case TOKEN_SINGLE_PLUS:
            string_extend_cstr(&a_main, output, "add nsw i32 ");
            break;
        case TOKEN_ASTERISK:
            string_extend_cstr(&a_main, output, "mul nsw i32 ");
            break;
        case TOKEN_SLASH:
            string_extend_cstr(&a_main, output, "sdiv i32 ");
            break;
        case TOKEN_LESS_THAN:
            string_extend_cstr(&a_main, output, "icmp slt i32 ");
            break;
        case TOKEN_GREATER_THAN:
            string_extend_cstr(&a_main, output, "icmp sgt i32 ");
            break;
        case TOKEN_DOUBLE_EQUAL:
            string_extend_cstr(&a_main, output, "icmp eq i32 ");
            break;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(operator->token_type));
    }
}

static void emit_operator_operand(String* output, const Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL:
            string_extend_strv(&a_main, output, operand->str_data);
            break;
        case NODE_SYMBOL_TYPED:
            // fallthrough
        case NODE_FUNCTION_RETURN_VALUE_SYM:
            // fallthrough
        case NODE_OPERATOR_RETURN_VALUE_SYM:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, operand->node_src->llvm_id);
            break;
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static void emit_operator(String* output, const Node* operator) {
    const Node* lhs = nodes_get_child_const(operator, 0);
    const Node* rhs = nodes_get_child_const(operator, 1);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, operator->llvm_id);
    string_extend_cstr(&a_main, output, " = ");
    emit_operator_type(output, operator);

    emit_operator_operand(output, lhs);
    string_extend_cstr(&a_main, output, ", ");
    emit_operator_operand(output, rhs);

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_load_variable(String* output, const Node* variable_call) {
    Node* variable_def;
    node_printf(variable_call);
    if (!sym_tbl_lookup(&variable_def, variable_call->name)) {
        unreachable("attempt to load a symbol that is undefined:"NODE_FMT"\n", node_print(variable_call));
    }
    assert(get_store_dest_id(variable_call) > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, variable_call->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
    extend_type_call_str(output, variable_def->lang_type);
    string_extend_cstr(&a_main, output, ", ");
    string_extend_cstr(&a_main, output, "ptr");
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, get_store_dest_id(variable_call));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_load_another_node(String* output, const Node* load_node) {
    assert(load_node->node_src->llvm_id > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, load_node->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
    extend_type_call_str(output, load_node->lang_type);
    string_extend_cstr(&a_main, output, ", ");
    string_extend_cstr(&a_main, output, "ptr");
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, load_node->node_src->llvm_id);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_llvm_store_struct_literal(String* output, const Node* memcpy_node) {
    assert(memcpy_node->type == NODE_LLVM_STORE_STRUCT_LITERAL);

    size_t alloca_dest_id = get_store_dest_id(memcpy_node);
    string_extend_cstr(&a_main, output, "    call void @llvm.memcpy.p0.p0.i64(ptr align 4 %");
    string_extend_size_t(&a_main, output, alloca_dest_id);
    string_extend_cstr(&a_main, output, ", ptr align 4 @__const.main.");
    string_extend_strv(&a_main, output, nodes_single_child_const(memcpy_node)->name);
    string_extend_cstr(&a_main, output, ", i64 ");
    string_extend_size_t(&a_main, output, sizeof_struct(nodes_single_child_const(memcpy_node)));
    string_extend_cstr(&a_main, output, ", i1 false)\n");
}

static void emit_store_another_node(String* output, const Node* store) {
    assert(store->type == NODE_STORE_ANOTHER_NODE);
    assert(store->lang_type.count > 0);
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(output, store->lang_type);
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, store->node_src->llvm_id);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, store->node_dest->llvm_id);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_llvm_store_literal(String* output, const Node* store) {
    assert(store->type == NODE_LLVM_STORE_LITERAL);
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(output, store->lang_type);
    extend_literal_decl_prefix(output, nodes_single_child_const(store));
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, store->node_dest->llvm_id);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_function_definition(String* output, const Node* fun_def) {
    string_extend_cstr(&a_main, output, "define dso_local ");

    extend_type_call_str(output, return_type_from_function_definition(fun_def)->lang_type);

    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_def->name);

    const Node* params = nodes_get_child_of_type_const(fun_def, NODE_FUNCTION_PARAMETERS);
    string_append(&a_main, output, '(');
    emit_function_params(output, params);
    string_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");

    const Node* block = nodes_get_child_of_type_const(fun_def, NODE_BLOCK);
    emit_block(output, block);

    string_extend_cstr(&a_main, output, "}\n");
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
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def->lang_type);
            string_extend_cstr(&a_main, output, " ");
            string_extend_strv(&a_main, output, sym_to_return->str_data);
            string_extend_cstr(&a_main, output, "\n");
            break;
        case NODE_SYMBOL_TYPED:
            if (sym_to_return->parent && sym_to_return->parent->type == NODE_STRUCT_MEMBER_SYM) {
                break;
            }
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, sym_to_return->node_src->llvm_id);
            string_extend_cstr(&a_main, output, "\n");
            break;
        case NODE_STRUCT_MEMBER_SYM: {
            Node* struct_def;
            if (!sym_tbl_lookup(&struct_def, sym_to_rtn_def->lang_type)) {
                unreachable("");
            }
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(output, sym_to_return->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, sym_to_return->node_src->llvm_id);
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        default:
            unreachable("");
    }
}

static void emit_function_declaration(String* output, const Node* fun_decl) {
    string_extend_cstr(&a_main, output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_decl->name);
    string_append(&a_main, output, '(');
    emit_function_params(output, nodes_get_child_of_type_const(fun_decl, NODE_FUNCTION_PARAMETERS));
    string_append(&a_main, output, ')');
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_label(String* output, const Node* label) {
    string_extend_cstr(&a_main, output, "\n");
    string_extend_size_t(&a_main, output, label->llvm_id);
    string_extend_cstr(&a_main, output, ":\n");
}

static void emit_goto(String* output, const Node* lang_goto) {
    string_extend_cstr(&a_main, output, "    br label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(lang_goto));
    string_append(&a_main, output, '\n');
}

static void emit_cond_goto(String* output, const Node* cond_goto) {
    const Node* label_if_true = nodes_get_child_const(cond_goto, 1);
    const Node* label_if_false = nodes_get_child_const(cond_goto, 2);

    Node* llvm_cmp_dest = 
        nodes_get_child_of_type_const(cond_goto, NODE_OPERATOR_RETURN_VALUE_SYM)->node_src;

    string_extend_cstr(&a_main, output, "    br i1 %");
    string_extend_size_t(&a_main, output, llvm_cmp_dest->llvm_id);
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(label_if_true));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(label_if_false));
    string_append(&a_main, output, '\n');
}

static void emit_struct_definition(String* output, const Node* statement) {
    string_extend_cstr(&a_main, output, "%struct.");
    string_extend_strv(&a_main, output, statement->name);
    string_extend_cstr(&a_main, output, " = type { ");
    bool is_first = true;
    nodes_foreach_child(member, statement) {
        if (!is_first) {
            string_extend_cstr(&a_main, output, ", ");
        }
        extend_type_decl_str(output, member, false);
        is_first = false;
    }
    string_extend_cstr(&a_main, output, " }\n");
}

static void emit_load_struct_element_pointer(String* output, const Node* load_elem_ptr) {
    Node* var_def;
    if (!sym_tbl_lookup(&var_def, load_elem_ptr->name)) {
        unreachable("");
    }
    Node* struct_def;
    if (!sym_tbl_lookup(&struct_def, var_def->lang_type)) {
        unreachable("");
    }
    size_t member_idx = get_member_index(struct_def, nodes_single_child_const(load_elem_ptr));
    assert(var_def->lang_type.count > 0);
    string_extend_cstr(&a_main, output, "    %"); 
    string_extend_size_t(&a_main, output, load_elem_ptr->llvm_id);
    string_extend_cstr(&a_main, output, " = getelementptr inbounds %struct.");
    string_extend_strv(&a_main, output, var_def->lang_type);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, load_elem_ptr->node_src->llvm_id);
    string_extend_cstr(&a_main, output, ", i32 0");
    string_extend_cstr(&a_main, output, ", i32 ");
    string_extend_size_t(&a_main, output, member_idx);
    string_append(&a_main, output, '\n');
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
                unreachable("an assignment should not still be present at this point");
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
            case NODE_STORE_VARIABLE:
                unreachable("");
                break;
            case NODE_LLVM_STORE_LITERAL:
                emit_llvm_store_literal(output, statement);
                break;
            case NODE_LLVM_STORE_STRUCT_LITERAL:
                emit_llvm_store_struct_literal(output, statement);
                break;
            case NODE_LOAD_VARIABLE:
                emit_load_variable(output, statement);
                break;
            case NODE_STRUCT_DEFINITION:
                emit_struct_definition(output, statement);
                break;
            case NODE_STORE_STRUCT_MEMBER:
                unreachable("");
                break;
            case NODE_OPERATOR:
                emit_operator(output, statement);
                break;
            case NODE_LOAD_STRUCT_ELEMENT_PTR:
                emit_load_struct_element_pointer(output, statement);
                break;
            case NODE_LOAD_ANOTHER_NODE:
                emit_load_another_node(output, statement);
                break;
            case NODE_STORE_ANOTHER_NODE:
                emit_store_another_node(output, statement);
                break;
            case NODE_STRUCT_MEMBER_SYM:
                break;
            case NODE_FOR_LOOP:
                unreachable("for loop should not still be present at this point\n");
            default:
                log(LOG_ERROR, STRING_FMT"\n", string_print(*output));
                node_printf(statement);
                todo();
        }
    }
    //get_block_return_id(fun_block) = get_block_return_id(fun_block->left_child);
}

static void emit_symbol(String* output, const Symbol_table_node node) {
    size_t literal_width = node.node->str_data.count + 1 -
                           get_count_excape_seq(node.node->str_data);

    string_extend_cstr(&a_main, output, "@.");
    string_extend_strv(&a_main, output, node.key);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant [ ");
    string_extend_size_t(&a_main, output, literal_width);
    string_extend_cstr(&a_main, output, " x i8] c\"");
    string_extend_strv_eval_escapes(&a_main, output, node.node->str_data);
    string_extend_cstr(&a_main, output, "\\00\", align 1");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_struct_literal(String* output, const Node* literal) {
    assert(literal->lang_type.count > 0);
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, literal->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    string_extend_strv(&a_main, output, literal->lang_type);
    string_extend_cstr(&a_main, output, " {");

    size_t is_first = true;
    nodes_foreach_child(member_store, literal) {
        if (!is_first) {
            string_append(&a_main, output, ',');
        }
        assert(member_store->type == NODE_STORE_VARIABLE);
        const Node* member = nodes_single_child_const(member_store);
        extend_literal_decl(output, member, false);
        is_first = false;
    }

    string_extend_cstr(&a_main, output, "} , align 4\n");
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

