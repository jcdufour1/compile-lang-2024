
#include "../node.h"
#include "../nodes.h"
#include "../newstring.h"
#include "passes.h"
#include "../symbol_table.h"
#include "../parameters.h"
#include "../file.h"
#include "../parser_utils.h"
#include "../node_utils.h"

static void emit_block(Env* env, String* output, const Node_block* fun_block);

static void extend_literal(String* output, const Node_literal* literal) {
    switch (literal->type) {
        case NODE_LIT_STRING:
            string_extend_strv(&a_main, output, node_unwrap_lit_string_const(literal)->data);
            return;
        case NODE_LIT_NUMBER:
            string_extend_int64_t(&a_main, output, node_unwrap_lit_number_const(literal)->data);
            return;
        case NODE_LIT_VOID:
            return;
    }
    unreachable("");
}

// \n excapes are actually stored as is in tokens and nodes, but should be printed as \0a
static void string_extend_strv_eval_escapes(Arena* arena, String* string, Str_view str_view) {
    while (str_view.count > 0) {
        char front_char = str_view_consume(&str_view);
        if (front_char == '\\') {
            vec_append(arena, string, '\\');
            switch (str_view_consume(&str_view)) {
                case 'n':
                    string_extend_hex_2_digits(arena, string, 0x0a);
                    break;
                default:
                    unreachable("");
            }
        } else {
            vec_append(arena, string, front_char);
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

static void extend_type_call_str(const Env* env, String* output, Lang_type lang_type) {
    assert(lang_type.str.count > 0);
    if (lang_type.pointer_depth != 0) {
        string_extend_cstr(&a_main, output, "ptr");
        return;
    }

    Node* struct_def;
    if (symbol_lookup(&struct_def, env, lang_type.str)) {
        string_extend_cstr(&a_main, output, "%struct.");
    }
    extend_lang_type_to_string(&a_main, output, lang_type, false);
}

static bool is_variadic(const Node* node) {
    switch (node->type) {
        case NODE_VARIABLE_DEF:
            return node_unwrap_variable_def_const(node)->is_variadic;
        case NODE_LITERAL:
            return false;
        default:
            unreachable(NODE_FMT, node_print(node));
    }
}

static void extend_type_decl_str(const Env* env, String* output, const Node* var_def_or_lit, bool noundef) {
    if (is_variadic(var_def_or_lit)) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    extend_type_call_str(env, output, get_lang_type(var_def_or_lit));
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void extend_literal_decl_prefix(String* output, const Node_literal* literal) {
    assert(literal->lang_type.str.count > 0);
    if (str_view_cstr_is_equal(literal->lang_type.str, "u8")) {
        if (literal->lang_type.pointer_depth != 1) {
            todo();
        }
        string_extend_cstr(&a_main, output, " @.");
        string_extend_strv(&a_main, output, literal->name);
    } else if (is_i_lang_type(literal->lang_type)) {
        if (literal->lang_type.pointer_depth != 0) {
            todo();
        }
        vec_append(&a_main, output, ' ');
        extend_literal(output, literal);
    } else {
        log(LOG_ERROR, NODE_FMT"\n", node_print(literal));
        todo();
    }
}

static void extend_literal_decl(const Env* env, String* output, const Node_literal* literal, bool noundef) {
    extend_type_decl_str(env, output, node_wrap_literal_const(literal), noundef);
    extend_literal_decl_prefix(output, literal);
}

static const Node_lang_type* return_type_from_function_definition(const Node_function_definition* fun_def) {
    const Node_function_return_types* return_types = fun_def->declaration->return_types;
    if (return_types->child) {
        return return_types->child;
    }
    unreachable("");
}

static void emit_function_params(const Env* env, String* output, const Node_function_params* fun_params) {
    for (size_t idx = 0; idx < fun_params->params.info.count; idx++) {
        const Node_variable_def* curr_param = node_unwrap_variable_def_const(
            vec_at(&fun_params->params, idx)
        );

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        if (is_struct_variable_definition(env, curr_param)) {
            if (curr_param->lang_type.pointer_depth < 0) {
                unreachable("");
            } else if (curr_param->lang_type.pointer_depth > 0) {
                extend_type_call_str(env, output, curr_param->lang_type);
            } else {
                string_extend_cstr(&a_main, output, "ptr noundef byval(");
                extend_type_call_str(env, output, curr_param->lang_type);
                string_extend_cstr(&a_main, output, ")");
            }
        } else {
            extend_type_decl_str(env, output, node_wrap_variable_def_const(curr_param), true);
        }
        if (curr_param->is_variadic) {
            return;
        }
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, curr_param->llvm_id);
    }
}

static void emit_function_call_arguments(const Env* env, String* output, const Node_function_call* fun_call) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        const Node* argument = vec_at(&fun_call->args, idx);

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        switch (argument->type) {
            case NODE_LITERAL: {
                extend_literal_decl(env, output, node_unwrap_literal_const(argument), true);
                break;
            }
            case NODE_STRUCT_MEMBER_SYM_TYPED:
                unreachable("");
                break;
            case NODE_STRUCT_LITERAL:
                todo();
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_SYMBOL_TYPED:
                unreachable("typed symbols should not still be present");
            case NODE_PTR_BYVAL_SYM:
                string_extend_cstr(&a_main, output, "ptr noundef byval(");
                extend_type_call_str(env, output, get_lang_type(argument));
                string_extend_cstr(&a_main, output, ")");
                string_extend_cstr(&a_main, output, " %");
                string_extend_size_t(&a_main, output, get_llvm_id(node_unwrap_ptr_byval_sym_const(argument)->node_src));
                break;
            case NODE_LLVM_REGISTER_SYM:
                extend_type_call_str(env, output, get_lang_type(argument));
                string_extend_cstr(&a_main, output, " %");
                string_extend_size_t(&a_main, output, get_llvm_id(node_unwrap_llvm_register_sym_const(argument)->node_src));
                break;
            case NODE_FUNCTION_CALL:
                unreachable(""); // this function call should be changed to assign to a variable 
                               // before reaching emit_llvm stage, then assign that variable here. 
            default:
                unreachable(NODE_FMT"\n", node_print(argument));
        }
    }
}

static void emit_function_call(const Env* env, String* output, const Node_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, output, "    ");
    if (!lang_type_is_equal(fun_call->lang_type, lang_type_from_cstr("void", 0))) {
        string_extend_cstr(&a_main, output, "%");
        string_extend_size_t(&a_main, output, fun_call->llvm_id);
        string_extend_cstr(&a_main, output, " = ");
    }
    string_extend_cstr(&a_main, output, "call ");
    extend_type_call_str(env, output, fun_call->lang_type);
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_call->name);

    // arguments
    string_extend_cstr(&a_main, output, "(");
    emit_function_call_arguments(env, output, fun_call);
    string_extend_cstr(&a_main, output, ")");

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_alloca(const Env* env, String* output, const Node_alloca* alloca) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, alloca->llvm_id);
    string_extend_cstr(&a_main, output, " = alloca ");
    extend_type_call_str(env, output, get_symbol_def_from_alloca(env, node_wrap_alloca_const(alloca))->lang_type);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_unary_type(const Env* env, String* output, const Node_unary* unary) {
    switch (unary->token_type) {
        case TOKEN_UNSAFE_CAST:
            if (!is_i_lang_type(unary->lang_type) || !is_i_lang_type(get_lang_type(unary->child))) {
                unreachable("");
            }
            if (i_lang_type_to_bit_width(unary->lang_type) > i_lang_type_to_bit_width(get_lang_type(unary->child))) {
                string_extend_cstr(&a_main, output, "zext ");
            } else {
                string_extend_cstr(&a_main, output, "trunc ");
            }
            extend_type_call_str(env, output, get_lang_type(unary->child));
            string_extend_cstr(&a_main, output, " ");
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_operator(node_wrap_operator_generic(unary))));
    }
}

