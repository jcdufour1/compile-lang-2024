#include <uast.h>
#include <uast_utils.h>
#include <tast_utils.h>
#include <ir_utils.h>
#include <util.h>
#include <ulang_type.h>
#include <lang_type_print.h>
#include <pos_util.h>
#include <symbol_table.h>
#include <loc.h>

static void extend_scope(String* buf, Scope_id scope_id, Indent indent) {
    string_extend_cstr_indent(&a_temp, buf, "scope: ", indent);
    string_extend_size_t(&a_temp, buf, scope_id);
    string_extend_cstr(&a_temp, buf, "\n");
    string_extend_cstr_indent(&a_temp, buf, "name of scope: ", indent);
    extend_name(NAME_LOG, buf, scope_to_name_tbl_lookup(scope_id));
}

Strv uast_binary_print_internal(UAST_MODE mode, const Uast_binary* binary, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "binary", indent);
            string_extend_strv(&a_temp, &buf, binary_type_to_strv(binary->token_type));
            extend_pos(&buf, binary->pos);
            string_extend_cstr(&a_temp, &buf, "\n");

            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, binary->lhs, indent));
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, binary->rhs, indent));
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, binary->lhs, indent));
            string_extend_cstr(&a_temp, &buf, ")*(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, binary->rhs, indent));
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_unary_print_internal(UAST_MODE mode, const Uast_unary* unary, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "unary", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, unary_type_to_strv(unary->token_type));
            extend_pos(&buf, unary->pos);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_cstr_indent(&a_temp, &buf, "", indent);
            string_extend_strv(&a_temp, &buf, ulang_type_print_internal(LANG_TYPE_MODE_LOG, unary->lang_type));

            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, unary->child, indent));
            break;
        case UAST_MSG:
            msg_todo("", unary->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_symbol_print_internal(UAST_MODE mode, const Uast_symbol* sym, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "symbol_untyped", indent);
            extend_pos(&buf, sym->pos);
            extend_name(NAME_LOG, &buf, sym->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            extend_name(NAME_MSG, &buf, sym->name);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_member_access_print_internal(UAST_MODE mode, const Uast_member_access* access, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "member_access_untyped", indent);
            extend_pos(&buf, access->pos);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, access->callee, indent + INDENT_WIDTH));
            string_extend_strv(&a_temp, &buf, uast_symbol_print_internal(mode, access->member_name, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, access->callee, indent + INDENT_WIDTH));
            string_extend_cstr(&a_temp, &buf, ").");
            string_extend_strv(&a_temp, &buf, uast_symbol_print_internal(mode, access->member_name, indent + INDENT_WIDTH));
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_index_print_internal(UAST_MODE mode, const Uast_index* index, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "index_untyped", indent);
            extend_pos(&buf, index->pos);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, index->callee, indent + INDENT_WIDTH));
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, index->index, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, index->callee, indent + INDENT_WIDTH));
            string_extend_cstr(&a_temp, &buf, ")[");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, index->index, indent + INDENT_WIDTH));
            string_extend_cstr(&a_temp, &buf, "]");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_literal_print_internal(UAST_MODE mode, const Uast_literal* lit, Indent indent) {
    switch (lit->type) {
        case UAST_INT:
            return uast_int_print_internal(mode, uast_int_const_unwrap(lit), indent);
        case UAST_FLOAT:
            return uast_float_print_internal(mode, uast_float_const_unwrap(lit), indent);
        case UAST_STRING:
            return uast_string_print_internal(mode, uast_string_const_unwrap(lit), indent);
        case UAST_VOID:
            return uast_void_print_internal(mode, uast_void_const_unwrap(lit), indent);
    }
    unreachable("");
}

