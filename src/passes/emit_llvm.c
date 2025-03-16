
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
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <lang_type_print.h>

static void emit_block(Env* env, String* struct_defs, String* output, String* literals, const Llvm_block* fun_block);

static void emit_sometimes(Env* env, String* struct_defs, String* output, String* literals, const Llvm* llvm);

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
        case LLVM_FUNCTION_NAME:
            string_extend_strv(&a_main, output, llvm_function_name_const_unwrap(literal)->fun_name);
            return;
    }
    unreachable("");
}

// TODO: remove tast_extend* functions
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
        case TAST_RAW_UNION_LIT:
            unreachable("");
        case TAST_FUNCTION_LIT:
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

static void extend_type_call_str(Env* env, String* output, Lang_type lang_type) {
    if (lang_type_get_pointer_depth(lang_type) != 0) {
        string_extend_cstr(&a_main, output, "ptr");
        return;
    }

    switch (lang_type.type) {
        case LANG_TYPE_FN:
            todo();
        case LANG_TYPE_TUPLE:
            unreachable("");
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&a_main, output, "%struct.");
            string_extend_strv(&a_main, output, serialize_lang_type(env, lang_type));
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&a_main, output, "%union.");
            string_extend_strv(&a_main, output, serialize_lang_type(env, lang_type));
            return;
        case LANG_TYPE_ENUM:
            lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(64, 0))),
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lang_type);
            return;
        case LANG_TYPE_VOID:
            lang_type = lang_type_void_const_wrap(lang_type_void_new(0));
            string_extend_strv(&a_main, output, serialize_lang_type(env, lang_type));
            return;
        case LANG_TYPE_PRIMITIVE:
            log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
            if (lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_UNSIGNED_INT) {
                log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
                log(LOG_DEBUG, "%d\n", lang_type_primitive_const_unwrap(lang_type).type);
                Lang_type_unsigned_int old_num = lang_type_unsigned_int_const_unwrap(lang_type_primitive_const_unwrap(lang_type));
                lang_type = lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(old_num.bit_width, old_num.pointer_depth)));
            }
            log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, lang_type));
            //assert(lang_type_primitive_const_unwrap(lang_type).type == LANG_TYPE_SIGNED_INT);
            extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lang_type);
            return;
        case LANG_TYPE_SUM:
            string_extend_cstr(&a_main, output, "%struct.");
            string_extend_strv(&a_main, output, serialize_lang_type(env, lang_type));
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

static void tast_extend_type_decl_str(Env* env, String* output, const Tast_stmt* var_def_or_lit, bool noundef) {
    if (tast_is_variadic(var_def_or_lit)) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    extend_type_call_str(env, output, tast_stmt_get_lang_type(var_def_or_lit));
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void llvm_extend_type_decl_str(Env* env, String* output, const Llvm* var_def_or_lit, bool noundef) {
    if (llvm_is_variadic(var_def_or_lit)) {
        string_extend_cstr(&a_main, output, "...");
        return;
    }

    log(LOG_DEBUG, TAST_FMT, llvm_print(var_def_or_lit));
    log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, llvm_get_lang_type(var_def_or_lit)));
    extend_type_call_str(env, output, llvm_get_lang_type(var_def_or_lit));
    if (noundef) {
        string_extend_cstr(&a_main, output, " noundef");
    }
}

