#include <uast.h>
#include <uast_utils.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <util.h>
#include <ulang_type.h>

#include <symbol_table.h>

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&print_arena, buf, "(( line:");
    string_extend_int64_t(&print_arena, buf, pos.line);
    string_extend_cstr(&print_arena, buf, " ))");
}

void extend_ulang_type_to_string(String* string, LANG_TYPE_MODE mode, Ulang_type lang_type) {
    switch (lang_type.type) {
        case ULANG_TYPE_REGULAR: {
            Ulang_type_atom atom = ulang_type_regular_const_unwrap(lang_type).atom;

            if (mode == LANG_TYPE_MODE_LOG) {
                vec_append(&print_arena, string, '<');
            }

            if (atom.str.count > 1) {
                string_extend_strv(&print_arena, string, atom.str);
            } else {
                string_extend_cstr(&print_arena, string, "<null>");
            }
            if (atom.pointer_depth < 0) {
                todo();
            }
            for (int16_t idx = 0; idx < atom.pointer_depth; idx++) {
                vec_append(&print_arena, string, '*');
            }
            if (mode == LANG_TYPE_MODE_LOG) {
                vec_append(&print_arena, string, '>');
            }
            return;
        }
        case ULANG_TYPE_TUPLE: {
            vec_append(&print_arena, string, '(');
            Ulang_type_tuple tuple = ulang_type_tuple_const_unwrap(lang_type);
            for (size_t idx = 0; idx < tuple.ulang_types.info.count; idx++) {
                if (mode == LANG_TYPE_MODE_MSG && idx > 0) {
                    string_extend_cstr(&print_arena, string, ", ");
                }
                extend_ulang_type_to_string(string, mode, vec_at(&tuple.ulang_types, idx));
            }
            vec_append(&print_arena, string, ')');
            return;
        }
        case ULANG_TYPE_FN: {
            string_extend_cstr(&a_main, string, "fn");
            Ulang_type_fn fn = ulang_type_fn_const_unwrap(lang_type);
            extend_ulang_type_to_string(string, mode, ulang_type_tuple_const_wrap(fn.params));
            extend_ulang_type_to_string(string, mode, *fn.return_type);
            return;
        }
    }
    unreachable("");
}

Str_view ulang_type_print_internal(LANG_TYPE_MODE mode, Ulang_type lang_type) {
    String buf = {0};
    extend_ulang_type_to_string(&buf, mode, lang_type);
    if (mode == LANG_TYPE_MODE_LOG) {
        string_extend_cstr(&print_arena, &buf, "\n");
    }
    return string_to_strv(buf);
}

