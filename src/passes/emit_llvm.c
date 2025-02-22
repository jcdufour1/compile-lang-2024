
#include <llvm.h>
#include <llvms.h>
#include <newstring.h>
#include "passes.h"
#include <symbol_table.h>
#include <parameters.h>
#include <file.h>
#include <parser_utils.h>
#include <llvm_utils.h>
#include <errno.h>
#include <serialize.h>

static void emit_block(Env* env, String* struct_defs, String* output, String* literals, const Llvm_block* fun_block);

static void emit_symbol_normal(String* literals, Str_view key, const Llvm_literal* lit);

static bool llvm_is_literal(const Llvm* llvm) {
    if (llvm->type != LLVM_EXPR) {
        return false;
    }
    return llvm_expr_const_unwrap(llvm)->type == LLVM_LITERAL;
}

static void extend_literal(String* output, const Llvm_literal* literal) {
    switch (literal->type) {
        case LLVM_STRING:
            string_extend_strv(&a_main, output, llvm_string_const_unwrap(literal)->data);
            return;
        case LLVM_NUMBER:
            string_extend_int64_t(&a_main, output, llvm_number_const_unwrap(literal)->data);
            return;
        case LLVM_ENUM_LIT:
            string_extend_int64_t(&a_main, output, llvm_enum_lit_const_unwrap(literal)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_CHAR:
            string_extend_int64_t(&a_main, output, (int64_t)llvm_char_const_unwrap(literal)->data);
            return;
    }
    unreachable("");
}

static void tast_extend_literal(String* output, const Tast_literal* literal) {
    switch (literal->type) {
        case TAST_STRING:
            string_extend_strv(&a_main, output, tast_string_const_unwrap(literal)->data);
            return;
        case TAST_NUMBER:
            string_extend_int64_t(&a_main, output, tast_number_const_unwrap(literal)->data);
            return;
        case TAST_ENUM_LIT:
            string_extend_int64_t(&a_main, output, tast_enum_lit_const_unwrap(literal)->data);
            return;
        case TAST_VOID:
            return;
        case TAST_CHAR:
            string_extend_int64_t(&a_main, output, (int64_t)tast_char_const_unwrap(literal)->data);
            return;
        case TAST_SUM_LIT:
            unreachable("");
        case TAST_UNION_LIT:
            unreachable("");
    }
    unreachable("");
}

// \n excapes are actually stored as is in tokens and llvms, but should be printed as \0a
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
    if (lang_type_get_pointer_depth(lang_type) != 0) {
        string_extend_cstr(&a_main, output, "ptr");
        return;
    }

    switch (lang_type.type) {
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&a_main, output, "%struct.");
            extend_serialize_lang_type_to_string(env, output, lang_type, false);
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&a_main, output, "%union.");
            extend_serialize_lang_type_to_string(env, output, lang_type, false);
            return;
        case LANG_TYPE_ENUM:
            lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0))),
            extend_lang_type_to_string(output, lang_type, false, false);
            return;
        case LANG_TYPE_VOID:
            lang_type = lang_type_void_const_wrap(lang_type_void_new(0));
            extend_lang_type_to_string(output, lang_type, false, false);
            return;
        case LANG_TYPE_PRIMITIVE:
            if (lang_type_atom_is_unsigned(lang_type_get_atom(lang_type))) {
                lang_type_set_atom(&lang_type, lang_type_atom_unsigned_to_signed(lang_type_get_atom(lang_type)));
            }
            extend_lang_type_to_string(output, lang_type, false, false);
            return;
        case LANG_TYPE_SUM:
            string_extend_cstr(&a_main, output, "%struct.");
            extend_serialize_lang_type_to_string(env, output, lang_type, false);
            return;
    }
    unreachable("");
}

static bool llvm_is_variadic(const Llvm* llvm) {
    if (llvm->type == LLVM_EXPR) {
        switch (llvm_expr_const_unwrap(llvm)->type) {
            case LLVM_LITERAL:
                return false;
            default:
                unreachable(LLVM_FMT, llvm_print(llvm));
        }
    } else {
        const Llvm_def* def = llvm_def_const_unwrap(llvm);
        switch (def->type) {
            case LLVM_VARIABLE_DEF:
                return llvm_variable_def_const_unwrap(llvm_def_const_unwrap(llvm))->is_variadic;
            default:
                unreachable(LLVM_FMT, llvm_print(llvm));
        }
    }
}

static bool tast_is_variadic(const Tast_stmt* tast) {
    if (tast->type == TAST_EXPR) {
        switch (tast_expr_const_unwrap(tast)->type) {
            case TAST_LITERAL:
                return false;
            default:
                unreachable(TAST_FMT, tast_stmt_print(tast));
        }
    } else {
        const Tast_def* def = tast_def_const_unwrap(tast);
        switch (def->type) {
            case TAST_VARIABLE_DEF:
                return tast_variable_def_const_unwrap(tast_def_const_unwrap(tast))->is_variadic;
            default:
                unreachable(TAST_FMT, tast_stmt_print(tast));
        }
    }
}

