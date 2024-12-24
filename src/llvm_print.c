#include <llvm.h>
#include <llvms.h>
#include <node.h>
#include <llvms.h>
#include <util.h>

static int recursion_depth = 0;

typedef enum {
    INDENT_WIDTH = 2,
} INDENT_WIDTH_;

void llvm_print_assert_recursion_depth_zero(void) {
    assert(recursion_depth == 0);
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

Str_view llvm_binary_print_internal(const Llvm_binary* binary) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", recursion_depth);
    extend_lang_type(&buf, binary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
    string_extend_cstr(&print_arena, &buf, "\n");

    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(binary->lhs));
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(binary->rhs));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_unary_print_internal(const Llvm_unary* unary) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", recursion_depth);
    extend_lang_type(&buf, unary->lang_type, true);

    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, token_type_to_str_view(unary->token_type), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(unary->child), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

void llvm_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view llvm_primitive_sym_print_internal(const Llvm_primitive_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_sym", recursion_depth);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_struct_sym_print_internal(const Llvm_struct_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_sym", recursion_depth);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_raw_union_sym_print_internal(const Llvm_raw_union_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_sym", recursion_depth);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_enum_sym_print_internal(const Llvm_enum_sym* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_sym", recursion_depth);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_symbol_typed_print_internal(const Llvm_symbol_typed* sym) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_print_internal(llvm_unwrap_primitive_sym_const(sym));
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_print_internal(llvm_unwrap_struct_sym_const(sym));
        case LLVM_RAW_UNION_SYM:
            return llvm_raw_union_sym_print_internal(llvm_unwrap_raw_union_sym_const(sym));
        case LLVM_ENUM_SYM:
            return llvm_enum_sym_print_internal(llvm_unwrap_enum_sym_const(sym));
    }
    unreachable("");
}

Str_view llvm_symbol_untyped_print_internal(const Llvm_symbol_untyped* sym) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "symbol_untyped", recursion_depth);
    string_extend_strv(&print_arena, &buf, sym->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_member_access_untyped_print_internal(const Llvm_member_access_untyped* access) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_untyped", recursion_depth);
    string_extend_strv(&print_arena, &buf, access->member_name);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(access->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_member_access_typed_print_internal(const Llvm_member_access_typed* access) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "member_access_typed", recursion_depth);
    string_extend_strv(&print_arena, &buf, access->member_name);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(access->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_index_untyped_print_internal(const Llvm_index_untyped* index) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_untyped", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(index->index), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(index->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_index_typed_print_internal(const Llvm_index_typed* index) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "index_typed", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(index->index), recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, llvm_expr_print_internal(index->callee), recursion_depth);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_literal_print_internal(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_print_internal(llvm_unwrap_number_const(lit));
        case LLVM_STRING:
            return llvm_string_print_internal(llvm_unwrap_string_const(lit));
        case LLVM_VOID:
            return llvm_void_print_internal(llvm_unwrap_void_const(lit));
        case LLVM_ENUM_LIT:
            return llvm_enum_lit_print_internal(llvm_unwrap_enum_lit_const(lit));
        case LLVM_CHAR:
            return llvm_char_print_internal(llvm_unwrap_char_const(lit));
    }
    unreachable("");
}

