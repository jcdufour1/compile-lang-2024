#include <node.h>
#include <nodes.h>
#include <util.h>

#include <symbol_table.h>

void extend_lang_type_to_string(Arena* arena, String* string, Lang_type lang_type, bool surround_in_lt_gt) {
    if (surround_in_lt_gt) {
        vec_append(arena, string, '<');
    }

    if (lang_type.str.count > 1) {
        string_extend_strv(arena, string, lang_type.str);
    } else {
        string_extend_cstr(arena, string, "<null>");
    }
    if (lang_type.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < lang_type.pointer_depth; idx++) {
        vec_append(arena, string, '*');
    }
    if (surround_in_lt_gt) {
        vec_append(arena, string, '>');
    }
}

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&print_arena, buf, "(( line:");
    string_extend_int64_t(&print_arena, buf, pos.line);
    string_extend_cstr(&print_arena, buf, " ))");
}

Str_view lang_type_print_internal(Arena* arena, Lang_type lang_type, bool surround_in_lt_gt) {
    String buf = {0};
    extend_lang_type_to_string(arena, &buf, lang_type, surround_in_lt_gt);
    Str_view str_view = {.str = buf.buf, .count = buf.info.count};
    return str_view;
}

static void extend_lang_type(String* string, Lang_type lang_type, bool surround_in_lt_gt) {
    if (surround_in_lt_gt) {
        vec_append(&print_arena, string, '<');
    }

    if (lang_type.str.count > 1) {
        string_extend_strv(&print_arena, string, lang_type.str);
    } else {
        string_extend_cstr(&print_arena, string, "<null>");
    }
    if (lang_type.pointer_depth < 0) {
        todo();
    }
    for (int16_t idx = 0; idx < lang_type.pointer_depth; idx++) {
        vec_append(&print_arena, string, '*');
    }
    if (surround_in_lt_gt) {
        vec_append(&print_arena, string, '>');
    }
}

