#include <uast.h>
#include <uast_utils.h>
#include <tast_utils.h>
#include <ir_utils.h>
#include <util.h>
#include <ulang_type.h>
#include <lang_type_print.h>

#include <symbol_table.h>

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&a_print, buf, "(( line:");
    string_extend_int64_t(&a_print, buf, pos.line);
    string_extend_cstr(&a_print, buf, " ))");
}

Strv uast_binary_print_internal(const Uast_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "binary", indent);
    string_extend_strv(&a_print, &buf, binary_type_to_strv(binary->token_type));
    extend_pos(&buf, binary->pos);
    string_extend_cstr(&a_print, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(binary->lhs, indent));
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(binary->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_unary_print_internal(const Uast_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "unary", indent);
    string_extend_strv(&a_print, &buf, unary_type_to_strv(unary->token_type));
    extend_pos(&buf, unary->pos);
    string_extend_cstr(&a_print, &buf, "\n");

    string_extend_strv(&a_print, &buf, uast_expr_print_internal(unary->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_symbol_print_internal(const Uast_symbol* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "symbol_untyped", indent);
    extend_pos(&buf, sym->pos);
    extend_name(NAME_LOG, &buf, sym->name);
    for (size_t idx = 0; idx < sym->name.gen_args.info.count; idx++) {
        extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, vec_at(&sym->name.gen_args, idx));
    }
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_member_access_print_internal(const Uast_member_access* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "member_access_untyped", indent);
    extend_pos(&buf, access->pos);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(access->callee, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, uast_symbol_print_internal(access->member_name, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_index_print_internal(const Uast_index* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "index_untyped", indent);
    extend_pos(&buf, index->pos);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_literal_print_internal(const Uast_literal* lit, int indent) {
    switch (lit->type) {
        case UAST_INT:
            return uast_int_print_internal(uast_int_const_unwrap(lit), indent);
        case UAST_FLOAT:
            return uast_float_print_internal(uast_float_const_unwrap(lit), indent);
        case UAST_STRING:
            return uast_string_print_internal(uast_string_const_unwrap(lit), indent);
        case UAST_VOID:
            return uast_void_print_internal(uast_void_const_unwrap(lit), indent);
        case UAST_CHAR:
            return uast_char_print_internal(uast_char_const_unwrap(lit), indent);
    }
    unreachable("");
}

Strv uast_function_call_print_internal(const Uast_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_call\n", indent);
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(fun_call->callee, indent + INDENT_WIDTH));

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Strv arg_text = uast_expr_print_internal(vec_at(&fun_call->args, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Strv uast_struct_literal_print_internal(const Uast_struct_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "struct_literal", indent);
    string_extend_cstr(&a_print, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Strv memb_text = uast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Strv uast_array_literal_print_internal(const Uast_array_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "array_literal", indent);
    string_extend_cstr(&a_print, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Strv memb_text = uast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Strv uast_tuple_print_internal(const Uast_tuple* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "tuple", indent);
    string_extend_cstr(&a_print, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Strv memb_text = uast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Strv uast_enum_access_print_internal(const Uast_enum_access* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_access", indent);
    
    string_extend_strv(&a_print, &buf, tast_enum_tag_lit_print_internal(access->tag, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(access->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_enum_get_tag_print_internal(const Uast_enum_get_tag* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_get_tag", indent);
    
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(access->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_unknown_print_internal(const Uast_unknown* unknown, int indent) {
    (void) unknown;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "unknown\n", indent);
    
    return string_to_strv(buf);
}

Strv uast_int_print_internal(const Uast_int* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "number", indent);
    string_extend_int64_t(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_float_print_internal(const Uast_float* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "float", indent);
    string_extend_double(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_string_print_internal(const Uast_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "string", indent);
    string_extend_strv_in_par(&a_print, &buf, lit->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_void_print_internal(const Uast_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Strv uast_char_print_internal(const Uast_char* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "char", indent);
    vec_append(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_block_print_internal(const Uast_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "block\n", indent);

    string_extend_cstr_indent(&a_print, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
    usymbol_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).usymbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&a_print, &buf, "symbol_table\n", indent + INDENT_WIDTH);
    symbol_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).symbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&a_print, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    alloca_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).alloca_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Strv arg_text = uast_stmt_print_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Strv uast_function_params_print_internal(const Uast_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Strv arg_text = uast_param_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&a_print, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_for_lower_bound_print_internal(const Uast_for_lower_bound* lower, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "for_lower_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(lower->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_for_upper_bound_print_internal(const Uast_for_upper_bound* upper, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "for_upper_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(upper->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_condition_print_internal(const Uast_condition* cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "condition\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_operator_print_internal(cond->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_for_with_cond_print_internal(const Uast_for_with_cond* for_cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "for_with_cond\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_condition_print_internal(for_cond->condition, indent));
    string_extend_strv(&a_print, &buf, uast_block_print_internal(for_cond->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_switch_print_internal(const Uast_switch* lang_switch, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "switch\n", indent);
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(lang_switch->operand, indent + INDENT_WIDTH));
    for (size_t idx = 0; idx < lang_switch->cases.info.count; idx++) {
        string_extend_strv(&a_print, &buf, uast_case_print_internal(vec_at(&lang_switch->cases, idx), indent + INDENT_WIDTH));
    }

    return string_to_strv(buf);
}

Strv uast_defer_print_internal(const Uast_defer* defer, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "defer", indent);
    string_extend_strv(&a_print, &buf, uast_stmt_print_internal(defer->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_break_print_internal(const Uast_break* lang_break, int indent) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "break\n", indent);

    return string_to_strv(buf);
}

Strv uast_continue_print_internal(const Uast_continue* lang_continue, int indent) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "continue\n", indent);

    return string_to_strv(buf);
}

Strv uast_assignment_print_internal(const Uast_assignment* assign, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "assignment\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(assign->lhs, indent));
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(assign->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_if_print_internal(const Uast_if* lang_if, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "if\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_condition_print_internal(lang_if->condition, indent));
    string_extend_strv(&a_print, &buf, uast_block_print_internal(lang_if->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_case_print_internal(const Uast_case* lang_case, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "case\n", indent);
    if (lang_case->is_default) {
        string_extend_cstr_indent(&a_print, &buf, "default\n", indent + INDENT_WIDTH);
    } else {
        string_extend_strv(&a_print, &buf, uast_expr_print_internal(lang_case->expr, indent + INDENT_WIDTH));
    }
    string_extend_strv(&a_print, &buf, uast_stmt_print_internal(lang_case->if_true, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_param_print_internal(const Uast_param* param, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "param", indent);
    extend_pos(&buf, param->pos);
    string_extend_cstr_indent(&a_print, &buf, "\n", indent);
    string_extend_strv(&a_print, &buf, uast_variable_def_print_internal(param->base, indent + INDENT_WIDTH));
    if (param->is_optional) {
        string_extend_strv(&a_print, &buf, uast_expr_print_internal(param->optional_default, indent + INDENT_WIDTH));
    }
    if (param->is_variadic) {
        string_extend_cstr_indent(&a_print, &buf, "is_variadic\n", indent + INDENT_WIDTH);
    }

    return string_to_strv(buf);
}

Strv uast_label_print_internal(const Uast_label* label, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "label", indent);
    extend_pos(&buf, label->pos);
    extend_name(NAME_LOG, &buf, label->name);

    return string_to_strv(buf);
}
Strv uast_import_path_print_internal(const Uast_import_path* import, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "import_path", indent);
    extend_name(NAME_LOG, &buf, import->mod_path);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, uast_block_print_internal(import->block, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_mod_alias_print_internal(const Uast_mod_alias* alias, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "mod_alias", indent);
    string_extend_cstr(&a_print, &buf, "(");
    extend_name(NAME_LOG, &buf, alias->name);
    string_extend_cstr(&a_print, &buf, ")");
    string_extend_cstr(&a_print, &buf, "(");
    extend_name(NAME_LOG, &buf, alias->mod_path);
    string_extend_cstr(&a_print, &buf, ")");
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_generic_param_print_internal(const Uast_generic_param* params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "generic_params\n", indent);
    string_extend_strv(&a_print, &buf, uast_symbol_print_internal(params->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_poison_def_print_internal(const Uast_poison_def* poison, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "poison_def", indent);
    extend_name(NAME_LOG, &buf, poison->name);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_lang_def_print_internal(const Uast_lang_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "lang_def", indent);
    extend_name(NAME_LOG, &buf, def->alias_name);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(def->expr, indent + INDENT_WIDTH));
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_void_def_print_internal(const Uast_void_def* def, int indent) {
    (void) def;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "void_def\n", indent);

    return string_to_strv(buf);
}

Strv uast_return_print_internal(const Uast_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "return\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_expr_print_internal(lang_rtn->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_macro_print_internal(const Uast_macro* macro, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "macro", indent);
    string_extend_strv(&a_print, &buf, macro->name);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_if_else_chain_print_internal(const Uast_if_else_chain* if_else, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "if_else_chain\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->uasts.info.count; idx++) {
        Strv arg_text = uast_if_print_internal(vec_at(&if_else->uasts, idx), indent);
        string_extend_strv(&a_print, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_function_decl_print_internal(const Uast_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_decl", indent);
    extend_name(NAME_LOG, &buf, fun_decl->name);
    extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_decl->return_type);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, uast_function_params_print_internal(fun_decl->params, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv uast_function_def_print_internal(const Uast_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, uast_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&a_print, &buf, uast_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_ustruct_def_base(String* buf, const char* type_name, Ustruct_def_base base, int indent, Pos pos) {
    string_extend_cstr_indent(&a_print, buf, type_name, indent);
    extend_pos(buf, pos);
    extend_name(NAME_LOG, buf, base.name);
    string_extend_cstr(&a_print, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Strv memb_text = uast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, buf, memb_text);
    }
}

Strv ustruct_def_base_print_internal(Ustruct_def_base base, int indent) {
    String buf = {0};
    extend_ustruct_def_base(&buf, "<unknown>", base, indent, (Pos) {0});
    return string_to_strv(buf);
}

Strv uast_struct_def_print_internal(const Uast_struct_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "struct_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Strv uast_raw_union_def_print_internal(const Uast_raw_union_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "raw_union_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Strv uast_enum_def_print_internal(const Uast_enum_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "enum_def", def->base, indent, def->pos);

    return string_to_strv(buf);
}

Strv uast_primitive_def_print_internal(const Uast_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_cstr(&a_print, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv uast_variable_def_print_internal(const Uast_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "variable_def", indent);
    extend_ulang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    extend_name(NAME_LOG, &buf, def->name);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv uast_operator_print_internal(const Uast_operator* operator, int indent) {
    switch (operator->type) {
        case UAST_BINARY:
            return uast_binary_print_internal(uast_binary_const_unwrap(operator), indent);
        case UAST_UNARY:
            return uast_unary_print_internal(uast_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Strv uast_def_print_internal(const Uast_def* def, int indent) {
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
        case UAST_PRIMITIVE_DEF:
            return uast_primitive_def_print_internal(uast_primitive_def_const_unwrap(def), indent);
        case UAST_IMPORT_PATH:
            return uast_import_path_print_internal(uast_import_path_const_unwrap(def), indent);
        case UAST_MOD_ALIAS:
            return uast_mod_alias_print_internal(uast_mod_alias_const_unwrap(def), indent);
        case UAST_GENERIC_PARAM:
            return uast_generic_param_print_internal(uast_generic_param_const_unwrap(def), indent);
        case UAST_POISON_DEF:
            return uast_poison_def_print_internal(uast_poison_def_const_unwrap(def), indent);
        case UAST_LANG_DEF:
            return uast_lang_def_print_internal(uast_lang_def_const_unwrap(def), indent);
        case UAST_VOID_DEF:
            return uast_void_def_print_internal(uast_void_def_const_unwrap(def), indent);
        case UAST_LABEL:
            return uast_label_print_internal(uast_label_const_unwrap(def), indent);
    }
    unreachable("");
}

Strv uast_expr_print_internal(const Uast_expr* expr, int indent) {
    if (!expr) {
        return sv("<nothing>\n");
    }
    switch (expr->type) {
        case UAST_OPERATOR:
            return uast_operator_print_internal(uast_operator_const_unwrap(expr), indent);
        case UAST_SYMBOL:
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
        case UAST_ARRAY_LITERAL:
            return uast_array_literal_print_internal(uast_array_literal_const_unwrap(expr), indent);
        case UAST_TUPLE:
            return uast_tuple_print_internal(uast_tuple_const_unwrap(expr), indent);
        case UAST_ENUM_ACCESS:
            return uast_enum_access_print_internal(uast_enum_access_const_unwrap(expr), indent);
        case UAST_UNKNOWN:
            return uast_unknown_print_internal(uast_unknown_const_unwrap(expr), indent);
        case UAST_ENUM_GET_TAG:
            return uast_enum_get_tag_print_internal(uast_enum_get_tag_const_unwrap(expr), indent);
        case UAST_SWITCH:
            return uast_switch_print_internal(uast_switch_const_unwrap(expr), indent);
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_print_internal(uast_if_else_chain_const_unwrap(expr), indent);
        case UAST_MACRO:
            return uast_macro_print_internal(uast_macro_const_unwrap(expr), indent);
    }
    unreachable("");
}

Strv uast_stmt_print_internal(const Uast_stmt* stmt, int indent) {
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
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_print_internal(uast_for_with_cond_const_unwrap(stmt), indent);
        case UAST_DEFER:
            return uast_defer_print_internal(uast_defer_const_unwrap(stmt), indent);
    }
    unreachable("");
}

Strv uast_print_internal(const Uast* uast, int indent) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_print_internal(uast_stmt_const_unwrap(uast), indent);
        case UAST_FUNCTION_PARAMS:
            return uast_function_params_print_internal(uast_function_params_const_unwrap(uast), indent);
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
