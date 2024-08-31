
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"
#include "parameters.h"
#include "file.h"
#include "parser_utils.h"

static void emit_block(String* output, Node_id fun_block);
static void emit_llvm_main(String* output, Node_id root);

static void extend_type_call_str(String* output, Node_id variable_def) {
    assert(nodes_at(variable_def)->lang_type.count > 0);
    if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
        string_extend_cstr(output, "i32");
    } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "ptr")) {
        string_extend_cstr(output, "ptr");
    } else {
        todo();
    }
}

static void extend_type_decl_str(String* output, Node_id variable_def) {
    if (nodes_at(variable_def)->is_variadic) {
        string_extend_cstr(output, "...");
        return;
    }

    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, " noundef");
}

static void extend_literal_decl_prefix(String* output, Node_id var_decl_or_def) {
    Str_view lang_type = nodes_at(var_decl_or_def)->lang_type;
    if (0 == str_view_cmp_cstr(lang_type, "ptr")) {
        string_extend_cstr(output, " @.");
        string_extend_strv(output, nodes_at(var_decl_or_def)->name);
    } else if (0 == str_view_cmp_cstr(lang_type, "i32")) {
        string_append(output, ' ');
        string_extend_strv(output, nodes_at(var_decl_or_def)->str_data);
    } else {
        todo();
    }
}

static void extend_literal_decl(String* output, Node_id var_decl_or_def) {
    extend_type_decl_str(output, var_decl_or_def);
    extend_literal_decl_prefix(output, var_decl_or_def);
}

static Node_id return_type_from_function_definition(Node_id fun_def) {
    Node_id return_types = nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES);
    assert(nodes_count_children(return_types) < 2);
    if (nodes_count_children(return_types) > 0) {
        return nodes_at(return_types)->left_child;
    }
    unreachable("");
}

static void emit_function_params(String* output, Node_id fun_params) {
    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        extend_type_decl_str(output, param);
        if (nodes_at(param)->is_variadic) {
            return;
        }
        string_extend_cstr(output, " %");
        string_extend_size_t(output, nodes_at(param)->llvm_id);
    }
}

