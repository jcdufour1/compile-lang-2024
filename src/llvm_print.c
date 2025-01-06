#include <llvm.h>
#include <llvms.h>
#include <tast.h>
#include <llvms.h>
#include <util.h>
#include <llvm_utils.h>

static void extend_llvm_id(String* buf, const char* location, Llvm_id llvm_id) {
    string_extend_cstr(&print_arena, buf, " (& ");
    string_extend_cstr(&print_arena, buf, location);
    string_extend_cstr(&print_arena, buf, ":");
    string_extend_size_t(&print_arena, buf, llvm_id);
    string_extend_cstr(&print_arena, buf, " &) ");
}

static void extend_child_name(String* buf, const char* location, Str_view child_name) {
    string_extend_cstr(&print_arena, buf, " (* ");
    string_extend_cstr(&print_arena, buf, location);
    string_extend_cstr(&print_arena, buf, ":");
    string_extend_strv(&print_arena, buf, child_name);
    string_extend_cstr(&print_arena, buf, " *) ");
}

static void extend_name(String* buf, Str_view name) {
    string_extend_strv_in_par(&print_arena, buf, name);
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

Str_view llvm_binary_print_internal(const Llvm_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    extend_lang_type(&buf, binary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(binary->token_type));
    string_extend_strv_in_par(&print_arena, &buf, binary->name);
    extend_child_name(&buf, "lhs", binary->lhs);
    extend_child_name(&buf, "rhs", binary->rhs);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_unary_print_internal(const Llvm_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", indent);
    extend_lang_type(&buf, unary->lang_type, true);
    string_extend_strv(&print_arena, &buf, token_type_to_str_view(unary->token_type));
    string_extend_strv_in_par(&print_arena, &buf, unary->name);
    extend_child_name(&buf, "child", unary->child);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

void llvm_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type(string, base.lang_type, true);
    string_extend_strv(&print_arena, string, base.name);
    string_extend_cstr(&print_arena, string, "\n");
}

Str_view llvm_primitive_sym_print_internal(const Llvm_primitive_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_sym", indent);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_struct_sym_print_internal(const Llvm_struct_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "struct_sym", indent);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_raw_union_sym_print_internal(const Llvm_raw_union_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "raw_union_sym", indent);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_enum_sym_print_internal(const Llvm_enum_sym* sym, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_sym", indent);
    llvm_extend_sym_typed_base(&buf, sym->base);

    return string_to_strv(buf);
}

Str_view llvm_symbol_typed_print_internal(const Llvm_symbol_typed* sym, int indent) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_print_internal(llvm_unwrap_primitive_sym_const(sym), indent);
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_print_internal(llvm_unwrap_struct_sym_const(sym), indent);
        case LLVM_RAW_UNION_SYM:
            return llvm_raw_union_sym_print_internal(llvm_unwrap_raw_union_sym_const(sym), indent);
        case LLVM_ENUM_SYM:
            return llvm_enum_sym_print_internal(llvm_unwrap_enum_sym_const(sym), indent);
    }
    unreachable("");
}

Str_view llvm_literal_print_internal(const Llvm_literal* lit, int indent) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_print_internal(llvm_unwrap_number_const(lit), indent);
        case LLVM_STRING:
            return llvm_string_print_internal(llvm_unwrap_string_const(lit), indent);
        case LLVM_VOID:
            return llvm_void_print_internal(llvm_unwrap_void_const(lit), indent);
        case LLVM_ENUM_LIT:
            return llvm_enum_lit_print_internal(llvm_unwrap_enum_lit_const(lit), indent);
        case LLVM_CHAR:
            return llvm_char_print_internal(llvm_unwrap_char_const(lit), indent);
    }
    unreachable("");
}

Str_view llvm_function_call_print_internal(const Llvm_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", indent);
    extend_name(&buf, fun_call->name_self);
    extend_child_name(&buf, "function_to_call:", fun_call->name_fun_to_call);
    extend_lang_type(&buf, fun_call->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        string_extend_strv_indent(&print_arena, &buf, vec_at(&fun_call->args, idx), indent + INDENT_WIDTH);
        string_extend_cstr(&print_arena, &buf, "\n");
    }

    return string_to_strv(buf);
}

