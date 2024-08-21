
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"

static Str_view block_to_strv(Node_id fun_block);
static uint32_t count_local = 1;

static Str_view function_params_to_strv(Node_id fun_params) {
    String buf = {0};

    nodes_foreach_child(param, fun_params) {
        todo(); // adding function parameter not implemented
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static Str_view function_arguments_to_strv(Node_id fun_args) {
    String buf = {0};
    todo();

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static Str_view function_call_to_strv(Node_id statement) {
    String buf = {0};

    char num_str[20];
    sprintf(num_str, "%d", count_local++);

    string_extend_cstr(&buf, "    %");
    string_extend_cstr(&buf, num_str);
    string_extend_cstr(&buf, " = call i32 @");
    string_extend_strv(&buf, nodes_at(statement)->name);

    // arguments
    string_extend_cstr(&buf, "(");
    nodes_foreach_child(argument, statement) {
        switch (nodes_at(argument)->type) {
            case NODE_LITERAL: {
                Str_view literal_name = nodes_at(argument)->name;
                string_extend_cstr(&buf, "ptr noundef @.");
                string_extend_strv(&buf, literal_name);
                break;
            }
            default:
                todo();
        }
    }
    string_extend_cstr(&buf, ")");

    string_extend_cstr(&buf, "\n");

    log(LOG_DEBUG, STRING_FMT"\n", string_print(buf));

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static Str_view function_definition_to_strv(Node_id fun_def) {
    String buf = {0};

    string_extend_cstr(&buf, "define dso_local i32 @");
    string_extend_strv(&buf, nodes_at(fun_def)->name);
    Node_id params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    string_extend_strv_in_par(&buf, function_params_to_strv(params));
    string_extend_cstr(&buf, " {\n");

    log(LOG_DEBUG, STRING_FMT"\n", string_print(buf));

    Node_id block = nodes_get_child_of_type(fun_def, NODE_BLOCK);
    string_extend_strv(&buf, block_to_strv(block));

    string_extend_cstr(&buf, "}\n");

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static Str_view block_to_strv(Node_id fun_block) {
    String buf = {0};

    nodes_foreach_child(statement, fun_block) {
        switch (nodes_at(statement)->type) {
            case NODE_FUNCTION_DEFINITION:
                log(LOG_DEBUG, "function_defintion thing\n");
                string_extend_strv(&buf, function_definition_to_strv(statement));
                break;
            case NODE_FUNCTION_CALL:
                log(LOG_DEBUG, "function_call thing\n");
                string_extend_strv(&buf, function_call_to_strv(statement));
                break;
            default:
                todo();
        }
        log(LOG_DEBUG, STRING_FMT"\n", string_print(buf));
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}


/*
static Str_view block_to_strv(Node_id block) {
    String buf = {0};

    switch (nodes_at(nodes_at(block)->left_child)->type) {
        case NODE_FUNCTION_DEFINITION:
            string_extend_strv(&buf, function_definition_to_strv(nodes_at(block)->left_child));
            break;
        default:
            todo();
    }

    Str_view str_view = {.str = buf.buf, .count = buf.count};
    return str_view;
}
*/

static Str_view llvm_from_tree_to_strv(Node_id root) {

    switch (nodes_at(root)->type) {
        case NODE_BLOCK:
            return block_to_strv(root);
        case NODE_FUNCTION_DEFINITION:
            return function_definition_to_strv(root);
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

static Str_view symbols_to_strv(Node_id root) {
    static String buf = {0};

    for (size_t idx = 0; idx < symbol_table.capacity; idx++) {
        Symbol_table_node curr_node = symbol_table.table_nodes[idx];
        if (curr_node.status != SYM_TBL_OCCUPIED) {
            continue;
        }

        char width_literal[20];
        sprintf(width_literal, "%zu", nodes_at(curr_node.node)->str_data.count + 1);

        string_extend_cstr(&buf, "@.");
        string_extend_strv(&buf, curr_node.key);
        string_extend_cstr(&buf, " = private unnamed_addr_constant [");
        string_extend_cstr(&buf, width_literal);
        string_extend_cstr(&buf, "xi8] c\"");
        string_extend_strv(&buf, nodes_at(curr_node.node)->str_data);
        string_extend_cstr(&buf, "\\00\", align 1");
        string_extend_cstr(&buf, "\n");
    }

    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

void emit_llvm_from_tree(Node_id root) {
    Str_view str_view = llvm_from_tree_to_strv(root);
    log(LOG_NOTE, "\n"STR_VIEW_FMT"\n", str_view_print(str_view));

    Str_view symbols = symbols_to_strv(root);
    log(LOG_NOTE, "\n"STR_VIEW_FMT"\n", str_view_print(symbols));

    Str_view extern_functions = str_view_from_cstr("declare i32 @puts(ptr noundef)\n");
    log(LOG_NOTE, "\n"STR_VIEW_FMT"\n", str_view_print(extern_functions));

    String file_text = string_new_from_strv(str_view);
    assert(!file_text.buf[file_text.info.count] && "string is not null terminated");
    //write_file("main.ll", file_text.buf);
}

