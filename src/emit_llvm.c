
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"
#include "parameters.h"
#include "file.h"

static void emit_block(String* output, Node_id fun_block);
static void emit_llvm_main(String* output, Node_id root);

static size_t llvm_id_for_next_var = 1;

static void extend_type_call_str(String* output, Node_id variable_def) {
    if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "String")) {
        string_extend_cstr(output, "ptr");
    } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
        string_extend_cstr(output, "i32");
    } else {
        todo();
    }
}

static void extend_type_decl_str(String* output, Node_id variable_def) {
    if (nodes_at(variable_def)->is_variadic) {
        string_extend_cstr(output, "...");
        return;
    }

    Str_view lang_type = nodes_at(variable_def)->lang_type;
    if (0 == str_view_cmp_cstr(lang_type, "String")) {
        string_extend_cstr(output, "ptr noundef");
    } else if (0 == str_view_cmp_cstr(lang_type, "i32")) {
        string_extend_cstr(output, "i32 noundef");
    } else {
        todo();
    }
}

static void extend_literal_decl(String* output, Node_id var_decl_or_def) {
    extend_type_decl_str(output, var_decl_or_def);

    Str_view lang_type = nodes_at(var_decl_or_def)->lang_type;
    if (0 == str_view_cmp_cstr(lang_type, "String")) {
        string_extend_cstr(output, " @.");
        string_extend_strv(output, nodes_at(var_decl_or_def)->name);
    } else if (0 == str_view_cmp_cstr(lang_type, "i32")) {
        string_append(output, ' ');
        string_extend_strv(output, nodes_at(var_decl_or_def)->str_data);
    } else {
        todo();
    }
}

static Node_id return_type_from_function_definition(Node_id fun_def) {
    Node_id return_types = nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES);
    assert(nodes_count_children(return_types) < 2);
    if (nodes_count_children(return_types) > 0) {
        return nodes_at(return_types)->left_child;
    }
    unreachable();
}

static void emit_function_params(String* output, Node_id fun_params) {
    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        nodes_at(param)->llvm_id_variable.def = llvm_id_for_next_var;
        extend_type_decl_str(output, param);
        if (nodes_at(param)->is_variadic) {
            return;
        }
        string_extend_cstr(output, " %");
        string_extend_size_t(output, nodes_at(param)->llvm_id_variable.def);

        llvm_id_for_next_var++;
    }
}

static void emit_function_call_arguments(String* output, Node_id statement) {
    size_t idx = 0;
    nodes_foreach_child(argument, statement) {
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
                size_t llvm_id = nodes_at(var_decl_or_def)->llvm_id_variable.new_load_dest;
                assert(llvm_id > 0);
                extend_type_call_str(output, var_decl_or_def);
                string_extend_cstr(output, " %");
                string_extend_size_t(output, llvm_id);
                llvm_id_for_next_var++;
                break;
            }
            default:
                todo();
        }
    }
}

static void emit_load_variable(String* output, Node_id variable_call) {
    if (nodes_at(variable_call)->llvm_id_variable.def > 0) {
        todo();
    }

    size_t load_dest_id = llvm_id_for_next_var;

    Node_id variable_def;
    if (!sym_tbl_lookup(&variable_def, nodes_at(variable_call)->name)) {
        todo();
    }
    nodes_at(variable_def)->llvm_id_variable.new_load_dest = load_dest_id;
    assert(nodes_at(variable_def)->llvm_id_variable.store_dest > 0);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, load_dest_id);
    string_extend_cstr(output, " = load ");
    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, ", ");
    string_extend_cstr(output, "ptr");
    string_extend_cstr(output, " %");
    string_extend_size_t(output, nodes_at(variable_def)->llvm_id_variable.store_dest);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");

    llvm_id_for_next_var++;
}