static void emit_function_call_arguments(String* output, Node_id fun_call) {
    size_t idx = 0;
    nodes_foreach_child(argument, fun_call) {
        Node_id var_decl_or_def;
        if (!sym_tbl_lookup(&var_decl_or_def, nodes_at(argument)->name)) {
            msg(
                LOG_WARNING,
                params.input_file_name,
                nodes_at(argument)->line_num,
                "unknown variable: "STR_VIEW_FMT"\n",
                str_view_print(nodes_at(argument)->name)
            );
            break;
        }
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }
        switch (nodes_at(argument)->type) {
            case NODE_LITERAL: {
                extend_literal_decl(output, var_decl_or_def);
                break;
            }
            case NODE_SYMBOL: {
                size_t llvm_id = get_prev_load_id(argument);
                log_tree(LOG_DEBUG, argument);
                assert(llvm_id > 0);
                extend_type_call_str(output, var_decl_or_def);
                string_extend_cstr(output, " %");
                string_extend_size_t(output, llvm_id);
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

static void emit_function_call(String* output, Node_id fun_call) {
    //assert(nodes_at(fun_call)->llvm_id == 0);

    // load arguments
    nodes_foreach_child(argument, fun_call) {
        if (nodes_at(argument)->type == NODE_SYMBOL) {
            //emit_load_variable(output, argument);
        } else if (nodes_at(argument)->type == NODE_LITERAL) {
        } else {
            todo();
        }
    }

    Node_id fun_def;
    if (!sym_tbl_lookup(&fun_def, nodes_at(fun_call)->name)) {
        unreachable("");
    }

    // start of actual function call
    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(fun_call)->llvm_id);
    string_extend_cstr(output, " = call ");
    extend_type_call_str(output, return_type_from_function_definition(fun_def));
    string_extend_cstr(output, " @");
    string_extend_strv(output, nodes_at(fun_call)->name);

    // arguments
    string_extend_cstr(output, "(");
    emit_function_call_arguments(output, fun_call);
    string_extend_cstr(output, ")");

    string_extend_cstr(output, "\n");
}

static void emit_alloca(String* output, Node_id alloca) {
    assert(nodes_at(alloca)->type == NODE_ALLOCA);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(alloca)->llvm_id);
    string_extend_cstr(output, " = alloca ");
    node_printf(alloca);
    extend_type_call_str(output, get_symbol_def_from_alloca(alloca));
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_src_of_assignment(String* output, Node_id variable_def, void* item) {
    Node_id rhs = *(Node_id*)item;

    switch (nodes_at(rhs)->type) {
        case NODE_SYMBOL:
            // fallthrough
        case NODE_LITERAL:
            extend_literal_decl_prefix(output, variable_def);
            break;
        case NODE_FUNCTION_CALL:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_block_return_id(rhs));
            break;
        default:
            todo();
    }
}

static void emit_src_literal(String* output, Node_id src) {
    extend_literal_decl_prefix(output, src);
}

static void emit_src_function_call_result(String* output, Node_id store) {
    Node_id fun_call = nodes_at(store)->left_child;
    assert(!node_is_null(fun_call));
    assert(nodes_at(fun_call)->type == NODE_FUNCTION_CALL);

    string_extend_cstr(output, " %");
    string_extend_size_t(output, nodes_at(fun_call)->llvm_id);
}

static void emit_src(String* output, Node_id store) {
    log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
    Node_id src = nodes_single_child(store);
    switch (nodes_at(src)->type) {
        case NODE_LITERAL:
            emit_src_literal(output, src);
            break;
        case NODE_FUNCTION_CALL:
            emit_src_function_call_result(output, store);
            break;
        default:
            todo();
    }
}

static void emit_store(String* output, Node_id store) {
    size_t alloca_dest_id = get_store_dest_id(store);
    Node_id var_def = get_symbol_def_from_alloca(store);
    assert(alloca_dest_id > 0);

    Node_id src = nodes_single_child(store);
    if (nodes_at(src)->type == NODE_FUNCTION_CALL) {
        emit_function_call(output, src);
    }

    string_extend_cstr(output, "    store ");
    extend_type_call_str(output, var_def);

    emit_src(output, store);

    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_src_of_fun_params(String* output, Node_id variable_def, void* item) {
    (void) item;
    size_t src_llvm_id = nodes_at(variable_def)->llvm_id;

    switch (nodes_at(variable_def)->type) {
        case NODE_VARIABLE_DEFINITION:
            // fallthrough
        case NODE_LANG_TYPE:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, src_llvm_id);
            break;
        default:
            todo();
    }
}

static void emit_function_definition(String* output, Node_id fun_def) {
    string_extend_cstr(output, "define dso_local ");

    extend_type_call_str(output, return_type_from_function_definition(fun_def));

    string_extend_cstr(output, " @");
    string_extend_strv(output, nodes_at(fun_def)->name);

    Node_id params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    string_append(output, '(');
    emit_function_params(output, params);
    string_append(output, ')');

    string_extend_cstr(output, " {\n");

    Node_id block = nodes_get_child_of_type(fun_def, NODE_BLOCK);
    emit_block(output, block);

    string_extend_cstr(output, "}\n");
}

static void emit_operator(String* output, Node_id operator) {
    Node_id child_lhs = nodes_at(operator)->left_child;
    Node_id child_rhs = nodes_at(child_lhs)->next;

    if (nodes_at(child_lhs)->type != NODE_LITERAL || nodes_at(child_rhs)->type != NODE_LITERAL) {
        todo();
    }

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(operator)->llvm_id);

    string_extend_cstr(output, " = mul nsw i32 ");
    string_extend_strv(output, nodes_at(child_lhs)->str_data);

    string_extend_cstr(output, ", ");

    string_extend_strv(output, nodes_at(child_rhs)->str_data);

    string_extend_cstr(output, "\n");
}