Strv uast_function_call_print_internal(UAST_MODE mode, const Uast_function_call* fun_call, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "function_call\n", indent);
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, fun_call->callee, indent + INDENT_WIDTH));

            for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
                Strv arg_text = uast_expr_print_internal(mode, darr_at(fun_call->args, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, arg_text);
            }

            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, fun_call->callee, indent + INDENT_WIDTH));
            string_extend_cstr(&a_temp, &buf, ")(");
            for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_temp, &buf, ", ");
                }
                Strv arg_text = uast_expr_print_internal(mode, darr_at(fun_call->args, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, arg_text);
            }
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_struct_literal_print_internal(UAST_MODE mode, const Uast_struct_literal* lit, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "struct_literal", indent);
            string_extend_cstr(&a_temp, &buf, "\n");

            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                Strv memb_text = uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, memb_text);
            }

            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "{");
            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                Strv memb_text = strv_from_f(
                    &a_temp,
                    "%s"FMT,
                    idx > 0 ? ", " : "",
                    strv_print(uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH))
                );
                string_extend_strv(&a_temp, &buf, memb_text);
            }
            string_extend_cstr(&a_temp, &buf, "}");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_array_literal_print_internal(UAST_MODE mode, const Uast_array_literal* lit, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "array_literal", indent);
            string_extend_cstr(&a_temp, &buf, "\n");

            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                Strv memb_text = uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, memb_text);
            }

            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "[");
            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_temp, &buf, ", ");
                }
                Strv memb_text = uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, memb_text);
            }
            string_extend_cstr(&a_temp, &buf, "]");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_tuple_print_internal(UAST_MODE mode, const Uast_tuple* lit, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "tuple", indent);
            string_extend_cstr(&a_temp, &buf, "\n");

            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                Strv memb_text = uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, memb_text);
            }

            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            for (size_t idx = 0; idx < lit->members.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_temp, &buf, ", ");
                }
                Strv memb_text = uast_expr_print_internal(mode, darr_at(lit->members, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, memb_text);
            }
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_enum_access_print_internal(UAST_MODE mode, const Uast_enum_access* access, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "enum_access", indent);
            string_extend_strv(&a_temp, &buf, tast_enum_tag_lit_print_internal(access->tag, indent + INDENT_WIDTH));
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, access->callee, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            msg_todo("", access->pos);
            break;
        default:
            unreachable("");

    }
    return string_to_strv(buf);
}

Strv uast_enum_get_tag_print_internal(UAST_MODE mode, const Uast_enum_get_tag* access, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "enum_get_tag", indent);
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, access->callee, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            msg_todo("", access->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_unknown_print_internal(UAST_MODE mode, const Uast_unknown* unknown, Indent indent) {
    (void) unknown;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "unknown\n", indent);
            break;
        case UAST_MSG:
            break;
        default:
            unreachable("");
    }
    
    return string_to_strv(buf);
}

