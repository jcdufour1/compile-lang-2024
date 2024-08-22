
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"
#include "parameters.h"
#include "file.h"

static void block_to_strv(String* output, Node_id fun_block);

static size_t llvm_id_for_next_var = 1;

static void function_params_to_strv(String* output, Node_id fun_params) {

    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        char num_str[20];
        nodes_at(param)->llvm_id = llvm_id_for_next_var;
        sprintf(num_str, "%zu", llvm_id_for_next_var);
        string_extend_cstr(output, "ptr noundef %");
        string_extend_cstr(output, num_str);

        llvm_id_for_next_var++;
    }
}

static void function_arguments_to_strv(String* output, Node_id fun_args) {
    (void) output;
    (void) fun_args;
    todo();
}

static void function_call_arguments(String* output, Node_id statement) {
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
                    msg(LOG_ERROR, params.input_file_name, nodes_at(argument)->line_num, "unknown variable: "STR_VIEW_FMT"\n", str_view_print(nodes_at(argument)->name));
                    abort();
                }
                size_t llvm_id = nodes_at(variable_declaration)->llvm_id;
                assert(llvm_id > 0);
                char llvm_id_str[20];
                sprintf(llvm_id_str, "%19zu", llvm_id);
                string_extend_cstr(output, "ptr noundef %");
                string_extend_cstr(output, llvm_id_str);
                llvm_id_for_next_var++;
                break;
            }
            default:
                todo();
        }
    }
}

static void function_call_to_strv(String* output, Node_id statement) {
    char num_str[20];
    sprintf(num_str, "%zu", llvm_id_for_next_var++);

    string_extend_cstr(output, "    %");
    string_extend_cstr(output, num_str);
    string_extend_cstr(output, " = call i32 @");
    string_extend_strv(output, nodes_at(statement)->name);

    // arguments
    string_extend_cstr(output, "(");
    function_call_arguments(output, statement);
    string_extend_cstr(output, ")");

    string_extend_cstr(output, "\n");
}

static void function_definition_to_strv(String* output, Node_id fun_def) {
    string_extend_cstr(output, "define dso_local i32 @");
    string_extend_strv(output, nodes_at(fun_def)->name);

    Node_id params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    string_append(output, '(');
    function_params_to_strv(output, params);
    string_append(output, ')');
    llvm_id_for_next_var++;

    string_extend_cstr(output, " {\n");

    Node_id block = nodes_get_child_of_type(fun_def, NODE_BLOCK);
    block_to_strv(output, block);

    string_extend_cstr(output, "    ret i32 0\n");

    string_extend_cstr(output, "}\n");
}

static void block_to_strv(String* output, Node_id fun_block) {
    nodes_foreach_child(statement, fun_block) {
        switch (nodes_at(statement)->type) {
            case NODE_FUNCTION_DEFINITION:
                function_definition_to_strv(output, statement);
                break;
            case NODE_FUNCTION_CALL:
                function_call_to_strv(output, statement);
                break;
            default:
                todo();
        }
    }
}

static void llvm_from_tree_to_strv(String* output, Node_id root) {
    switch (nodes_at(root)->type) {
        case NODE_BLOCK:
            block_to_strv(output, root);
            return;
        case NODE_FUNCTION_DEFINITION:
            function_definition_to_strv(output, root);
            return;
        case NODE_FUNCTION_PARAMETERS:
            todo();
        case NODE_FUNCTION_RETURN_TYPES:
            todo();
        case NODE_FUNCTION_BODY:
            todo();
        case NODE_FUNCTION_CALL:
            todo();
        case NODE_FUNCTION_ARGUMENTS:
            todo();
        case NODE_LITERAL:
            todo();
        case NODE_LANG_TYPE:
            todo();
        case NODE_OPERATOR:
            todo();
        case NODE_SYMBOL:
            todo();
        case NODE_NO_TYPE:
            unreachable();
        default:
            unreachable();
    }
    
    unreachable();
}

static void symbols_to_strv(String* output) {
    for (size_t idx = 0; idx < symbol_table.capacity; idx++) {
        const Symbol_table_node curr_node = symbol_table.table_nodes[idx];
        if (curr_node.status != SYM_TBL_OCCUPIED) {
            continue;
        }
        if (nodes_at(curr_node.node)->type != NODE_LITERAL) {
            continue;
        }

        char width_literal[20];
        sprintf(width_literal, "%zu", nodes_at(curr_node.node)->str_data.count + 1);

        string_extend_cstr(output, "@.");
        string_extend_strv(output, curr_node.key);
        string_extend_cstr(output, " = private unnamed_addr constant [ ");
        string_extend_cstr(output, width_literal);
        string_extend_cstr(output, " x i8] c\"");
        string_extend_strv(output, nodes_at(curr_node.node)->str_data);
        string_extend_cstr(output, "\\00\", align 1");
        string_extend_cstr(output, "\n");
    }
}

void emit_llvm_from_tree(Node_id root) {
    String output = {0};
    llvm_from_tree_to_strv(&output, root);

    symbols_to_strv(&output);

    string_extend_cstr(&output, "declare i32 @puts(ptr noundef)\n");

    log(LOG_NOTE, "\n"STRING_FMT"\n", string_print(output));

    Str_view final_output = {.str = output.buf, .count = output.info.count};
    write_file("test.ll", final_output);
}