static void tast_extend_type_decl_str(const Env* env, String* output, const Tast_stmt* var_def_or_lit, bool noundef) {
    if (tast_is_variadic(var_def_or_lit)) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    extend_type_call_str(env, output, tast_stmt_get_lang_type(var_def_or_lit));
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void llvm_extend_type_decl_str(const Env* env, String* output, const Llvm* var_def_or_lit, bool noundef) {
    if (llvm_is_variadic(var_def_or_lit)) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    extend_type_call_str(env, output, llvm_get_lang_type(var_def_or_lit));
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void extend_literal_decl_prefix(String* output, String* literals, const Llvm_literal* literal) {
    log(LOG_DEBUG, "entering thing\n");
    if (str_view_cstr_is_equal(lang_type_get_str(llvm_literal_get_lang_type(literal)), "u8")) {
        assert(llvm_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE);
        if (lang_type_get_pointer_depth(llvm_literal_get_lang_type(literal)) != 1) {
            todo();
        }
        string_extend_cstr(&a_main, output, " @.");
        string_extend_strv(&a_main, output, llvm_literal_get_name(literal));
    } else if (lang_type_atom_is_signed(lang_type_get_atom(llvm_literal_get_lang_type(literal)))) {
        assert(llvm_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE);
        if (lang_type_get_pointer_depth(llvm_literal_get_lang_type(literal)) != 0) {
            todo();
        }
        vec_append(&a_main, output, ' ');
        extend_literal(output, literal);
    } else if (llvm_literal_get_lang_type(literal).type == LANG_TYPE_ENUM) {
        vec_append(&a_main, output, ' ');
        extend_literal(output, literal);
    } else {
        unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(llvm_literal_const_wrap(literal))));
    }

    if (literal->type == LLVM_STRING) {
        emit_symbol_normal(literals, llvm_literal_get_name(literal), literal);
    }
}

static void tast_extend_literal_decl_prefix(String* output, const Tast_literal* literal) {
    log(LOG_DEBUG, "entering thing\n");
    assert(lang_type_get_str(tast_literal_get_lang_type(literal)).count > 0);
    if (str_view_cstr_is_equal(lang_type_get_str(tast_literal_get_lang_type(literal)), "u8")) {
        if (lang_type_get_pointer_depth(tast_literal_get_lang_type(literal)) != 1) {
            todo();
        }
        string_extend_cstr(&a_main, output, " @.");
        string_extend_strv(&a_main, output, tast_literal_get_name(literal));
    } else if (lang_type_atom_is_signed(lang_type_get_atom(tast_literal_get_lang_type(literal)))) {
        assert(tast_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE);
        if (lang_type_get_pointer_depth(tast_literal_get_lang_type(literal)) != 0) {
            todo();
        }
        vec_append(&a_main, output, ' ');
        tast_extend_literal(output, literal);
    } else if (tast_literal_get_lang_type(literal).type == LANG_TYPE_ENUM) {
        vec_append(&a_main, output, ' ');
        tast_extend_literal(output, literal);
    } else {
        unreachable(LLVM_FMT"\n", tast_stmt_print(tast_expr_const_wrap(tast_literal_const_wrap(literal))));
    }
}

static void extend_literal_decl(const Env* env, String* output, String* literals, const Llvm_literal* literal, bool noundef) {
    llvm_extend_type_decl_str(env, output, llvm_expr_const_wrap(llvm_literal_const_wrap(literal)), noundef);
    extend_literal_decl_prefix(output, literals, literal);
}

static void tast_extend_literal_decl(const Env* env, String* output, const Tast_literal* literal, bool noundef) {
    tast_extend_type_decl_str(env, output, tast_expr_const_wrap(tast_literal_const_wrap(literal)), noundef);
    tast_extend_literal_decl_prefix(output, literal);
}

static const Llvm_lang_type* return_type_from_function_def(const Llvm_function_def* fun_def) {
    const Llvm_lang_type* return_type = fun_def->decl->return_type;
    if (return_type) {
        return return_type;
    }
    unreachable("");
}

static void emit_function_params(const Env* env, String* output, const Llvm_function_params* fun_params) {
    for (size_t idx = 0; idx < fun_params->params.info.count; idx++) {
        const Llvm_variable_def* curr_param = vec_at(&fun_params->params, idx);

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        if (is_struct_like(curr_param->lang_type.type)) {
            if (lang_type_get_pointer_depth(curr_param->lang_type) < 0) {
                unreachable("");
            } else if (lang_type_get_pointer_depth(curr_param->lang_type) > 0) {
                extend_type_call_str(env, output, curr_param->lang_type);
            } else {
                string_extend_cstr(&a_main, output, "ptr noundef byval(");
                extend_type_call_str(env, output, curr_param->lang_type);
                string_extend_cstr(&a_main, output, ")");
            }
        } else {
            llvm_extend_type_decl_str(env, output, llvm_def_const_wrap(llvm_variable_def_const_wrap(curr_param)), true);
        }

        if (curr_param->is_variadic) {
            return;
        }
        string_extend_cstr(&a_main, output, " %");
        string_extend_size_t(&a_main, output, curr_param->llvm_id);
    }
}