static void extend_literal_decl_prefix(String* output, String* literals, const Llvm_literal* literal) {
    if (literal->type == LLVM_FUNCTION_NAME) {
        string_extend_cstr(&a_main, output, " @");
        string_extend_strv(&a_main, output, llvm_function_name_const_unwrap(literal)->fun_name);
    } else if (
        llvm_literal_get_lang_type(literal).type == LANG_TYPE_PRIMITIVE &&
        lang_type_primitive_const_unwrap(llvm_literal_get_lang_type(literal)).type == LANG_TYPE_CHAR &&
        lang_type_get_pointer_depth(llvm_literal_get_lang_type(literal)) > 0
    ) {
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
    } else if (lang_type_atom_is_unsigned(lang_type_get_atom(llvm_literal_get_lang_type(literal)))) {
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
        log(LOG_DEBUG, TAST_FMT, lang_type_print(LANG_TYPE_MODE_LOG, llvm_literal_get_lang_type(literal)));
        unreachable(LLVM_FMT"\n", llvm_print(llvm_expr_const_wrap(llvm_literal_const_wrap(literal))));
    }

    if (literal->type == LLVM_STRING) {
        emit_symbol_normal(literals, llvm_literal_get_name(literal), literal);
    }
}

static void tast_extend_literal_decl_prefix(String* output, const Tast_literal* literal) {
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

static void extend_literal_decl(Env* env, String* output, String* literals, const Llvm_literal* literal, bool noundef) {
    llvm_extend_type_decl_str(env, output, llvm_expr_const_wrap(llvm_literal_const_wrap(literal)), noundef);
    extend_literal_decl_prefix(output, literals, literal);
}

static void tast_extend_literal_decl(Env* env, String* output, const Tast_literal* literal, bool noundef) {
    tast_extend_type_decl_str(env, output, tast_expr_const_wrap(tast_literal_const_wrap(literal)), noundef);
    tast_extend_literal_decl_prefix(output, literal);
}

static void emit_function_params(Env* env, String* output, const Llvm_function_params* fun_params) {
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
    Env* env,
    String* output,
    String* literals,
    const Llvm_load_another_llvm* load
) {
    Llvm_id llvm_id = 0;

    Llvm* src = NULL;
    unwrap(alloca_lookup(&src, env, load->name));

    if (llvm_is_literal(src)) {
        extend_literal_decl(env, output, literals, llvm_literal_const_unwrap(llvm_expr_const_unwrap(src)), true);
    } else {
        if (is_struct_like(load->lang_type.type)) {
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

static void emit_function_arg_expr(Env* env, String* output, String* literals, const Llvm_expr* argument) {
        switch (argument->type) {
            case LLVM_LITERAL:
                log(LOG_DEBUG, TAST_FMT, llvm_expr_print(argument));
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

static void emit_function_call_arguments(Env* env, String* output, String* literals, const Llvm_function_call* fun_call) {
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_name = vec_at(&fun_call->args, idx);
        Llvm* argument = NULL;
        unwrap(alloca_lookup(&argument, env, arg_name));

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

static void emit_function_call(Env* env, String* output, String* literals, const Llvm_function_call* fun_call) {
    //assert(fun_call->llvm_id == 0);

    // start of actual function call
    string_extend_cstr(&a_main, output, "    ");
    if (fun_call->lang_type.type != LANG_TYPE_VOID) {
        string_extend_cstr(&a_main, output, "%");
        string_extend_size_t(&a_main, output, fun_call->llvm_id);
        string_extend_cstr(&a_main, output, " = ");
    } else {
        assert(!str_view_cstr_is_equal(lang_type_get_str(fun_call->lang_type), "void"));
    }
    string_extend_cstr(&a_main, output, "call ");
    extend_type_call_str(env, output, fun_call->lang_type);
    Llvm* callee = NULL;
    unwrap(alloca_lookup(&callee, env, fun_call->callee));
    log(LOG_DEBUG, TAST_FMT, llvm_print(callee));

    switch (callee->type) {
        case LLVM_EXPR:
            string_extend_cstr(&a_main, output, " @");
            string_extend_strv(&a_main, output, llvm_function_name_unwrap(llvm_literal_unwrap(llvm_expr_unwrap(((callee)))))->fun_name);
            break;
        case LLVM_LOAD_ANOTHER_LLVM:
            string_extend_cstr(&a_main, output, "%");
            string_extend_size_t(&a_main, output, llvm_load_another_llvm_const_unwrap(callee)->llvm_id);
            break;
        default:
            unreachable("");
    }
    //string_extend_strv(&a_main, output, fun_call->name_fun_to_call);

    // arguments
    string_extend_cstr(&a_main, output, "(");
    emit_function_call_arguments(env, output, literals, fun_call);
    string_extend_cstr(&a_main, output, ")");

    string_extend_cstr(&a_main, output, "\n");
}

static void emit_alloca(Env* env, String* output, const Llvm_alloca* alloca) {
    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, alloca->llvm_id);
    string_extend_cstr(&a_main, output, " = alloca ");
    extend_type_call_str(env, output, alloca->lang_type);
    string_extend_cstr(&a_main, output, ", align 8");
    string_extend_cstr(&a_main, output, "\n");
}

static void emit_unary_type(Env* env, String* output, const Llvm_unary* unary) {
    switch (unary->token_type) {
        case UNARY_UNSAFE_CAST:
            if (lang_type_get_pointer_depth(unary->lang_type) > 0 && lang_type_is_number(lang_type_from_get_name(env, unary->child))) {
                string_extend_cstr(&a_main, output, "inttoptr ");
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_number(unary->lang_type) && lang_type_get_pointer_depth(lang_type_from_get_name(env, unary->child)) > 0) {
                string_extend_cstr(&a_main, output, "ptrtoint ");
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_unsigned(unary->lang_type) && lang_type_is_number(lang_type_from_get_name(env, unary->child))) {
                if (i_lang_type_atom_to_bit_width(lang_type_get_atom(unary->lang_type)) > i_lang_type_atom_to_bit_width(lang_type_get_atom(lang_type_from_get_name(env, unary->child)))) {
                    string_extend_cstr(&a_main, output, "zext ");
                } else {
                    string_extend_cstr(&a_main, output, "trunc ");
                }
                extend_type_call_str(env, output, lang_type_from_get_name(env, unary->child));
                string_extend_cstr(&a_main, output, " ");
            } else if (lang_type_is_signed(unary->lang_type) && lang_type_is_number(lang_type_from_get_name(env, unary->child))) {
                if (i_lang_type_atom_to_bit_width(lang_type_get_atom(unary->lang_type)) > i_lang_type_atom_to_bit_width(lang_type_get_atom(lang_type_from_get_name(env, unary->child)))) {
                    string_extend_cstr(&a_main, output, "sext ");
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

static void emit_binary_type_signed(String* output, const Llvm_binary* binary) {
    switch (binary->token_type) {
        case BINARY_SINGLE_EQUAL:
            unreachable("= should not still be a binary expr at this point");
            return;
        case BINARY_SUB:
            string_extend_cstr(&a_main, output, "sub nsw ");
            return;
        case BINARY_ADD:
            string_extend_cstr(&a_main, output, "add nsw ");
            return;
        case BINARY_MULTIPLY:
            string_extend_cstr(&a_main, output, "mul nsw ");
            return;
        case BINARY_DIVIDE:
            string_extend_cstr(&a_main, output, "sdiv ");
            return;
        case BINARY_MODULO:
            string_extend_cstr(&a_main, output, "srem ");
            return;
        case BINARY_LESS_THAN:
            string_extend_cstr(&a_main, output, "icmp slt ");
            return;
        case BINARY_LESS_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp sle ");
            return;
        case BINARY_GREATER_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp sge ");
            return;
        case BINARY_GREATER_THAN:
            string_extend_cstr(&a_main, output, "icmp sgt ");
            return;
        case BINARY_DOUBLE_EQUAL:
            string_extend_cstr(&a_main, output, "icmp eq ");
            return;
        case BINARY_NOT_EQUAL:
            string_extend_cstr(&a_main, output, "icmp ne ");
            return;
        case BINARY_BITWISE_XOR:
            string_extend_cstr(&a_main, output, "xor ");
            return;
        case BINARY_BITWISE_AND:
            string_extend_cstr(&a_main, output, "and ");
            return;
        case BINARY_BITWISE_OR:
            string_extend_cstr(&a_main, output, "or ");
            return;
        case BINARY_SHIFT_LEFT:
            unreachable("");
        case BINARY_SHIFT_RIGHT:
            unreachable("");
        case BINARY_LOGICAL_AND:
            // fallthrough
        case BINARY_LOGICAL_OR:
            unreachable("logical operators should not make it this far");
    }
    unreachable("");
}

static void emit_binary_type_unsigned(String* output, const Llvm_binary* binary) {
    switch (binary->token_type) {
        case BINARY_SINGLE_EQUAL:
            unreachable("= should not still be a binary expr at this point");
            return;
        case BINARY_SUB:
            string_extend_cstr(&a_main, output, "sub nsw ");
            return;
        case BINARY_ADD:
            string_extend_cstr(&a_main, output, "add nsw ");
            return;
        case BINARY_MULTIPLY:
            string_extend_cstr(&a_main, output, "mul nsw ");
            return;
        case BINARY_DIVIDE:
            string_extend_cstr(&a_main, output, "udiv ");
            return;
        case BINARY_MODULO:
            string_extend_cstr(&a_main, output, "urem ");
            return;
        case BINARY_LESS_THAN:
            string_extend_cstr(&a_main, output, "icmp ult ");
            return;
        case BINARY_LESS_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp ule ");
            return;
        case BINARY_GREATER_OR_EQUAL:
            string_extend_cstr(&a_main, output, "icmp uge ");
            return;
        case BINARY_GREATER_THAN:
            string_extend_cstr(&a_main, output, "icmp ugt ");
            return;
        case BINARY_DOUBLE_EQUAL:
            string_extend_cstr(&a_main, output, "icmp eq ");
            return;
        case BINARY_NOT_EQUAL:
            string_extend_cstr(&a_main, output, "icmp ne ");
            return;
        case BINARY_BITWISE_XOR:
            string_extend_cstr(&a_main, output, "xor ");
            return;
        case BINARY_BITWISE_AND:
            string_extend_cstr(&a_main, output, "and ");
            return;
        case BINARY_BITWISE_OR:
            string_extend_cstr(&a_main, output, "or ");
            return;
        case BINARY_SHIFT_LEFT:
            string_extend_cstr(&a_main, output, "shl ");
            return;
        case BINARY_SHIFT_RIGHT:
            string_extend_cstr(&a_main, output, "lshr ");
            return;
        case BINARY_LOGICAL_AND:
            // fallthrough
        case BINARY_LOGICAL_OR:
            unreachable("logical operators should not make it this far");
    }
    unreachable("");
}

static void emit_binary_type(Env* env, String* output, const Llvm_binary* binary) {
    if (lang_type_is_signed(lang_type_from_get_name(env, binary->lhs))) {
        emit_binary_type_signed(output, binary);
    } else {
        emit_binary_type_unsigned(output, binary);
    }

    extend_type_call_str(env, output, lang_type_from_get_name(env, binary->lhs));
    string_extend_cstr(&a_main, output, " ");
}

static void emit_unary_suffix(Env* env, String* output, const Llvm_unary* unary) {
    switch (unary->token_type) {
        case UNARY_UNSAFE_CAST:
            string_extend_cstr(&a_main, output, " to ");
            extend_type_call_str(env, output, unary->lang_type);
            return;
        case UNARY_DEREF:
            // TODO: return here to simplify things?
            unreachable("suffix not needed for UNARY_DEREF");
        case UNARY_REFER:
            unreachable("suffix not needed for UNARY_REFER");
        case UNARY_NOT:
            unreachable("not should not still be present here");
    }
    unreachable("");
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

static void emit_operator_operand(Env* env, String* output, const Str_view operand_name) {
    Llvm* operand = NULL;
    unwrap(alloca_lookup(&operand, env, operand_name));

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

static void emit_operator(Env* env, String* output, const Llvm_operator* operator) {
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

static void emit_load_another_llvm(Env* env, String* output, const Llvm_load_another_llvm* load_llvm) {
    Llvm_id llvm_id = llvm_id_from_get_name(env, load_llvm->llvm_src);
    assert(llvm_id > 0);

    string_extend_cstr(&a_main, output, "    %");
    string_extend_size_t(&a_main, output, load_llvm->llvm_id);
    string_extend_cstr(&a_main, output, " = load ");
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
            return;
        case LLVM_FUNCTION_NAME:
            unreachable("");
    }
    unreachable("");
}

static void emit_store_another_llvm_src_expr(Env* env, String* output, String* literals, const Llvm_expr* expr) {
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

static void emit_store_another_llvm(Env* env, String* output, String* literals, const Llvm_store_another_llvm* store) {
    Llvm* src = NULL;
    unwrap(alloca_lookup(&src, env, store->llvm_src));

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

    extend_type_call_str(env, output, fun_def->decl->return_type);

    string_extend_cstr(&a_main, output, " @");
    string_extend_strv(&a_main, output, llvm_tast_get_name(llvm_def_const_wrap(llvm_function_def_const_wrap(fun_def))));

    vec_append(&a_main, output, '(');
    emit_function_params(env, output, fun_def->decl->params);
    vec_append(&a_main, output, ')');

    string_extend_cstr(&a_main, output, " {\n");
    emit_block(env, struct_defs, output, literals, fun_def->body);
    string_extend_cstr(&a_main, output, "}\n");
}

static void emit_return_expr(Env* env, String* output, const Llvm_expr* child) {
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

static void emit_return(Env* env, String* output, const Llvm_return* fun_return) {
    Llvm* sym_to_return = NULL;
    unwrap(alloca_lookup(&sym_to_return, env, fun_return->child));

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

static void emit_function_decl(Env* env, String* output, const Llvm_function_decl* fun_decl) {
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

static void emit_goto(Env* env, String* output, const Llvm_goto* lang_goto) {
    string_extend_cstr(&a_main, output, "    br label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, lang_goto->name));
    vec_append(&a_main, output, '\n');
}

static void emit_cond_goto(Env* env, String* output, const Llvm_cond_goto* cond_goto) {
    string_extend_cstr(&a_main, output, "    br i1 %");
    string_extend_size_t(&a_main, output, llvm_id_from_get_name(env, cond_goto->condition));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_true));
    string_extend_cstr(&a_main, output, ", label %");
    string_extend_size_t(&a_main, output, get_matching_label_id(env, cond_goto->if_false));
    vec_append(&a_main, output, '\n');
}

static void emit_struct_def_base(Env* env, String* output, const Struct_def_base* base, bool largest_only) {
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

static void emit_struct_def(Env* env, String* output, const Llvm_struct_def* struct_def) {
    log(LOG_DEBUG, TAST_FMT, llvm_struct_def_print(struct_def));
    string_extend_cstr(&a_main, output, "%struct.");
    emit_struct_def_base(env, output, &struct_def->base, false);
}

static void emit_raw_union_def(Env* env, String* output, const Llvm_raw_union_def* raw_union_def) {
    log(LOG_DEBUG, TAST_FMT, llvm_raw_union_def_print(raw_union_def));
    string_extend_cstr(&a_main, output, "%union.");
    emit_struct_def_base(env, output, &raw_union_def->base, true);
}

static void emit_load_element_ptr(Env* env, String* output, const Llvm_load_element_ptr* load_elem_ptr) {
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
    unwrap(alloca_lookup(&struct_index, env, load_elem_ptr->struct_index));
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
                emit_load_element_ptr(env, output, llvm_load_element_ptr_const_unwrap(statement));
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

    Alloca_table table = vec_top(&env->ancesters)->alloca_table;
    for (size_t idx = 0; idx < table.capacity; idx++) {
        if (table.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }
        emit_sometimes(env, struct_defs, output, literals, table.table_tasts[idx].tast);
    }

    //get_block_return_id(fun_block) = get_block_return_id(fun_block->left_child);
    vec_rem_last(&env->ancesters);
}

// this is only intended for alloca_table, etc.
static void emit_def_sometimes(Env* env, String* struct_defs, String* output, String* literals, const Llvm_def* def) {
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
            return;
        case LLVM_STRUCT_DEF:
            emit_struct_def(env, struct_defs, llvm_struct_def_const_unwrap(def));
            return;
        case LLVM_RAW_UNION_DEF:
            emit_raw_union_def(env, struct_defs, llvm_raw_union_def_const_unwrap(def));
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

// this is only intended for alloca_table, etc.
static void emit_sometimes(Env* env, String* struct_defs, String* output, String* literals, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_DEF:
            emit_def_sometimes(env, struct_defs, output, literals, llvm_def_const_unwrap(llvm));
            return;
        case LLVM_BLOCK:
            todo();
            emit_block(env, struct_defs, output, literals, llvm_block_const_unwrap(llvm));
            return;
        case LLVM_EXPR:
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            return;
        case LLVM_FUNCTION_PARAMS:
            todo();
            emit_function_params(env, output, llvm_function_params_const_unwrap(llvm));
            return;
        case LLVM_RETURN:
            unreachable("");
            return;
        case LLVM_GOTO:
            unreachable("");
            return;
        case LLVM_ALLOCA:
            return;
        case LLVM_COND_GOTO:
            unreachable("");
            return;
        case LLVM_STORE_ANOTHER_LLVM:
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            return;
    }
    unreachable("");
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

static void tast_emit_struct_literal(Env* env, String* output, const Tast_struct_lit_def* lit_def) {
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, lit_def->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lit_def->lang_type);
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

static void emit_struct_literal(Env* env, String* output, String* literals, const Llvm_struct_lit_def* lit_def) {
    string_extend_cstr(&a_main, output, "@__const.main.");
    string_extend_strv(&a_main, output, lit_def->name);
    string_extend_cstr(&a_main, output, " = private unnamed_addr constant %struct.");
    extend_lang_type_to_string(output, LANG_TYPE_MODE_EMIT_LLVM, lit_def->lang_type);
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

//static void emit_symbols(Env* env, String* output) {
//    for (size_t idx = 0; idx < env->global_literals.capacity; idx++) {
//        const Symbol_table_tast curr_tast = env->global_literals.table_tasts[idx];
//        if (curr_tast.status != SYM_TBL_OCCUPIED) {
//            continue;
//        }
//
//        const Tast_literal_def* def = tast_literal_def_const_unwrap(curr_tast.tast);
//
//        switch (def->type) {
//            case TAST_STRUCT_LIT_DEF:
//                tast_emit_struct_literal(env, output, tast_struct_lit_def_const_unwrap(def));
//                break;
//            case TAST_STRING_DEF:
//                tast_emit_symbol(output, curr_tast.key, tast_string_def_const_unwrap(def));
//                break;
//            default:
//                todo();
//        }
//    }
//}

void emit_llvm_from_tree(Env* env, const Llvm_block* root) {
    String struct_defs = {0};
    String output = {0};
    String literals = {0};
    emit_block(env, &struct_defs, &output, &literals, root);

    FILE* file = fopen("test.ll", "w");
    if (!file) {
        msg(
            LOG_FATAL, EXPECT_FAIL_NONE, dummy_file_text, dummy_pos, "could not open file %s: errno %d (%s)\n",
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

    msg(
        LOG_NOTE, EXPECT_FAIL_NONE, dummy_file_text, dummy_pos, "file %s built\n",
        params.input_file_name
    );

    fclose(file);
}

