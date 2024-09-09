
#include "node.h"
#include "nodes.h"
#include "emit_llvm.h"
#include "newstring.h"
#include "symbol_table.h"
#include "parameters.h"
#include "file.h"
#include "parser_utils.h"

static void emit_block(String* output, Node* fun_block);
static void emit_llvm_main(String* output, Node* root);

// \n excapes are actually stored as is in tokens and nodes, but should be printed as \0a
// number of excape sequences are returned
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

static void extend_type_call_str(String* output, Node* variable_def) {
    assert(variable_def->lang_type.count > 0);
    if (str_view_cstr_is_equal(variable_def->lang_type, "i32")) {
        string_extend_cstr(output, "i32");
    } else if (str_view_cstr_is_equal(variable_def->lang_type, "ptr")) {
        string_extend_cstr(output, "ptr");
    } else {
        todo();
    }
}

static void extend_type_decl_str(String* output, Node* variable_def) {
    if (variable_def->is_variadic) {
        string_extend_cstr(output, "...");
        return;
    }

    extend_type_call_str(output, variable_def);
    string_extend_cstr(output, " noundef");
}

static void extend_literal_decl_prefix(String* output, Node* var_decl_or_def) {
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

static void extend_literal_decl(String* output, Node* var_decl_or_def) {
    extend_type_decl_str(output, var_decl_or_def);
    extend_literal_decl_prefix(output, var_decl_or_def);
}

static Node* return_type_from_function_definition(Node* fun_def) {
    Node* return_types = nodes_get_child_of_type(fun_def, NODE_FUNCTION_RETURN_TYPES);
    assert(nodes_count_children(return_types) < 2);
    if (nodes_count_children(return_types) > 0) {
        return return_types->left_child;
    }
    unreachable("");
}

static void emit_function_params(String* output, Node* fun_params) {
    size_t idx = 0;
    nodes_foreach_child(param, fun_params) {
        if (idx++ > 0) {
            string_extend_cstr(output, ", ");
        }

        extend_type_decl_str(output, param);
        if (param->is_variadic) {
            return;
        }
        string_extend_cstr(output, " %");
        string_extend_size_t(output, param->llvm_id);
    }
}

static void emit_function_call_arguments(String* output, Node* fun_call) {
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

static void emit_function_call(String* output, Node* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // load arguments
    nodes_foreach_child(argument, fun_call) {
        if (argument->type == NODE_SYMBOL) {
            //emit_load_variable(output, argument);
        } else if (argument->type == NODE_LITERAL) {
        } else {
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

static void emit_alloca(String* output, Node* alloca) {
    assert(alloca->type == NODE_ALLOCA);

    string_extend_cstr(output, "    %");
    string_extend_size_t(output, alloca->llvm_id);
    string_extend_cstr(output, " = alloca ");
    node_printf(alloca);
    extend_type_call_str(output, get_symbol_def_from_alloca(alloca));
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_src_of_assignment(String* output, Node* variable_def, void* item) {
    Node* rhs = *(Node**)item;

    switch (rhs->type) {
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

static void emit_src_literal(String* output, Node* var_decl_or_def) {
    extend_literal_decl_prefix(output, var_decl_or_def);
}

static void emit_src_function_call_result(String* output, Node* fun_call) {
    assert(fun_call->type == NODE_FUNCTION_CALL);

    string_extend_cstr(output, " %");
    string_extend_size_t(output, fun_call->llvm_id);
}

static void emit_src_operator(String* output, Node* operator) {
    assert(operator->type == NODE_OPERATOR);

    string_extend_cstr(output, " %");
    string_extend_size_t(output, operator->llvm_id);
}

static void emit_src_symbol(String* output, Node* symbol) {
    assert(symbol->type == NODE_SYMBOL);

    string_extend_cstr(output, " %");
    string_extend_size_t(output, get_prev_load_id(symbol));
}

static void emit_operator_type(String* output, Node* operator) {
    assert(operator->type == NODE_OPERATOR);
    
    switch (operator->token_type) {
        case TOKEN_SINGLE_MINUS:
            todo();
        case TOKEN_SINGLE_PLUS:
            string_extend_cstr(output, "add nsw i32 ");
            break;
        case TOKEN_ASTERISK:
            string_extend_cstr(output, "mul nsw i32 ");
            break;
        default:
             unreachable(TOKEN_TYPE_FMT"\n", token_type_print(operator->token_type));
    }
}

static void emit_operator_operand(String* output, Node* operand) {
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

static void emit_operator(String* output, Node* operator) {
    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);

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

static void emit_src(String* output, Node* src) {
    //log(LOG_DEBUG, STRING_FMT"\n", string_print(*output));
    switch (src->type) {
        case NODE_LITERAL:
            emit_src_literal(output, src);
            break;
        case NODE_FUNCTION_CALL:
            emit_src_function_call_result(output, src);
            break;
        case NODE_OPERATOR:
            emit_src_operator(output, src);
            break;
        case NODE_SYMBOL:
            emit_src_symbol(output, src);
            break;
        default:
            todo();
    }
}

static void emit_load_variable(String* output, Node* variable_call) {
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

static void emit_store(String* output, Node* store) {
    size_t alloca_dest_id = get_store_dest_id(store);
    Node* var_def = get_symbol_def_from_alloca(store);
    assert(alloca_dest_id > 0);

    // emit prerequisite call, etc before actually storing
    Node* src = nodes_single_child(store);
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
        default:
            log_tree(LOG_DEBUG, store);
            unreachable(NODE_FMT"\n", node_print(src));
    }

    string_extend_cstr(output, "    store ");
    extend_type_call_str(output, var_def);

    emit_src(output, src);

    string_extend_cstr(output, ", ptr %");
    string_extend_size_t(output, alloca_dest_id);
    string_extend_cstr(output, ", align 8");
    string_extend_cstr(output, "\n");
}

static void emit_src_of_fun_params(String* output, Node* variable_def, void* item) {
    (void) item;
    size_t src_llvm_id = variable_def->llvm_id;

    switch (variable_def->type) {
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

static void emit_function_definition(String* output, Node* fun_def) {
    string_extend_cstr(output, "define dso_local ");

    extend_type_call_str(output, return_type_from_function_definition(fun_def));

    string_extend_cstr(output, " @");
    string_extend_strv(output, fun_def->name);

    Node* params = nodes_get_child_of_type(fun_def, NODE_FUNCTION_PARAMETERS);
    string_append(output, '(');
    emit_function_params(output, params);
    string_append(output, ')');

    string_extend_cstr(output, " {\n");

    Node* block = nodes_get_child_of_type(fun_def, NODE_BLOCK);
    emit_block(output, block);

    string_extend_cstr(output, "}\n");
}

static void emit_function_return_statement(String* output, Node* fun_return) {
    Node* sym_to_return = fun_return->left_child;
    Node* sym_to_rtn_def;
    if (!sym_tbl_lookup(&sym_to_rtn_def, sym_to_return->name)) {
        todo();
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
            Node* symbol_def;
            if (!sym_tbl_lookup(&symbol_def, sym_to_return->name)) {
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

static void emit_function_declaration(String* output, Node* fun_decl) {
    string_extend_cstr(output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(output, " @");
    string_extend_strv(output, fun_decl->name);
    string_append(output, '(');
    emit_function_params(output, nodes_get_child_of_type(fun_decl, NODE_FUNCTION_PARAMETERS));
    string_append(output, ')');
    string_extend_cstr(output, "\n");
}

static void emit_label(String* output, Node* label) {
    string_extend_cstr(output, "\n");
    string_extend_size_t(output, label->llvm_id);
    string_extend_cstr(output, ":\n");
}

static void emit_cmp_less_than(String* output, size_t llvm_cmp_dest, Node* lhs, Node* rhs) {
    string_extend_cstr(output, "    %");
    string_extend_size_t(output, llvm_cmp_dest);
    string_extend_cstr(output, " = icmp slt i32 ");
    emit_operator_operand(output, lhs);
    string_extend_cstr(output, ", ");
    emit_operator_operand(output, rhs);
    string_extend_cstr(output, "\n");
}

static void emit_goto(String* output, Node* lang_goto) {
    string_extend_cstr(output, "    br label %");
    string_extend_size_t(output, get_matching_label_id(lang_goto));
    string_append(output, '\n');
}

static void emit_cond_goto(String* output, Node* cond_goto) {
    Node* label_if_true = nodes_get_child(cond_goto, 1);
    Node* label_if_false = nodes_get_child(cond_goto, 2);
    Node* if_true_def;
    if (!sym_tbl_lookup(&if_true_def, label_if_true->name)) {
        todo();
    }
    Node* if_false_def;
    if (!sym_tbl_lookup(&if_false_def, label_if_false->name)) {
        todo();
    }

    Node* operator = nodes_get_child_of_type(cond_goto, NODE_OPERATOR);
    switch (operator->token_type) {
        case TOKEN_LESS_THAN:
            break;
        default:
            unreachable("");
    }
    assert(nodes_count_children(operator) == 2);
    Node* lhs = nodes_get_child(operator, 0);
    Node* rhs = nodes_get_child(operator, 1);
    size_t llvm_cmp_dest = operator->llvm_id;
    emit_cmp_less_than(output, llvm_cmp_dest, lhs, rhs);

    string_extend_cstr(output, "    br i1 %");
    string_extend_size_t(output, llvm_cmp_dest);
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_true));
    string_extend_cstr(output, ", label %");
    string_extend_size_t(output, get_matching_label_id(label_if_false));
    string_append(output, '\n');
}

static void emit_block(String* output, Node* fun_block) {
    nodes_foreach_child(statement, fun_block) {
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

static void emit_llvm_main(String* output, Node* root) {
    switch (root->type) {
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
        switch (curr_node.node->type) {
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

        size_t literal_width = curr_node.node->str_data.count + 1 - \
                               get_count_excape_seq(curr_node.node->str_data);

        string_extend_cstr(output, "@.");
        string_extend_strv(output, curr_node.key);
        string_extend_cstr(output, " = private unnamed_addr constant [ ");
        string_extend_size_t(output, literal_width);
        string_extend_cstr(output, " x i8] c\"");
        string_extend_strv_eval_escapes(output, curr_node.node->str_data);
        string_extend_cstr(output, "\\00\", align 1");
        string_extend_cstr(output, "\n");
    }
}

void emit_llvm_from_tree(Node* root) {
    String output = {0};

    emit_llvm_main(&output, root);

    emit_symbols(&output);

    log(LOG_NOTE, "\n"STRING_FMT"\n", string_print(output));

    Str_view final_output = {.str = output.buf, .count = output.info.count};
    write_file("test.ll", final_output);
}