static void emit_function_call_arg_load_another_llvm(
    const Env* env,
    String* output,
    String* literals,
    const Llvm_load_another_llvm* load
) {
    Llvm_id llvm_id = 0;
    log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(load->lang_type));

    Llvm* src = NULL;
    try(alloca_lookup(&src, env, load->name));

    if (llvm_is_literal(src)) {
        extend_literal_decl(env, output, literals, llvm_literal_const_unwrap(llvm_expr_const_unwrap(src)), true);
    } else {
        if (is_struct_like(load->lang_type.type)) {
            log(LOG_DEBUG, STR_VIEW_FMT, llvm_load_another_llvm_print(load));
            llvm_id = llvm_id_from_get_name(env, get_storage_location(env, load->llvm_src));
            assert(llvm_id > 0);

            string_extend_cstr(&a_main, output, "ptr noundef byval(");
            extend_type_call_str(env, output, load->lang_type);
            string_extend_cstr(&a_main, output, ")");
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_id);
        } else {
            llvm_id = load->llvm_id;
            extend_type_call_str(env, output, load->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_id);
        }
    }
}

static void emit_function_arg_expr(const Env* env, String* output, String* literals, const Llvm_expr* argument) {
        switch (argument->type) {
            case LLVM_LITERAL:
                extend_literal_decl(env, output, literals, llvm_literal_const_unwrap(argument), true);
                break;
            case LLVM_SYMBOL:
                unreachable("typed symbols should not still be present");
            case LLVM_LLVM_PLACEHOLDER: {
                unreachable("");
                //const Llvm_llvm_placeholder* placeholder = llvm_llvm_placeholder_const_unwrap(argument);
                //emit_function_call_arg_load_another_llvm(env, output, llvm_load_another_llvm_const_unwrap(placeholder->));
                break;
            }
            case LLVM_FUNCTION_CALL:
                extend_type_call_str(env, output, llvm_expr_get_lang_type(argument));
                string_extend_cstr(&a_main, output, "%");
                string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(argument));
                break;
            case LLVM_OPERATOR:
                extend_type_call_str(env, output, llvm_expr_get_lang_type(argument));
                string_extend_cstr(&a_main, output, "%");
                string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(argument));
                break;
            default:
                unreachable("");
        }
}

static void emit_function_call_arguments(const Env* env, String* output, String* literals, const Llvm_function_call* fun_call) {
    log(LOG_DEBUG, LLVM_FMT, llvm_function_call_print(fun_call));
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_name = vec_at(&fun_call->args, idx);
        Llvm* argument = NULL;
        try(alloca_lookup(&argument, env, arg_name));

        if (idx > 0) {
            string_extend_cstr(&a_main, output, ", ");
        }

        switch (argument->type) {
            case LLVM_EXPR:
                emit_function_arg_expr(env, output, literals, llvm_expr_const_unwrap(argument));
                break;
            case LLVM_LOAD_ANOTHER_LLVM:
                emit_function_call_arg_load_another_llvm(env, output, literals, llvm_load_another_llvm_const_unwrap(argument));
                break;
            case LLVM_ALLOCA:
                string_extend_cstr(&a_main, output, "ptr %");
                string_extend_size_t(&a_main, output, llvm_get_llvm_id(argument));
                break;
            default:
                unreachable(TAST_FMT"\n", llvm_print(argument));
        }
    }
}