Strv uast_int_print_internal(UAST_MODE mode, const Uast_int* num, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "number", indent);
            string_extend_int64_t(&a_temp, &buf, num->data);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            string_extend_int64_t(&a_temp, &buf, num->data);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_float_print_internal(UAST_MODE mode, const Uast_float* num, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "float", indent);
            string_extend_double(&a_temp, &buf, num->data);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            string_extend_double(&a_temp, &buf, num->data);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_string_print_internal(UAST_MODE mode, const Uast_string* lit, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "string", indent);
            string_extend_strv_in_par(&a_temp, &buf, lit->data);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "\"");
            string_extend_strv_in_par(&a_temp, &buf, lit->data);
            string_extend_cstr(&a_temp, &buf, "\"");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_void_print_internal(UAST_MODE mode, const Uast_void* num, Indent indent) {
    (void) num;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "void\n", indent);
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "void\n");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_block_print_internal(UAST_MODE mode, const Uast_block* block, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "block\n", indent);

            extend_scope(&buf, block->scope_id, indent + INDENT_WIDTH);
            string_extend_cstr(&a_temp, &buf, "\n");

            string_extend_cstr_indent(&a_temp, &buf, "parent_block_scope: ", indent + INDENT_WIDTH);
            string_extend_size_t(&a_temp, &buf, scope_get_parent_tbl_lookup(block->scope_id));
            string_extend_cstr(&a_temp, &buf, "\n");

            string_extend_cstr_indent(&a_temp, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
            usymbol_extend_table_internal(&buf, darr_at(env.symbol_tables, block->scope_id).usymbol_table, indent + 2*INDENT_WIDTH);

            string_extend_cstr_indent(&a_temp, &buf, "symbol_table\n", indent + INDENT_WIDTH);
            symbol_extend_table_internal(&buf, darr_at(env.symbol_tables, block->scope_id).symbol_table, indent + 2*INDENT_WIDTH);

            string_extend_cstr_indent(&a_temp, &buf, "alloca_table\n", indent + INDENT_WIDTH);
            ir_extend_table_internal(&buf, darr_at(env.symbol_tables, block->scope_id).ir_table, indent + 2*INDENT_WIDTH);

            for (size_t idx = 0; idx < block->children.info.count; idx++) {
                Strv arg_text = uast_stmt_print_internal(mode, darr_at(block->children, idx), indent + INDENT_WIDTH);
                string_extend_strv(&a_temp, &buf, arg_text);
            }

            break;
        case UAST_MSG:
            msg_todo("", block->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_function_params_print_internal(UAST_MODE mode, const Uast_function_params* function_params, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "function_params\n", indent);
            indent += INDENT_WIDTH;
            for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
                Strv arg_text = uast_param_print_internal(mode, darr_at(function_params->params, idx), indent);
                string_extend_strv(&a_temp, &buf, arg_text);
            }
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
                if (idx > 0) {
                    string_extend_cstr(&a_temp, &buf, ", ");
                }
                Strv arg_text = uast_param_print_internal(mode, darr_at(function_params->params, idx), indent);
                string_extend_strv(&a_temp, &buf, arg_text);
            }
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");

    }

    return string_to_strv(buf);
}

