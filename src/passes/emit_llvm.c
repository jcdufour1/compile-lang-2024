
#include <node.h>
#include <nodes.h>
#include <newstring.h>
#include "passes.h"
#include <symbol_table.h>
#include <parameters.h>
#include <file.h>
#include <parser_utils.h>
#include <node_utils.h>

static void emit_block(Env* env, String* output, const Node_block* fun_block);

static bool node_is_literal(const Node* node) {
    if (node->type != NODE_EXPR) {
        return false;
    }
    return node_unwrap_expr_const(node)->type == NODE_LITERAL;
}

static void extend_literal(String* output, const Node_literal* literal) {
    switch (literal->type) {
        case NODE_STRING:
            string_extend_strv(&a_main, output, node_unwrap_string_const(literal)->data);
            return;
        case NODE_NUMBER:
            string_extend_int64_t(&a_main, output, node_unwrap_number_const(literal)->data);
            return;
        case NODE_ENUM_LIT:
            string_extend_int64_t(&a_main, output, node_unwrap_enum_lit_const(literal)->data);
            return;
        case NODE_VOID:
            return;
        case NODE_CHAR:
            string_extend_int64_t(&a_main, output, (int64_t)node_unwrap_char_const(literal)->data);
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

    if (lang_type_is_struct(env, lang_type)) {
        string_extend_cstr(&a_main, output, "%struct.");
    } else if (lang_type_is_raw_union(env, lang_type)) {
        string_extend_cstr(&a_main, output, "%union.");
    } else if (lang_type_is_enum(env, lang_type)) {
        lang_type = lang_type_new_from_cstr("i32", 0);
    } else if (lang_type_is_primitive(env, lang_type)) {
        if (lang_type_is_unsigned(lang_type)) {
            lang_type = lang_type_unsigned_to_signed(lang_type);
        }
    } else {
        unreachable("");
    }

    assert(lang_type.str.str[0] != 'u');
    extend_lang_type_to_string(&a_main, output, lang_type, false);
}

static bool is_variadic(const Node* node) {
    if (node->type == NODE_EXPR) {
        switch (node_unwrap_expr_const(node)->type) {
            case NODE_LITERAL:
                return false;
            default:
                unreachable(NODE_FMT, node_print(node));
        }
    } else {
        const Node_def* def = node_unwrap_def_const(node);
        switch (def->type) {
            case NODE_VARIABLE_DEF:
                return node_unwrap_variable_def_const(node_unwrap_def_const(node))->is_variadic;
            default:
                unreachable(NODE_FMT, node_print(node));
        }
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

static void extend_literal_decl_prefix(const Env* env, String* output, const Node_literal* literal) {
    log(LOG_DEBUG, "entering thing\n");
    assert(get_lang_type_literal(literal).str.count > 0);
    if (str_view_cstr_is_equal(get_lang_type_literal(literal).str, "u8")) {
        if (get_lang_type_literal(literal).pointer_depth != 1) {
            todo();
        }
        string_extend_cstr(&a_main, output, " @.");
        string_extend_strv(&a_main, output, get_literal_name(literal));
    } else if (lang_type_is_signed(get_lang_type_literal(literal))) {
        if (get_lang_type_literal(literal).pointer_depth != 0) {
            todo();
        }
        vec_append(&a_main, output, ' ');
        extend_literal(output, literal);
    } else if (lang_type_is_enum(env, get_lang_type_literal(literal))) {
        vec_append(&a_main, output, ' ');
        extend_literal(output, literal);
    } else {
        log(LOG_DEBUG, BOOL_FMT"\n", bool_print(lang_type_is_enum(env, get_lang_type_literal(literal))));
        log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(get_lang_type_literal(literal)));
        unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(node_wrap_literal_const(literal))));
    }
}

static void extend_literal_decl(const Env* env, String* output, const Node_literal* literal, bool noundef) {
    extend_type_decl_str(env, output, node_wrap_expr_const(node_wrap_literal_const(literal)), noundef);
    extend_literal_decl_prefix(env, output, literal);
}

static const Node_lang_type* return_type_from_function_def(const Node_function_def* fun_def) {
    const Node_lang_type* return_type = fun_def->declaration->return_type;
    if (return_type) {
        return return_type;
    }
    unreachable("");
}

