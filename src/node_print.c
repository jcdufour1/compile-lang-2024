#include <node.h>
#include <nodes.h>
#include <util.h>

static int recursion_depth = 0;

typedef enum {
    INDENT_WIDTH = 2,
} INDENT_WIDTH_;

void node_print_assert_recursion_depth_zero(void) {
    assert(recursion_depth == 0);
}

void extend_lang_type(String* string, Lang_type lang_type, bool surround_in_lt_gt) {
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

Str_view node_binary_print_internal(const Node_binary* binary) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", recursion_depth);
    extend_lang_type(&buf, binary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(binary->lhs));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(binary->rhs));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_unary_print_internal(const Node_unary* unary) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", recursion_depth);
    extend_lang_type(&buf, unary->lang_type, true);

    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, token_type_to_str_view(unary->token_type), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(unary->child), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

void extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view node_primitive_sym_print_internal(const Node_primitive_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_sym", recursion_depth);
    extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_struct_sym_print_internal(const Node_struct_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_sym", recursion_depth);
    extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_raw_union_sym_print_internal(const Node_raw_union_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_sym", recursion_depth);
    extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_enum_sym_print_internal(const Node_enum_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_sym", recursion_depth);
    extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view node_symbol_typed_print_internal(const Node_symbol_typed* sym) {
    switch (sym->type) {
        case NODE_PRIMITIVE_SYM:
            return node_primitive_sym_print_internal(node_unwrap_primitive_sym_const(sym));
        case NODE_STRUCT_SYM:
            return node_struct_sym_print_internal(node_unwrap_struct_sym_const(sym));
        case NODE_RAW_UNION_SYM:
            return node_raw_union_sym_print_internal(node_unwrap_raw_union_sym_const(sym));
        case NODE_ENUM_SYM:
            return node_enum_sym_print_internal(node_unwrap_enum_sym_const(sym));
    }
    unreachable("");
}

Str_view node_symbol_untyped_print_internal(const Node_symbol_untyped* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol_untyped", recursion_depth);
    string_extend_strv(&print_arena, &buf, sym->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_member_access_untyped_print_internal(const Node_member_access_untyped* access) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", recursion_depth);
    string_extend_strv(&print_arena, &buf, access->member_name);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(access->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_member_access_typed_print_internal(const Node_member_access_typed* access) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_typed", recursion_depth);
    string_extend_strv(&print_arena, &buf, access->member_name);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(access->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_index_untyped_print_internal(const Node_index_untyped* index) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_untyped", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(index->index), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(index->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_index_typed_print_internal(const Node_index_typed* index) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_typed", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(index->index), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, node_expr_print_internal(index->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_literal_print_internal(const Node_literal* lit) {
    switch (lit->type) {
        case NODE_NUMBER:
            return node_number_print_internal(node_unwrap_number_const(lit));
        case NODE_STRING:
            return node_string_print_internal(node_unwrap_string_const(lit));
        case NODE_VOID:
            return node_void_print_internal(node_unwrap_void_const(lit));
        case NODE_ENUM_LIT:
            return node_enum_lit_print_internal(node_unwrap_enum_lit_const(lit));
        case NODE_CHAR:
            return node_char_print_internal(node_unwrap_char_const(lit));
    }
    unreachable("");
}

Str_view node_function_call_print_internal(const Node_function_call* fun_call) {
    String buf = {0};

    recursion_depth += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "function_call", recursion_depth);
    string_extend_strv_in_par(&print_arena, &buf, fun_call->name);
    extend_lang_type(&buf, fun_call->lang_type, true);
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_text = node_expr_print_internal(vec_at(&fun_call->args, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_struct_literal_print_internal(const Node_struct_literal* lit) {
    String buf = {0};

    recursion_depth += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_literal", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, lit->name, recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&lit->members, idx));
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_number_print_internal(const Node_number* num) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", recursion_depth);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_string_print_internal(const Node_string* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_enum_lit_print_internal(const Node_enum_lit* num) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", recursion_depth);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_void_print_internal(const Node_void* num) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view node_char_print_internal(const Node_char* num) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", recursion_depth);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_llvm_placeholder_print_internal(const Node_llvm_placeholder* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "llvm_placeholder\n", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_load_element_ptr_print_internal(const Node_load_element_ptr* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_element_ptr\n", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_block_print_internal(const Node_block* block) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = node_print_internal(vec_at(&block->children, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_params_print_internal(const Node_function_params* function_params) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = node_variable_def_print_internal(vec_at(&function_params->params, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_lang_type_print_internal(const Node_lang_type* lang_type) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", recursion_depth);
    extend_lang_type(&buf, lang_type->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_for_lower_bound_print_internal(const Node_for_lower_bound* lower) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_lower_bound\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(lower->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_upper_bound_print_internal(const Node_for_upper_bound* upper) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_upper_bound\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(upper->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_condition_print_internal(const Node_condition* cond) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "condition\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_operator_print_internal(cond->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_range_print_internal(const Node_for_range* for_range) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_range\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_variable_def_print_internal(for_range->var_def));
    string_extend_strv(&print_arena, &buf, node_for_lower_bound_print_internal(for_range->lower_bound));
    string_extend_strv(&print_arena, &buf, node_for_upper_bound_print_internal(for_range->upper_bound));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(for_range->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_for_with_cond_print_internal(const Node_for_with_cond* for_cond) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_with_cond\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_condition_print_internal(for_cond->condition));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(for_cond->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_break_print_internal(const Node_break* lang_break) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "break\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view node_continue_print_internal(const Node_continue* lang_continue) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "continue\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view node_assignment_print_internal(const Node_assignment* assign) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "assignment\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_print_internal(assign->lhs));
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(assign->rhs));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_if_print_internal(const Node_if* lang_if) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_condition_print_internal(lang_if->condition));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(lang_if->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_return_print_internal(const Node_return* lang_rtn) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_expr_print_internal(lang_rtn->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_goto_print_internal(const Node_goto* lang_goto) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "goto\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, lang_goto->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_cond_goto_print_internal(const Node_cond_goto* cond_goto) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "cond_goto\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, cond_goto->if_true, recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, cond_goto->if_false, recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_alloca_print_internal(const Node_alloca* alloca) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "alloca", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, alloca->lang_type, true);
    string_extend_strv(&print_arena, &buf, alloca->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_load_another_node_print_internal(const Node_load_another_node* load) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_another_node\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, load->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_store_another_node_print_internal(const Node_store_another_node* store) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "store_another_node", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, store->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_if_else_chain_print_internal(const Node_if_else_chain* if_else) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if_else_chain\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->nodes.info.count; idx++) {
        Str_view arg_text = node_if_print_internal(vec_at(&if_else->nodes, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_decl_print_internal(const Node_function_decl* fun_decl) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, fun_decl->name);
    string_extend_strv(&print_arena, &buf, node_function_params_print_internal(fun_decl->parameters));
    string_extend_strv(&print_arena, &buf, node_lang_type_print_internal(fun_decl->return_type));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_function_def_print_internal(const Node_function_def* fun_def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, node_function_decl_print_internal(fun_def->declaration));
    string_extend_strv(&print_arena, &buf, node_block_print_internal(fun_def->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_struct_def_base(String* buf, Struct_def_base base) {
    string_extend_strv(&print_arena, buf, base.name);

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&base.members, idx));
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

Str_view node_struct_def_print_internal(const Node_struct_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_raw_union_def_print_internal(const Node_raw_union_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_enum_def_print_internal(const Node_enum_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_primitive_def_print_internal(const Node_primitive_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_string_def_print_internal(const Node_string_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_struct_lit_def_print_internal(const Node_struct_lit_def* def) {
    String buf = {0};

    recursion_depth += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def\n", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, def->name, recursion_depth);
    extend_lang_type(&buf, def->lang_type, true);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = node_print_internal(vec_at(&def->members, idx));
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view node_label_print_internal(const Node_label* label) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "label\n", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, label->name, recursion_depth);
    string_extend_cstr_indent(&print_arena, &buf, "\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view node_literal_def_print_internal(const Node_literal_def* def) {
    switch (def->type) {
        case NODE_STRING_DEF:
            return node_string_def_print_internal(node_unwrap_string_def_const(def));
        case NODE_STRUCT_LIT_DEF:
            return node_struct_lit_def_print_internal(node_unwrap_struct_lit_def_const(def));
    }
    unreachable("");
}

Str_view node_variable_def_print_internal(const Node_variable_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", recursion_depth);
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view node_operator_print_internal(const Node_operator* operator) {
    switch (operator->type) {
        case NODE_BINARY:
            return node_binary_print_internal(node_unwrap_binary_const(operator));
        case NODE_UNARY:
            return node_unary_print_internal(node_unwrap_unary_const(operator));
    }
    unreachable("");
}

Str_view node_def_print_internal(const Node_def* def) {
    switch (def->type) {
        case NODE_FUNCTION_DEF:
            return node_function_def_print_internal(node_unwrap_function_def_const(def));
        case NODE_FUNCTION_DECL:
            return node_function_decl_print_internal(node_unwrap_function_decl_const(def));
        case NODE_VARIABLE_DEF:
            return node_variable_def_print_internal(node_unwrap_variable_def_const(def));
        case NODE_STRUCT_DEF:
            return node_struct_def_print_internal(node_unwrap_struct_def_const(def));
        case NODE_RAW_UNION_DEF:
            return node_raw_union_def_print_internal(node_unwrap_raw_union_def_const(def));
        case NODE_ENUM_DEF:
            return node_enum_def_print_internal(node_unwrap_enum_def_const(def));
        case NODE_PRIMITIVE_DEF:
            return node_primitive_def_print_internal(node_unwrap_primitive_def_const(def));
        case NODE_LABEL:
            return node_label_print_internal(node_unwrap_label_const(def));
        case NODE_LITERAL_DEF:
            return node_literal_def_print_internal(node_unwrap_literal_def_const(def));
    }
    unreachable("");
}

Str_view node_expr_print_internal(const Node_expr* expr) {
    switch (expr->type) {
        case NODE_OPERATOR:
            return node_operator_print_internal(node_unwrap_operator_const(expr));
        case NODE_SYMBOL_TYPED:
            return node_symbol_typed_print_internal(node_unwrap_symbol_typed_const(expr));
        case NODE_SYMBOL_UNTYPED:
            return node_symbol_untyped_print_internal(node_unwrap_symbol_untyped_const(expr));
        case NODE_MEMBER_ACCESS_UNTYPED:
            return node_member_access_untyped_print_internal(node_unwrap_member_access_untyped_const(expr));
        case NODE_MEMBER_ACCESS_TYPED:
            return node_member_access_typed_print_internal(node_unwrap_member_access_typed_const(expr));
        case NODE_INDEX_UNTYPED:
            return node_index_untyped_print_internal(node_unwrap_index_untyped_const(expr));
        case NODE_INDEX_TYPED:
            return node_index_typed_print_internal(node_unwrap_index_typed_const(expr));
        case NODE_LITERAL:
            return node_literal_print_internal(node_unwrap_literal_const(expr));
        case NODE_FUNCTION_CALL:
            return node_function_call_print_internal(node_unwrap_function_call_const(expr));
        case NODE_STRUCT_LITERAL:
            return node_struct_literal_print_internal(node_unwrap_struct_literal_const(expr));
        case NODE_LLVM_PLACEHOLDER:
            return node_llvm_placeholder_print_internal(node_unwrap_llvm_placeholder_const(expr));
    }
    unreachable("");
}

Str_view node_print_internal(const Node* node) {
    switch (node->type) {
        case NODE_BLOCK:
            return node_block_print_internal(node_unwrap_block_const(node));
        case NODE_EXPR:
            return node_expr_print_internal(node_unwrap_expr_const(node));
        case NODE_LOAD_ELEMENT_PTR:
            return node_load_element_ptr_print_internal(node_unwrap_load_element_ptr_const(node));
        case NODE_FUNCTION_PARAMS:
            return node_function_params_print_internal(node_unwrap_function_params_const(node));
        case NODE_LANG_TYPE:
            return node_lang_type_print_internal(node_unwrap_lang_type_const(node));
        case NODE_FOR_LOWER_BOUND:
            return node_for_lower_bound_print_internal(node_unwrap_for_lower_bound_const(node));
        case NODE_FOR_UPPER_BOUND:
            return node_for_upper_bound_print_internal(node_unwrap_for_upper_bound_const(node));
        case NODE_DEF:
            return node_def_print_internal(node_unwrap_def_const(node));
        case NODE_CONDITION:
            return node_condition_print_internal(node_unwrap_condition_const(node));
        case NODE_FOR_RANGE:
            return node_for_range_print_internal(node_unwrap_for_range_const(node));
        case NODE_FOR_WITH_COND:
            return node_for_with_cond_print_internal(node_unwrap_for_with_cond_const(node));
        case NODE_BREAK:
            return node_break_print_internal(node_unwrap_break_const(node));
        case NODE_CONTINUE:
            return node_continue_print_internal(node_unwrap_continue_const(node));
        case NODE_ASSIGNMENT:
            return node_assignment_print_internal(node_unwrap_assignment_const(node));
        case NODE_IF:
            return node_if_print_internal(node_unwrap_if_const(node));
        case NODE_RETURN:
            return node_return_print_internal(node_unwrap_return_const(node));
        case NODE_GOTO:
            return node_goto_print_internal(node_unwrap_goto_const(node));
        case NODE_COND_GOTO:
            return node_cond_goto_print_internal(node_unwrap_cond_goto_const(node));
        case NODE_ALLOCA:
            return node_alloca_print_internal(node_unwrap_alloca_const(node));
        case NODE_LOAD_ANOTHER_NODE:
            return node_load_another_node_print_internal(node_unwrap_load_another_node_const(node));
        case NODE_STORE_ANOTHER_NODE:
            return node_store_another_node_print_internal(node_unwrap_store_another_node_const(node));
        case NODE_IF_ELSE_CHAIN:
            return node_if_else_chain_print_internal(node_unwrap_if_else_chain_const(node));
    }
    unreachable("");
}