static void emit_function_call(const Env* env, String* output, String* literals, const Llvm_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, output, "    ");
    log(LOG_DEBUG, LANG_TYPE_FMT"\n", lang_type_print(fun_call->lang_type));
    log(LOG_DEBUG, LANG_TYPE_FMT"\n", str_view_print(lang_type_get_str(fun_call->lang_type)));
    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
        string_extend_cstr(&a_main, output, "%");
        string_extend_size_t(&a_main, output, fun_call->llvm_id);
        string_extend_cstr(&a_main, output, " = ");
    } else {
        assert(!str_view_cstr_is_equal(lang_type_get_str(fun_call->lang_type), "void"));
    }
    string_extend_cstr(&a_main, output, "call ");
    extend_type_call_str(env, output, fun_call->lang_type);
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_call->name_fun_to_call);

    // arguments
    string_extend_cstr(&a_main, output, "(");
    emit_function_call_arguments(env, output, literals, fun_call);
    string_extend_cstr(&a_main, output, ")");

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_alloca(const Env* env, String* output, const Llvm_alloca* alloca) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, alloca->llvm_id);
    string_extend_cstr(&a_main, output, " = alloca ");
    extend_type_call_str(env, output, alloca->lang_type);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_unary_type(const Env* env, String* output, const Llvm_unary* unary) {
    switch (unary->token_type) {
        case TOKEN_UNSAFE_CAST:
            if (lang_type_get_pointer_depth(unary->lang_type) > 0 && lang_type_is_number(lang_type_from_get_name(env, unary->child))) {
                string_extend_cstr(&a_main, output, "inttoptr ");
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_number(unary->lang_type) && lang_type_get_pointer_depth(lang_type_from_get_name(env, unary->child)) > 0) {
                string_extend_cstr(&a_main, output, "ptrtoint ");
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_number(unary->lang_type) && lang_type_is_number(lang_type_from_get_name(env, unary->child))) {
                if (i_lang_type_atom_to_bit_width(lang_type_get_atom(unary->lang_type)) > i_lang_type_atom_to_bit_width(lang_type_get_atom(lang_type_from_get_name(env, unary->child)))) {
                    string_extend_cstr(&a_main, output, "zext ");
                } else {
                    string_extend_cstr(&a_main, output, "trunc ");
                }
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else {
                log(LOG_DEBUG, TAST_FMT, llvm_unary_print(unary));
                todo();
            }
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_unary_print(unary));
    }
}

static void emit_binary_type(const Env* env, String* output, const Llvm_binary* binary) {
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
        case TOKEN_LESS_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp sle ");
            break;
        case TOKEN_GREATER_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp sge ");
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

static void emit_unary_suffix(const Env* env, String* output, const Llvm_unary* unary) {
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
    const Llvm_expr* expr
) {
    switch (expr->type) {
        case LLVM_LITERAL:
            extend_literal(output, llvm_literal_const_unwrap(expr));
            break;
        case LLVM_OPERATOR:
            // fallthrough
        case LLVM_FUNCTION_CALL:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = llvm_get_llvm_id_expr(expr);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(expr)));
    }
}

static void emit_operator_operand_llvm_placeholder(
    String* output,
    const Llvm_llvm_placeholder* placeholder
) {
    const Llvm* llvm = placeholder->llvm_reg.llvm;

    switch (llvm->type) {
        case LLVM_EXPR:
            emit_operator_operand_llvm_placeholder_expr(output, llvm_expr_const_unwrap(llvm));
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = llvm_get_llvm_id(llvm);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(placeholder->llvm_reg.llvm));
    }
}

static void emit_operator_operand_expr(String* output, const Llvm_expr* operand) {
    switch (operand->type) {
        case LLVM_LITERAL:
            extend_literal(output, llvm_literal_const_unwrap(operand));
            break;
        case LLVM_SYMBOL:
            unreachable("");
        case LLVM_LLVM_PLACEHOLDER:
            emit_operator_operand_llvm_placeholder(
                output, llvm_llvm_placeholder_const_unwrap(operand)
            );
            break;
        case LLVM_FUNCTION_CALL:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, llvm_function_call_const_unwrap(operand)->llvm_id);
            break;
        case LLVM_OPERATOR:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(operand));
            break;
        default:
            unreachable(LLVM_FMT, llvm_expr_print(operand));
    }
}

static void emit_operator_operand(const Env* env, String* output, const Str_view operand_name) {
    Llvm* operand = NULL;
    log(LOG_DEBUG, STR_VIEW_FMT"\n", str_view_print(operand_name));
    try(alloca_lookup(&operand, env, operand_name));

    switch (operand->type) {
        case LLVM_EXPR:
            emit_operator_operand_expr(output, llvm_expr_const_unwrap(operand));
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, llvm_load_another_llvm_const_unwrap(operand)->llvm_id);
            break;
        default:
            unreachable(LLVM_FMT, llvm_print(operand));
    }
}

