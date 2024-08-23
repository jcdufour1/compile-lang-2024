
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

static void emit_function_params(String* output, Node_id fun_params) {

    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        nodes_at(param)->llvm_id_variable.def = llvm_id_for_next_var;
        string_extend_cstr(output, "ptr noundef %");
        string_extend_size_t(output, nodes_at(param)->llvm_id_variable.def);

        llvm_id_for_next_var++;
    }
}

static void emit_function_call_arguments(String* output, Node_id statement) {
    size_t idx = 0;
    nodes_foreach_child(argument, statement) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }
        switch (nodes_at(argument)->type) {
            case NODE_LITERAL: {
                Str_view literal_name = nodes_at(argument)->name;
                string_extend_cstr(output, "ptr noundef @.");
                string_extend_strv(output, literal_name);
                break;
            }
            case NODE_SYMBOL: {
                Node_id variable_declaration;
                if (!sym_tbl_lookup(&variable_declaration, nodes_at(argument)->name)) {
                    msg(
                        LOG_WARNING,
                        params.input_file_name,
                        nodes_at(argument)->line_num,
                        "unknown variable: "STR_VIEW_FMT"\n",
                        str_view_print(nodes_at(argument)->name)
                    );
                    break;
                }
                Node_id variable_def;
                if (!sym_tbl_lookup(&variable_def, nodes_at(argument)->name)) {
                    todo();
                }
                size_t llvm_id = nodes_at(variable_def)->llvm_id_variable.new_load_dest;
                assert(llvm_id > 0);
                string_extend_cstr(output, "ptr noundef %");
                string_extend_size_t(output, llvm_id);
                llvm_id_for_next_var++;
                break;
            }
            default:
                todo();
        }
    }
}

static void extend_type_str(String* output, Node_id variable_def) {
    if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "String")) {
        string_extend_cstr(output, "ptr");
    } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
        string_extend_cstr(output, "i32");
    } else {
        todo();
    }
}

static void emit_load_variable(String* output, Node_id variable_call) {
    if (nodes_at(variable_call)->llvm_id_variable.def > 0) {
        todo();
    }

    size_t load_dest_id = llvm_id_for_next_var;
    assert(load_dest_id > 0);

    Node_id variable_def;
    if (!sym_tbl_lookup(&variable_def, nodes_at(variable_call)->name)) {
        todo();
    }
    nodes_at(variable_def)->llvm_id_variable.new_load_dest = load_dest_id;

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, load_dest_id);
    string_extend_cstr(output, " = load ");
    extend_type_str(output, variable_def);
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

    nodes_at(fun_call)->llvm_id_variable.def = llvm_id_for_next_var;

    // start of actual function call
    string_extend_cstr(output, "    %");
    string_extend_size_t(output, nodes_at(fun_call)->llvm_id_variable.def);
    string_extend_cstr(output, " = call i32 @");
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
    extend_type_str(output, symbol_def);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");

    llvm_id_for_next_var++;
}

static void emit_store(String* output, Node_id variable_def){
    size_t src_llvm_id = nodes_at(variable_def)->llvm_id_variable.def;
    size_t alloca_dest_id = nodes_at(variable_def)->llvm_id_variable.store_dest;
    assert(src_llvm_id > 0);
    assert(alloca_dest_id > 0);

    switch (nodes_at(variable_def)->type) {
        case NODE_VARIABLE_DEFINITION:
            if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "String")) {
                string_extend_cstr(output, "    store ptr @.");
                string_extend_strv(output, nodes_at(variable_def)->name);
                string_extend_cstr(output, ", ptr %");
                string_extend_size_t(output, alloca_dest_id);
                string_extend_cstr(output, ", align 8");
                string_extend_cstr(output, "\n");
            } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
                string_extend_cstr(output, "    store i32 ");
                string_extend_strv(output, nodes_at(variable_def)->str_data);
                string_extend_cstr(output, ", ptr %");
                string_extend_size_t(output, alloca_dest_id);
                string_extend_cstr(output, ", align 8");
                string_extend_cstr(output, "\n");
            } else {
                todo();
            }
            break;
        case NODE_LANG_TYPE:
            if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "String")) {
                string_extend_cstr(output, "    store ptr %");
                string_extend_size_t(output, src_llvm_id);
                string_extend_cstr(output, ", ptr %");
                string_extend_size_t(output, alloca_dest_id);
                string_extend_cstr(output, ", align 8");
                string_extend_cstr(output, "\n");
            } else if (0 == str_view_cmp_cstr(nodes_at(variable_def)->lang_type, "i32")) {
                string_extend_cstr(output, "    store i32 %");
                string_extend_size_t(output, src_llvm_id);
                string_extend_cstr(output, ", ptr %");
                string_extend_size_t(output, alloca_dest_id);
                string_extend_cstr(output, ", align 8");
                string_extend_cstr(output, "\n");
            } else {
                todo();
            }
            break;
        default:
            todo();
    }
}

static void emit_store_fun_params(String* output, Node_id parameters) {
    nodes_foreach_child(param, parameters) {
        assert(nodes_at(param)->llvm_id_variable.def > 0);

        emit_alloca(output, param);

        emit_store(output, param);

        llvm_id_for_next_var++;
    }
}

static void emit_function_definition(String* output, Node_id fun_def) {
    string_extend_cstr(output, "define dso_local i32 @");
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
    switch (nodes_at(child)->type) {
        case NODE_LITERAL:
            if (nodes_at(child)->token_type != TOKEN_NUM_LITERAL) {
                todo();
            }
            string_extend_cstr(output, "    ret i32 ");
            string_extend_strv(output, nodes_at(child)->str_data);
            string_extend_cstr(output, "\n");
            break;
        case NODE_FUNCTION_CALL:
            todo();
        case NODE_BLOCK: {
            emit_block(output, child);
            string_extend_cstr(output, "    ret i32 %");
            string_extend_size_t(output, nodes_at(child)->llvm_id_block_return);
            string_extend_cstr(output, "\n");
            break;
        }
        case NODE_OPERATOR: {
            todo();
            emit_operator(output, child);
            string_extend_cstr(output, "    ret i32 %");
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

            string_extend_cstr(output, "    ret i32 %");
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
    nodes_at(variable_def)->llvm_id_variable.def = llvm_id_for_next_var;

    emit_alloca(output, variable_def);

    emit_store(output, variable_def);
}

static void emit_function_declaration(String* output, Node_id fun_decl) {
    string_extend_cstr(output, "declare i32 @");
    string_extend_strv(output, nodes_at(fun_decl)->name);
    string_extend_cstr(output, " (ptr noundef) ");
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
                break;
            case NODE_LANG_TYPE:
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