static void emit_function_call(String* output, Node_id fun_call) {
    assert(nodes_at(fun_call)->llvm_id_variable.def == 0);

    // load arguments
    nodes_foreach_child(argument, fun_call) {
        if (nodes_at(argument)->type == NODE_SYMBOL) {
            emit_load_variable(output, argument);
        } else if (nodes_at(argument)->type == NODE_LITERAL) {
        } else {
            todo();
        }
    }

    Node_id fun_def;
    if (!sym_tbl_lookup(&fun_def, nodes_at(fun_call)->name)) {
        unreachable();
    }

    nodes_at(fun_call)->llvm_id_variable.def = llvm_id_for_next_var;

    // start of actual function call
    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(fun_call)->llvm_id_variable.def);
    string_extend_cstr(output, " = call ");
    extend_type_call_str(output, return_type_from_function_definition(fun_def));
    string_extend_cstr(output, " @");
    string_extend_strv(output, nodes_at(fun_call)->name);

    // arguments
    string_extend_cstr(output, "(");
    emit_function_call_arguments(output, fun_call);
    string_extend_cstr(output, ")");

    string_extend_cstr(output, "\n");

    llvm_id_for_next_var++;
}

static void emit_alloca(String* output, Node_id symbol_def) {
    assert(nodes_at(symbol_def)->llvm_id_variable.def > 0);
    nodes_at(symbol_def)->llvm_id_variable.store_dest = llvm_id_for_next_var;

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(symbol_def)->llvm_id_variable.store_dest);
    string_extend_cstr(output, " = alloca ");
    extend_type_call_str(output, symbol_def);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");

    llvm_id_for_next_var++;
}

static void emit_store(String* output, Node_id variable_def, void (emit_src)(String* output, Node_id variable_def, void* item), void* item){
    size_t alloca_dest_id = nodes_at(variable_def)->llvm_id_variable.store_dest;
    assert(alloca_dest_id > 0);

    string_extend_cstr(output, "    store ");
    extend_type_call_str(output, variable_def);
    string_append(output, ' ');

    emit_src(output, variable_def, item);

    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_src_of_fun_params(String* output, Node_id variable_def, void* item) {
    (void) item;
    size_t src_llvm_id = nodes_at(variable_def)->llvm_id_variable.def;

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

static void emit_store_fun_params(String* output, Node_id parameters) {
    assert(nodes_at(parameters)->type == NODE_FUNCTION_PARAMETERS);
    nodes_foreach_child(param, parameters) {
        assert(nodes_at(param)->llvm_id_variable.def > 0);
        emit_alloca(output, param);
        emit_store(output, param, emit_src_of_fun_params, NULL);
        llvm_id_for_next_var++;
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
    llvm_id_for_next_var++;

    string_extend_cstr(output, " {\n");

    emit_store_fun_params(output, params);

    Node_id block = nodes_get_child_of_type(fun_def, NODE_BLOCK);
    emit_block(output, block);

    string_extend_cstr(output, "}\n");
}

static void emit_operator(String* output, Node_id operator) {
    nodes_at(operator)->llvm_id_variable.def = llvm_id_for_next_var;

    Node_id child_lhs = nodes_at(operator)->left_child;
    Node_id child_rhs = nodes_at(child_lhs)->next;

    if (nodes_at(child_lhs)->type != NODE_LITERAL || nodes_at(child_rhs)->type != NODE_LITERAL) {
        todo();
    }

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, llvm_id_for_next_var);

    string_extend_cstr(output, " = mul nsw i32 ");
    string_extend_strv(output, nodes_at(child_lhs)->str_data);

    string_extend_cstr(output, ", ");

    string_extend_strv(output, nodes_at(child_rhs)->str_data);

    string_extend_cstr(output, "\n");

    llvm_id_for_next_var++;
}

static void emit_function_return_statement(String* output, Node_id statement) {
    Node_id child = nodes_at(statement)->left_child;
    Node_id child_def;
    if (!sym_tbl_lookup(&child_def, nodes_at(child)->name)) {
        todo();
    }

    switch (nodes_at(child)->type) {
        case NODE_LITERAL:
            if (nodes_at(child)->token_type != TOKEN_NUM_LITERAL) {
                todo();
            }
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, child_def);
            string_extend_cstr(output, " ");
            string_extend_strv(output, nodes_at(child)->str_data);
            string_extend_cstr(output, "\n");
            break;
        case NODE_FUNCTION_CALL: {
            emit_function_call(output, child);
            string_extend_cstr(output, "    ret ");

            Node_id return_types = nodes_get_child_of_type(child_def, NODE_FUNCTION_RETURN_TYPES);
            assert(nodes_count_children(return_types) == 1 && "not implemented");
            Node_id return_type = nodes_at(return_types)->left_child;

            extend_type_call_str(output, return_type);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, nodes_at(child)->llvm_id_block_return);
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_BLOCK: {
            emit_block(output, child);
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, child_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, nodes_at(child)->llvm_id_block_return);
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_OPERATOR: {
            todo();
            emit_operator(output, child);
            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, child_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, nodes_at(child)->llvm_id_block_return);
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_SYMBOL: {
            emit_load_variable(output, child);

            Node_id symbol_def;
            if (!sym_tbl_lookup(&symbol_def, nodes_at(child)->name)) {
                todo();
            }

            string_extend_cstr(output, "    ret ");
            extend_type_call_str(output, child_def);
            string_extend_cstr(output, " %");
            string_extend_size_t(output, nodes_at(symbol_def)->llvm_id_variable.new_load_dest);
            string_extend_cstr(output, "\n");
            break;
        }
        default:
            todo();
    }
    llvm_id_for_next_var++;
}

static void emit_variable_definition(String* output, Node_id variable_def) {
    assert(nodes_at(variable_def)->llvm_id_variable.def == 0 && "redefinition of variable");
    nodes_at(variable_def)->llvm_id_variable.def = llvm_id_for_next_var;
    emit_alloca(output, variable_def);
}

static void emit_src_of_assignment(String* output, Node_id variable_def, void* item) {
    size_t src_llvm_id = nodes_at(variable_def)->llvm_id_variable.store_dest;
    (void) src_llvm_id;

    Node_id rhs = *(Node_id*)item;
    Str_view symbol_name = nodes_at(rhs)->name;
    Str_view num_str = nodes_at(rhs)->str_data;

    switch (nodes_at(rhs)->type) {
        case NODE_SYMBOL:
            // fallthrough
        case NODE_LITERAL:
            if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "String")) {
                string_extend_cstr(output, " @.");
                string_extend_strv(output, symbol_name);
            } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
                string_extend_strv(output, num_str);
            } else {
                todo();
            }
            break;
        case NODE_FUNCTION_CALL:
            string_extend_cstr(output, " %");
            string_extend_size_t(output, nodes_at(rhs)->llvm_id_block_return);
            break;
        default:
            todo();
    }
}