static void emit_function_params(const Env* env, String* output, const Node_function_params* fun_params) {
    for (size_t idx = 0; idx < fun_params->params.info.count; idx++) {
        const Node_variable_def* curr_param = vec_at(&fun_params->params, idx);

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        if (lang_type_is_struct(env, curr_param->lang_type) || lang_type_is_raw_union(env, curr_param->lang_type)) {
            if (curr_param->lang_type.pointer_depth < 0) {
                unreachable("");
            } else if (curr_param->lang_type.pointer_depth > 0) {
                extend_type_call_str(env, output, curr_param->lang_type);
            } else {
                string_extend_cstr(&a_main, output, "ptr noundef byval(");
                extend_type_call_str(env, output, curr_param->lang_type);
                string_extend_cstr(&a_main, output, ")");
            }
        } else if (lang_type_is_enum(env, curr_param->lang_type) || lang_type_is_primitive(env, curr_param->lang_type)) {
            extend_type_decl_str(env, output, node_wrap_def_const(node_wrap_variable_def_const(curr_param)), true);
        } else {
            unreachable("");
        }

        if (curr_param->is_variadic) {
            return;
        }
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, curr_param->llvm_id);
    }
}

static void emit_function_call_arg_llvm_placeholder(
    const Env* env,
    String* output,
    const Node_llvm_placeholder* placeholder
) {
    Llvm_id llvm_id = 0;
    log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(placeholder->lang_type));
    if (lang_type_is_struct(env, placeholder->lang_type) && placeholder->lang_type.pointer_depth == 0) {
        log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(get_node_name(placeholder->llvm_reg.node)));
        llvm_id = get_llvm_id(get_storage_location(env, get_node_name(placeholder->llvm_reg.node)).node);
        assert(llvm_id > 0);
        string_extend_cstr(&a_main, output, "ptr noundef byval(");
        extend_type_call_str(env, output, placeholder->lang_type);
        string_extend_cstr(&a_main, output, ")");
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, llvm_id);
    } else if (lang_type_is_enum(env, placeholder->lang_type)) {
        llvm_id = get_llvm_id(placeholder->llvm_reg.node);
        extend_type_call_str(env, output, placeholder->lang_type);
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, llvm_id);
    } else if (node_is_literal(placeholder->llvm_reg.node)) {
        extend_literal_decl(env, output, node_unwrap_literal_const(node_unwrap_expr_const(placeholder->llvm_reg.node)), true);
    } else {
        log(LOG_DEBUG, NODE_FMT"\n", node_print(placeholder->llvm_reg.node));
        llvm_id = get_llvm_id(placeholder->llvm_reg.node);
        extend_type_call_str(env, output, placeholder->lang_type);
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, llvm_id);
    }
}

static void emit_function_call_arguments(const Env* env, String* output, const Node_function_call* fun_call) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        const Node_expr* argument = vec_at(&fun_call->args, idx);

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        switch (argument->type) {
            case NODE_LITERAL:
                extend_literal_decl(env, output, node_unwrap_literal_const(argument), true);
                break;
            case NODE_MEMBER_ACCESS_TYPED:
                // TODO: make test case with member access in function arg directly
                unreachable("");
                break;
            case NODE_STRUCT_LITERAL:
                todo();
            case NODE_SYMBOL_UNTYPED:
                unreachable("untyped symbols should not still be present");
            case NODE_SYMBOL_TYPED:
                unreachable("typed symbols should not still be present");
            case NODE_LLVM_PLACEHOLDER: {
                const Node_llvm_placeholder* placeholder = node_unwrap_llvm_placeholder_const(argument);
                emit_function_call_arg_llvm_placeholder(env, output, placeholder);
                break;
            }
            case NODE_FUNCTION_CALL:
                unreachable("");
            default:
                unreachable("");
        }
    }
}