static void emit_operator(const Env* env, String* output, const Llvm_operator* operator) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(llvm_operator_const_wrap(operator)));
    string_extend_cstr(&a_main, output, " = ");

    if (operator->type == LLVM_UNARY) {
        emit_unary_type(env, output, llvm_unary_const_unwrap(operator));
    } else if (operator->type == LLVM_BINARY) {
        emit_binary_type(env, output, llvm_binary_const_unwrap(operator));
    } else {
        unreachable("");
    }

    if (operator->type == LLVM_UNARY) {
        emit_operator_operand(env, output, llvm_unary_const_unwrap(operator)->child);
    } else if (operator->type == LLVM_BINARY) {
        const Llvm_binary* binary = llvm_binary_const_unwrap(operator);
        emit_operator_operand(env, output, binary->lhs);
        string_extend_cstr(&a_main, output, ", ");
        emit_operator_operand(env, output, binary->rhs);
    } else {
        unreachable("");
    }

    if (operator->type == LLVM_UNARY) {
        emit_unary_suffix(env, output, llvm_unary_const_unwrap(operator));
    } else if (operator->type == LLVM_BINARY) {
    } else {
        unreachable("");
    }

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_load_another_llvm(const Env* env, String* output, const Llvm_load_another_llvm* load_llvm) {
    Llvm_id llvm_id = llvm_id_from_get_name(env, load_llvm->llvm_src);
    assert(llvm_id > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, load_llvm->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
    log(LOG_DEBUG, LLVM_FMT, llvm_load_another_llvm_print(load_llvm));
    extend_type_call_str(env, output, load_llvm->lang_type);
    string_extend_cstr(&a_main, output, ", ");
    string_extend_cstr(&a_main, output, "ptr");
    string_extend_cstr(&a_main, output, " %");
    string_extend_size_t(&a_main, output, llvm_id);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_store_another_llvm_src_literal(
    String* output,
    const Llvm_literal* literal
) {
    string_extend_cstr(&a_main, output, " ");

    switch (literal->type) {
        case LLVM_STRING:
            string_extend_cstr(&a_main, output, " @.");
            string_extend_strv(&a_main, output, llvm_string_const_unwrap(literal)->name);
            return;
        case LLVM_NUMBER:
            string_extend_int64_t(&a_main, output, llvm_number_const_unwrap(literal)->data);
            return;
        case LLVM_ENUM_LIT:
            string_extend_int64_t(&a_main, output, llvm_enum_lit_const_unwrap(literal)->data);
            return;
        case LLVM_VOID:
            return;
        case LLVM_CHAR:
            string_extend_int64_t(&a_main, output, llvm_char_const_unwrap(literal)->data);
    }
    unreachable("");
}

static void emit_store_another_llvm_src_expr(const Env* env, String* output, String* literals, const Llvm_expr* expr) {
    (void) env;

    switch (expr->type) {
        case LLVM_LITERAL: {
            const Llvm_literal* lit = llvm_literal_const_unwrap(expr);
            if (lit->type == LLVM_STRING) {
                emit_symbol_normal(literals, llvm_literal_get_name(lit), lit);
            }
            emit_store_another_llvm_src_literal(output, lit);
            return;
        }
        case LLVM_FUNCTION_CALL:
            // fallthrough
        case LLVM_OPERATOR:
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(expr));
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(expr)));
    }
}

static void emit_store_another_llvm(const Env* env, String* output, String* literals, const Llvm_store_another_llvm* store) {
    Llvm* src = NULL;
    try(alloca_lookup(&src, env, store->llvm_src));

    string_extend_cstr(&a_main, output, "    store ");
    extend_type_call_str(env, output, store->lang_type);
    string_extend_cstr(&a_main, output, " ");

    switch (src->type) {
        case LLVM_DEF: {
            const Llvm_def* src_def = llvm_def_const_unwrap(src);
            const Llvm_variable_def* src_var_def = llvm_variable_def_const_unwrap(src_def);
            (void) src_var_def;
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id(src));
            break;
        }
        case LLVM_EXPR:
            emit_store_another_llvm_src_expr(env, output, literals, llvm_expr_const_unwrap(src));
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            string_extend_cstr(&a_main, output, "%");
            Llvm_id llvm_id = llvm_get_llvm_id(src);
            assert(llvm_id > 0);
            string_extend_size_t(&a_main, output, llvm_id);
            break;
        case LLVM_ALLOCA:
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id(src));
            break;
        default:
            unreachable(LLVM_FMT"\n", llvm_print(src));
    }
    //string_extend_cstr(&a_main, output, " %");
    //string_extend_size_t(&a_main, output, llvm_get_llvm_id(store->llvm_src.llvm));

    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, llvm_id_from_get_name(env, store->llvm_dest));
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_function_def(Env* env, String* struct_defs, String* output, String* literals, const Llvm_function_def* fun_def) {
    string_extend_cstr(&a_main, output, "define dso_local ");

    extend_type_call_str(env, output, return_type_from_function_def(fun_def)->lang_type);

    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, llvm_tast_get_name(llvm_def_const_wrap(llvm_function_def_const_wrap(fun_def))));

    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_def->decl->params);
    vec_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");
    emit_block(env, struct_defs, output, literals, fun_def->body);
    string_extend_cstr(&a_main, output, "}\n");
}