static void emit_assignment(String* output, Node_id assignment) {
    Node_id lhs = nodes_at(assignment)->left_child;
    Node_id rhs = nodes_at(lhs)->next;

    Node_id lhs_def;
    if (!sym_tbl_lookup(&lhs_def, nodes_at(lhs)->name)) {
        todo();
    }

    switch (nodes_at(rhs)->type) {
        case NODE_LITERAL:
            if (nodes_at(lhs_def)->llvm_id_variable.def == 0) {
                emit_variable_definition(output, lhs_def);
            }
            assert(nodes_at(rhs)->str_data.count > 0);
            emit_store(output, lhs_def, emit_src_of_assignment, &rhs);
            break;
        case NODE_BLOCK:
            todo();
        case NODE_FUNCTION_CALL:
            if (nodes_at(lhs_def)->llvm_id_variable.def == 0) {
                emit_variable_definition(output, lhs_def);
            }
            emit_function_call(output, rhs);
            size_t fun_call_result_id = nodes_at(rhs)->llvm_id_block_return;
            assert(fun_call_result_id > 0);
            emit_store(output, lhs_def, emit_src_of_assignment, &rhs);
            break;
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
                emit_variable_definition(output, statement);
                break;
            case NODE_FUNCTION_DECLARATION:
                emit_function_declaration(output, statement);
                break;
            case NODE_ASSIGNMENT:
                emit_assignment(output, statement);
                break;
            default:
                todo();
        }
    }
    nodes_at(fun_block)->llvm_id_block_return = nodes_at(nodes_at(fun_block)->left_child)->llvm_id_block_return;
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
        case NODE_FUNCTION_BODY:
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
            unreachable();
    }
    
    unreachable();
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