Str_view node_binary_print_internal(const Node_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    extend_lang_type(&buf, binary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(binary->lhs, indent));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(binary->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_unary_print_internal(const Node_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", indent);
    extend_lang_type(&buf, unary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(unary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(unary->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

void node_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view node_primitive_sym_print_internal(const Node_primitive_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_sym", indent);
    extend_pos(&buf, sym->pos);
    node_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_struct_sym_print_internal(const Node_struct_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_sym", indent);
    node_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_raw_union_sym_print_internal(const Node_raw_union_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_sym", indent);
    node_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_enum_sym_print_internal(const Node_enum_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_sym", indent);
    node_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_symbol_typed_print_internal(const Node_symbol_typed* sym, int indent) {
    switch (sym->type) {
        case NODE_PRIMITIVE_SYM:
            return node_primitive_sym_print_internal(node_unwrap_primitive_sym_const(sym), indent);
        case NODE_STRUCT_SYM:
            return node_struct_sym_print_internal(node_unwrap_struct_sym_const(sym), indent);
        case NODE_RAW_UNION_SYM:
            return node_raw_union_sym_print_internal(node_unwrap_raw_union_sym_const(sym), indent);
        case NODE_ENUM_SYM:
            return node_enum_sym_print_internal(node_unwrap_enum_sym_const(sym), indent);
    }
    unreachable("");
}

Str_view node_symbol_untyped_print_internal(const Node_symbol_untyped* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol_untyped", indent);
    string_extend_strv(&print_arena, &buf, sym->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_member_access_untyped_print_internal(const Node_member_access_untyped* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", indent);
    string_extend_strv_in_par(&print_arena, &buf, access->member_name);

    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(access->callee, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_member_access_typed_print_internal(const Node_member_access_typed* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", indent);
    extend_lang_type(&buf, access->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, access->member_name);

    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(access->callee, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_index_untyped_print_internal(const Node_index_untyped* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_untyped\n", indent);
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view node_index_typed_print_internal(const Node_index_typed* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_typed\n", indent);
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view node_literal_print_internal(const Node_literal* lit, int indent) {
    switch (lit->type) {
        case NODE_NUMBER:
            return node_number_print_internal(node_unwrap_number_const(lit), indent);
        case NODE_STRING:
            return node_string_print_internal(node_unwrap_string_const(lit), indent);
        case NODE_VOID:
            return node_void_print_internal(node_unwrap_void_const(lit), indent);
        case NODE_ENUM_LIT:
            return node_enum_lit_print_internal(node_unwrap_enum_lit_const(lit), indent);
        case NODE_CHAR:
            return node_char_print_internal(node_unwrap_char_const(lit), indent);
    }
    unreachable("");
}

Str_view node_function_call_print_internal(const Node_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", indent);
    string_extend_strv_in_par(&print_arena, &buf, fun_call->name);
    extend_lang_type(&buf, fun_call->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_text = node_expr_print_internal(vec_at(&fun_call->args, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_struct_literal_print_internal(const Node_struct_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_literal", indent);
    string_extend_strv(&print_arena, &buf, lit->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;

    extend_lang_type(&buf, lit->lang_type, true);
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&lit->members, idx), indent);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_number_print_internal(const Node_number* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", indent);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_string_print_internal(const Node_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", indent);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_enum_lit_print_internal(const Node_enum_lit* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", indent);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_void_print_internal(const Node_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Str_view node_char_print_internal(const Node_char* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", indent);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_block_print_internal(const Node_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", indent);
    // TODO: extend table here to buf instead of this
    symbol_log_table(LOG_DEBUG, block->symbol_collection.symbol_table);
    alloca_log_table(LOG_DEBUG, block->symbol_collection.alloca_table);

    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = node_print_internal(vec_at(&block->children, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_params_print_internal(const Node_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = node_variable_def_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_lang_type_print_internal(const Node_lang_type* lang_type, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", indent);
    extend_lang_type(&buf, lang_type->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_for_lower_bound_print_internal(const Node_for_lower_bound* lower, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_lower_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(lower->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_upper_bound_print_internal(const Node_for_upper_bound* upper, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_upper_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(upper->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_condition_print_internal(const Node_condition* cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "condition\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_operator_print_internal(cond->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_range_print_internal(const Node_for_range* for_range, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_range\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_variable_def_print_internal(for_range->var_def, indent));
    string_extend_strv(&print_arena, &buf, node_for_lower_bound_print_internal(for_range->lower_bound, indent));
    string_extend_strv(&print_arena, &buf, node_for_upper_bound_print_internal(for_range->upper_bound, indent));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(for_range->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_with_cond_print_internal(const Node_for_with_cond* for_cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_with_cond\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_condition_print_internal(for_cond->condition, indent));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(for_cond->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_break_print_internal(const Node_break* lang_break, int indent) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "break\n", indent);

    return string_to_strv(buf);
}

Str_view node_continue_print_internal(const Node_continue* lang_continue, int indent) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "continue\n", indent);

    return string_to_strv(buf);
}

Str_view node_assignment_print_internal(const Node_assignment* assign, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "assignment\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_print_internal(assign->lhs, indent));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(assign->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_if_print_internal(const Node_if* lang_if, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_condition_print_internal(lang_if->condition, indent));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(lang_if->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_return_print_internal(const Node_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(lang_rtn->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_if_else_chain_print_internal(const Node_if_else_chain* if_else, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if_else_chain\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
        Str_view arg_text = node_if_print_internal(vec_at(&if_else->nodes, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_decl_print_internal(const Node_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    string_extend_strv_in_par(&print_arena, &buf, fun_decl->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, node_function_params_print_internal(fun_decl->params, indent));
    string_extend_strv(&print_arena, &buf, node_lang_type_print_internal(fun_decl->return_type, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_def_print_internal(const Node_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_struct_def_base(String* buf, const char* type_name, Struct_def_base base, int indent) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    string_extend_strv_in_par(&print_arena, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&base.members, idx), indent);
        string_extend_strv(&print_arena, buf, memb_text);
    }
    indent -= INDENT_WIDTH;
}

Str_view node_struct_def_print_internal(const Node_struct_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view node_raw_union_def_print_internal(const Node_raw_union_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "raw_union_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view node_enum_def_print_internal(const Node_enum_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "enum_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view node_primitive_def_print_internal(const Node_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_string_def_print_internal(const Node_string_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_struct_lit_def_print_internal(const Node_struct_lit_def* def, int indent) {
    String buf = {0};

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def\n", indent);
    string_extend_strv_indent(&print_arena, &buf, def->name, indent);
    extend_lang_type(&buf, def->lang_type, true);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&def->members, idx), indent);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_literal_def_print_internal(const Node_literal_def* def, int indent) {
    switch (def->type) {
        case NODE_STRING_DEF:
            return node_string_def_print_internal(node_unwrap_string_def_const(def), indent);
        case NODE_STRUCT_LIT_DEF:
            return node_struct_lit_def_print_internal(node_unwrap_struct_lit_def_const(def), indent);
    }
    unreachable("");
}

Str_view node_variable_def_print_internal(const Node_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_operator_print_internal(const Node_operator* operator, int indent) {
    switch (operator->type) {
        case NODE_BINARY:
            return node_binary_print_internal(node_unwrap_binary_const(operator), indent);
        case NODE_UNARY:
            return node_unary_print_internal(node_unwrap_unary_const(operator), indent);
    }
    unreachable("");
}

Str_view node_def_print_internal(const Node_def* def, int indent) {
    switch (def->type) {
        case NODE_FUNCTION_DEF:
            return node_function_def_print_internal(node_unwrap_function_def_const(def), indent);
        case NODE_FUNCTION_DECL:
            return node_function_decl_print_internal(node_unwrap_function_decl_const(def), indent);
        case NODE_VARIABLE_DEF:
            return node_variable_def_print_internal(node_unwrap_variable_def_const(def), indent);
        case NODE_STRUCT_DEF:
            return node_struct_def_print_internal(node_unwrap_struct_def_const(def), indent);
        case NODE_RAW_UNION_DEF:
            return node_raw_union_def_print_internal(node_unwrap_raw_union_def_const(def), indent);
        case NODE_ENUM_DEF:
            return node_enum_def_print_internal(node_unwrap_enum_def_const(def), indent);
        case NODE_PRIMITIVE_DEF:
            return node_primitive_def_print_internal(node_unwrap_primitive_def_const(def), indent);
        case NODE_LITERAL_DEF:
            return node_literal_def_print_internal(node_unwrap_literal_def_const(def), indent);
    }
    unreachable("");
}

Str_view node_expr_print_internal(const Node_expr* expr, int indent) {
    switch (expr->type) {
        case NODE_OPERATOR:
            return node_operator_print_internal(node_unwrap_operator_const(expr), indent);
        case NODE_SYMBOL_TYPED:
            return node_symbol_typed_print_internal(node_unwrap_symbol_typed_const(expr), indent);
        case NODE_SYMBOL_UNTYPED:
            return node_symbol_untyped_print_internal(node_unwrap_symbol_untyped_const(expr), indent);
        case NODE_MEMBER_ACCESS_UNTYPED:
            return node_member_access_untyped_print_internal(node_unwrap_member_access_untyped_const(expr), indent);
        case NODE_MEMBER_ACCESS_TYPED:
            return node_member_access_typed_print_internal(node_unwrap_member_access_typed_const(expr), indent);
        case NODE_INDEX_UNTYPED:
            return node_index_untyped_print_internal(node_unwrap_index_untyped_const(expr), indent);
        case NODE_INDEX_TYPED:
            return node_index_typed_print_internal(node_unwrap_index_typed_const(expr), indent);
        case NODE_LITERAL:
            return node_literal_print_internal(node_unwrap_literal_const(expr), indent);
        case NODE_FUNCTION_CALL:
            return node_function_call_print_internal(node_unwrap_function_call_const(expr), indent);
        case NODE_STRUCT_LITERAL:
            return node_struct_literal_print_internal(node_unwrap_struct_literal_const(expr), indent);
    }
    unreachable("");
}

Str_view node_print_internal(const Node* node, int indent) {
    switch (node->type) {
        case NODE_BLOCK:
            return node_block_print_internal(node_unwrap_block_const(node), indent);
        case NODE_EXPR:
            return node_expr_print_internal(node_unwrap_expr_const(node), indent);
        case NODE_FUNCTION_PARAMS:
            return node_function_params_print_internal(node_unwrap_function_params_const(node), indent);
        case NODE_LANG_TYPE:
            return node_lang_type_print_internal(node_unwrap_lang_type_const(node), indent);
        case NODE_FOR_LOWER_BOUND:
            return node_for_lower_bound_print_internal(node_unwrap_for_lower_bound_const(node), indent);
        case NODE_FOR_UPPER_BOUND:
            return node_for_upper_bound_print_internal(node_unwrap_for_upper_bound_const(node), indent);
        case NODE_DEF:
            return node_def_print_internal(node_unwrap_def_const(node), indent);
        case NODE_CONDITION:
            return node_condition_print_internal(node_unwrap_condition_const(node), indent);
        case NODE_FOR_RANGE:
            return node_for_range_print_internal(node_unwrap_for_range_const(node), indent);
        case NODE_FOR_WITH_COND:
            return node_for_with_cond_print_internal(node_unwrap_for_with_cond_const(node), indent);
        case NODE_BREAK:
            return node_break_print_internal(node_unwrap_break_const(node), indent);
        case NODE_CONTINUE:
            return node_continue_print_internal(node_unwrap_continue_const(node), indent);
        case NODE_ASSIGNMENT:
            return node_assignment_print_internal(node_unwrap_assignment_const(node), indent);
        case NODE_IF:
            return node_if_print_internal(node_unwrap_if_const(node), indent);
        case NODE_RETURN:
            return node_return_print_internal(node_unwrap_return_const(node), indent);
        case NODE_IF_ELSE_CHAIN:
            return node_if_else_chain_print_internal(node_unwrap_if_else_chain_const(node), indent);
    }
    unreachable("");
}
