#include <llvm.h>
#include <tast.h>
#include <util.h>
#include <llvm_utils.h>
#include <lang_type_print.h>

static void extend_llvm_id(String* buf, const char* location, Llvm_id llvm_id) {
    string_extend_cstr(&print_arena, buf, " (& ");
    string_extend_cstr(&print_arena, buf, location);
    string_extend_cstr(&print_arena, buf, ":");
    string_extend_size_t(&print_arena, buf, llvm_id);
    string_extend_cstr(&print_arena, buf, " &) ");
}

static void extend_child_name(String* buf, const char* location, Name child_name) {
    string_extend_cstr(&print_arena, buf, " (* ");
    string_extend_cstr(&print_arena, buf, location);
    string_extend_cstr(&print_arena, buf, ":");
    extend_name(false, buf, child_name);
    string_extend_cstr(&print_arena, buf, " *) ");
}

Str_view llvm_binary_print_internal(const Llvm_binary* binary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "binary", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, binary->lang_type);
    string_extend_strv(&print_arena, &buf, binary_type_to_str_view(binary->token_type));
    extend_name(false, &buf, binary->name);
    extend_child_name(&buf, "lhs", binary->lhs);
    extend_child_name(&buf, "rhs", binary->rhs);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_unary_print_internal(const Llvm_unary* unary, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "unary", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, unary->lang_type);
    string_extend_strv(&print_arena, &buf, unary_type_to_str_view(unary->token_type));
    extend_name(false, &buf, unary->name);
    extend_child_name(&buf, "child", unary->child);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

void llvm_extend_sym_typed_base(String* string, Sym_typed_base base) {
    extend_lang_type_to_string(string, LANG_TYPE_MODE_LOG, base.lang_type);
    extend_name(false, string, base.name);
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

Str_view llvm_symbol_typed_print_internal(const Llvm_symbol* sym, int indent) {
    switch (sym->type) {
        case LLVM_PRIMITIVE_SYM:
            return llvm_primitive_sym_print_internal(llvm_primitive_sym_const_unwrap(sym), indent);
        case LLVM_STRUCT_SYM:
            return llvm_struct_sym_print_internal(llvm_struct_sym_const_unwrap(sym), indent);
    }
    unreachable("");
}

Str_view llvm_literal_print_internal(const Llvm_literal* lit, int indent) {
    switch (lit->type) {
        case LLVM_NUMBER:
            return llvm_number_print_internal(llvm_number_const_unwrap(lit), indent);
        case LLVM_STRING:
            return llvm_string_print_internal(llvm_string_const_unwrap(lit), indent);
        case LLVM_VOID:
            return llvm_void_print_internal(llvm_void_const_unwrap(lit), indent);
        case LLVM_FUNCTION_NAME:
            return llvm_function_name_print_internal(llvm_function_name_const_unwrap(lit), indent);
    }
    unreachable("");
}

Str_view llvm_function_call_print_internal(const Llvm_function_call* fun_call, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_call", indent);
    extend_name(false, &buf, fun_call->name_self);
    extend_child_name(&buf, "function_to_call:", fun_call->name_self);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_call->lang_type);
    string_extend_cstr(&print_arena, &buf, "\n");

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        string_extend_cstr_indent(&print_arena, &buf, " ", indent + INDENT_WIDTH);
        extend_name(false, &buf, vec_at(&fun_call->args, idx));
        string_extend_cstr(&print_arena, &buf, "\n");
    }

    return string_to_strv(buf);
}