// TODO: make Node_untyped_binary
static void emit_binary_type(const Env* env, String* output, const Node_binary* binary) {
    // TODO: do signed and unsigned operators correctly
    switch (binary->token_type) {
        case TOKEN_SINGLE_MINUS:
            string_extend_cstr(&a_main, output, "sub nsw ");
            break;
        case TOKEN_SINGLE_PLUS:
            string_extend_cstr(&a_main, output, "add nsw ");
            break;
        case TOKEN_ASTERISK:
            string_extend_cstr(&a_main, output, "mul nsw ");
            break;
        case TOKEN_SLASH:
            string_extend_cstr(&a_main, output, "sdiv ");
            break;
        case TOKEN_LESS_THAN:
            string_extend_cstr(&a_main, output, "icmp slt ");
            break;
        case TOKEN_GREATER_THAN:
            string_extend_cstr(&a_main, output, "icmp sgt ");
            break;
        case TOKEN_DOUBLE_EQUAL:
            string_extend_cstr(&a_main, output, "icmp eq ");
            break;
        case TOKEN_NOT_EQUAL:
            string_extend_cstr(&a_main, output, "icmp ne ");
            break;
        case TOKEN_XOR:
            string_extend_cstr(&a_main, output, "xor ");
            break;
        default:
            unreachable(TOKEN_TYPE_FMT"\n", token_type_print(binary->token_type));
    }

    extend_type_call_str(env, output, binary->lang_type);
    string_extend_cstr(&a_main, output, " ");
}

