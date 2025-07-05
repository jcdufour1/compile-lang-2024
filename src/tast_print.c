#include <tast.h>
#include <ir_utils.h>
#include <util.h>
#include <tast_serialize.h>
#include <lang_type_print.h>

#include <symbol_table.h>

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&a_print, buf, "(( line:");
    string_extend_int64_t(&a_print, buf, pos.line);
    string_extend_cstr(&a_print, buf, " ))");
}

Strv tast_binary_print_internal(const Tast_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "binary", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, binary->lang_type);
    string_extend_strv(&a_print, &buf, binary_type_to_strv(binary->token_type));
    string_extend_cstr(&a_print, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(binary->lhs, indent));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(binary->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_unary_print_internal(const Tast_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "unary", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, unary->lang_type);
    string_extend_strv(&a_print, &buf, unary_type_to_strv(unary->token_type));
    string_extend_cstr(&a_print, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(unary->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

void tast_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type_to_string(string, LANG_TYPE_MODE_LOG, base.lang_type);
    extend_name(NAME_LOG, string, base.name);
    string_extend_cstr(&a_print, string, "\n");
}

Strv tast_symbol_print_internal(const Tast_symbol* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "symbol", indent);
    extend_pos(&buf, sym->pos);
    tast_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Strv tast_member_access_print_internal(const Tast_member_access* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "member_access_typed", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, access->lang_type);
    string_extend_strv_in_par(&a_print, &buf, access->member_name);

    string_extend_cstr(&a_print, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(access->callee, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_index_print_internal(const Tast_index* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "index_typed\n", indent);
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_literal_print_internal(const Tast_literal* lit, int indent) {
    switch (lit->type) {
        case TAST_INT:
            return tast_int_print_internal(tast_int_const_unwrap(lit), indent);
        case TAST_FLOAT:
            return tast_float_print_internal(tast_float_const_unwrap(lit), indent);
        case TAST_STRING:
            return tast_string_print_internal(tast_string_const_unwrap(lit), indent);
        case TAST_VOID:
            return tast_void_print_internal(tast_void_const_unwrap(lit), indent);
        case TAST_ENUM_TAG_LIT:
            return tast_enum_tag_lit_print_internal(tast_enum_tag_lit_const_unwrap(lit), indent);
        case TAST_CHAR:
            return tast_char_print_internal(tast_char_const_unwrap(lit), indent);
        case TAST_ENUM_LIT:
            return tast_enum_lit_print_internal(tast_enum_lit_const_unwrap(lit), indent);
        case TAST_RAW_UNION_LIT:
            return tast_raw_union_lit_print_internal(tast_raw_union_lit_const_unwrap(lit), indent);
        case TAST_FUNCTION_LIT:
            return tast_function_lit_print_internal(tast_function_lit_const_unwrap(lit), indent);
    }
    unreachable("");
}

Strv tast_function_call_print_internal(const Tast_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_call", indent);
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, fun_call->lang_type));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(fun_call->callee, indent + INDENT_WIDTH));

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Strv arg_text = tast_expr_print_internal(vec_at(&fun_call->args, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Strv tast_struct_literal_print_internal(const Tast_struct_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "struct_literal", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, lit->lang_type);
    extend_name(NAME_LOG, &buf, lit->name);
    string_extend_cstr(&a_print, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Strv memb_text = tast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Strv tast_tuple_print_internal(const Tast_tuple* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "tuple", indent);
    
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, lang_type_tuple_const_wrap(lit->lang_type)));

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Strv memb_text = tast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Strv tast_enum_callee_print_internal(const Tast_enum_callee* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_callee", indent);
    
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, lit->enum_lang_type));
    string_extend_strv(&a_print, &buf, tast_enum_tag_lit_print_internal(lit->tag, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_enum_case_print_internal(const Tast_enum_case* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_case", indent);
    
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, lit->enum_lang_type));
    string_extend_strv(&a_print, &buf, tast_enum_tag_lit_print_internal(lit->tag, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_enum_access_print_internal(const Tast_enum_access* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_access", indent);
    
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, lit->lang_type));
    string_extend_strv(&a_print, &buf, tast_enum_tag_lit_print_internal(lit->tag, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(lit->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_enum_get_tag_print_internal(const Tast_enum_get_tag* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_get_tag\n", indent);
    
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(lit->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_int_print_internal(const Tast_int* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "number", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    string_extend_int64_t(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_float_print_internal(const Tast_float* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "float", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    string_extend_double(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_string_print_internal(const Tast_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "string", indent);
    string_extend_strv_in_par(&a_print, &buf, lit->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_enum_tag_lit_print_internal(const Tast_enum_tag_lit* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_tag_lit", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    string_extend_int64_t(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_enum_lit_print_internal(const Tast_enum_lit* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "enum_lit", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, lit->enum_lang_type);
    extend_pos(&buf, lit->pos);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, tast_enum_tag_lit_print_internal(lit->tag, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(lit->item, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_raw_union_lit_print_internal(const Tast_raw_union_lit* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "raw_union_lit", indent);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(lit->item, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_function_lit_print_internal(const Tast_function_lit* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_lit", indent);
    extend_name(NAME_LOG, &buf, lit->name);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, lit->lang_type);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_void_print_internal(const Tast_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Strv tast_char_print_internal(const Tast_char* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "char", indent);
    vec_append(&a_print, &buf, num->data);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_block_print_internal(const Tast_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "block\n", indent);

    string_extend_cstr_indent(&a_print, &buf, "lang_type: ", indent + INDENT_WIDTH);
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, block->lang_type));

    string_extend_cstr_indent(&a_print, &buf, "block_scope: ", indent + INDENT_WIDTH);
    string_extend_size_t(&a_print, &buf, block->scope_id);
    string_extend_cstr(&a_print, &buf, "\n");

    string_extend_cstr_indent(&a_print, &buf, "parent_block_scope: ", indent + INDENT_WIDTH);
    string_extend_size_t(&a_print, &buf, scope_get_parent_tbl_lookup(block->scope_id));
    string_extend_cstr(&a_print, &buf, "\n");

    //string_extend_cstr_indent(&a_print, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    //alloca_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).alloca_table, indent + 2*INDENT_WIDTH);

    //string_extend_cstr_indent(&a_print, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
    //usymbol_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).usymbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&a_print, &buf, "symbol_table\n", indent + INDENT_WIDTH);
    symbol_extend_table_internal(&buf, vec_at(&env.symbol_tables, block->scope_id).symbol_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Strv arg_text = tast_stmt_print_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Strv tast_function_params_print_internal(const Tast_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Strv arg_text = tast_variable_def_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&a_print, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_condition_print_internal(const Tast_condition* cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "condition\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_operator_print_internal(cond->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_for_with_cond_print_internal(const Tast_for_with_cond* for_cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "for_with_cond\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_condition_print_internal(for_cond->condition, indent));
    string_extend_strv(&a_print, &buf, tast_block_print_internal(for_cond->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_break_print_internal(const Tast_break* lang_break, int indent) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "break\n", indent);

    return string_to_strv(buf);
}

Strv tast_yield_print_internal(const Tast_yield* yield, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "yield\n", indent);
    string_extend_cstr_indent(&a_print, &buf, "break_out_of: ", indent + INDENT_WIDTH);
    extend_name(NAME_LOG, &buf, yield->break_out_of);
    string_extend_cstr(&a_print, &buf, "\n");
    if (yield->do_yield_expr) {
        string_extend_strv(&a_print, &buf, tast_expr_print_internal(yield->yield_expr, indent + INDENT_WIDTH));
    }

    return string_to_strv(buf);
}

Strv tast_continue2_print_internal(const Tast_continue2* cont, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "continue2\n", indent);
    string_extend_cstr_indent(&a_print, &buf, "break_out_of: ", indent + INDENT_WIDTH);
    extend_name(NAME_LOG, &buf, cont->break_out_of);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_continue_print_internal(const Tast_continue* lang_continue, int indent) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "continue\n", indent);

    return string_to_strv(buf);
}

Strv tast_assignment_print_internal(const Tast_assignment* assign, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "assignment\n", indent);
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(assign->lhs, indent + INDENT_WIDTH));
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(assign->rhs, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_if_print_internal(const Tast_if* lang_if, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "if\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_condition_print_internal(lang_if->condition, indent));
    string_extend_strv(&a_print, &buf, tast_block_print_internal(lang_if->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_return_print_internal(const Tast_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "return\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_expr_print_internal(lang_rtn->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_label_print_internal(const Tast_label* label, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "label", indent);
    extend_name(NAME_LOG, &buf, label->name);
    extend_pos(&buf, label->pos);
    string_extend_cstr(&a_main, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_defer_print_internal(const Tast_defer* defer, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "defer", indent);
    string_extend_strv(&a_print, &buf, tast_stmt_print_internal(defer->child, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Strv tast_if_else_chain_print_internal(const Tast_if_else_chain* if_else, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "if_else_chain\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        Strv arg_text = tast_if_print_internal(vec_at(&if_else->tasts, idx), indent);
        string_extend_strv(&a_print, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_module_alias_print_internal(const Tast_module_alias* alias, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "module_alias", indent);
    extend_name(NAME_LOG, &buf, alias->alias_name);
    string_extend_strv(&a_print, &buf, alias->mod_path);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_function_decl_print_internal(const Tast_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    extend_name(NAME_LOG, &buf, fun_decl->name);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_decl->return_type);
    string_extend_cstr(&a_print, &buf, "\n");
    string_extend_strv(&a_print, &buf, tast_function_params_print_internal(fun_decl->params, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_function_def_print_internal(const Tast_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "function_def", indent);
    extend_pos(&buf, fun_def->pos);
    string_extend_cstr(&a_print, &buf, "\n");
    indent += INDENT_WIDTH;
    string_extend_strv(&a_print, &buf, tast_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&a_print, &buf, tast_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_struct_def_base(String* buf, const char* type_name, Struct_def_base base, int indent) {
    string_extend_cstr_indent(&a_print, buf, type_name, indent);
    extend_name(NAME_LOG, buf, base.name);
    string_extend_cstr(&a_print, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Strv memb_text = tast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, buf, memb_text);
    }
}

Strv tast_struct_def_print_internal(const Tast_struct_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Strv tast_raw_union_def_print_internal(const Tast_raw_union_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "raw_union_def", def->base, indent);

    return string_to_strv(buf);
}

Strv tast_primitive_def_print_internal(const Tast_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_cstr(&a_print, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Strv tast_enum_def_print_internal(const Tast_enum_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "enum_def", def->base, indent);

    return string_to_strv(buf);
}

Strv tast_variable_def_print_internal(const Tast_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&a_print, &buf, "variable_def", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    extend_name(NAME_LOG, &buf, def->name);
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

Strv tast_operator_print_internal(const Tast_operator* operator, int indent) {
    switch (operator->type) {
        case TAST_BINARY:
            return tast_binary_print_internal(tast_binary_const_unwrap(operator), indent);
        case TAST_UNARY:
            return tast_unary_print_internal(tast_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Strv tast_def_print_internal(const Tast_def* def, int indent) {
    switch (def->type) {
        case TAST_FUNCTION_DEF:
            return tast_function_def_print_internal(tast_function_def_const_unwrap(def), indent);
        case TAST_FUNCTION_DECL:
            return tast_function_decl_print_internal(tast_function_decl_const_unwrap(def), indent);
        case TAST_VARIABLE_DEF:
            return tast_variable_def_print_internal(tast_variable_def_const_unwrap(def), indent);
        case TAST_STRUCT_DEF:
            return tast_struct_def_print_internal(tast_struct_def_const_unwrap(def), indent);
        case TAST_RAW_UNION_DEF:
            return tast_raw_union_def_print_internal(tast_raw_union_def_const_unwrap(def), indent);
        case TAST_PRIMITIVE_DEF:
            return tast_primitive_def_print_internal(tast_primitive_def_const_unwrap(def), indent);
        case TAST_ENUM_DEF:
            return tast_enum_def_print_internal(tast_enum_def_const_unwrap(def), indent);
        case TAST_IMPORT:
            todo();
        case TAST_LABEL:
            return tast_label_print_internal(tast_label_const_unwrap(def), indent);
    }
    unreachable("");
}

Strv tast_expr_print_internal(const Tast_expr* expr, int indent) {
    switch (expr->type) {
        case TAST_BLOCK:
            return tast_block_print_internal(tast_block_const_unwrap(expr), indent);
        case TAST_OPERATOR:
            return tast_operator_print_internal(tast_operator_const_unwrap(expr), indent);
        case TAST_SYMBOL:
            return tast_symbol_print_internal(tast_symbol_const_unwrap(expr), indent);
        case TAST_MEMBER_ACCESS:
            return tast_member_access_print_internal(tast_member_access_const_unwrap(expr), indent);
        case TAST_INDEX:
            return tast_index_print_internal(tast_index_const_unwrap(expr), indent);
        case TAST_LITERAL:
            return tast_literal_print_internal(tast_literal_const_unwrap(expr), indent);
        case TAST_FUNCTION_CALL:
            return tast_function_call_print_internal(tast_function_call_const_unwrap(expr), indent);
        case TAST_STRUCT_LITERAL:
            return tast_struct_literal_print_internal(tast_struct_literal_const_unwrap(expr), indent);
        case TAST_TUPLE:
            return tast_tuple_print_internal(tast_tuple_const_unwrap(expr), indent);
        case TAST_ENUM_CALLEE:
            return tast_enum_callee_print_internal(tast_enum_callee_const_unwrap(expr), indent);
        case TAST_ENUM_CASE:
            return tast_enum_case_print_internal(tast_enum_case_const_unwrap(expr), indent);
        case TAST_ENUM_ACCESS:
            return tast_enum_access_print_internal(tast_enum_access_const_unwrap(expr), indent);
        case TAST_ENUM_GET_TAG:
            return tast_enum_get_tag_print_internal(tast_enum_get_tag_const_unwrap(expr), indent);
        case TAST_ASSIGNMENT:
            return tast_assignment_print_internal(tast_assignment_const_unwrap(expr), indent);
        case TAST_IF_ELSE_CHAIN:
            return tast_if_else_chain_print_internal(tast_if_else_chain_const_unwrap(expr), indent);
        case TAST_MODULE_ALIAS:
            return tast_module_alias_print_internal(tast_module_alias_const_unwrap(expr), indent);
    }
    unreachable("");
}

Strv tast_stmt_print_internal(const Tast_stmt* stmt, int indent) {
    switch (stmt->type) {
        case TAST_EXPR:
            return tast_expr_print_internal(tast_expr_const_unwrap(stmt), indent);
        case TAST_DEF:
            return tast_def_print_internal(tast_def_const_unwrap(stmt), indent);
        case TAST_FOR_WITH_COND:
            return tast_for_with_cond_print_internal(tast_for_with_cond_const_unwrap(stmt), indent);
        case TAST_BREAK:
            return tast_break_print_internal(tast_break_const_unwrap(stmt), indent);
        case TAST_YIELD:
            return tast_yield_print_internal(tast_yield_const_unwrap(stmt), indent);
        case TAST_CONTINUE:
            return tast_continue_print_internal(tast_continue_const_unwrap(stmt), indent);
        case TAST_RETURN:
            return tast_return_print_internal(tast_return_const_unwrap(stmt), indent);
        case TAST_DEFER:
            return tast_defer_print_internal(tast_defer_const_unwrap(stmt), indent);
        case TAST_CONTINUE2:
            return tast_continue2_print_internal(tast_continue2_const_unwrap(stmt), indent);
    }
    unreachable("");
}

Strv tast_print_internal(const Tast* tast, int indent) {
    switch (tast->type) {
        case TAST_STMT:
            return tast_stmt_print_internal(tast_stmt_const_unwrap(tast), indent);
        case TAST_FUNCTION_PARAMS:
            return tast_function_params_print_internal(tast_function_params_const_unwrap(tast), indent);
        case TAST_CONDITION:
            return tast_condition_print_internal(tast_condition_const_unwrap(tast), indent);
        case TAST_IF:
            return tast_if_print_internal(tast_if_const_unwrap(tast), indent);
    }
    unreachable("");
}