static void emit_return_expr(const Env* env, String* output, const Llvm_expr* child) {
    switch (child->type) {
        case LLVM_LITERAL: {
            const Llvm_literal* literal = llvm_literal_const_unwrap(child);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, llvm_literal_get_lang_type(literal));
            string_extend_cstr(&a_main, output, " ");
            extend_literal(output, literal);
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        case LLVM_OPERATOR: {
            const Llvm_operator* operator = llvm_operator_const_unwrap(child);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, llvm_operator_get_lang_type(operator));
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(child));
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        case LLVM_SYMBOL:
            unreachable("");
        case LLVM_LLVM_PLACEHOLDER: {
            const Llvm_llvm_placeholder* memb_sym = llvm_llvm_placeholder_const_unwrap(child);
            if (llvm_is_literal(memb_sym->llvm_reg.llvm)) {
                const Llvm_expr* memb_expr = llvm_expr_const_unwrap(memb_sym->llvm_reg.llvm);
                const Llvm_literal* literal = llvm_literal_const_unwrap(memb_expr);
                string_extend_cstr(&a_main, output, "    ret ");
                extend_type_call_str(env, output, llvm_literal_get_lang_type(literal));
                string_extend_cstr(&a_main, output, " ");
                extend_literal(output, literal);
                string_extend_cstr(&a_main, output, "\n");
                break;
            } else {
                string_extend_cstr(&a_main, output, "    ret ");
                extend_type_call_str(env, output, memb_sym->lang_type);
                string_extend_cstr(&a_main, output, " %");
                string_extend_size_t(&a_main, output, llvm_get_llvm_id(memb_sym->llvm_reg.llvm));
                string_extend_cstr(&a_main, output, "\n");
            }
            break;
        }
        case LLVM_FUNCTION_CALL: {
            const Llvm_function_call* function_call = llvm_function_call_const_unwrap(child);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, function_call->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, llvm_get_llvm_id_expr(child));
            string_extend_cstr(&a_main, output, "\n");
            break;
        }
        default:
            unreachable(TAST_FMT"\n", llvm_expr_print(child));
    }
}

static void emit_return(const Env* env, String* output, const Llvm_return* fun_return) {
    Llvm* sym_to_return = NULL;
    try(alloca_lookup(&sym_to_return, env, fun_return->child));

    switch (sym_to_return->type) {
        case LLVM_EXPR:
            emit_return_expr(env, output, llvm_expr_const_unwrap(sym_to_return));
            return;
        case LLVM_LOAD_ANOTHER_LLVM: {
            const Llvm_load_another_llvm* load = llvm_load_another_llvm_const_unwrap(sym_to_return);
            string_extend_cstr(&a_main, output, "    ret ");
            extend_type_call_str(env, output, load->lang_type);
            string_extend_cstr(&a_main, output, " %");
            string_extend_size_t(&a_main, output, load->llvm_id);
            string_extend_cstr(&a_main, output, "\n");
            return;
        }
        default:
            unreachable(TAST_FMT"\n", llvm_print(sym_to_return));
    }
}

static void emit_function_decl(const Env* env, String* output, const Llvm_function_decl* fun_decl) {
    string_extend_cstr(&a_main, output, "declare i32");
    //extend_literal_decl(output, fun_decl); // TODO
    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, fun_decl->name);
    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_decl->params);
    vec_append(&a_main, output, ')');
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_label(String* output, const Llvm_label* label) {
    string_extend_cstr(&a_main, output, "\n");
    string_extend_size_t(&a_main, output, label->llvm_id);
    string_extend_cstr(&a_main, output, ":\n");
}

static void emit_goto(const Env* env, String* output, const Llvm_goto* lang_goto) {
    string_extend_cstr(&a_main, output, "    br label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, lang_goto->name));
    vec_append(&a_main, output, '\n');
}

static void emit_cond_goto(const Env* env, String* output, const Llvm_cond_goto* cond_goto) {
    string_extend_cstr(&a_main, output, "    br i1 %");
    string_extend_size_t(&a_main, output, llvm_id_from_get_name(env, cond_goto->condition));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_true));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_false));
    vec_append(&a_main, output, '\n');
}

static void emit_struct_def_base(const Env* env, String* output, const Struct_def_base* base, bool largest_only) {
    string_extend_strv(&a_main, output, base->name);
    string_extend_cstr(&a_main, output, " = type { ");
    bool is_first = true;

    if (largest_only) {
        size_t idx = struct_def_base_get_idx_largest_member(env, *base);
        tast_extend_type_decl_str(env, output, tast_def_wrap(tast_variable_def_wrap(vec_at(&base->members, idx))), false);
    } else {
        for (size_t idx = 0; idx < base->members.info.count; idx++) {
            if (!is_first) {
                string_extend_cstr(&a_main, output, ", ");
            }
            tast_extend_type_decl_str(env, output, tast_def_wrap(tast_variable_def_wrap(vec_at(&base->members, idx))), false);
            is_first = false;
        }
    }

    string_extend_cstr(&a_main, output, " }\n");
}

static void emit_struct_def(const Env* env, String* output, const Llvm_struct_def* struct_def) {
    string_extend_cstr(&a_main, output, "%struct.");
    emit_struct_def_base(env, output, &struct_def->base, false);
}

static void emit_raw_union_def(const Env* env, String* output, const Llvm_raw_union_def* raw_union_def) {
    string_extend_cstr(&a_main, output, "%union.");
    emit_struct_def_base(env, output, &raw_union_def->base, true);
}