static void emit_function_call(const Env* env, String* output, const Node_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, output, "    ");
    if (!lang_type_is_equal(fun_call->lang_type, lang_type_new_from_cstr("void", 0))) {
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
            if (unary->lang_type.pointer_depth > 0 && lang_type_is_number(get_lang_type_expr(unary->child))) {
                string_extend_cstr(&a_main, output, "inttoptr ");
                extend_type_call_str(env, output, get_lang_type_expr(unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_number(unary->lang_type) && get_lang_type_expr(unary->child).pointer_depth > 0) {
                string_extend_cstr(&a_main, output, "ptrtoint ");
                extend_type_call_str(env, output, get_lang_type_expr(unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_number(unary->lang_type) && lang_type_is_number(get_lang_type_expr(unary->child))) {
                if (i_lang_type_to_bit_width(unary->lang_type) > i_lang_type_to_bit_width(get_lang_type_expr(unary->child))) {
                    string_extend_cstr(&a_main, output, "zext ");
                } else {
                    string_extend_cstr(&a_main, output, "trunc ");
                }
                extend_type_call_str(env, output, get_lang_type_expr(unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else {
                todo();
            }
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(node_wrap_operator_const(node_wrap_unary_const(unary)))));
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
            unreachable("");
    }
}

static void emit_operator_operand_llvm_placeholder_expr(
    String* output,
    const Node_expr* expr
) {
    switch (expr->type) {
        case NODE_LITERAL:
            extend_literal(output, node_unwrap_literal_const(expr));
            break;
        case NODE_OPERATOR:
            // fallthrough
        case NODE_FUNCTION_CALL:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = get_llvm_id_expr(expr);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(expr)));
    }
}

static void emit_operator_operand_llvm_placeholder(
    String* output,
    const Node_llvm_placeholder* placeholder
) {
    const Node* node = placeholder->llvm_reg.node;

    switch (node->type) {
        case NODE_EXPR:
            emit_operator_operand_llvm_placeholder_expr(output, node_unwrap_expr_const(node));
            break;
        case NODE_LOAD_ANOTHER_NODE:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = get_llvm_id(node);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(placeholder->llvm_reg.node));
    }
}

static void emit_operator_operand(String* output, const Node_expr* operand) {
    switch (operand->type) {
        case NODE_LITERAL:
            extend_literal(output, node_unwrap_literal_const(operand));
            break;
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_LLVM_PLACEHOLDER:
            emit_operator_operand_llvm_placeholder(
                output, node_unwrap_llvm_placeholder_const(operand)
            );
            break;
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        case NODE_FUNCTION_CALL:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, node_unwrap_function_call_const(operand)->llvm_id);
            break;
        default:
            unreachable(NODE_FMT, node_print((Node*)operand));
    }
}

static void emit_operator(const Env* env, String* output, const Node_operator* operator) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, get_llvm_id_expr(node_wrap_operator_const(operator)));
    string_extend_cstr(&a_main, output, " = ");

    if (operator->type == NODE_UNARY) {
        emit_unary_type(env, output, node_unwrap_unary_const(operator));
    } else if (operator->type == NODE_BINARY) {
        emit_binary_type(env, output, node_unwrap_binary_const(operator));
    } else {
        unreachable("");
    }

    if (operator->type == NODE_UNARY) {
        emit_operator_operand(output, node_unwrap_unary_const(operator)->child);
    } else if (operator->type == NODE_BINARY) {
        const Node_binary* binary = node_unwrap_binary_const(operator);
        emit_operator_operand(output, binary->lhs);
        string_extend_cstr(&a_main, output, ", ");
        emit_operator_operand(output, binary->rhs);
    } else {
        unreachable("");
    }

    if (operator->type == NODE_UNARY) {
        emit_unary_suffix(env, output, node_unwrap_unary_const(operator));
    } else if (operator->type == NODE_BINARY) {
    } else {
        unreachable("");
    }

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_load_another_node(const Env* env, String* output, const Node_load_another_node* load_node) {
    Llvm_id llvm_id = get_llvm_id(load_node->node_src.node);
    log(LOG_DEBUG, NODE_FMT"\n", node_print(node_wrap_load_another_node_const(load_node)));
    log(LOG_DEBUG, NODE_FMT"\n", node_print(load_node->node_src.node));
    assert(llvm_id > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, load_node->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
    extend_type_call_str(env, output, load_node->lang_type);
    string_extend_cstr(&a_main, output, ", ");
    string_extend_cstr(&a_main, output, "ptr");
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, llvm_id);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_memcpy_struct_literal(
    const Env* env,
    String* output,
    Llvm_id dest,
    const Node_struct_literal* literal
) {
    string_extend_cstr(&a_main, output, "    call void @llvm.memcpy.p0.p0.i64(ptr align 4 %");
    string_extend_size_t(&a_main, output, dest);
    string_extend_cstr(&a_main, output, ", ptr align 4 @__const.main.");
    string_extend_strv(&a_main, output, literal->name);
    string_extend_cstr(&a_main, output, ", i64 ");
    string_extend_size_t(&a_main, output, sizeof_struct_literal(env, literal));
    string_extend_cstr(&a_main, output, ", i1 false)\n");
}

static void emit_store_another_node_src_literal(
    String* output,
    const Node_literal* literal
) {
    string_extend_cstr(&a_main, output, " ");

    switch (literal->type) {
        case NODE_STRING:
            string_extend_cstr(&a_main, output, " @.");
            string_extend_strv(&a_main, output, node_unwrap_string_const(literal)->name);
            return;
        case NODE_NUMBER:
            string_extend_int64_t(&a_main, output, node_unwrap_number_const(literal)->data);
            return;
        case NODE_ENUM_LIT:
            string_extend_int64_t(&a_main, output, node_unwrap_enum_lit_const(literal)->data);
            return;
        case NODE_VOID:
            return;
        case NODE_CHAR:
            string_extend_int64_t(&a_main, output, node_unwrap_char_const(literal)->data);
    }
    unreachable("");
}

static void emit_store_another_node_src_expr(const Env* env, String* output, const Node_expr* expr) {
    (void) env;

    switch (expr->type) {
        case NODE_LITERAL:
            emit_store_another_node_src_literal(output, node_unwrap_literal_const(expr));
            return;
        case NODE_FUNCTION_CALL:
            // fallthrough
        case NODE_OPERATOR:
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, get_llvm_id_expr(expr));
            break;
        case NODE_STRUCT_LITERAL:
            unreachable("");
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(node_wrap_expr_const(expr)));
    }
}