static void emit_unary_suffix(const Env* env, String* output, const Node_unary* unary) {
    switch (unary->token_type) {
        case TOKEN_UNSAFE_CAST:
            string_extend_cstr(&a_main, output, " to ");
            extend_type_call_str(env, output, unary->lang_type);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_operator(node_wrap_operator_generic(unary))));
    }
}

static void emit_operator_operand(String* output, const Node* operand) {
    switch (operand->type) {
        case NODE_LITERAL:
            extend_literal(output, node_unwrap_literal_const(operand));
            break;
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_LLVM_REGISTER_SYM:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, get_llvm_id(node_unwrap_llvm_register_sym_const(operand)->node_src));
            break;
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        default:
            unreachable(NODE_FMT"\n", node_print(operand));
    }
}

static void emit_operator(const Env* env, String* output, const Node_operator* operator) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, get_llvm_id(node_wrap_operator(node_wrap_operator_generic(operator))));
    string_extend_cstr(&a_main, output, " = ");

    if (operator->type == NODE_OP_UNARY) {
        emit_unary_type(env, output, node_unwrap_op_unary_const(operator));
    } else if (operator->type == NODE_OP_BINARY) {
        emit_binary_type(env, output, node_unwrap_op_binary_const(operator));
    } else {
        unreachable("");
    }

    if (operator->type == NODE_OP_UNARY) {
        emit_operator_operand(output, node_unwrap_op_unary_const(operator)->child);
    } else if (operator->type == NODE_OP_BINARY) {
        const Node_binary* binary = node_unwrap_op_binary_const(operator);
        emit_operator_operand(output, binary->lhs);
        string_extend_cstr(&a_main, output, ", ");
        emit_operator_operand(output, binary->rhs);
    } else {
        unreachable("");
    }

    if (operator->type == NODE_OP_UNARY) {
        emit_unary_suffix(env, output, node_unwrap_op_unary_const(operator));
    } else if (operator->type == NODE_OP_BINARY) {
    } else {
        unreachable("");
    }

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_load_another_node(const Env* env, String* output, const Node_load_another_node* load_node) {
    assert(get_llvm_id(load_node->node_src) > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, load_node->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
    extend_type_call_str(env, output, load_node->lang_type);
    string_extend_cstr(&a_main, output, ", ");
    string_extend_cstr(&a_main, output, "ptr");
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, get_llvm_id(load_node->node_src));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_llvm_store_struct_literal(const Env* env, String* output, const Node_llvm_store_struct_literal* store) {
    string_extend_cstr(&a_main, output, "    call void @llvm.memcpy.p0.p0.i64(ptr align 4 %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_dest));
    string_extend_cstr(&a_main, output, ", ptr align 4 @__const.main.");
    string_extend_strv(&a_main, output, store->child->name);
    string_extend_cstr(&a_main, output, ", i64 ");
    string_extend_size_t(&a_main, output, sizeof_struct_literal(env, store->child));
    string_extend_cstr(&a_main, output, ", i1 false)\n");
}

static void emit_store_another_node(const Env* env, String* output, const Node_store_another_node* store) {
    assert(store->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(env, output, store->lang_type);
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_src));
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_dest));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_llvm_store_literal(const Env* env, String* output, const Node_llvm_store_literal* store) {
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(env, output, store->lang_type);
    extend_literal_decl_prefix(output, store->child);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_dest));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_function_definition(Env* env, String* output, const Node_function_definition* fun_def) {
    string_extend_cstr(&a_main, output, "define dso_local ");

    extend_type_call_str(env, output, return_type_from_function_definition(fun_def)->lang_type);

    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, get_node_name(node_wrap_function_definition_const(fun_def)));

    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_def->declaration->parameters);
    vec_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");
    emit_block(env, output, fun_def->body);
    string_extend_cstr(&a_main, output, "}\n");
}