static void emit_sum_def(const Env* env, String* output, const Llvm_sum_def* sum_def) {
    (void) env;
    (void) output;
    (void) sum_def;
    unreachable("");
}

static void emit_load_struct_element_pointer(const Env* env, String* output, const Llvm_load_element_ptr* load_elem_ptr) {
    string_extend_cstr(&a_main, output, "    %"); 
    string_extend_size_t(&a_main, output, load_elem_ptr->llvm_id);

    string_extend_cstr(&a_main, output, " = getelementptr inbounds ");

    Lang_type lang_type = lang_type_from_get_name(env, load_elem_ptr->llvm_src);
    lang_type_set_pointer_depth(&lang_type, 0);
    extend_type_call_str(env, output, lang_type);
    string_extend_cstr(&a_main, output, ", ptr %");
    string_extend_size_t(&a_main, output, llvm_id_from_get_name(env, load_elem_ptr->llvm_src));
    if (load_elem_ptr->is_from_struct) {
        string_extend_cstr(&a_main, output, ", i32 0");
    }
    string_extend_cstr(&a_main, output, ", ");
    if (load_elem_ptr->is_from_struct) {
        string_extend_cstr(&a_main, output, "i32");
    } else {
        extend_type_call_str(env, output, lang_type_from_get_name(env, load_elem_ptr->struct_index));
    }
    string_extend_cstr(&a_main, output, " ");

    Llvm* struct_index = NULL;
    try(alloca_lookup(&struct_index, env, load_elem_ptr->struct_index));
    if (struct_index->type == LLVM_LOAD_ANOTHER_LLVM) {
        string_extend_cstr(&a_main, output, "%");
        string_extend_size_t(&a_main, output, llvm_get_llvm_id(struct_index));
    } else {
        emit_operator_operand(env, output, load_elem_ptr->struct_index);
    }

    vec_append(&a_main, output, '\n');
}

static void emit_expr(Env* env, String* output, String* literals, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            emit_operator(env, output, llvm_operator_const_unwrap(expr));
            break;
        case LLVM_FUNCTION_CALL:
            emit_function_call(env, output, literals, llvm_function_call_const_unwrap(expr));
            break;
        case LLVM_LITERAL:
            extend_literal_decl(env, output, literals, llvm_literal_const_unwrap(expr), true);
            break;
        default:
            unreachable("");
    }
}

static void emit_def(Env* env, String* struct_defs, String* output, String* literals, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            emit_function_def(env, struct_defs, output, literals, llvm_function_def_const_unwrap(def));
            return;
        case LLVM_VARIABLE_DEF:
            return;
        case LLVM_FUNCTION_DECL:
            emit_function_decl(env, output, llvm_function_decl_const_unwrap(def));
            return;
        case LLVM_LABEL:
            emit_label(output, llvm_label_const_unwrap(def));
            return;
        case LLVM_STRUCT_DEF:
            emit_struct_def(env, struct_defs, llvm_struct_def_const_unwrap(def));
            return;
        case LLVM_RAW_UNION_DEF:
            emit_raw_union_def(env, struct_defs, llvm_raw_union_def_const_unwrap(def));
            return;
        case LLVM_SUM_DEF:
            emit_sum_def(env, struct_defs, llvm_sum_def_const_unwrap(def));
            return;
        case LLVM_ENUM_DEF:
            return;
        case LLVM_PRIMITIVE_DEF:
            todo();
        case LLVM_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void emit_block(Env* env, String* struct_defs, String* output, String* literals, const Llvm_block* block) {
    vec_append(&a_main, &env->ancesters, (Symbol_collection*)&block->symbol_collection);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        const Llvm* statement = vec_at(&block->children, idx);

        switch (statement->type) {
            case LLVM_EXPR:
                emit_expr(env, output, literals, llvm_expr_const_unwrap(statement));
                break;
            case LLVM_DEF:
                emit_def(env, struct_defs, output, literals, llvm_def_const_unwrap(statement));
                break;
            case LLVM_RETURN:
                emit_return(env, output, llvm_return_const_unwrap(statement));
                break;
            case LLVM_BLOCK:
                emit_block(env, struct_defs, output, literals, llvm_block_const_unwrap(statement));
                break;
            case LLVM_COND_GOTO:
                emit_cond_goto(env, output, llvm_cond_goto_const_unwrap(statement));
                break;
            case LLVM_GOTO:
                emit_goto(env, output, llvm_goto_const_unwrap(statement));
                break;
            case LLVM_ALLOCA:
                emit_alloca(env, output, llvm_alloca_const_unwrap(statement));
                break;
            case LLVM_LOAD_ELEMENT_PTR:
                emit_load_struct_element_pointer(env, output, llvm_load_element_ptr_const_unwrap(statement));
                break;
            case LLVM_LOAD_ANOTHER_LLVM:
                emit_load_another_llvm(env, output, llvm_load_another_llvm_const_unwrap(statement));
                break;
            case LLVM_STORE_ANOTHER_LLVM:
                emit_store_another_llvm(env, output, literals, llvm_store_another_llvm_const_unwrap(statement));
                break;
            default:
                log(LOG_ERROR, STRING_FMT"\n", string_print(*output));
                llvm_printf(statement);
                todo();
        }
    }
    //get_block_return_id(fun_block) = get_block_return_id(fun_block->left_child);
    vec_rem_last(&env->ancesters);
}