Str_view uast_binary_print_internal(const Uast_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    string_extend_strv(&print_arena, &buf, binary_type_to_str_view(binary->token_type));
    extend_pos(&buf, binary->pos);
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(binary->lhs, indent));
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(binary->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_unary_print_internal(const Uast_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", indent);
    string_extend_strv(&print_arena, &buf, unary_type_to_str_view(unary->token_type));
    extend_pos(&buf, unary->pos);
    string_extend_cstr(&print_arena, &buf, "\n");

    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(unary->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

void uast_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type_to_string(string, LANG_TYPE_MODE_LOG, base.lang_type);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view uast_symbol_print_internal(const Uast_symbol* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol_untyped", indent);
    extend_pos(&buf, sym->pos);
    string_extend_strv(&print_arena, &buf, sym->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_member_access_print_internal(const Uast_member_access* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", indent);
    string_extend_strv_in_par(&print_arena, &buf, access->member_name);
    extend_pos(&buf, access->pos);
    string_extend_cstr(&print_arena, &buf, "\n");

    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(access->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_index_print_internal(const Uast_index* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_untyped", indent);
    extend_pos(&buf, index->pos);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_literal_print_internal(const Uast_literal* lit, int indent) {
    switch (lit->type) {
        case UAST_NUMBER:
            return uast_number_print_internal(uast_number_const_unwrap(lit), indent);
        case UAST_STRING:
            return uast_string_print_internal(uast_string_const_unwrap(lit), indent);
        case UAST_VOID:
            return uast_void_print_internal(uast_void_const_unwrap(lit), indent);
        case UAST_ENUM_LIT:
            return uast_enum_lit_print_internal(uast_enum_lit_const_unwrap(lit), indent);
        case UAST_CHAR:
            return uast_char_print_internal(uast_char_const_unwrap(lit), indent);
    }
    unreachable("");
}

Str_view uast_function_call_print_internal(const Uast_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call\n", indent);
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(fun_call->callee, indent + INDENT_WIDTH));

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_text = uast_expr_print_internal(vec_at(&fun_call->args, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Str_view uast_struct_literal_print_internal(const Uast_struct_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_literal", indent);
    string_extend_strv(&print_arena, &buf, lit->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = uast_stmt_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Str_view uast_tuple_print_internal(const Uast_tuple* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "tuple", indent);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = uast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Str_view uast_sum_access_print_internal(const Uast_sum_access* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "sum_access", indent);
    
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, lit->lang_type));
    string_extend_strv(&print_arena, &buf, tast_enum_lit_print_internal(lit->tag, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(lit->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_number_print_internal(const Uast_number* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", indent);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_string_print_internal(const Uast_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", indent);
    string_extend_strv_in_par(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_enum_lit_print_internal(const Uast_enum_lit* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", indent);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_void_print_internal(const Uast_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Str_view uast_char_print_internal(const Uast_char* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", indent);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_block_print_internal(const Uast_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", indent);

    string_extend_cstr_indent(&print_arena, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    alloca_extend_table_internal(&buf, block->symbol_collection.alloca_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
    usymbol_extend_table_internal(&buf, block->symbol_collection.usymbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "symbol_table\n", indent + INDENT_WIDTH);
    symbol_extend_table_internal(&buf, block->symbol_collection.symbol_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = uast_stmt_print_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Str_view uast_function_params_print_internal(const Uast_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = uast_param_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_lang_type_print_internal(const Uast_lang_type* lang_type, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", indent);
    string_extend_strv(&print_arena, &buf, ulang_type_print_internal(LANG_TYPE_MODE_LOG, lang_type->lang_type));

    return string_to_strv(buf);
}

Str_view uast_for_lower_bound_print_internal(const Uast_for_lower_bound* lower, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_lower_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(lower->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_for_upper_bound_print_internal(const Uast_for_upper_bound* upper, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_upper_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(upper->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_condition_print_internal(const Uast_condition* cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "condition\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_operator_print_internal(cond->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_for_range_print_internal(const Uast_for_range* for_range, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_range\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_variable_def_print_internal(for_range->var_def, indent));
    string_extend_strv(&print_arena, &buf, uast_for_lower_bound_print_internal(for_range->lower_bound, indent));
    string_extend_strv(&print_arena, &buf, uast_for_upper_bound_print_internal(for_range->upper_bound, indent));
    string_extend_strv(&print_arena, &buf, uast_block_print_internal(for_range->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_for_with_cond_print_internal(const Uast_for_with_cond* for_cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_with_cond\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_condition_print_internal(for_cond->condition, indent));
    string_extend_strv(&print_arena, &buf, uast_block_print_internal(for_cond->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_switch_print_internal(const Uast_switch* lang_switch, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "switch\n", indent);
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(lang_switch->operand, indent + INDENT_WIDTH));
    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        string_extend_strv(&print_arena, &buf, uast_case_print_internal(vec_at(&lang_switch->cases, idx), indent + INDENT_WIDTH));
    }

    return string_to_strv(buf);
}

Str_view uast_break_print_internal(const Uast_break* lang_break, int indent) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "break\n", indent);

    return string_to_strv(buf);
}

Str_view uast_continue_print_internal(const Uast_continue* lang_continue, int indent) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "continue\n", indent);

    return string_to_strv(buf);
}

Str_view uast_assignment_print_internal(const Uast_assignment* assign, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "assignment\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_stmt_print_internal(assign->lhs, indent));
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(assign->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_if_print_internal(const Uast_if* lang_if, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_condition_print_internal(lang_if->condition, indent));
    string_extend_strv(&print_arena, &buf, uast_block_print_internal(lang_if->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_case_print_internal(const Uast_case* lang_case, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "case\n", indent);
    if (lang_case->is_default) {
        string_extend_cstr_indent(&print_arena, &buf, "default\n", indent + INDENT_WIDTH);
    } else {
        string_extend_strv(&print_arena, &buf, uast_expr_print_internal(lang_case->expr, indent + INDENT_WIDTH));
    }
    string_extend_strv(&print_arena, &buf, uast_stmt_print_internal(lang_case->if_true, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_param_print_internal(const Uast_param* param, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "param\n", indent);
    string_extend_strv(&print_arena, &buf, uast_variable_def_print_internal(param->base, indent + INDENT_WIDTH));
    if (param->is_optional) {
        string_extend_strv(&print_arena, &buf, uast_expr_print_internal(param->optional_default, indent + INDENT_WIDTH));
    }
    if (param->is_variadic) {
        string_extend_cstr_indent(&print_arena, &buf, "is_variadic\n", indent + INDENT_WIDTH);
    }

    return string_to_strv(buf);
}

Str_view uast_return_print_internal(const Uast_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(lang_rtn->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_if_else_chain_print_internal(const Uast_if_else_chain* if_else, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if_else_chain\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
        Str_view arg_text = uast_if_print_internal(vec_at(&if_else->uasts, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_function_decl_print_internal(const Uast_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", indent);
    string_extend_strv_in_par(&print_arena, &buf, fun_decl->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, uast_lang_type_print_internal(fun_decl->return_type, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, uast_function_params_print_internal(fun_decl->params, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_function_def_print_internal(const Uast_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, uast_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&print_arena, &buf, uast_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_ustruct_def_base(String* buf, const char* type_name, Ustruct_def_base base, int indent, Pos pos) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    extend_pos(buf, pos);
    string_extend_strv_in_par(&print_arena, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = uast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

Str_view uast_struct_def_print_internal(const Uast_struct_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "struct_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Str_view uast_raw_union_def_print_internal(const Uast_raw_union_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "raw_union_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Str_view uast_enum_def_print_internal(const Uast_enum_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "enum_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Str_view uast_sum_def_print_internal(const Uast_sum_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "sum_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Str_view uast_primitive_def_print_internal(const Uast_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_string_def_print_internal(const Uast_string_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_struct_lit_def_print_internal(const Uast_struct_lit_def* def, int indent) {
    String buf = {0};

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def", indent);
    string_extend_strv_indent(&print_arena, &buf, def->name, indent);
    string_extend_cstr(&print_arena, &buf, "\n");
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = uast_print_internal(vec_at(&def->members, idx), indent);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_void_def_print_internal(const Uast_void_def* def, int indent) {
    (void) def;
    String buf = {0};

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "void_def", indent);
    string_extend_cstr(&print_arena, &buf, "\n");

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_literal_def_print_internal(const Uast_literal_def* def, int indent) {
    switch (def->type) {
        case UAST_STRING_DEF:
            return uast_string_def_print_internal(uast_string_def_const_unwrap(def), indent);
        case UAST_STRUCT_LIT_DEF:
            return uast_struct_lit_def_print_internal(uast_struct_lit_def_const_unwrap(def), indent);
        case UAST_VOID_DEF:
            return uast_void_def_print_internal(uast_void_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view uast_variable_def_print_internal(const Uast_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_operator_print_internal(const Uast_operator* operator, int indent) {
    switch (operator->type) {
        case UAST_BINARY:
            return uast_binary_print_internal(uast_binary_const_unwrap(operator), indent);
        case UAST_UNARY:
            return uast_unary_print_internal(uast_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Str_view uast_def_print_internal(const Uast_def* def, int indent) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            return uast_function_def_print_internal(uast_function_def_const_unwrap(def), indent);
        case UAST_FUNCTION_DECL:
            return uast_function_decl_print_internal(uast_function_decl_const_unwrap(def), indent);
        case UAST_VARIABLE_DEF:
            return uast_variable_def_print_internal(uast_variable_def_const_unwrap(def), indent);
        case UAST_STRUCT_DEF:
            return uast_struct_def_print_internal(uast_struct_def_const_unwrap(def), indent);
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_print_internal(uast_raw_union_def_const_unwrap(def), indent);
        case UAST_ENUM_DEF:
            return uast_enum_def_print_internal(uast_enum_def_const_unwrap(def), indent);
        case UAST_SUM_DEF:
            return uast_sum_def_print_internal(uast_sum_def_const_unwrap(def), indent);
        case UAST_PRIMITIVE_DEF:
            return uast_primitive_def_print_internal(uast_primitive_def_const_unwrap(def), indent);
        case UAST_LITERAL_DEF:
            return uast_literal_def_print_internal(uast_literal_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view uast_expr_print_internal(const Uast_expr* expr, int indent) {
    switch (expr->type) {
        case UAST_OPERATOR:
            return uast_operator_print_internal(uast_operator_const_unwrap(expr), indent);
        case TAST_SYMBOL:
            return uast_symbol_print_internal(uast_symbol_const_unwrap(expr), indent);
        case UAST_MEMBER_ACCESS:
            return uast_member_access_print_internal(uast_member_access_const_unwrap(expr), indent);
        case UAST_INDEX:
            return uast_index_print_internal(uast_index_const_unwrap(expr), indent);
        case UAST_LITERAL:
            return uast_literal_print_internal(uast_literal_const_unwrap(expr), indent);
        case UAST_FUNCTION_CALL:
            return uast_function_call_print_internal(uast_function_call_const_unwrap(expr), indent);
        case UAST_STRUCT_LITERAL:
            return uast_struct_literal_print_internal(uast_struct_literal_const_unwrap(expr), indent);
        case UAST_TUPLE:
            return uast_tuple_print_internal(uast_tuple_const_unwrap(expr), indent);
        case UAST_SUM_ACCESS:
            return uast_sum_access_print_internal(uast_sum_access_const_unwrap(expr), indent);
    }
    unreachable("");
}

Str_view uast_stmt_print_internal(const Uast_stmt* stmt, int indent) {
    switch (stmt->type) {
        case UAST_BLOCK:
            return uast_block_print_internal(uast_block_const_unwrap(stmt), indent);
        case UAST_EXPR:
            return uast_expr_print_internal(uast_expr_const_unwrap(stmt), indent);
        case UAST_DEF:
            return uast_def_print_internal(uast_def_const_unwrap(stmt), indent);
        case UAST_BREAK:
            return uast_break_print_internal(uast_break_const_unwrap(stmt), indent);
        case UAST_CONTINUE:
            return uast_continue_print_internal(uast_continue_const_unwrap(stmt), indent);
        case UAST_ASSIGNMENT:
            return uast_assignment_print_internal(uast_assignment_const_unwrap(stmt), indent);
        case UAST_RETURN:
            return uast_return_print_internal(uast_return_const_unwrap(stmt), indent);
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_print_internal(uast_if_else_chain_const_unwrap(stmt), indent);
        case UAST_FOR_RANGE:
            return uast_for_range_print_internal(uast_for_range_const_unwrap(stmt), indent);
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_print_internal(uast_for_with_cond_const_unwrap(stmt), indent);
        case UAST_SWITCH:
            return uast_switch_print_internal(uast_switch_const_unwrap(stmt), indent);
    }
    unreachable("");
}

Str_view uast_print_internal(const Uast* uast, int indent) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_print_internal(uast_stmt_const_unwrap(uast), indent);
        case UAST_FUNCTION_PARAMS:
            return uast_function_params_print_internal(uast_function_params_const_unwrap(uast), indent);
        case UAST_LANG_TYPE:
            return uast_lang_type_print_internal(uast_lang_type_const_unwrap(uast), indent);
        case UAST_FOR_LOWER_BOUND:
            return uast_for_lower_bound_print_internal(uast_for_lower_bound_const_unwrap(uast), indent);
        case UAST_FOR_UPPER_BOUND:
            return uast_for_upper_bound_print_internal(uast_for_upper_bound_const_unwrap(uast), indent);
        case UAST_CONDITION:
            return uast_condition_print_internal(uast_condition_const_unwrap(uast), indent);
        case UAST_IF:
            return uast_if_print_internal(uast_if_const_unwrap(uast), indent);
        case UAST_CASE:
            return uast_case_print_internal(uast_case_const_unwrap(uast), indent);
        case UAST_PARAM:
            return uast_param_print_internal(uast_param_const_unwrap(uast), indent);
    }
    unreachable("");
}