static void emit_return_statement(const Env* env, String* output, const Node_return_statement* fun_return) {
    const Node* sym_to_return = fun_return->child;
    assert(get_lang_type(sym_to_return).str.count > 0);

    switch (sym_to_return->type) {
        case NODE_LITERAL: {
            const Node_literal* literal = node_unwrap_literal_const(sym_to_return);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, literal->lang_type);
            string_extend_cstr(&a_main, output, " ");
            extend_literal(output, literal);
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        case NODE_SYMBOL_TYPED:
           unreachable("");
        case NODE_STRUCT_MEMBER_SYM_TYPED:
             unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        case NODE_LLVM_REGISTER_SYM: {
            const Node_llvm_register_sym* memb_sym = node_unwrap_llvm_register_sym_const(sym_to_return);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, memb_sym->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, get_llvm_id(memb_sym->node_src));
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        default:
            unreachable(NODE_FMT, node_print(sym_to_return));
    }
}

static void emit_function_declaration(const Env* env, String* output, const Node_function_declaration* fun_decl) {
    string_extend_cstr(&a_main, output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_decl->name);
    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_decl->parameters);
    vec_append(&a_main, output, ')');
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_label(String* output, const Node_label* label) {
    string_extend_cstr(&a_main, output, "\n");
    string_extend_size_t(&a_main, output, label->llvm_id);
    string_extend_cstr(&a_main, output, ":\n");
}

static void emit_goto(const Env* env, String* output, const Node_goto* lang_goto) {
    string_extend_cstr(&a_main, output, "    br label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, lang_goto->name));
    vec_append(&a_main, output, '\n');
}

static void emit_cond_goto(const Env* env, String* output, const Node_cond_goto* cond_goto) {
    string_extend_cstr(&a_main, output, "    br i1 %");
    string_extend_size_t(&a_main, output, get_llvm_id(node_wrap_operator(cond_goto->node_src)));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_true->name));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_false->name));
    vec_append(&a_main, output, '\n');
}

static void emit_struct_definition(const Env* env, String* output, const Node_struct_def* struct_def) {
    string_extend_cstr(&a_main, output, "%struct.");
    string_extend_strv(&a_main, output, struct_def->name);
    string_extend_cstr(&a_main, output, " = type { ");
    bool is_first = true;
    for (size_t idx = 0; idx < struct_def->members.info.count; idx++) {
        if (!is_first) {
            string_extend_cstr(&a_main, output, ", ");
        }
        extend_type_decl_str(env, output, vec_at(&struct_def->members, idx), false);
        is_first = false;
    }
    string_extend_cstr(&a_main, output, " }\n");
}

static void emit_load_struct_element_pointer(String* output, const Node_load_element_ptr* load_elem_ptr) {
    assert(load_elem_ptr->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "    %"); 
    string_extend_size_t(&a_main, output, load_elem_ptr->llvm_id);
    string_extend_cstr(&a_main, output, " = getelementptr inbounds %struct.");
    if (get_lang_type(load_elem_ptr->node_src).pointer_depth != 0) {
    }
    Lang_type lang_type = get_lang_type(load_elem_ptr->node_src);
    lang_type.pointer_depth = 0;
    extend_lang_type_to_string(&a_main, output, lang_type, false);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(load_elem_ptr->node_src));
    string_extend_cstr(&a_main, output, ", i32 0");
    string_extend_cstr(&a_main, output, ", i32 ");
    string_extend_size_t(&a_main, output, load_elem_ptr->struct_index);
    vec_append(&a_main, output, '\n');
}