Str_view llvm_function_call_print_internal(const Llvm_function_call* fun_call) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", recursion_depth);
    string_extend_strv_in_par(&print_arena, &buf, fun_call->name);
    extend_lang_type(&buf, fun_call->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        Str_view arg_text = llvm_expr_print_internal(vec_at(&fun_call->args, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_struct_literal_print_internal(const Llvm_struct_literal* lit) {
    String buf = {0};

    recursion_depth += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_literal", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, lit->name, recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    for (size_t idx = 0; idx < lit->members.info.count; idx++) {
        Str_view memb_text = llvm_print_internal(vec_at(&lit->members, idx));
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_number_print_internal(const Llvm_number* num) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", recursion_depth);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_string_print_internal(const Llvm_string* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_enum_lit_print_internal(const Llvm_enum_lit* num) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", recursion_depth);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_void_print_internal(const Llvm_void* num) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view llvm_char_print_internal(const Llvm_char* num) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", recursion_depth);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_llvm_placeholder_print_internal(const Llvm_llvm_placeholder* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "llvm_placeholder", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_load_element_ptr_print_internal(const Llvm_load_element_ptr* lit) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_element_ptr", recursion_depth);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_block_print_internal(const Llvm_block* block) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = llvm_print_internal(vec_at(&block->children, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_function_params_print_internal(const Llvm_function_params* function_params) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = llvm_variable_def_print_internal(vec_at(&function_params->params, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_lang_type_print_internal(const Llvm_lang_type* lang_type) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", recursion_depth);
    extend_lang_type(&buf, lang_type->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_for_lower_bound_print_internal(const Llvm_for_lower_bound* lower) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_lower_bound\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(lower->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_for_upper_bound_print_internal(const Llvm_for_upper_bound* upper) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_upper_bound\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(upper->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_condition_print_internal(const Llvm_condition* cond) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "condition\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_operator_print_internal(cond->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_for_range_print_internal(const Llvm_for_range* for_range) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_range\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_variable_def_print_internal(for_range->var_def));
    string_extend_strv(&print_arena, &buf, llvm_for_lower_bound_print_internal(for_range->lower_bound));
    string_extend_strv(&print_arena, &buf, llvm_for_upper_bound_print_internal(for_range->upper_bound));
    string_extend_strv(&print_arena, &buf, llvm_block_print_internal(for_range->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_for_with_cond_print_internal(const Llvm_for_with_cond* for_cond) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "for_with_cond\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_condition_print_internal(for_cond->condition));
    string_extend_strv(&print_arena, &buf, llvm_block_print_internal(for_cond->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_break_print_internal(const Llvm_break* lang_break) {
    (void) lang_break;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "break\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view llvm_continue_print_internal(const Llvm_continue* lang_continue) {
    (void) lang_continue;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "continue\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view llvm_assignment_print_internal(const Llvm_assignment* assign) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "assignment\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_print_internal(assign->lhs));
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(assign->rhs));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_if_print_internal(const Llvm_if* lang_if) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_condition_print_internal(lang_if->condition));
    string_extend_strv(&print_arena, &buf, llvm_block_print_internal(lang_if->body));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_return_print_internal(const Llvm_return* lang_rtn) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_expr_print_internal(lang_rtn->child));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_goto_print_internal(const Llvm_goto* lang_goto) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "goto\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, lang_goto->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_cond_goto_print_internal(const Llvm_cond_goto* cond_goto) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "cond_goto", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_in_par(&print_arena, &buf, cond_goto->if_true);
    string_extend_strv_in_par(&print_arena, &buf, cond_goto->if_false);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_alloca_print_internal(const Llvm_alloca* alloca) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "alloca", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, alloca->lang_type, true);
    string_extend_strv(&print_arena, &buf, alloca->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_load_another_llvm_print_internal(const Llvm_load_another_llvm* load) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_another_llvm", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, load->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_store_another_llvm_print_internal(const Llvm_store_another_llvm* store) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "store_another_llvm", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, store->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_if_else_chain_print_internal(const Llvm_if_else_chain* if_else) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "if_else_chain\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    for (size_t idx = 0; idx < if_else->llvms.info.count; idx++) {
        Str_view arg_text = llvm_if_print_internal(vec_at(&if_else->llvms, idx));
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_function_decl_print_internal(const Llvm_function_decl* fun_decl) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv_in_par(&print_arena, &buf, fun_decl->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, llvm_function_params_print_internal(fun_decl->parameters));
    string_extend_strv(&print_arena, &buf, llvm_lang_type_print_internal(fun_decl->return_type));
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_function_def_print_internal(const Llvm_function_def* fun_def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_function_decl_print_internal(fun_def->declaration));
    string_extend_strv(&print_arena, &buf, llvm_block_print_internal(fun_def->body));
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

Str_view llvm_struct_def_print_internal(const Llvm_struct_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_raw_union_def_print_internal(const Llvm_raw_union_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_enum_def_print_internal(const Llvm_enum_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_struct_def_base(&buf, def->base);
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_primitive_def_print_internal(const Llvm_primitive_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_string_def_print_internal(const Llvm_string_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", recursion_depth);
    recursion_depth += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_struct_lit_def_print_internal(const Llvm_struct_lit_def* def) {
    String buf = {0};

    recursion_depth += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def\n", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, def->name, recursion_depth);
    extend_lang_type(&buf, def->lang_type, true);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = llvm_print_internal(vec_at(&def->members, idx));
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    recursion_depth -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_label_print_internal(const Llvm_label* label) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "label\n", recursion_depth);
    string_extend_strv_indent(&print_arena, &buf, label->name, recursion_depth);
    string_extend_cstr_indent(&print_arena, &buf, "\n", recursion_depth);

    return string_to_strv(buf);
}

Str_view llvm_literal_def_print_internal(const Llvm_literal_def* def) {
    switch (def->type) {
        case LLVM_STRING_DEF:
            return llvm_string_def_print_internal(llvm_unwrap_string_def_const(def));
        case LLVM_STRUCT_LIT_DEF:
            return llvm_struct_lit_def_print_internal(llvm_unwrap_struct_lit_def_const(def));
    }
    unreachable("");
}

Str_view llvm_variable_def_print_internal(const Llvm_variable_def* def) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", recursion_depth);
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_strv_in_par(&print_arena, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_operator_print_internal(const Llvm_operator* operator) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_binary_print_internal(llvm_unwrap_binary_const(operator));
        case LLVM_UNARY:
            return llvm_unary_print_internal(llvm_unwrap_unary_const(operator));
    }
    unreachable("");
}

Str_view llvm_def_print_internal(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_print_internal(llvm_unwrap_function_def_const(def));
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_print_internal(llvm_unwrap_function_decl_const(def));
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_print_internal(llvm_unwrap_variable_def_const(def));
        case LLVM_STRUCT_DEF:
            return llvm_struct_def_print_internal(llvm_unwrap_struct_def_const(def));
        case LLVM_RAW_UNION_DEF:
            return llvm_raw_union_def_print_internal(llvm_unwrap_raw_union_def_const(def));
        case LLVM_ENUM_DEF:
            return llvm_enum_def_print_internal(llvm_unwrap_enum_def_const(def));
        case LLVM_PRIMITIVE_DEF:
            return llvm_primitive_def_print_internal(llvm_unwrap_primitive_def_const(def));
        case LLVM_LABEL:
            return llvm_label_print_internal(llvm_unwrap_label_const(def));
        case LLVM_LITERAL_DEF:
            return llvm_literal_def_print_internal(llvm_unwrap_literal_def_const(def));
    }
    unreachable("");
}

Str_view llvm_expr_print_internal(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_operator_print_internal(llvm_unwrap_operator_const(expr));
        case LLVM_SYMBOL_TYPED:
            return llvm_symbol_typed_print_internal(llvm_unwrap_symbol_typed_const(expr));
        case LLVM_SYMBOL_UNTYPED:
            return llvm_symbol_untyped_print_internal(llvm_unwrap_symbol_untyped_const(expr));
        case LLVM_MEMBER_ACCESS_UNTYPED:
            return llvm_member_access_untyped_print_internal(llvm_unwrap_member_access_untyped_const(expr));
        case LLVM_MEMBER_ACCESS_TYPED:
            return llvm_member_access_typed_print_internal(llvm_unwrap_member_access_typed_const(expr));
        case LLVM_INDEX_UNTYPED:
            return llvm_index_untyped_print_internal(llvm_unwrap_index_untyped_const(expr));
        case LLVM_INDEX_TYPED:
            return llvm_index_typed_print_internal(llvm_unwrap_index_typed_const(expr));
        case LLVM_LITERAL:
            return llvm_literal_print_internal(llvm_unwrap_literal_const(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_print_internal(llvm_unwrap_function_call_const(expr));
        case LLVM_STRUCT_LITERAL:
            return llvm_struct_literal_print_internal(llvm_unwrap_struct_literal_const(expr));
        case LLVM_LLVM_PLACEHOLDER:
            return llvm_llvm_placeholder_print_internal(llvm_unwrap_llvm_placeholder_const(expr));
    }
    unreachable("");
}

Str_view llvm_print_internal(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return llvm_block_print_internal(llvm_unwrap_block_const(llvm));
        case LLVM_EXPR:
            return llvm_expr_print_internal(llvm_unwrap_expr_const(llvm));
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_print_internal(llvm_unwrap_load_element_ptr_const(llvm));
        case LLVM_FUNCTION_PARAMS:
            return llvm_function_params_print_internal(llvm_unwrap_function_params_const(llvm));
        case LLVM_LANG_TYPE:
            return llvm_lang_type_print_internal(llvm_unwrap_lang_type_const(llvm));
        case LLVM_FOR_LOWER_BOUND:
            return llvm_for_lower_bound_print_internal(llvm_unwrap_for_lower_bound_const(llvm));
        case LLVM_FOR_UPPER_BOUND:
            return llvm_for_upper_bound_print_internal(llvm_unwrap_for_upper_bound_const(llvm));
        case LLVM_DEF:
            return llvm_def_print_internal(llvm_unwrap_def_const(llvm));
        case LLVM_CONDITION:
            return llvm_condition_print_internal(llvm_unwrap_condition_const(llvm));
        case LLVM_FOR_RANGE:
            return llvm_for_range_print_internal(llvm_unwrap_for_range_const(llvm));
        case LLVM_FOR_WITH_COND:
            return llvm_for_with_cond_print_internal(llvm_unwrap_for_with_cond_const(llvm));
        case LLVM_BREAK:
            return llvm_break_print_internal(llvm_unwrap_break_const(llvm));
        case LLVM_CONTINUE:
            return llvm_continue_print_internal(llvm_unwrap_continue_const(llvm));
        case LLVM_ASSIGNMENT:
            return llvm_assignment_print_internal(llvm_unwrap_assignment_const(llvm));
        case LLVM_IF:
            return llvm_if_print_internal(llvm_unwrap_if_const(llvm));
        case LLVM_RETURN:
            return llvm_return_print_internal(llvm_unwrap_return_const(llvm));
        case LLVM_GOTO:
            return llvm_goto_print_internal(llvm_unwrap_goto_const(llvm));
        case LLVM_COND_GOTO:
            return llvm_cond_goto_print_internal(llvm_unwrap_cond_goto_const(llvm));
        case LLVM_ALLOCA:
            return llvm_alloca_print_internal(llvm_unwrap_alloca_const(llvm));
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_print_internal(llvm_unwrap_load_another_llvm_const(llvm));
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_print_internal(llvm_unwrap_store_another_llvm_const(llvm));
        case LLVM_IF_ELSE_CHAIN:
            return llvm_if_else_chain_print_internal(llvm_unwrap_if_else_chain_const(llvm));
    }
    unreachable("");
}