Str_view llvm_number_print_internal(const Llvm_number* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", indent);
    extend_lang_type(&buf, num->lang_type, true);
    extend_name(&buf, num->name);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_string_print_internal(const Llvm_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", indent);
    extend_lang_type(&buf, lit->lang_type, true);
    extend_name(&buf, lit->name);
    string_extend_strv(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_enum_lit_print_internal(const Llvm_enum_lit* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "enum_lit", indent);
    extend_lang_type(&buf, num->lang_type, true);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_void_print_internal(const Llvm_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Str_view llvm_char_print_internal(const Llvm_char* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "char", indent);
    vec_append(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_llvm_placeholder_print_internal(const Llvm_llvm_placeholder* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "llvm_placeholder", indent);
    extend_lang_type(&buf, lit->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_load_element_ptr_print_internal(const Llvm_load_element_ptr* load, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_element_ptr", indent);
    extend_lang_type(&buf, load->lang_type, true);
    extend_llvm_id(&buf, "self", load->llvm_id);
    extend_name(&buf, load->name_self);
    extend_child_name(&buf, "member_name", load->name_self);
    extend_child_name(&buf, "src", load->llvm_src);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_block_print_internal(const Llvm_block* block, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "block\n", indent);

    string_extend_cstr_indent(&print_arena, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    alloca_extend_table_internal(&buf, block->symbol_collection.alloca_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "usymbol_table\n", indent + INDENT_WIDTH);
    usymbol_extend_table_internal(&buf, block->symbol_collection.usymbol_table, indent + 2*INDENT_WIDTH);

    string_extend_cstr_indent(&print_arena, &buf, "symbol_table\n", indent + INDENT_WIDTH);
    symbol_extend_table_internal(&buf, block->symbol_collection.symbol_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = llvm_print_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, &buf, arg_text);
    }

    return string_to_strv(buf);
}

Str_view llvm_function_params_print_internal(const Llvm_function_params* function_params, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Str_view arg_text = llvm_variable_def_print_internal(vec_at(&function_params->params, idx), indent);
        string_extend_strv(&print_arena, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_lang_type_print_internal(const Llvm_lang_type* lang_type, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "lang_type", indent);
    extend_lang_type(&buf, lang_type->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_return_print_internal(const Llvm_return* lang_rtn, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "return", indent);
    extend_child_name(&buf, "child", lang_rtn->child);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_goto_print_internal(const Llvm_goto* lang_goto, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "goto", indent);
    string_extend_strv_in_par(&print_arena, &buf, lang_goto->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_cond_goto_print_internal(const Llvm_cond_goto* cond_goto, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "cond_goto", indent);
    string_extend_strv_in_par(&print_arena, &buf, cond_goto->if_true);
    string_extend_strv_in_par(&print_arena, &buf, cond_goto->if_false);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_alloca_print_internal(const Llvm_alloca* alloca, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "alloca", indent);
    extend_lang_type(&buf, alloca->lang_type, true);
    extend_name(&buf, alloca->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_load_another_llvm_print_internal(const Llvm_load_another_llvm* load, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_another_llvm", indent);
    extend_lang_type(&buf, load->lang_type, true);
    extend_name(&buf, load->name);
    extend_child_name(&buf, "src", load->llvm_src);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_store_another_llvm_print_internal(const Llvm_store_another_llvm* store, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "store_another_llvm", indent);
    extend_lang_type(&buf, store->lang_type, true);
    extend_name(&buf, store->name);
    extend_child_name(&buf, "src", store->llvm_src);
    extend_child_name(&buf, "dest", store->llvm_dest);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_function_decl_print_internal(const Llvm_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    string_extend_strv_in_par(&print_arena, &buf, fun_decl->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, llvm_function_params_print_internal(fun_decl->params, indent));
    string_extend_strv(&print_arena, &buf, llvm_lang_type_print_internal(fun_decl->return_type, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_function_def_print_internal(const Llvm_function_def* fun_def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, llvm_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&print_arena, &buf, llvm_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

static void extend_struct_def_base(String* buf, const char* type_name, Struct_def_base base, int indent) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    string_extend_strv_in_par(&print_arena, buf, base.name);
    extend_llvm_id(buf, "self", base.llvm_id);
    string_extend_cstr(&print_arena, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = tast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

Str_view llvm_struct_def_print_internal(const Llvm_struct_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view llvm_raw_union_def_print_internal(const Llvm_raw_union_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "raw_union_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view llvm_enum_def_print_internal(const Llvm_enum_def* def, int indent) {
    String buf = {0};

    extend_struct_def_base(&buf, "enum_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view llvm_primitive_def_print_internal(const Llvm_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type(&buf, def->lang_type, true);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_string_def_print_internal(const Llvm_string_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&print_arena, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_struct_lit_def_print_internal(const Llvm_struct_lit_def* def, int indent) {
    String buf = {0};

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def\n", indent);
    string_extend_strv_indent(&print_arena, &buf, def->name, indent);
    extend_lang_type(&buf, def->lang_type, true);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Str_view memb_text = llvm_expr_print_internal(vec_at(&def->members, idx), indent);
        string_extend_strv(&print_arena, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_label_print_internal(const Llvm_label* label, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "label", indent);
    string_extend_strv_in_par(&print_arena, &buf, label->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_literal_def_print_internal(const Llvm_literal_def* def, int indent) {
    switch (def->type) {
        case LLVM_STRING_DEF:
            return llvm_string_def_print_internal(llvm_unwrap_string_def_const(def), indent);
        case LLVM_STRUCT_LIT_DEF:
            return llvm_struct_lit_def_print_internal(llvm_unwrap_struct_lit_def_const(def), indent);
    }
    unreachable("");
}

Str_view llvm_variable_def_print_internal(const Llvm_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_lang_type(&buf, def->lang_type, true);
    extend_name(&buf, def->name_self);
    extend_child_name(&buf, "corrs_param", def->name_corr_param);
    extend_llvm_id(&buf, "self", def->llvm_id);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_operator_print_internal(const Llvm_operator* operator, int indent) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_binary_print_internal(llvm_unwrap_binary_const(operator), indent);
        case LLVM_UNARY:
            return llvm_unary_print_internal(llvm_unwrap_unary_const(operator), indent);
    }
    unreachable("");
}

Str_view llvm_def_print_internal(const Llvm_def* def, int indent) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_print_internal(llvm_unwrap_function_def_const(def), indent);
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_print_internal(llvm_unwrap_function_decl_const(def), indent);
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_print_internal(llvm_unwrap_variable_def_const(def), indent);
        case LLVM_STRUCT_DEF:
            return llvm_struct_def_print_internal(llvm_unwrap_struct_def_const(def), indent);
        case LLVM_RAW_UNION_DEF:
            return llvm_raw_union_def_print_internal(llvm_unwrap_raw_union_def_const(def), indent);
        case LLVM_ENUM_DEF:
            return llvm_enum_def_print_internal(llvm_unwrap_enum_def_const(def), indent);
        case LLVM_PRIMITIVE_DEF:
            return llvm_primitive_def_print_internal(llvm_unwrap_primitive_def_const(def), indent);
        case LLVM_LABEL:
            return llvm_label_print_internal(llvm_unwrap_label_const(def), indent);
        case LLVM_LITERAL_DEF:
            return llvm_literal_def_print_internal(llvm_unwrap_literal_def_const(def), indent);
    }
    unreachable("");
}

Str_view llvm_expr_print_internal(const Llvm_expr* expr, int indent) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_operator_print_internal(llvm_unwrap_operator_const(expr), indent);
        case LLVM_SYMBOL_TYPED:
            return llvm_symbol_typed_print_internal(llvm_unwrap_symbol_typed_const(expr), indent);
        case LLVM_LITERAL:
            return llvm_literal_print_internal(llvm_unwrap_literal_const(expr), indent);
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_print_internal(llvm_unwrap_function_call_const(expr), indent);
        case LLVM_LLVM_PLACEHOLDER:
            return llvm_llvm_placeholder_print_internal(llvm_unwrap_llvm_placeholder_const(expr), indent);
    }
    unreachable("");
}

Str_view llvm_print_internal(const Llvm* llvm, int indent) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return llvm_block_print_internal(llvm_unwrap_block_const(llvm), indent);
        case LLVM_EXPR:
            return llvm_expr_print_internal(llvm_unwrap_expr_const(llvm), indent);
        case LLVM_DEF:
            return llvm_def_print_internal(llvm_unwrap_def_const(llvm), indent);
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_print_internal(llvm_unwrap_load_element_ptr_const(llvm), indent);
        case LLVM_FUNCTION_PARAMS:
            return llvm_function_params_print_internal(llvm_unwrap_function_params_const(llvm), indent);
        case LLVM_LANG_TYPE:
            return llvm_lang_type_print_internal(llvm_unwrap_lang_type_const(llvm), indent);
        case LLVM_RETURN:
            return llvm_return_print_internal(llvm_unwrap_return_const(llvm), indent);
        case LLVM_GOTO:
            return llvm_goto_print_internal(llvm_unwrap_goto_const(llvm), indent);
        case LLVM_COND_GOTO:
            return llvm_cond_goto_print_internal(llvm_unwrap_cond_goto_const(llvm), indent);
        case LLVM_ALLOCA:
            return llvm_alloca_print_internal(llvm_unwrap_alloca_const(llvm), indent);
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_print_internal(llvm_unwrap_load_another_llvm_const(llvm), indent);
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_print_internal(llvm_unwrap_store_another_llvm_const(llvm), indent);
    }
    unreachable("");
}