static void emit_function_return_statement(String* output, Node_id fun_return) {
    Node_id sym_to_return = nodes_at(fun_return)->left_child;
    Node_id sym_to_rtn_def;
    if (!sym_tbl_lookup(&sym_to_rtn_def, nodes_at(sym_to_return)->name)) {
        todo();
    }

    switch (nodes_at(sym_to_return)->type) {
        case NODE_LITERAL:
            if (nodes_at(sym_to_return)->token_type != TOKEN_NUM_LITERAL) {
                todo();
            }
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " ");
            string_extend_strv(output, nodes_at(sym_to_return)->str_data);
            string_extend_cstr(output, "\n");
            break;
        case NODE_FUNCTION_CALL: {
            emit_function_call(output, sym_to_return);
            string_extend_cstr(output, "    ret ");

            Node_id return_types = nodes_get_child_of_type(sym_to_rtn_def, NODE_FUNCTION_RETURN_TYPES);
            assert(nodes_count_children(return_types) == 1 && "not implemented");
            Node_id return_type = nodes_at(return_types)->left_child;

            extend_type_call_str(output, return_type);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_block_return_id(sym_to_return));
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
        case NODE_OPERATOR: {
            todo();
            emit_operator(output, sym_to_return);
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_block_return_id(sym_to_return));
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_SYMBOL: {
            Node_id symbol_def;
            if (!sym_tbl_lookup(&symbol_def, nodes_at(sym_to_return)->name)) {
                todo();
            }

            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, sym_to_rtn_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, get_prev_load_id(sym_to_return));
            string_extend_cstr(output, "\n");
            break;
        }
        default:
            todo();
    }
}

static void emit_function_declaration(String* output, Node_id fun_decl) {
    string_extend_cstr(output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(output, " @");
    string_extend_strv(output, nodes_at(fun_decl)->name);
    string_append(output, '(');
    emit_function_params(output, nodes_get_child_of_type(fun_decl, NODE_FUNCTION_PARAMETERS));
    string_append(output, ')');
    string_extend_cstr(output, "\n");
}

static void emit_label(String* output, Node_id label) {
    string_extend_cstr(output, "\n");
    string_extend_size_t(output, nodes_at(label)->llvm_id);
    string_extend_cstr(output, ":\n");
}

static void emit_cmp_less_than(String* output, size_t llvm_cmp_dest, Node_id lhs, Node_id rhs) {
    Node_id var_to_cmp_def;
    if (!sym_tbl_lookup(&var_to_cmp_def, nodes_at(lhs)->name)) {
        todo();
    }

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, llvm_cmp_dest);
    string_extend_cstr(output, " = icmp slt i32 %");
    log_tree(LOG_DEBUG, var_to_cmp_def);
    string_extend_size_t(output, get_prev_load_id(lhs));
    string_extend_cstr(output, ", ");
    string_extend_strv(output, nodes_at(rhs)->str_data);
    string_extend_cstr(output, "\n");
}

static void emit_goto(String* output, Node_id lang_goto) {
    string_extend_cstr(output, "    br label %");
    string_extend_size_t(output, get_matching_label_id(lang_goto));
    string_append(output, '\n');
}