static void emit_store_another_node(const Env* env, String* output, const Node_store_another_node* store) {
    log_tree(LOG_DEBUG, (Node*)store);
    log_tree(LOG_DEBUG, store->node_src.node);
    log_tree(LOG_DEBUG, store->node_dest.node);
    //log(LOG_DEBUG, "%zu\n", get_llvm_id(store->node_src.node));
    //log(LOG_DEBUG, "%zu\n", get_llvm_id(store->node_dest.node));

    const Node* src = store->node_src.node;

    switch (src->type) {
        case NODE_EXPR: {
            const Node_expr* src_expr = node_unwrap_expr_const(src);
            switch (src_expr->type) {
                case NODE_STRUCT_LITERAL:
                    emit_memcpy_struct_literal(
                        env,
                        output,
                        get_llvm_id(store->node_dest.node),
                        node_unwrap_struct_literal_const(src_expr)
                    );
                    return;
                default:
                    break;
            }
        }
        default:
            break;
    }

    assert(store->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(env, output, store->lang_type);
    string_extend_cstr(&a_main, output, " ");

    switch (src->type) {
        case NODE_DEF: {
            const Node_def* src_def = node_unwrap_def_const(src);
            const Node_variable_def* src_var_def = node_unwrap_variable_def_const(src_def);
            (void) src_var_def;
            log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(string_to_strv(*output)));
            string_extend_cstr(&a_main, output, " %");

            //string_extend_size_t(&a_main, output, get_llvm_id(
            //    get_storage_location(env, node_unwrap_variable_def_const(src)->name).node
            //));
            string_extend_size_t(&a_main, output, get_llvm_id(src));

            log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(string_to_strv(*output)));
            break;
        }
        case NODE_EXPR:
            emit_store_another_node_src_expr(env, output, node_unwrap_expr_const(src));
            break;
        case NODE_LOAD_ANOTHER_NODE:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = get_llvm_id(src);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        default:
            unreachable(NODE_FMT"\n", node_print(src));
    }
    //string_extend_cstr(&a_main, output, " %");
    //string_extend_size_t(&a_main, output, get_llvm_id(store->node_src.node));

    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(string_to_strv(*output)));
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_dest.node));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_llvm_store_literal(const Env* env, String* output, const Node_llvm_store_literal* store) {
    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(env, output, store->lang_type);
    extend_literal_decl_prefix(env, output, store->child);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(store->node_dest.node));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_function_def(Env* env, String* output, const Node_function_def* fun_def) {
    string_extend_cstr(&a_main, output, "define dso_local ");

    extend_type_call_str(env, output, return_type_from_function_def(fun_def)->lang_type);

    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, get_node_name(node_wrap_def_const(node_wrap_function_def_const(fun_def))));

    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_def->declaration->parameters);
    vec_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");
    emit_block(env, output, fun_def->body);
    string_extend_cstr(&a_main, output, "}\n");
}