static void emit_symbol_normal(String* literals, Str_view key, const Llvm_literal* lit) {
    Str_view data = {0};
    switch (lit->type) {
        case LLVM_STRING:
            data = llvm_string_const_unwrap(lit)->data;
            break;
        default:
            todo();
    }

    size_t literal_width = data.count + 1 - get_count_excape_seq(data);

    string_extend_cstr(&a_main, literals, "@.");
    string_extend_strv(&a_main, literals, key);
    string_extend_cstr(&a_main, literals, " = private unnamed_addr constant [ ");
    string_extend_size_t(&a_main, literals, literal_width);
    string_extend_cstr(&a_main, literals, " x i8] c\"");
    string_extend_strv_eval_escapes(&a_main, literals, data);
    string_extend_cstr(&a_main, literals, "\\00\", align 1");
    string_extend_cstr(&a_main, literals, "\n");
}

static void emit_symbol(String* output, Str_view key, const Llvm_string_def* def) {
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

static void tast_emit_symbol(String* output, Str_view key, const Tast_string_def* def) {
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

static void tast_emit_struct_literal(const Env* env, String* output, const Tast_struct_lit_def* lit_def) {
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, lit_def->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(output, lit_def->lang_type, false, false);
    string_extend_cstr(&a_main, output, " {");

    size_t is_first = true;
    for (size_t idx = 0; idx < lit_def->members.info.count; idx++) {
        const Tast_expr* memb_literal = vec_at(&lit_def->members, idx);
        if (!is_first) {
            vec_append(&a_main, output, ',');
        }
        tast_extend_literal_decl(env, output, tast_literal_const_unwrap(memb_literal), false);
        is_first = false;
    }

    string_extend_cstr(&a_main, output, "} , align 4\n");
}

static void emit_struct_literal(const Env* env, String* output, String* literals, const Llvm_struct_lit_def* lit_def) {
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, lit_def->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(output, lit_def->lang_type, false, false);
    string_extend_cstr(&a_main, output, " {");

    size_t is_first = true;
    for (size_t idx = 0; idx < lit_def->members.info.count; idx++) {
        const Llvm_expr* memb_literal = vec_at(&lit_def->members, idx);
        if (!is_first) {
            vec_append(&a_main, output, ',');
        }
        extend_literal_decl(env, output, literals, llvm_literal_const_unwrap(memb_literal), false);
        is_first = false;
    }

    string_extend_cstr(&a_main, output, "} , align 4\n");
}

static void emit_symbols(const Env* env, String* output) {
    for (size_t idx = 0; idx < env->global_literals.capacity; idx++) {
        const Symbol_table_tast curr_tast = env->global_literals.table_tasts[idx];
        if (curr_tast.status != SYM_TBL_OCCUPIED) {
            continue;
        }

        const Tast_literal_def* def = tast_literal_def_const_unwrap(curr_tast.tast);

        switch (def->type) {
            case TAST_STRUCT_LIT_DEF:
                tast_emit_struct_literal(env, output, tast_struct_lit_def_const_unwrap(def));
                break;
            case TAST_STRING_DEF:
                tast_emit_symbol(output, curr_tast.key, tast_string_def_const_unwrap(def));
                break;
            default:
                todo();
        }
    }
}

void emit_llvm_from_tree(Env* env, const Llvm_block* root) {
    String struct_defs = {0};
    String output = {0};
    String literals = {0};
    emit_block(env, &struct_defs, &output, &literals, root);
    //emit_symbols(env, &output);
    log(LOG_DEBUG, "\n"STRING_FMT"\n"STRING_FMT"\n", string_print(struct_defs), string_print(output));

    FILE* file = fopen("test.ll", "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_TYPE_NONE, dummy_file_text, dummy_pos, "could not open file %s: errno %d (%s)\n",
            params.input_file_name, errno, strerror(errno)
        );
        exit(EXIT_CODE_FAIL);
    }

    for (size_t idx = 0; idx < struct_defs.info.count; idx++) {
        if (EOF == fputc(vec_at(&struct_defs, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < output.info.count; idx++) {
        if (EOF == fputc(vec_at(&output, idx), file)) {
            todo();
        }
    }

    for (size_t idx = 0; idx < literals.info.count; idx++) {
        if (EOF == fputc(vec_at(&literals, idx), file)) {
            todo();
        }
    }

    fclose(file);
}

