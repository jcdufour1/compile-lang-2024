#include <uast.h>
#include <uast_utils.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <util.h>

#include <symbol_table.h>

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&print_arena, buf, "(( line:");
    string_extend_int64_t(&print_arena, buf, pos.line);
    string_extend_cstr(&print_arena, buf, " ))");
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

Str_view uast_binary_print_internal(const Uast_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
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
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(unary->token_type));
    extend_pos(&buf, unary->pos);
    string_extend_cstr(&print_arena, &buf, "\n");

    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(unary->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

void uast_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view uast_symbol_untyped_print_internal(const Uast_symbol_untyped* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol_untyped", indent);
    extend_pos(&buf, sym->pos);
    string_extend_strv(&print_arena, &buf, sym->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_member_access_untyped_print_internal(const Uast_member_access_untyped* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", indent);
    string_extend_strv_in_par(&print_arena, &buf, access->member_name);
    extend_pos(&buf, access->pos);
    string_extend_cstr(&print_arena, &buf, "\n");

    string_extend_strv(&print_arena, &buf, uast_expr_print_internal(access->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view uast_index_untyped_print_internal(const Uast_index_untyped* index, int indent) {
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
            return uast_number_print_internal(uast_unwrap_number_const(lit), indent);
        case UAST_STRING:
            return uast_string_print_internal(uast_unwrap_string_const(lit), indent);
        case UAST_VOID:
            return uast_void_print_internal(uast_unwrap_void_const(lit), indent);
        case UAST_ENUM_LIT:
            return uast_enum_lit_print_internal(uast_unwrap_enum_lit_const(lit), indent);
        case UAST_CHAR:
            return uast_char_print_internal(uast_unwrap_char_const(lit), indent);
    }
    unreachable("");
}

Str_view uast_function_call_print_internal(const Uast_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", indent);
    string_extend_strv_in_par(&print_arena, &buf, fun_call->name);
    string_extend_cstr(&print_arena, &buf, "\n");

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
        Str_view arg_text = uast_variable_def_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_lang_type_print_internal(const Uast_lang_type* lang_type, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", indent);
    string_extend_strv(&print_arena, &buf, lang_type_vec_print_internal(lang_type->lang_type, false));

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

static void extend_ustruct_def_base(String* buf, const char* type_name, Ustruct_def_base base, int indent) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    string_extend_strv_in_par(&print_arena, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = uast_stmt_print_internal(vec_at(&base.members, idx), indent);
        string_extend_strv(&print_arena, buf, memb_text);
    }
    indent -= INDENT_WIDTH;
}

Str_view uast_struct_def_print_internal(const Uast_struct_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view uast_raw_union_def_print_internal(const Uast_raw_union_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "raw_union_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view uast_enum_def_print_internal(const Uast_enum_def* def, int indent) {
    String buf = {0};

    extend_ustruct_def_base(&buf, "enum_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view uast_primitive_def_print_internal(const Uast_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
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

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def\n", indent);
    string_extend_strv_indent(&print_arena, &buf, def->name, indent);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = uast_print_internal(vec_at(&def->members, idx), indent);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view uast_literal_def_print_internal(const Uast_literal_def* def, int indent) {
    switch (def->type) {
        case UAST_STRING_DEF:
            return uast_string_def_print_internal(uast_unwrap_string_def_const(def), indent);
        case UAST_STRUCT_LIT_DEF:
            return uast_struct_lit_def_print_internal(uast_unwrap_struct_lit_def_const(def), indent);
    }
    unreachable("");
}

Str_view uast_variable_def_print_internal(const Uast_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view uast_operator_print_internal(const Uast_operator* operator, int indent) {
    switch (operator->type) {
        case UAST_BINARY:
            return uast_binary_print_internal(uast_unwrap_binary_const(operator), indent);
        case UAST_UNARY:
            return uast_unary_print_internal(uast_unwrap_unary_const(operator), indent);
    }
    unreachable("");
}

Str_view uast_def_print_internal(const Uast_def* def, int indent) {
    switch (def->type) {
        case UAST_FUNCTION_DEF:
            return uast_function_def_print_internal(uast_unwrap_function_def_const(def), indent);
        case UAST_FUNCTION_DECL:
            return uast_function_decl_print_internal(uast_unwrap_function_decl_const(def), indent);
        case UAST_VARIABLE_DEF:
            return uast_variable_def_print_internal(uast_unwrap_variable_def_const(def), indent);
        case UAST_STRUCT_DEF:
            return uast_struct_def_print_internal(uast_unwrap_struct_def_const(def), indent);
        case UAST_RAW_UNION_DEF:
            return uast_raw_union_def_print_internal(uast_unwrap_raw_union_def_const(def), indent);
        case UAST_ENUM_DEF:
            return uast_enum_def_print_internal(uast_unwrap_enum_def_const(def), indent);
        case UAST_PRIMITIVE_DEF:
            return uast_primitive_def_print_internal(uast_unwrap_primitive_def_const(def), indent);
        case UAST_LITERAL_DEF:
            return uast_literal_def_print_internal(uast_unwrap_literal_def_const(def), indent);
    }
    unreachable("");
}

Str_view uast_expr_print_internal(const Uast_expr* expr, int indent) {
    switch (expr->type) {
        case UAST_OPERATOR:
            return uast_operator_print_internal(uast_unwrap_operator_const(expr), indent);
        case UAST_SYMBOL_UNTYPED:
            return uast_symbol_untyped_print_internal(uast_unwrap_symbol_untyped_const(expr), indent);
        case UAST_MEMBER_ACCESS_UNTYPED:
            return uast_member_access_untyped_print_internal(uast_unwrap_member_access_untyped_const(expr), indent);
        case UAST_INDEX_UNTYPED:
            return uast_index_untyped_print_internal(uast_unwrap_index_untyped_const(expr), indent);
        case UAST_LITERAL:
            return uast_literal_print_internal(uast_unwrap_literal_const(expr), indent);
        case UAST_FUNCTION_CALL:
            return uast_function_call_print_internal(uast_unwrap_function_call_const(expr), indent);
        case UAST_STRUCT_LITERAL:
            return uast_struct_literal_print_internal(uast_unwrap_struct_literal_const(expr), indent);
        case UAST_TUPLE:
            return uast_tuple_print_internal(uast_unwrap_tuple_const(expr), indent);
    }
    unreachable("");
}

Str_view uast_stmt_print_internal(const Uast_stmt* stmt, int indent) {
    switch (stmt->type) {
        case UAST_BLOCK:
            return uast_block_print_internal(uast_unwrap_block_const(stmt), indent);
        case UAST_EXPR:
            return uast_expr_print_internal(uast_unwrap_expr_const(stmt), indent);
        case UAST_DEF:
            return uast_def_print_internal(uast_unwrap_def_const(stmt), indent);
        case UAST_BREAK:
            return uast_break_print_internal(uast_unwrap_break_const(stmt), indent);
        case UAST_CONTINUE:
            return uast_continue_print_internal(uast_unwrap_continue_const(stmt), indent);
        case UAST_ASSIGNMENT:
            return uast_assignment_print_internal(uast_unwrap_assignment_const(stmt), indent);
        case UAST_RETURN:
            return uast_return_print_internal(uast_unwrap_return_const(stmt), indent);
        case UAST_IF_ELSE_CHAIN:
            return uast_if_else_chain_print_internal(uast_unwrap_if_else_chain_const(stmt), indent);
        case UAST_FOR_RANGE:
            return uast_for_range_print_internal(uast_unwrap_for_range_const(stmt), indent);
        case UAST_FOR_WITH_COND:
            return uast_for_with_cond_print_internal(uast_unwrap_for_with_cond_const(stmt), indent);
    }
    unreachable("");
}

Str_view uast_print_internal(const Uast* uast, int indent) {
    switch (uast->type) {
        case UAST_STMT:
            return uast_stmt_print_internal(uast_unwrap_stmt_const(uast), indent);
        case UAST_FUNCTION_PARAMS:
            return uast_function_params_print_internal(uast_unwrap_function_params_const(uast), indent);
        case UAST_LANG_TYPE:
            return uast_lang_type_print_internal(uast_unwrap_lang_type_const(uast), indent);
        case UAST_FOR_LOWER_BOUND:
            return uast_for_lower_bound_print_internal(uast_unwrap_for_lower_bound_const(uast), indent);
        case UAST_FOR_UPPER_BOUND:
            return uast_for_upper_bound_print_internal(uast_unwrap_for_upper_bound_const(uast), indent);
        case UAST_CONDITION:
            return uast_condition_print_internal(uast_unwrap_condition_const(uast), indent);
        case UAST_IF:
            return uast_if_print_internal(uast_unwrap_if_const(uast), indent);
    }
    unreachable("");
}