Str_view llvm_number_print_internal(const Llvm_number* num, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "number", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    extend_name(false, &buf, num->name);
    string_extend_int64_t(&print_arena, &buf, num->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_string_print_internal(const Llvm_string* lit, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string", indent);
    extend_name(false, &buf, lit->name);
    string_extend_strv(&print_arena, &buf, lit->data);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_void_print_internal(const Llvm_void* num, int indent) {
    (void) num;
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "void\n", indent);

    return string_to_strv(buf);
}

Str_view llvm_function_name_print_internal(const Llvm_function_name* name, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_name", indent);
    extend_child_name(&buf, "name_self", name->name_self);
    extend_child_name(&buf, "fun_name", name->fun_name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_load_element_ptr_print_internal(const Llvm_load_element_ptr* load, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_element_ptr", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, load->lang_type);
    extend_llvm_id(&buf, "self", load->llvm_id);
    extend_name(false, &buf, load->name_self);
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
    extend_name(false, &buf, lang_goto->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_cond_goto_print_internal(const Llvm_cond_goto* cond_goto, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "cond_goto", indent);
    extend_name(false, &buf, cond_goto->if_true);
    extend_name(false, &buf, cond_goto->if_false);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_alloca_print_internal(const Llvm_alloca* alloca, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "alloca", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, alloca->lang_type);
    extend_name(false, &buf, alloca->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_load_another_llvm_print_internal(const Llvm_load_another_llvm* load, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "load_another_llvm", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, load->lang_type);
    extend_name(false, &buf, load->name);
    extend_child_name(&buf, "src", load->llvm_src);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_store_another_llvm_print_internal(const Llvm_store_another_llvm* store, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "store_another_llvm", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, store->lang_type);
    extend_name(false, &buf, store->name);
    extend_child_name(&buf, "src", store->llvm_src);
    extend_child_name(&buf, "dest", store->llvm_dest);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_function_decl_print_internal(const Llvm_function_decl* fun_decl, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    extend_name(false, &buf, fun_decl->name);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_decl->return_type);
    string_extend_cstr(&print_arena, &buf, "\n");
    string_extend_strv(&print_arena, &buf, llvm_function_params_print_internal(fun_decl->params, indent));
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
    extend_name(false, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = tast_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

static void extend_llvm_struct_def_base(String* buf, const char* type_name, Llvm_struct_def_base base, int indent) {
    string_extend_cstr_indent(&print_arena, buf, type_name, indent);
    extend_name(false, buf, base.name);
    string_extend_cstr(&print_arena, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Str_view memb_text = llvm_variable_def_print_internal(vec_at(&base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&print_arena, buf, memb_text);
    }
}

Str_view llvm_struct_def_print_internal(const Llvm_struct_def* def, int indent) {
    String buf = {0};

    extend_llvm_struct_def_base(&buf, "struct_def", def->base, indent);

    return string_to_strv(buf);
}

Str_view llvm_primitive_def_print_internal(const Llvm_primitive_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_string_def_print_internal(const Llvm_string_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    extend_name(false, &buf, def->name);
    string_extend_strv(&print_arena, &buf, def->data);
    string_extend_cstr(&print_arena, &buf, "\n");
    indent -= INDENT_WIDTH;

    return string_to_strv(buf);
}

Str_view llvm_struct_lit_def_print_internal(const Llvm_struct_lit_def* def, int indent) {
    String buf = {0};

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&print_arena, &buf, "struct_lit_def", indent);
    extend_name(false, &buf, def->name);
    string_extend_cstr(&print_arena, &buf, "\n");
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
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
    extend_name(false, &buf, label->name);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_literal_def_print_internal(const Llvm_literal_def* def, int indent) {
    switch (def->type) {
        case LLVM_STRING_DEF:
            return llvm_string_def_print_internal(llvm_string_def_const_unwrap(def), indent);
        case LLVM_STRUCT_LIT_DEF:
            return llvm_struct_lit_def_print_internal(llvm_struct_lit_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view llvm_variable_def_print_internal(const Llvm_variable_def* def, int indent) {
    String buf = {0};

    string_extend_cstr_indent(&print_arena, &buf, "variable_def", indent);
    extend_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    extend_name(false, &buf, def->name_self);
    extend_child_name(&buf, "corrs_param", def->name_corr_param);
    extend_llvm_id(&buf, "self", def->llvm_id);
    string_extend_cstr(&print_arena, &buf, "\n");

    return string_to_strv(buf);
}

Str_view llvm_operator_print_internal(const Llvm_operator* operator, int indent) {
    switch (operator->type) {
        case LLVM_BINARY:
            return llvm_binary_print_internal(llvm_binary_const_unwrap(operator), indent);
        case LLVM_UNARY:
            return llvm_unary_print_internal(llvm_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Str_view llvm_def_print_internal(const Llvm_def* def, int indent) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_print_internal(llvm_function_def_const_unwrap(def), indent);
        case LLVM_FUNCTION_DECL:
            return llvm_function_decl_print_internal(llvm_function_decl_const_unwrap(def), indent);
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_print_internal(llvm_variable_def_const_unwrap(def), indent);
        case LLVM_STRUCT_DEF:
            return llvm_struct_def_print_internal(llvm_struct_def_const_unwrap(def), indent);
        case LLVM_PRIMITIVE_DEF:
            return llvm_primitive_def_print_internal(llvm_primitive_def_const_unwrap(def), indent);
        case LLVM_LABEL:
            return llvm_label_print_internal(llvm_label_const_unwrap(def), indent);
        case LLVM_LITERAL_DEF:
            return llvm_literal_def_print_internal(llvm_literal_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Str_view llvm_expr_print_internal(const Llvm_expr* expr, int indent) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            return llvm_operator_print_internal(llvm_operator_const_unwrap(expr), indent);
        case LLVM_SYMBOL:
            return llvm_symbol_typed_print_internal(llvm_symbol_const_unwrap(expr), indent);
        case LLVM_LITERAL:
            return llvm_literal_print_internal(llvm_literal_const_unwrap(expr), indent);
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_print_internal(llvm_function_call_const_unwrap(expr), indent);
    }
    unreachable("");
}

Str_view llvm_print_internal(const Llvm* llvm, int indent) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return llvm_block_print_internal(llvm_block_const_unwrap(llvm), indent);
        case LLVM_EXPR:
            return llvm_expr_print_internal(llvm_expr_const_unwrap(llvm), indent);
        case LLVM_DEF:
            return llvm_def_print_internal(llvm_def_const_unwrap(llvm), indent);
        case LLVM_LOAD_ELEMENT_PTR:
            return llvm_load_element_ptr_print_internal(llvm_load_element_ptr_const_unwrap(llvm), indent);
        case LLVM_FUNCTION_PARAMS:
            return llvm_function_params_print_internal(llvm_function_params_const_unwrap(llvm), indent);
        case LLVM_RETURN:
            return llvm_return_print_internal(llvm_return_const_unwrap(llvm), indent);
        case LLVM_GOTO:
            return llvm_goto_print_internal(llvm_goto_const_unwrap(llvm), indent);
        case LLVM_COND_GOTO:
            return llvm_cond_goto_print_internal(llvm_cond_goto_const_unwrap(llvm), indent);
        case LLVM_ALLOCA:
            return llvm_alloca_print_internal(llvm_alloca_const_unwrap(llvm), indent);
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_print_internal(llvm_load_another_llvm_const_unwrap(llvm), indent);
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_print_internal(llvm_store_another_llvm_const_unwrap(llvm), indent);
    }
    unreachable("");
}