static void emit_block(Env* env, String* output, const Node_block* block) {
    vec_append(&a_main, &env->ancesters, (Node*)node_wrap_block_const(block));

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        const Node* statement = vec_at(&block->children, idx);

        switch (statement->type) {
            case NODE_FUNCTION_DEFINITION:
                emit_function_definition(env, output, node_unwrap_function_definition_const(statement));
                break;
            case NODE_FUNCTION_CALL:
                emit_function_call(env, output, node_unwrap_function_call_const(statement));
                break;
            case NODE_RETURN_STATEMENT:
                emit_return_statement(env, output, node_unwrap_return_statement_const(statement));
                break;
            case NODE_VARIABLE_DEF:
                break;
            case NODE_FUNCTION_DECLARATION:
                emit_function_declaration(env, output, node_unwrap_function_declaration_const(statement));
                break;
            case NODE_ASSIGNMENT:
                unreachable("an assignment should not still be present at this point");
            case NODE_BLOCK:
                emit_block(env, output, node_unwrap_block_const(statement));
                break;
            case NODE_LABEL:
                emit_label(output, node_unwrap_label_const(statement));
                break;
            case NODE_COND_GOTO:
                emit_cond_goto(env, output, node_unwrap_cond_goto_const(statement));
                break;
            case NODE_GOTO:
                emit_goto(env, output, node_unwrap_goto_const(statement));
                break;
            case NODE_ALLOCA:
                emit_alloca(env, output, node_unwrap_alloca_const(statement));
                break;
            case NODE_LLVM_STORE_LITERAL:
                emit_llvm_store_literal(env, output, node_unwrap_llvm_store_literal_const(statement));
                break;
            case NODE_LLVM_STORE_STRUCT_LITERAL:
                emit_llvm_store_struct_literal(env, output, node_unwrap_llvm_store_struct_literal_const(statement));
                break;
            case NODE_STRUCT_DEF:
                emit_struct_definition(env, output, node_unwrap_struct_def_const(statement));
                break;
            case NODE_OPERATOR:
                emit_operator(env, output, node_unwrap_operator_const(statement));
                break;
            case NODE_LOAD_ELEMENT_PTR:
                emit_load_struct_element_pointer(output, node_unwrap_load_element_ptr_const(statement));
                break;
            case NODE_LOAD_ANOTHER_NODE:
                emit_load_another_node(env, output, node_unwrap_load_another_node_const(statement));
                break;
            case NODE_STORE_ANOTHER_NODE:
                emit_store_another_node(env, output, node_unwrap_store_another_node_const(statement));
                break;
            case NODE_STRUCT_MEMBER_SYM_TYPED:
                break;
            case NODE_FOR_RANGE:
                unreachable("for loop should not still be present at this point\n");
            case NODE_FOR_WITH_CONDITION:
                unreachable("for loop should not still be present at this point\n");
            default:
                log(LOG_ERROR, STRING_FMT"\n", string_print(*output));
                node_printf(statement);
                todo();
        }
    }
    //get_block_return_id(fun_block) = get_block_return_id(fun_block->left_child);
    vec_rem_last(&env->ancesters);
}

static void emit_symbol(String* output, const Symbol_table_node node) {
    Str_view str_data;
    const Node_literal* literal = node_unwrap_literal_const(node.node);
    switch (literal->type) {
        case NODE_LIT_STRING:
            str_data = node_unwrap_lit_string_const(literal)->data;
            break;
        case NODE_LIT_NUMBER:
            return;
        default:
            unreachable("");
    }

    size_t literal_width = str_data.count + 1 - get_count_excape_seq(str_data);

    string_extend_cstr(&a_main, output, "@.");
    string_extend_strv(&a_main, output, node.key);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant [ ");
    string_extend_size_t(&a_main, output, literal_width);
    string_extend_cstr(&a_main, output, " x i8] c\"");
    string_extend_strv_eval_escapes(&a_main, output, str_data);
    string_extend_cstr(&a_main, output, "\\00\", align 1");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_struct_literal(const Env* env, String* output, const Node_struct_literal* struct_literal) {
    assert(struct_literal->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, struct_literal->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(&a_main, output, struct_literal->lang_type, false);
    string_extend_cstr(&a_main, output, " {");

    size_t is_first = true;
    for (size_t idx = 0; idx < struct_literal->members.info.count; idx++) {
        const Node* memb_literal = vec_at(&struct_literal->members, idx);
        if (!is_first) {
            vec_append(&a_main, output, ',');
        }
        extend_literal_decl(env, output, node_unwrap_literal_const(memb_literal), false);
        is_first = false;
    }

    string_extend_cstr(&a_main, output, "} , align 4\n");
}

static void emit_symbols(const Env* env, String* output) {
    log(LOG_DEBUG, "env->global_literals: %p\n", (void*)&env->global_literals);
    for (size_t idx = 0; idx < env->global_literals.capacity; idx++) {
        const Symbol_table_node curr_node = env->global_literals.table_nodes[idx];
        if (curr_node.status != SYM_TBL_OCCUPIED) {
            continue;
        }

        switch (curr_node.node->type) {
            case NODE_LITERAL:
                emit_symbol(output, curr_node);
                break;
            case NODE_STRUCT_LITERAL:
                emit_struct_literal(env, output, node_unwrap_struct_literal(curr_node.node));
                break;
            case NODE_VARIABLE_DEF:
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
            case NODE_STRUCT_DEF:
                break;
            default:
                log(LOG_FETAL, NODE_FMT"\n", node_print(curr_node.node));
                todo();
        }
    }
}

void emit_llvm_from_tree(Env* env, const Node_block* root) {
    String output = {0};
    emit_block(env, &output, root);
    emit_symbols(env, &output);
    log(LOG_DEBUG, "\n"STRING_FMT"\n", string_print(output));
    Str_view final_output = {.str = output.buf, .count = output.info.count};
    write_file("test.ll", final_output);
}