Strv uast_for_lower_bound_print_internal(UAST_MODE mode, const Uast_for_lower_bound* lower, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "for_lower_bound\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, lower->child, indent));
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, lower->child, indent));
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_for_upper_bound_print_internal(UAST_MODE mode, const Uast_for_upper_bound* upper, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "for_upper_bound\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, upper->child, indent));
            break;
        case UAST_MSG:
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, upper->child, indent));
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_condition_print_internal(UAST_MODE mode, const Uast_condition* cond, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "condition\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_operator_print_internal(mode, cond->child, indent));
            break;
        case UAST_MSG:
            indent += INDENT_WIDTH;
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_temp, &buf, uast_operator_print_internal(mode, cond->child, indent));
            string_extend_cstr(&a_temp, &buf, ")");
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_for_with_cond_print_internal(UAST_MODE mode, const Uast_for_with_cond* for_cond, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "for_with_cond\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_condition_print_internal(mode, for_cond->condition, indent));
            string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, for_cond->body, indent));
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            msg_todo("", for_cond->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_switch_print_internal(UAST_MODE mode, const Uast_switch* lang_switch, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "switch\n", indent);
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, lang_switch->operand, indent + INDENT_WIDTH));
            for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
                string_extend_strv(&a_temp, &buf, uast_case_print_internal(mode, darr_at(lang_switch->cases, idx), indent + INDENT_WIDTH));
            }
            break;
        case UAST_MSG:
            msg_todo("", lang_switch->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_defer_print_internal(UAST_MODE mode, const Uast_defer* defer, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "defer", indent);
            string_extend_strv(&a_temp, &buf, uast_stmt_print_internal(mode, defer->child, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            msg_todo("", defer->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_using_print_internal(UAST_MODE mode, const Uast_using* using, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "using: ", indent);
            extend_name(NAME_LOG, &buf, using->sym_name);
            string_extend_cstr_indent(&a_temp, &buf, "\n", indent);
            break;
        case UAST_MSG:
            msg_todo("", using->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_yield_print_internal(UAST_MODE mode, const Uast_yield* yield, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "yield\n", indent);

            // TODO: make function for below, and also use function for uast_continue_print_internal
            string_extend_cstr_indent(&a_temp, &buf, "label to break_out_of: ", indent + INDENT_WIDTH);
            extend_name(NAME_LOG, &buf, yield->break_out_of);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_cstr_indent(&a_temp, &buf, "name of scope to break_out_of: ", indent + INDENT_WIDTH);
            Uast_def* label_ = NULL;
            if (usymbol_lookup(&label_, yield->break_out_of)) {
                Uast_label* label = uast_label_unwrap(label_);
                extend_name(NAME_LOG, &buf, label->block_scope);
            } else {
                string_extend_cstr(&a_temp, &buf, "<null>");
            }
            string_extend_cstr(&a_temp, &buf, "\n");

            if (yield->do_yield_expr) {
                string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, yield->yield_expr, indent + INDENT_WIDTH));
            }

            break;
        case UAST_MSG:
            msg_todo("", yield->pos);
            break;
        default:
            unreachable("");
    }
    return string_to_strv(buf);
}

Strv uast_continue_print_internal(UAST_MODE mode, const Uast_continue* cont, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "continue2\n", indent);
            string_extend_cstr_indent(&a_temp, &buf, "break_out_of: ", indent + INDENT_WIDTH);
            extend_name(NAME_LOG, &buf, cont->break_out_of);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", cont->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_stmt_removed_print_internal(UAST_MODE mode, const Uast_stmt_removed* removed, Indent indent) {
    (void) removed;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "stmt_removed\n", indent);
            break;
        case UAST_MSG:
            msg_todo("", removed->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_assignment_print_internal(UAST_MODE mode, const Uast_assignment* assign, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "assignment\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, assign->lhs, indent));
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, assign->rhs, indent));
            break;
        case UAST_MSG:
            msg_todo("", assign->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_if_print_internal(UAST_MODE mode, const Uast_if* lang_if, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "if\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_condition_print_internal(mode, lang_if->condition, indent));
            string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, lang_if->body, indent));
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            msg_todo("", lang_if->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_case_print_internal(UAST_MODE mode, const Uast_case* lang_case, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "case\n", indent);

            extend_scope(&buf, lang_case->scope_id, indent + INDENT_WIDTH);
            string_extend_cstr(&a_temp, &buf, "\n");

            if (lang_case->is_default) {
                string_extend_cstr_indent(&a_temp, &buf, "default\n", indent + INDENT_WIDTH);
            } else {
                string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, lang_case->expr, indent + INDENT_WIDTH));
            }
            string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, lang_case->if_true, indent + INDENT_WIDTH));

            break;
        case UAST_MSG:
            msg_todo("", lang_case->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_param_print_internal(UAST_MODE mode, const Uast_param* param, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "param", indent);
            extend_pos(&buf, param->pos);
            string_extend_cstr_indent(&a_temp, &buf, "\n", indent);
            string_extend_strv(&a_temp, &buf, uast_variable_def_print_internal(mode, param->base, indent + INDENT_WIDTH));
            if (param->is_optional) {
                string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, param->optional_default, indent + INDENT_WIDTH));
            }
            if (param->is_variadic) {
                string_extend_cstr_indent(&a_temp, &buf, "is_variadic\n", indent + INDENT_WIDTH);
            }
            break;
        case UAST_MSG:
            msg_todo("", param->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_label_print_internal(UAST_MODE mode, const Uast_label* label, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "label", indent);
            extend_pos(&buf, label->pos);
            extend_name(NAME_LOG, &buf, label->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_cstr_indent(&a_temp, &buf, "block_scope: ", indent + INDENT_WIDTH);
            extend_name(NAME_LOG, &buf, label->block_scope);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", label->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_builtin_def_print_internal(UAST_MODE mode, const Uast_builtin_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "builtin_def", indent);
            extend_name(NAME_LOG, &buf, def->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_import_path_print_internal(UAST_MODE mode, const Uast_import_path* import, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "import_path", indent);
            string_extend_strv(&a_temp, &buf, import->mod_path);
            string_extend_cstr(&a_temp, &buf, "\n");
            if (import->block) {
                string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, import->block, indent + INDENT_WIDTH));
            } else {
                string_extend_cstr_indent(&a_temp, &buf, "<block is null>\n", indent);
            }
            break;
        case UAST_MSG:
            msg_todo("", import->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_mod_alias_print_internal(UAST_MODE mode, const Uast_mod_alias* alias, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "mod_alias", indent);
            string_extend_cstr(&a_temp, &buf, "(");
            extend_name(NAME_LOG, &buf, alias->name);
            string_extend_cstr(&a_temp, &buf, ")");
            string_extend_cstr(&a_temp, &buf, "(");
            string_extend_strv(&a_main, &buf, alias->mod_path);
            string_extend_cstr(&a_temp, &buf, ")");
            string_extend_f(&a_temp, &buf, FMT, loc_print(loc_get(alias->loc)));
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", alias->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_generic_param_print_internal(UAST_MODE mode, const Uast_generic_param* param, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "generic_params\n", indent);
            string_extend_strv_indent(&a_temp, &buf, sv(""), indent + INDENT_WIDTH);
            if (param->is_expr) {
                string_extend_cstr(&a_temp, &buf, "expr: ");
                string_extend_strv(&a_temp, &buf, ulang_type_print_internal(LANG_TYPE_MODE_LOG, param->expr_lang_type));
                string_extend_cstr(&a_temp, &buf, "\n");
                string_extend_strv_indent(&a_temp, &buf, sv(""), indent + INDENT_WIDTH);
            }
            extend_name(NAME_LOG, &buf, param->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", param->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_poison_def_print_internal(UAST_MODE mode, const Uast_poison_def* poison, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "poison_def", indent);
            extend_name(NAME_LOG, &buf, poison->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", poison->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_lang_def_print_internal(UAST_MODE mode, const Uast_lang_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "lang_def", indent);
            extend_pos(&buf, def->pos);
            extend_name(NAME_LOG, &buf, def->alias_name);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, def->expr, indent + INDENT_WIDTH));
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_void_def_print_internal(UAST_MODE mode, const Uast_void_def* def, Indent indent) {
    (void) def;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "void_def\n", indent);
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_return_print_internal(UAST_MODE mode, const Uast_return* lang_rtn, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "return\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, lang_rtn->child, indent));
            break;
        case UAST_MSG:
            msg_todo("", lang_rtn->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_directive_print_internal(UAST_MODE mode, const Uast_directive* directive, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "directive", indent);
            string_extend_strv(&a_temp, &buf, directive->name);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", directive->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_orelse_print_internal(UAST_MODE mode, const Uast_orelse* orelse, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "orelse\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, orelse->expr_to_unwrap, indent));
            string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, orelse->if_error, indent));
            break;
        case UAST_MSG:
            msg_todo("", orelse->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_fn_print_internal(UAST_MODE mode, const Uast_fn* fn, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "fn", indent);
            string_extend_strv(&a_temp, &buf, ulang_type_print_internal(LANG_TYPE_MODE_LOG, ulang_type_fn_const_wrap(fn->ulang_type)));
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", fn->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_question_mark_print_internal(UAST_MODE mode, const Uast_question_mark* mark, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "question_mark\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_expr_print_internal(mode, mark->expr_to_unwrap, indent));
            break;
        case UAST_MSG:
            msg_todo("", mark->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_underscore_print_internal(UAST_MODE mode, const Uast_underscore* underscore, Indent indent) {
    (void) underscore;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "underscore\n", indent);
            break;
        case UAST_MSG:
            msg_todo("", underscore->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_expr_removed_print_internal(UAST_MODE mode, const Uast_expr_removed* removed, Indent indent) {
    (void) removed;
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "expr_removed", indent);
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", removed->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_if_else_chain_print_internal(UAST_MODE mode, const Uast_if_else_chain* if_else, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "if_else_chain\n", indent);
            indent += INDENT_WIDTH;
            for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
                Strv arg_text = uast_if_print_internal(mode, darr_at(if_else->uasts, idx), indent);
                string_extend_strv(&a_temp, &buf, arg_text);
            }
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            msg_todo("", if_else->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_function_decl_print_internal(UAST_MODE mode, const Uast_function_decl* fun_decl, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "function_decl", indent);
            extend_name(NAME_LOG, &buf, fun_decl->name);
            extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_decl->return_type);
            string_extend_cstr(&a_temp, &buf, "\n");
            string_extend_strv(&a_temp, &buf, uast_function_params_print_internal(mode, fun_decl->params, indent + INDENT_WIDTH));
            break;
        case UAST_MSG:
            msg_todo("", fun_decl->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_function_def_print_internal(UAST_MODE mode, const Uast_function_def* fun_def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "function_def\n", indent);
            indent += INDENT_WIDTH;
            string_extend_strv(&a_temp, &buf, uast_function_decl_print_internal(mode, fun_def->decl, indent));
            string_extend_strv(&a_temp, &buf, uast_block_print_internal(mode, fun_def->body, indent));
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            msg_todo("", fun_def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

static void extend_ustruct_def_base(String* buf, const void* main_ptr, const char* type_name, Ustruct_def_base base, Indent indent, Pos pos) {
    string_extend_cstr_indent(&a_temp, buf, type_name, indent);
    extend_pos(buf, pos);
    extend_name(NAME_LOG, buf, base.name);
    string_extend_f(&a_temp, buf, " %p ", main_ptr);
    string_extend_cstr(&a_temp, buf, " \n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Strv memb_text = uast_variable_def_print_internal(UAST_LOG/*TODO*/, darr_at(base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_temp, buf, memb_text);
    }
}

Strv ustruct_def_base_print_internal(Ustruct_def_base base, Indent indent) {
    String buf = {0};
    extend_ustruct_def_base(&buf, NULL, "<unknown>", base, indent, POS_BUILTIN);
    return string_to_strv(buf);
}

Strv uast_struct_def_print_internal(UAST_MODE mode, const Uast_struct_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            extend_ustruct_def_base(&buf, def, "struct_def", def->base, indent, def->pos);
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_raw_union_def_print_internal(UAST_MODE mode, const Uast_raw_union_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            extend_ustruct_def_base(&buf, def, "raw_union_def", def->base, indent, def->pos);
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_enum_def_print_internal(UAST_MODE mode, const Uast_enum_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            extend_ustruct_def_base(&buf, def, "enum_def", def->base, indent, def->pos);
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_primitive_def_print_internal(UAST_MODE mode, const Uast_primitive_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "primitive_def\n", indent);
            indent += INDENT_WIDTH;
            extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
            string_extend_cstr(&a_temp, &buf, "\n");
            indent -= INDENT_WIDTH;
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_variable_def_print_internal(UAST_MODE mode, const Uast_variable_def* def, Indent indent) {
    String buf = {0};

    switch (mode) {
        case UAST_LOG:
            string_extend_cstr_indent(&a_temp, &buf, "variable_def", indent);
            extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
            extend_name(NAME_LOG, &buf, def->name);
            string_extend_f(&a_temp, &buf, FMT, loc_print(loc_get(def->loc)));
            string_extend_cstr(&a_temp, &buf, "\n");
            break;
        case UAST_MSG:
            msg_todo("", def->pos);
            break;
        default:
            unreachable("");
    }

    return string_to_strv(buf);
}

Strv uast_operator_print_internal(UAST_MODE mode, const Uast_operator* operator, Indent indent) {
    switch (operator->type) {
        case UAST_BINARY:
            return uast_binary_print_internal(mode, uast_binary_const_unwrap(operator), indent);
        case UAST_UNARY:
            return uast_unary_print_internal(mode, uast_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Strv uast_def_print_internal(UAST_MODE mode, const Uast_def* def, Indent indent) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            return uast_function_def_print_internal(mode, uast_function_def_const_unwrap(def), indent);
        case UAST_FUNCTION_DECL:
            return uast_function_decl_print_internal(mode, uast_function_decl_const_unwrap(def), indent);
        case UAST_VARIABLE_DEF:
            return uast_variable_def_print_internal(mode, uast_variable_def_const_unwrap(def), indent);
        case UAST_STRUCT_DEF:
            return uast_struct_def_print_internal(mode, uast_struct_def_const_unwrap(def), indent);
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_print_internal(mode, uast_raw_union_def_const_unwrap(def), indent);
        case UAST_ENUM_DEF:
            return uast_enum_def_print_internal(mode, uast_enum_def_const_unwrap(def), indent);
        case UAST_PRIMITIVE_DEF:
            return uast_primitive_def_print_internal(mode, uast_primitive_def_const_unwrap(def), indent);
        case UAST_IMPORT_PATH:
            return uast_import_path_print_internal(mode, uast_import_path_const_unwrap(def), indent);
        case UAST_MOD_ALIAS:
            return uast_mod_alias_print_internal(mode, uast_mod_alias_const_unwrap(def), indent);
        case UAST_GENERIC_PARAM:
            return uast_generic_param_print_internal(mode, uast_generic_param_const_unwrap(def), indent);
        case UAST_POISON_DEF:
            return uast_poison_def_print_internal(mode, uast_poison_def_const_unwrap(def), indent);
        case UAST_LANG_DEF:
            return uast_lang_def_print_internal(mode, uast_lang_def_const_unwrap(def), indent);
        case UAST_VOID_DEF:
            return uast_void_def_print_internal(mode, uast_void_def_const_unwrap(def), indent);
        case UAST_LABEL:
            return uast_label_print_internal(mode, uast_label_const_unwrap(def), indent);
        case UAST_BUILTIN_DEF:
            return uast_builtin_def_print_internal(mode, uast_builtin_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Strv uast_expr_print_internal(UAST_MODE mode, const Uast_expr* expr, Indent indent) {
    if (!expr) {
        return sv("<nothing>\n");
    }
    switch (expr->type) {
        case UAST_BLOCK:
            return uast_block_print_internal(mode, uast_block_const_unwrap(expr), indent);
        case UAST_OPERATOR:
            return uast_operator_print_internal(mode, uast_operator_const_unwrap(expr), indent);
        case UAST_SYMBOL:
            return uast_symbol_print_internal(mode, uast_symbol_const_unwrap(expr), indent);
        case UAST_MEMBER_ACCESS:
            return uast_member_access_print_internal(mode, uast_member_access_const_unwrap(expr), indent);
        case UAST_INDEX:
            return uast_index_print_internal(mode, uast_index_const_unwrap(expr), indent);
        case UAST_LITERAL:
            return uast_literal_print_internal(mode, uast_literal_const_unwrap(expr), indent);
        case UAST_FUNCTION_CALL:
            return uast_function_call_print_internal(mode, uast_function_call_const_unwrap(expr), indent);
        case UAST_STRUCT_LITERAL:
            return uast_struct_literal_print_internal(mode, uast_struct_literal_const_unwrap(expr), indent);
        case UAST_ARRAY_LITERAL:
            return uast_array_literal_print_internal(mode, uast_array_literal_const_unwrap(expr), indent);
        case UAST_TUPLE:
            return uast_tuple_print_internal(mode, uast_tuple_const_unwrap(expr), indent);
        case UAST_ENUM_ACCESS:
            return uast_enum_access_print_internal(mode, uast_enum_access_const_unwrap(expr), indent);
        case UAST_UNKNOWN:
            return uast_unknown_print_internal(mode, uast_unknown_const_unwrap(expr), indent);
        case UAST_ENUM_GET_TAG:
            return uast_enum_get_tag_print_internal(mode, uast_enum_get_tag_const_unwrap(expr), indent);
        case UAST_SWITCH:
            return uast_switch_print_internal(mode, uast_switch_const_unwrap(expr), indent);
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_print_internal(mode, uast_if_else_chain_const_unwrap(expr), indent);
        case UAST_DIRECTIVE:
            return uast_directive_print_internal(mode, uast_directive_const_unwrap(expr), indent);
        case UAST_ORELSE:
            return uast_orelse_print_internal(mode, uast_orelse_const_unwrap(expr), indent);
        case UAST_FN:
            return uast_fn_print_internal(mode, uast_fn_const_unwrap(expr), indent);
        case UAST_QUESTION_MARK:
            return uast_question_mark_print_internal(mode, uast_question_mark_const_unwrap(expr), indent);
        case UAST_UNDERSCORE:
            return uast_underscore_print_internal(mode, uast_underscore_const_unwrap(expr), indent);
        case UAST_EXPR_REMOVED:
            return uast_expr_removed_print_internal(mode, uast_expr_removed_const_unwrap(expr), indent);
    }
    unreachable("");
}

Strv uast_stmt_print_internal(UAST_MODE mode, const Uast_stmt* stmt, Indent indent) {
    switch (stmt->type) {
        case UAST_EXPR:
            return uast_expr_print_internal(mode, uast_expr_const_unwrap(stmt), indent);
        case UAST_DEF:
            return uast_def_print_internal(mode, uast_def_const_unwrap(stmt), indent);
        case UAST_YIELD:
            return uast_yield_print_internal(mode, uast_yield_const_unwrap(stmt), indent);
        case UAST_ASSIGNMENT:
            return uast_assignment_print_internal(mode, uast_assignment_const_unwrap(stmt), indent);
        case UAST_RETURN:
            return uast_return_print_internal(mode, uast_return_const_unwrap(stmt), indent);
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_print_internal(mode, uast_for_with_cond_const_unwrap(stmt), indent);
        case UAST_DEFER:
            return uast_defer_print_internal(mode, uast_defer_const_unwrap(stmt), indent);
        case UAST_USING:
            return uast_using_print_internal(mode, uast_using_const_unwrap(stmt), indent);
        case UAST_CONTINUE:
            return uast_continue_print_internal(mode, uast_continue_const_unwrap(stmt), indent);
        case UAST_STMT_REMOVED:
            return uast_stmt_removed_print_internal(mode, uast_stmt_removed_const_unwrap(stmt), indent);
    }
    unreachable("");
}

Strv uast_print_internal(UAST_MODE mode, const Uast* uast, Indent indent) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_print_internal(mode, uast_stmt_const_unwrap(uast), indent);
        case UAST_FUNCTION_PARAMS:
            return uast_function_params_print_internal(mode, uast_function_params_const_unwrap(uast), indent);
        case UAST_FOR_LOWER_BOUND:
            return uast_for_lower_bound_print_internal(mode, uast_for_lower_bound_const_unwrap(uast), indent);
        case UAST_FOR_UPPER_BOUND:
            return uast_for_upper_bound_print_internal(mode, uast_for_upper_bound_const_unwrap(uast), indent);
        case UAST_CONDITION:
            return uast_condition_print_internal(mode, uast_condition_const_unwrap(uast), indent);
        case UAST_IF:
            return uast_if_print_internal(mode, uast_if_const_unwrap(uast), indent);
        case UAST_CASE:
            return uast_case_print_internal(mode, uast_case_const_unwrap(uast), indent);
        case UAST_PARAM:
            return uast_param_print_internal(mode, uast_param_const_unwrap(uast), indent);
    }
    unreachable("");
}
