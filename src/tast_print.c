#include <tast.h>
#include <tasts.h>
#include <llvm_utils.h>
#include <util.h>
#include <serialize.h>

#include <symbol_table.h>

static void extend_name(String* buf, Str_view name) {
    string_extend_strv_in_par(&print_arena, buf, name);
}

static void extend_lang_type_tag_to_string(String* buf, LANG_TYPE_TYPE type) {
    switch (type) {
        case LANG_TYPE_PRIMITIVE:
            string_extend_cstr(&print_arena, buf, "primitive");
            return;
        case LANG_TYPE_STRUCT:
            string_extend_cstr(&print_arena, buf, "struct");
            return;
        case LANG_TYPE_RAW_UNION:
            string_extend_cstr(&print_arena, buf, "raw_union");
            return;
        case LANG_TYPE_ENUM:
            string_extend_cstr(&print_arena, buf, "enum");
            return;
        case LANG_TYPE_SUM:
            string_extend_cstr(&print_arena, buf, "sum");
            return;
        case LANG_TYPE_TUPLE:
            string_extend_cstr(&print_arena, buf, "tuple");
            return;
        case LANG_TYPE_VOID:
            string_extend_cstr(&print_arena, buf, "void");
            return;
    }
    unreachable("");
}

void extend_serialize_lang_type_to_string(const Env* env, String* string, Lang_type lang_type, bool do_tag) {
    if (do_tag) {
        extend_lang_type_tag_to_string(string, lang_type.type);
    }

    if (lang_type.type == LANG_TYPE_TUPLE) {
        Lang_type_vec lang_types = lang_type_tuple_const_unwrap(lang_type).lang_types;
        for (size_t idx = 0; idx < lang_types.info.count; idx++) {
            extend_serialize_lang_type_to_string(env, string, vec_at(&lang_types, idx), do_tag);
        }
    } else {
        if (lang_type_get_str(lang_type).count > 1) {
            string_extend_strv(&print_arena, string, serialize_lang_type(env, lang_type));
        } else {
            string_extend_cstr(&print_arena, string, "<null>");
        }
        if (lang_type_get_pointer_depth(lang_type) < 0) {
            todo();
        }
        for (int16_t idx = 0; idx < lang_type_get_pointer_depth(lang_type); idx++) {
            vec_append(&print_arena, string, '*');
        }
    }
}

void extend_lang_type_to_string(String* string, Lang_type lang_type, bool surround_in_lt_gt, bool do_tag) {
    if (surround_in_lt_gt) {
        vec_append(&print_arena, string, '<');
    }

    if (do_tag) {
        extend_lang_type_tag_to_string(string, lang_type.type);
    }

    if (lang_type.type == LANG_TYPE_TUPLE) {
        Lang_type_vec lang_types = lang_type_tuple_const_unwrap(lang_type).lang_types;
        for (size_t idx = 0; idx < lang_types.info.count; idx++) {
            extend_lang_type_to_string(string, vec_at(&lang_types, idx), surround_in_lt_gt, do_tag);
        }
    } else {
        if (lang_type_get_str(lang_type).count > 1) {
            string_extend_strv(&print_arena, string, lang_type_get_str(lang_type));
        } else {
            string_extend_cstr(&print_arena, string, "<null>");
        }
        if (lang_type_get_pointer_depth(lang_type) < 0) {
            todo();
        }
        for (int16_t idx = 0; idx < lang_type_get_pointer_depth(lang_type); idx++) {
            vec_append(&print_arena, string, '*');
        }
        if (surround_in_lt_gt) {
            vec_append(&print_arena, string, '>');
        }
    }
}