static void emit_return(const Env* env, String* output, const Node_return* fun_return) {
    const Node_expr* sym_to_return = fun_return->child;
    assert(get_lang_type_expr(sym_to_return).str.count > 0);

    log_tree(LOG_DEBUG, (Node*)fun_return);
    switch (sym_to_return->type) {
        case NODE_LITERAL: {
            const Node_literal* literal = node_unwrap_literal_const(sym_to_return);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, get_lang_type_literal(literal));
            string_extend_cstr(&a_main, output, " ");
            extend_literal(output, literal);
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        case NODE_SYMBOL_TYPED:
            unreachable("");
        case NODE_SYMBOL_UNTYPED:
            unreachable("untyped symbols should not still be present");
        case NODE_LLVM_PLACEHOLDER: {
            const Node_llvm_placeholder* memb_sym = node_unwrap_llvm_placeholder_const(sym_to_return);
            if (node_is_literal(memb_sym->llvm_reg.node)) {
                const Node_expr* memb_expr = node_unwrap_expr_const(memb_sym->llvm_reg.node);
                const Node_literal* literal = node_unwrap_literal_const(memb_expr);
                string_extend_cstr(&a_main, output, "    ret ");
                extend_type_call_str(env, output, get_lang_type_literal(literal));
                string_extend_cstr(&a_main, output, " ");
                extend_literal(output, literal);
                string_extend_cstr(&a_main, output, "\n");
                break;
            } else {
                string_extend_cstr(&a_main, output, "    ret ");
                extend_type_call_str(env, output, memb_sym->lang_type);
                string_extend_cstr(&a_main, output, " %");
                string_extend_size_t(&a_main, output, get_llvm_id(memb_sym->llvm_reg.node));
                string_extend_cstr(&a_main, output, "\n");
            }
            break;
        }
        default:
            unreachable("");
    }
}

static void emit_function_decl(const Env* env, String* output, const Node_function_decl* fun_decl) {
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
    //log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(string_to_strv(*output)));
    string_extend_size_t(&a_main, output, get_matching_label_id(env, lang_goto->name));
    vec_append(&a_main, output, '\n');
}

static void emit_cond_goto(const Env* env, String* output, const Node_cond_goto* cond_goto) {
    string_extend_cstr(&a_main, output, "    br i1 %");
    string_extend_size_t(&a_main, output, get_llvm_id(cond_goto->node_src.node));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_true));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_false));
    vec_append(&a_main, output, '\n');
}

static void emit_struct_def_base(const Env* env, String* output, const Struct_def_base* base) {
    string_extend_strv(&a_main, output, base->name);
    string_extend_cstr(&a_main, output, " = type { ");
    bool is_first = true;
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        if (!is_first) {
            string_extend_cstr(&a_main, output, ", ");
        }
        extend_type_decl_str(env, output, vec_at(&base->members, idx), false);
        is_first = false;
    }
    string_extend_cstr(&a_main, output, " }\n");
}

static void emit_struct_def(const Env* env, String* output, const Node_struct_def* struct_def) {
    string_extend_cstr(&a_main, output, "%struct.");
    emit_struct_def_base(env, output, &struct_def->base);
}

static void emit_raw_union_def(const Env* env, String* output, const Node_raw_union_def* raw_union_def) {
    string_extend_cstr(&a_main, output, "%union.");
    emit_struct_def_base(env, output, &raw_union_def->base);
}

static void emit_load_struct_element_pointer(const Env* env, String* output, const Node_load_element_ptr* load_elem_ptr) {
    assert(load_elem_ptr->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "    %"); 
    string_extend_size_t(&a_main, output, load_elem_ptr->llvm_id);

    string_extend_cstr(&a_main, output, " = getelementptr inbounds ");

    Lang_type lang_type = get_lang_type(load_elem_ptr->node_src.node);
    lang_type.pointer_depth = 0;
    extend_type_call_str(env, output, lang_type);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, get_llvm_id(load_elem_ptr->node_src.node));
    if (load_elem_ptr->is_from_struct) {
        string_extend_cstr(&a_main, output, ", i32 0");
    }
    string_extend_cstr(&a_main, output, ", ");
    extend_type_call_str(env, output, get_lang_type(load_elem_ptr->struct_index.node));
    string_extend_cstr(&a_main, output, " ");
    log_tree(LOG_DEBUG, load_elem_ptr->struct_index.node);

    if (load_elem_ptr->struct_index.node->type == NODE_LOAD_ANOTHER_NODE) {
        string_extend_cstr(&a_main, output, "%");
        string_extend_size_t(&a_main, output, get_llvm_id(load_elem_ptr->struct_index.node));
    } else {
        emit_operator_operand(output, node_unwrap_expr_const(load_elem_ptr->struct_index.node));
    }

    vec_append(&a_main, output, '\n');
}