static void emit_cond_goto(String* output, Node_id cond_goto) {
    Node_id label_if_true = nodes_get_child(cond_goto, 1);
    Node_id label_if_false = nodes_get_child(cond_goto, 2);
    Node_id if_true_def;
    if (!sym_tbl_lookup(&if_true_def, nodes_at(label_if_true)->name)) {
        todo();
    }
    Node_id if_false_def;
    if (!sym_tbl_lookup(&if_false_def, nodes_at(label_if_false)->name)) {
        todo();
    }

    Node_id operator = nodes_get_child_of_type(cond_goto, NODE_OPERATOR);
    switch (nodes_at(operator)->token_type) {
        case TOKEN_LESS_THAN:
            break;
        default:
            unreachable("");
    }
    assert(nodes_count_children(operator) == 2);
    Node_id lhs = nodes_get_child(operator, 0);
    Node_id rhs = nodes_get_child(operator, 1);
    size_t llvm_cmp_dest = nodes_at(operator)->llvm_id;
    emit_cmp_less_than(output, llvm_cmp_dest, lhs, rhs);

    string_extend_cstr(output, "    br i1 %");
    string_extend_size_t(output, llvm_cmp_dest);
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_true));
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_false));
    string_append(output, '\n');
}

static void emit_load_variable(String* output, Node_id variable_call) {
    Node_id variable_def;
    node_printf(variable_call);
    if (!sym_tbl_lookup(&variable_def, nodes_at(variable_call)->name)) {
        unreachable("attempt to load a symbol that is undefined:"NODE_FMT"\n", node_print(variable_call));
    }
    log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
    log_tree(LOG_DEBUG, variable_call);
    assert(get_store_dest_id(variable_call) > 0);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(variable_call)->llvm_id);
    string_extend_cstr(output, " = load ");
    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, ", ");
    string_extend_cstr(output, "ptr");
    string_extend_cstr(output, " %");
    string_extend_size_t(output, get_store_dest_id(variable_call));
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_block(String* output, Node_id fun_block) {
    nodes_foreach_child(statement, fun_block) {
        switch (nodes_at(statement)->type) {
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
                //emit_assignment(output, statement);
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
            default:
                log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
                node_printf(statement);
                todo();
        }
    }
    //get_block_return_id(fun_block) = get_block_return_id(nodes_at(fun_block)->left_child);
}

static void emit_llvm_main(String* output, Node_id root) {
    switch (nodes_at(root)->type) {
        case NODE_BLOCK:
            emit_block(output, root);
            return;
        case NODE_FUNCTION_DEFINITION:
            emit_function_definition(output, root);
            return;
        case NODE_FUNCTION_PARAMETERS:
            todo();
        case NODE_FUNCTION_RETURN_TYPES:
            todo();
        case NODE_FUNCTION_CALL:
            todo();
        case NODE_LITERAL:
            todo();
        case NODE_LANG_TYPE:
            todo();
        case NODE_OPERATOR:
            todo();
        case NODE_SYMBOL:
            todo();
        default:
            unreachable("");
    }
    
    unreachable("");
}

static void emit_symbols(String* output) {
    for (size_t idx = 0; idx < symbol_table.capacity; idx++) {
        const Symbol_table_node curr_node = symbol_table.table_nodes[idx];
        if (curr_node.status != SYM_TBL_OCCUPIED) {
            continue;
        }
        switch (nodes_at(curr_node.node)->type) {
            case NODE_LITERAL:
                // fallthrough
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
                break;
            default:
                log(LOG_FETAL, NODE_FMT"\n", node_print(curr_node.node));
                todo();
        }

        size_t literal_width = nodes_at(curr_node.node)->str_data.count + 1;

        string_extend_cstr(output, "@.");
        string_extend_strv(output, curr_node.key);
        string_extend_cstr(output, " = private unnamed_addr constant [ ");
        string_extend_size_t(output, literal_width);
        string_extend_cstr(output, " x i8] c\"");
        string_extend_strv(output, nodes_at(curr_node.node)->str_data);
        string_extend_cstr(output, "\\00\", align 1");
        string_extend_cstr(output, "\n");
    }
}

void emit_llvm_from_tree(Node_id root) {
    String output = {0};

    emit_llvm_main(&output, root);

    emit_symbols(&output);

    log(LOG_NOTE, "\n"STRING_FMT"\n", string_print(output));

    Str_view final_output = {.str = output.buf, .count = output.info.count};
    write_file("test.ll", final_output);
}