void extend_lang_type_atom_to_string(
    String* string,
    Lang_type_atom atom,
    bool surround_in_lt_gt
) {
    if (surround_in_lt_gt) {
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
    if (surround_in_lt_gt) {
        vec_append(&print_arena, string, '>');
    }
}

static void extend_pos(String* buf, Pos pos) {
    string_extend_cstr(&print_arena, buf, "(( line:");
    string_extend_int64_t(&print_arena, buf, pos.line);
    string_extend_cstr(&print_arena, buf, " ))");
}

// TODO: make separate .c file for these
Str_view lang_type_print_internal(Lang_type lang_type, bool surround_in_lt_gt, bool do_tag) {
    String buf = {0};
    extend_lang_type_to_string(&buf, lang_type, surround_in_lt_gt, do_tag);
    string_extend_cstr(&print_arena, &buf, "\n");
    return string_to_strv(buf);
}

Str_view lang_type_atom_print_internal(Lang_type_atom atom, bool surround_in_lt_gt) {
    String buf = {0};
    // TODO: do not use `lang_type_primitive_new` here
    extend_lang_type_to_string(&buf, lang_type_primitive_const_wrap(lang_type_primitive_new(atom)), surround_in_lt_gt, true);
    return string_to_strv(buf);
}

static void extend_lang_type(String* string, Lang_type lang_type, bool surround_in_lt_gt) {
    extend_lang_type_to_string(string, lang_type, surround_in_lt_gt, true);
}

static void extend_lang_type_atom(String* string, Lang_type_atom atom, bool surround_in_lt_gt) {
    extend_lang_type_atom_to_string(string, atom, surround_in_lt_gt);
}

Str_view lang_type_vec_print_internal(Lang_type_vec types, bool surround_in_lt_gt) {
    String buf = {0};

    string_extend_cstr(&a_main, &buf, "<");
    for (size_t idx = 0; idx < types.info.count; idx++) {
        if (idx > 0) {
            string_extend_cstr(&a_main, &buf, ", ");
        }
        extend_lang_type(&buf, vec_at(&types, idx), surround_in_lt_gt);
    }
    string_extend_cstr(&a_main, &buf, ">\n");

    return string_to_strv(buf);
}

Str_view tast_binary_print_internal(const Tast_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    extend_lang_type(&buf, binary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(binary->lhs, indent));
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(binary->rhs, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_unary_print_internal(const Tast_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", indent);
    extend_lang_type(&buf, unary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(unary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(unary->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

void tast_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view tast_symbol_print_internal(const Tast_symbol* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol", indent);
    extend_pos(&buf, sym->pos);
    tast_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view tast_member_access_print_internal(const Tast_member_access* access, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_typed", indent);
    extend_lang_type(&buf, access->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, access->member_name);

    string_extend_cstr(&print_arena, &buf, "\n");

    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(access->callee, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_index_print_internal(const Tast_index* index, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_typed\n", indent);
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(index->index, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(index->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_literal_print_internal(const Tast_literal* lit, int indent) {
    switch (lit->type) {
        case TAST_NUMBER:
            return tast_number_print_internal(tast_number_const_unwrap(lit), indent);
        case TAST_STRING:
            return tast_string_print_internal(tast_string_const_unwrap(lit), indent);
        case TAST_VOID:
            return tast_void_print_internal(tast_void_const_unwrap(lit), indent);
        case TAST_ENUM_LIT:
            return tast_enum_lit_print_internal(tast_enum_lit_const_unwrap(lit), indent);
        case TAST_CHAR:
            return tast_char_print_internal(tast_char_const_unwrap(lit), indent);
        case TAST_SUM_LIT:
            return tast_sum_lit_print_internal(tast_sum_lit_const_unwrap(lit), indent);
        case TAST_UNION_LIT:
            return tast_union_lit_print_internal(tast_union_lit_const_unwrap(lit), indent);
    }
    unreachable("");
}

Str_view tast_function_call_print_internal(const Tast_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", indent);
    string_extend_strv_in_par(&print_arena, &buf, fun_call->name);
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(fun_call->lang_type, false, true));

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_text = tast_expr_print_internal(vec_at(&fun_call->args, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Str_view tast_struct_literal_print_internal(const Tast_struct_literal* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_literal", indent);
    extend_lang_type(&buf, lit->lang_type, true);
    extend_name(&buf, lit->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = tast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Str_view tast_tuple_print_internal(const Tast_tuple* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "tuple", indent);
    
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(lang_type_tuple_const_wrap(lit->lang_type), true, true));

    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = tast_expr_print_internal(vec_at(&lit->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Str_view tast_sum_callee_print_internal(const Tast_sum_callee* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "sum_callee", indent);
    
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(lit->sum_lang_type, true, true));
    string_extend_strv(&print_arena, &buf, tast_enum_lit_print_internal(lit->tag, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_sum_case_print_internal(const Tast_sum_case* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "sum_case", indent);
    
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(lit->sum_lang_type, true, true));
    string_extend_strv(&print_arena, &buf, tast_enum_lit_print_internal(lit->tag, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_sum_access_print_internal(const Tast_sum_access* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "sum_access", indent);
    
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(lit->lang_type, true, true));
    string_extend_strv(&print_arena, &buf, tast_enum_lit_print_internal(lit->tag, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(lit->callee, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_number_print_internal(const Tast_number* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", indent);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view tast_string_print_internal(const Tast_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", indent);
    extend_lang_type_atom(&buf, lit->lang_type.atom, true);
    string_extend_strv_in_par(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view tast_enum_lit_print_internal(const Tast_enum_lit* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", indent);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view tast_sum_lit_print_internal(const Tast_sum_lit* sum, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "sum_lit", indent);
    extend_lang_type(&buf, sum->lang_type, true);
    extend_pos(&buf, sum->pos);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, tast_enum_lit_print_internal(sum->tag, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(sum->item, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_union_lit_print_internal(const Tast_union_lit* sum, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "union_lit", indent);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(sum->item, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_void_print_internal(const Tast_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Str_view tast_char_print_internal(const Tast_char* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", indent);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view tast_block_print_internal(const Tast_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", indent);

    string_extend_cstr_indent(&print_arena, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    alloca_extend_table_internal(&buf, block->symbol_collection.alloca_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
    usymbol_extend_table_internal(&buf, block->symbol_collection.usymbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "symbol_table\n", indent + INDENT_WIDTH);
    symbol_extend_table_internal(&buf, block->symbol_collection.symbol_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = tast_stmt_print_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Str_view tast_function_params_print_internal(const Tast_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = tast_variable_def_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_lang_type_print_internal(const Tast_lang_type* lang_type, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", indent);
    string_extend_strv(&print_arena, &buf, lang_type_print_internal(lang_type->lang_type, false, true));

    return string_to_strv(buf);
}

Str_view tast_for_lower_bound_print_internal(const Tast_for_lower_bound* lower, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_lower_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(lower->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_for_upper_bound_print_internal(const Tast_for_upper_bound* upper, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_upper_bound\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(upper->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_condition_print_internal(const Tast_condition* cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "condition\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_operator_print_internal(cond->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_for_range_print_internal(const Tast_for_range* for_range, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_range\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_variable_def_print_internal(for_range->var_def, indent));
    string_extend_strv(&print_arena, &buf, tast_for_lower_bound_print_internal(for_range->lower_bound, indent));
    string_extend_strv(&print_arena, &buf, tast_for_upper_bound_print_internal(for_range->upper_bound, indent));
    string_extend_strv(&print_arena, &buf, tast_block_print_internal(for_range->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_for_with_cond_print_internal(const Tast_for_with_cond* for_cond, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_with_cond\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_condition_print_internal(for_cond->condition, indent));
    string_extend_strv(&print_arena, &buf, tast_block_print_internal(for_cond->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_break_print_internal(const Tast_break* lang_break, int indent) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "break\n", indent);

    return string_to_strv(buf);
}

Str_view tast_continue_print_internal(const Tast_continue* lang_continue, int indent) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "continue\n", indent);

    return string_to_strv(buf);
}

Str_view tast_assignment_print_internal(const Tast_assignment* assign, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "assignment\n", indent);
    string_extend_strv(&print_arena, &buf, tast_stmt_print_internal(assign->lhs, indent + INDENT_WIDTH));
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(assign->rhs, indent + INDENT_WIDTH));

    return string_to_strv(buf);
}

Str_view tast_if_print_internal(const Tast_if* lang_if, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_condition_print_internal(lang_if->condition, indent));
    string_extend_strv(&print_arena, &buf, tast_block_print_internal(lang_if->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_return_print_internal(const Tast_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_expr_print_internal(lang_rtn->child, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_if_else_chain_print_internal(const Tast_if_else_chain* if_else, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if_else_chain\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->tasts.info.count; idx++) {
        Str_view arg_text = tast_if_print_internal(vec_at(&if_else->tasts, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_function_decl_print_internal(const Tast_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    string_extend_strv_in_par(&print_arena, &buf, fun_decl->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, tast_lang_type_print_internal(fun_decl->return_type, indent));
    string_extend_strv(&print_arena, &buf, tast_function_params_print_internal(fun_decl->params, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_function_def_print_internal(const Tast_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, tast_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&print_arena, &buf, tast_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_struct_def_base(String* buf, const char* type_name, Struct_def_base base, int indent) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    string_extend_strv_in_par(&print_arena, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = tast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

Str_view tast_struct_def_print_internal(const Tast_struct_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view tast_raw_union_def_print_internal(const Tast_raw_union_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "raw_union_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view tast_enum_def_print_internal(const Tast_enum_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "enum_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view tast_primitive_def_print_internal(const Tast_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_string_def_print_internal(const Tast_string_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view tast_struct_lit_def_print_internal(const Tast_struct_lit_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def", indent);
    extend_lang_type(&buf, def->lang_type, true);
    extend_name(&buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = tast_expr_print_internal(vec_at(&def->members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    return string_to_strv(buf);
}

Str_view tast_literal_def_print_internal(const Tast_literal_def* def, int indent) {
    switch (def->type) {
        case TAST_STRING_DEF:
            return tast_string_def_print_internal(tast_string_def_const_unwrap(def), indent);
        case TAST_STRUCT_LIT_DEF:
            return tast_struct_lit_def_print_internal(tast_struct_lit_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view tast_sum_def_print_internal(const Tast_sum_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "sum_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view tast_variable_def_print_internal(const Tast_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view tast_operator_print_internal(const Tast_operator* operator, int indent) {
    switch (operator->type) {
        case TAST_BINARY:
            return tast_binary_print_internal(tast_binary_const_unwrap(operator), indent);
        case TAST_UNARY:
            return tast_unary_print_internal(tast_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Str_view tast_def_print_internal(const Tast_def* def, int indent) {
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
        case TAST_ENUM_DEF:
            return tast_enum_def_print_internal(tast_enum_def_const_unwrap(def), indent);
        case TAST_PRIMITIVE_DEF:
            return tast_primitive_def_print_internal(tast_primitive_def_const_unwrap(def), indent);
        case TAST_LITERAL_DEF:
            return tast_literal_def_print_internal(tast_literal_def_const_unwrap(def), indent);
        case TAST_SUM_DEF:
            return tast_sum_def_print_internal(tast_sum_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view tast_expr_print_internal(const Tast_expr* expr, int indent) {
    switch (expr->type) {
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
        case TAST_SUM_CALLEE:
            return tast_sum_callee_print_internal(tast_sum_callee_const_unwrap(expr), indent);
        case TAST_SUM_CASE:
            return tast_sum_case_print_internal(tast_sum_case_const_unwrap(expr), indent);
        case TAST_SUM_ACCESS:
            return tast_sum_access_print_internal(tast_sum_access_const_unwrap(expr), indent);
    }
    unreachable("");
}

Str_view tast_stmt_print_internal(const Tast_stmt* stmt, int indent) {
    switch (stmt->type) {
        case TAST_BLOCK:
            return tast_block_print_internal(tast_block_const_unwrap(stmt), indent);
        case TAST_EXPR:
            return tast_expr_print_internal(tast_expr_const_unwrap(stmt), indent);
        case TAST_DEF:
            return tast_def_print_internal(tast_def_const_unwrap(stmt), indent);
        case TAST_FOR_WITH_COND:
            return tast_for_with_cond_print_internal(tast_for_with_cond_const_unwrap(stmt), indent);
        case TAST_FOR_RANGE:
            return tast_for_range_print_internal(tast_for_range_const_unwrap(stmt), indent);
        case TAST_BREAK:
            return tast_break_print_internal(tast_break_const_unwrap(stmt), indent);
        case TAST_CONTINUE:
            return tast_continue_print_internal(tast_continue_const_unwrap(stmt), indent);
        case TAST_ASSIGNMENT:
            return tast_assignment_print_internal(tast_assignment_const_unwrap(stmt), indent);
        case TAST_IF_ELSE_CHAIN:
            return tast_if_else_chain_print_internal(tast_if_else_chain_const_unwrap(stmt), indent);
        case TAST_RETURN:
            return tast_return_print_internal(tast_return_const_unwrap(stmt), indent);
    }
    unreachable("");
}

Str_view tast_print_internal(const Tast* tast, int indent) {
    switch (tast->type) {
        case TAST_STMT:
            return tast_stmt_print_internal(tast_stmt_const_unwrap(tast), indent);
        case TAST_FUNCTION_PARAMS:
            return tast_function_params_print_internal(tast_function_params_const_unwrap(tast), indent);
        case TAST_LANG_TYPE:
            return tast_lang_type_print_internal(tast_lang_type_const_unwrap(tast), indent);
        case TAST_FOR_LOWER_BOUND:
            return tast_for_lower_bound_print_internal(tast_for_lower_bound_const_unwrap(tast), indent);
        case TAST_FOR_UPPER_BOUND:
            return tast_for_upper_bound_print_internal(tast_for_upper_bound_const_unwrap(tast), indent);
        case TAST_CONDITION:
            return tast_condition_print_internal(tast_condition_const_unwrap(tast), indent);
        case TAST_IF:
            return tast_if_print_internal(tast_if_const_unwrap(tast), indent);
    }
    unreachable("");
}