static void emit_expr(Env* env, String* output, const Node_expr* expr) {
    switch (expr->type) {
        case NODE_OPERATOR:
            emit_operator(env, output, node_unwrap_operator_const(expr));
            break;
        case NODE_FUNCTION_CALL:
            emit_function_call(env, output, node_unwrap_function_call_const(expr));
            break;
        case NODE_MEMBER_ACCESS_TYPED:
            break;
        case NODE_LITERAL:
            extend_literal_decl(env, output, node_unwrap_literal_const(expr), true);
            break;
        default:
            unreachable("");
    }
}

static void emit_def(Env* env, String* output, const Node_def* def) {
    switch (def->type) {
        case NODE_FUNCTION_DEF:
            emit_function_def(env, output, node_unwrap_function_def_const(def));
            break;
        case NODE_VARIABLE_DEF:
            break;
        case NODE_FUNCTION_DECL:
            emit_function_decl(env, output, node_unwrap_function_decl_const(def));
            break;
        case NODE_LABEL:
            emit_label(output, node_unwrap_label_const(def));
            break;
        case NODE_STRUCT_DEF:
            emit_struct_def(env, output, node_unwrap_struct_def_const(def));
            break;
        case NODE_RAW_UNION_DEF:
            emit_raw_union_def(env, output, node_unwrap_raw_union_def_const(def));
            break;
        case NODE_ENUM_DEF:
            break;
        default:
            unreachable("");
    }
}

static void emit_block(Env* env, String* output, const Node_block* block) {
    vec_append(&a_main, &env->ancesters, (Node*)node_wrap_block_const(block));

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        const Node* statement = vec_at(&block->children, idx);

        switch (statement->type) {
            case NODE_EXPR:
                emit_expr(env, output, node_unwrap_expr_const(statement));
                break;
            case NODE_DEF:
                emit_def(env, output, node_unwrap_def_const(statement));
                break;
            case NODE_RETURN:
                emit_return(env, output, node_unwrap_return_const(statement));
                break;
            case NODE_ASSIGNMENT:
                unreachable("an assignment should not still be present at this point");
            case NODE_BLOCK:
                emit_block(env, output, node_unwrap_block_const(statement));
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
                unreachable("");
                break;
            case NODE_LOAD_ELEMENT_PTR:
                emit_load_struct_element_pointer(env, output, node_unwrap_load_element_ptr_const(statement));
                break;
            case NODE_LOAD_ANOTHER_NODE:
                emit_load_another_node(env, output, node_unwrap_load_another_node_const(statement));
                break;
            case NODE_STORE_ANOTHER_NODE:
                emit_store_another_node(env, output, node_unwrap_store_another_node_const(statement));
                break;
            case NODE_FOR_RANGE:
                unreachable("for loop should not still be present at this point\n");
            case NODE_FOR_WITH_COND:
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

static void emit_symbol(String* output, Str_view key, const Node_string_def* def) {
    size_t literal_width = def->data.count + 1 - get_count_excape_seq(def->data);

    string_extend_cstr(&a_main, output, "@.");
    string_extend_strv(&a_main, output, key);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant [ ");
    string_extend_size_t(&a_main, output, literal_width);
    string_extend_cstr(&a_main, output, " x i8] c\"");
    string_extend_strv_eval_escapes(&a_main, output, def->data);
    string_extend_cstr(&a_main, output, "\\00\", align 1");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_struct_literal(const Env* env, String* output, const Node_struct_lit_def* lit_def) {
    assert(lit_def->lang_type.str.count > 0);
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, lit_def->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(&a_main, output, lit_def->lang_type, false);
    string_extend_cstr(&a_main, output, " {");

    size_t is_first = true;
    for (size_t idx = 0; idx < lit_def->members.info.count; idx++) {
        const Node* memb_literal = vec_at(&lit_def->members, idx);
        if (!is_first) {
            vec_append(&a_main, output, ',');
        }
        extend_literal_decl(env, output, node_unwrap_literal_const(node_unwrap_expr_const(memb_literal)), false);
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

        const Node_literal_def* def = node_unwrap_literal_def_const(curr_node.node);

        switch (def->type) {
            case NODE_STRUCT_LIT_DEF:
                emit_struct_literal(env, output, node_unwrap_struct_lit_def_const(def));
                break;
            case NODE_STRING_DEF:
                emit_symbol(output, curr_node.key, node_unwrap_string_def_const(def));
                break;
            default:
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

