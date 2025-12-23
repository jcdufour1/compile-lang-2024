#include <ir.h>
#include <tast.h>
#include <util.h>
#include <ir_utils.h>
#include <ir_lang_type_print.h>

static void extend_child_name(String* buf, const char* location, Ir_name child_name) {
    string_extend_cstr(&a_temp, buf, " (* ");
    string_extend_cstr(&a_temp, buf, location);
    string_extend_cstr(&a_temp, buf, ":");
    extend_ir_name(NAME_LOG, buf, child_name);
    string_extend_cstr(&a_temp, buf, " *) ");
}

static void extend_lhs_and_eq(String* buf, Ir_name lhs_sym, Indent indent) {
    string_extend_cstr_indent(&a_temp, buf, "%", indent);
    extend_ir_name(NAME_LOG, buf, lhs_sym);
    string_extend_cstr(&a_temp, buf, " = ");
}

Strv ir_binary_print_internal(const Ir_binary* binary, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "binary", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, binary->lang_type);
    string_extend_strv(&a_temp, &buf, ir_binary_type_to_strv(binary->token_type));
    extend_ir_name(NAME_LOG, &buf, binary->name);
    extend_child_name(&buf, "lhs", binary->lhs);
    extend_child_name(&buf, "rhs", binary->rhs);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_unary_print_internal(const Ir_unary* unary, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "unary", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, unary->lang_type);
    string_extend_strv(&a_temp, &buf, ir_unary_type_to_strv(unary->token_type));
    extend_ir_name(NAME_LOG, &buf, unary->name);
    extend_child_name(&buf, "child", unary->child);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

void ir_extend_sym_typed_base(String* string, Llvm_sym_typed_base base) {
    extend_ir_lang_type_to_string(string, LANG_TYPE_MODE_LOG, base.lang_type);
    extend_ir_name(NAME_LOG, string, base.name);
    string_extend_cstr(&a_temp, string, "\n");
}

Strv ir_literal_print_internal(const Ir_literal* lit, Indent indent) {
    switch (lit->type) {
        case IR_INT:
            return ir_int_print_internal(ir_int_const_unwrap(lit), indent);
        case IR_FLOAT:
            return ir_float_print_internal(ir_float_const_unwrap(lit), indent);
        case IR_STRING:
            return ir_string_print_internal(ir_string_const_unwrap(lit), indent);
        case IR_VOID:
            return ir_void_print_internal(ir_void_const_unwrap(lit), indent);
        case IR_FUNCTION_NAME:
            return ir_function_name_print_internal(ir_function_name_const_unwrap(lit), indent);
    }
    unreachable("");
}

Strv ir_function_call_print_internal(const Ir_function_call* fun_call, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "function_call", indent);
    extend_ir_name(NAME_LOG, &buf, fun_call->name_self);
    extend_child_name(&buf, "function_to_call:", fun_call->name_self);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_call->lang_type);
    string_extend_cstr(&a_temp, &buf, "\n");

    for (size_t idx = 0; idx < fun_call->args.info.count; idx++) {
        string_extend_cstr_indent(&a_temp, &buf, " ", indent + INDENT_WIDTH);
        extend_ir_name(NAME_LOG, &buf, darr_at(fun_call->args, idx));
        string_extend_cstr(&a_temp, &buf, "\n");
    }

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_int_print_internal(const Ir_int* num, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "number", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    extend_ir_name(NAME_LOG, &buf, num->name);
    string_extend_int64_t(&a_temp, &buf, num->data);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_float_print_internal(const Ir_float* num, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "float", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, num->lang_type);
    extend_ir_name(NAME_LOG, &buf, num->name);
    string_extend_double(&a_temp, &buf, num->data);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_string_print_internal(const Ir_string* lit, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "string", indent);
    extend_ir_name(NAME_LOG, &buf, lit->name);
    string_extend_strv(&a_temp, &buf, lit->data);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_void_print_internal(const Ir_void* num, Indent indent) {
    (void) num;

    bool old_is_printing = env.is_printing;
    env.is_printing = true;
    String buf = {0};

    string_extend_cstr_indent(&a_temp, &buf, "void\n", indent);

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_function_name_print_internal(const Ir_function_name* name, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "function_name", indent);
    extend_child_name(&buf, "name_self", name->name_self);
    extend_child_name(&buf, "fun_name", name->fun_name);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_load_element_ptr_print_internal(const Ir_load_element_ptr* load, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "load_element_ptr", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, load->lang_type);
    extend_ir_name(NAME_LOG, &buf, load->name_self);
    extend_child_name(&buf, "member_name", load->name_self);
    extend_child_name(&buf, "src", load->ir_src);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_array_access_print_internal(const Ir_array_access* load, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "array_access", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, load->lang_type);
    extend_ir_name(NAME_LOG, &buf, load->name_self);
    extend_child_name(&buf, "member_name", load->name_self);
    extend_child_name(&buf, "src", load->callee);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_block_print_internal(const Ir_block* block, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "block\n", indent);
    indent += INDENT_WIDTH;

    darr_foreach(idx, Cfg_node, curr, block->cfg) {
        string_extend_strv(&a_temp, &buf, cfg_node_print_internal(curr, idx, indent + INDENT_WIDTH));
    }

    string_extend_cstr_indent(&a_temp, &buf, "alloca_table\n", indent + INDENT_WIDTH);
    ir_extend_table_internal(&buf, darr_at(env.symbol_tables, block->scope_id).ir_table, indent + 2*INDENT_WIDTH);

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        string_extend_size_t(&a_temp, &buf, idx);
        string_extend_cstr(&a_temp, &buf, ": ");
        Strv arg_text = ir_print_internal(darr_at(block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_temp, &buf, arg_text);
    }

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_function_params_print_internal(const Ir_function_params* function_params, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "function_params\n", indent);
    indent += INDENT_WIDTH;
    for (size_t idx = 0; idx < function_params->params.info.count; idx++) {
        Strv arg_text = ir_variable_def_print_internal(darr_at(function_params->params, idx), indent);
        string_extend_strv(&a_temp, &buf, arg_text);
    }
    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_return_print_internal(const Ir_return* lang_rtn, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    env.is_printing = old_is_printing;
    string_extend_cstr_indent(&a_temp, &buf, "return", indent);
    extend_child_name(&buf, "child", lang_rtn->child);
    string_extend_cstr(&a_temp, &buf, "\n");

    return string_to_strv(buf);
}

Strv ir_goto_print_internal(const Ir_goto* lang_goto, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "goto", indent);
    extend_ir_name(NAME_LOG, &buf, lang_goto->name_self);
    extend_ir_name(NAME_LOG, &buf, lang_goto->label);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_cond_goto_print_internal(const Ir_cond_goto* cond_goto, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "cond_goto", indent);
    indent += INDENT_WIDTH;
    string_extend_cstr(&a_temp, &buf, "\n");
    string_extend_cstr_indent(&a_temp, &buf, "", indent);
    extend_ir_name(NAME_LOG, &buf, cond_goto->if_true);
    string_extend_cstr(&a_temp, &buf, "\n");
    string_extend_cstr_indent(&a_temp, &buf, "", indent);
    extend_ir_name(NAME_LOG, &buf, cond_goto->if_false);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_alloca_temp_internal(const Ir_alloca* lang_alloca, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    extend_lhs_and_eq(&buf, lang_alloca->name, indent);

    string_extend_cstr(&a_temp, &buf, "lang_alloca");
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, lang_alloca->lang_type);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_load_another_ir_print_internal(const Ir_load_another_ir* load, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "load_another_ir", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, load->lang_type);
    extend_ir_name(NAME_LOG, &buf, load->name);
    extend_child_name(&buf, "src", load->ir_src);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_store_another_ir_print_internal(const Ir_store_another_ir* store, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    extend_lhs_and_eq(&buf, store->ir_dest, indent);

    string_extend_cstr(&a_temp, &buf, "store_another_ir");
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, store->lang_type);
    extend_ir_name(NAME_LOG, &buf, store->name);
    extend_child_name(&buf, "src", store->ir_src);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_import_path_print_internal(const Ir_import_path* import, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "import_path", indent);
    string_extend_strv(&a_temp, &buf, import->mod_path);
    string_extend_cstr(&a_temp, &buf, "\n");
    if (import->block) {
        string_extend_strv(&a_temp, &buf, ir_block_print_internal(import->block, indent + INDENT_WIDTH));
    } else {
        string_extend_cstr_indent(&a_temp, &buf, "<block is null>\n", indent);
    }

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_removed_print_internal(const Ir_removed* removed, Indent indent) {
    (void) removed;

    bool old_is_printing = env.is_printing;
    env.is_printing = true;
    String buf = {0};

    string_extend_cstr_indent(&a_temp, &buf, "removed\n", indent);

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_function_decl_print_internal(const Ir_function_decl* fun_decl, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "function_decl", indent);
    indent += INDENT_WIDTH;
    extend_ir_name(NAME_LOG, &buf, fun_decl->name);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, fun_decl->return_type);
    string_extend_cstr(&a_temp, &buf, "\n");
    string_extend_strv(&a_temp, &buf, ir_function_params_print_internal(fun_decl->params, indent));
    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_function_def_print_internal(const Ir_function_def* fun_def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "function_def\n", indent);
    indent += INDENT_WIDTH;
    string_extend_strv(&a_temp, &buf, ir_function_decl_print_internal(fun_def->decl, indent));
    string_extend_strv(&a_temp, &buf, ir_block_print_internal(fun_def->body, indent));
    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

static void extend_ir_struct_def_base(String* buf, const char* type_name, Ir_struct_def_base base, Indent indent) {
    string_extend_cstr_indent(&a_temp, buf, type_name, indent);
    extend_ir_name(NAME_LOG, buf, base.name);
    string_extend_cstr(&a_temp, buf, "\n");

    for (size_t idx = 0; idx < base.members.info.count; idx++) {
        Strv memb_text = ir_struct_memb_def_print_internal(darr_at(base.members, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_temp, buf, memb_text);
    }
}

Strv ir_struct_def_print_internal(const Ir_struct_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    extend_ir_struct_def_base(&buf, "struct_def", def->base, indent);

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_primitive_def_print_internal(const Ir_primitive_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "primitive_def\n", indent);
    indent += INDENT_WIDTH;
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    string_extend_cstr(&a_temp, &buf, "\n");
    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_string_def_print_internal(const Ir_string_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "string_def", indent);
    indent += INDENT_WIDTH;
    extend_ir_name(NAME_LOG, &buf, def->name);
    string_extend_strv(&a_temp, &buf, def->data);
    string_extend_cstr(&a_temp, &buf, "\n");
    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_struct_lit_def_print_internal(const Ir_struct_lit_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    indent += INDENT_WIDTH;

    string_extend_cstr_indent(&a_temp, &buf, "struct_lit_def", indent);
    extend_ir_name(NAME_LOG, &buf, def->name);
    string_extend_cstr(&a_temp, &buf, "\n");
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    for (size_t idx = 0; idx < def->members.info.count; idx++) {
        Strv memb_text = ir_expr_print_internal(darr_at(def->members, idx), indent);
        string_extend_strv(&a_temp, &buf, memb_text);
    }

    indent -= INDENT_WIDTH;

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_label_print_internal(const Ir_label* label, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "LABEL[", indent);
    extend_ir_name(NAME_LOG, &buf, label->name);
    string_extend_cstr(&a_temp, &buf, "]:\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_literal_def_print_internal(const Ir_literal_def* def, Indent indent) {
    switch (def->type) {
        case IR_STRING_DEF:
            return ir_string_def_print_internal(ir_string_def_const_unwrap(def), indent);
        case IR_STRUCT_LIT_DEF:
            return ir_struct_lit_def_print_internal(ir_struct_lit_def_const_unwrap(def), indent);
    }
    unreachable("");
}

Strv ir_variable_def_print_internal(const Ir_variable_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "variable_def", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    extend_ir_name(NAME_LOG, &buf, def->name_self);
    extend_child_name(&buf, "corrs_param", def->name_corr_param);
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_struct_memb_def_print_internal(const Ir_struct_memb_def* def, Indent indent) {
    String buf = {0};

    bool old_is_printing = env.is_printing;
    env.is_printing = true;

    string_extend_cstr_indent(&a_temp, &buf, "struct_memb_def", indent);
    extend_ir_lang_type_to_string(&buf, LANG_TYPE_MODE_LOG, def->lang_type);
    extend_ir_name(NAME_LOG, &buf, def->name_self);
    string_extend_cstr(&a_temp, &buf, "[");
    string_extend_size_t(&a_temp, &buf, def->count);
    string_extend_cstr(&a_temp, &buf, "]");
    string_extend_cstr(&a_temp, &buf, "\n");

    env.is_printing = old_is_printing;
    return string_to_strv(buf);
}

Strv ir_operator_print_internal(const Ir_operator* operator, Indent indent) {
    switch (operator->type) {
        case IR_BINARY:
            return ir_binary_print_internal(ir_binary_const_unwrap(operator), indent);
        case IR_UNARY:
            return ir_unary_print_internal(ir_unary_const_unwrap(operator), indent);
    }
    unreachable("");
}

Strv ir_def_print_internal(const Ir_def* def, Indent indent) {
    Strv old_mod_path_curr_file = env.mod_path_curr_file;
    env.mod_path_curr_file = ir_def_get_name(LANG_TYPE_MODE_LOG/*TODO */, def).mod_path;

    Strv result = {0};
    switch (def->type) {
        case IR_FUNCTION_DEF:
            result = ir_function_def_print_internal(ir_function_def_const_unwrap(def), indent);
            break;
        case IR_FUNCTION_DECL:
            result = ir_function_decl_print_internal(ir_function_decl_const_unwrap(def), indent);
            break;
        case IR_VARIABLE_DEF:
            result = ir_variable_def_print_internal(ir_variable_def_const_unwrap(def), indent);
            break;
        case IR_STRUCT_DEF:
            result = ir_struct_def_print_internal(ir_struct_def_const_unwrap(def), indent);
            break;
        case IR_PRIMITIVE_DEF:
            result = ir_primitive_def_print_internal(ir_primitive_def_const_unwrap(def), indent);
            break;
        case IR_LABEL:
            result = ir_label_print_internal(ir_label_const_unwrap(def), indent);
            break;
        case IR_LITERAL_DEF:
            result = ir_literal_def_print_internal(ir_literal_def_const_unwrap(def), indent);
            break;
        default:
            unreachable("");
    }

    env.mod_path_curr_file = old_mod_path_curr_file;
    return result;
}

Strv ir_expr_print_internal(const Ir_expr* expr, Indent indent) {
    switch (expr->type) {
        case IR_OPERATOR:
            return ir_operator_print_internal(ir_operator_const_unwrap(expr), indent);
        case IR_LITERAL:
            return ir_literal_print_internal(ir_literal_const_unwrap(expr), indent);
        case IR_FUNCTION_CALL:
            return ir_function_call_print_internal(ir_function_call_const_unwrap(expr), indent);
    }
    unreachable("");
}

Strv ir_print_internal(const Ir* ir, Indent indent) {
    switch (ir->type) {
        case IR_BLOCK:
            return ir_block_print_internal(ir_block_const_unwrap(ir), indent);
        case IR_EXPR:
            return ir_expr_print_internal(ir_expr_const_unwrap(ir), indent);
        case IR_DEF:
            return ir_def_print_internal(ir_def_const_unwrap(ir), indent);
        case IR_LOAD_ELEMENT_PTR:
            return ir_load_element_ptr_print_internal(ir_load_element_ptr_const_unwrap(ir), indent);
        case IR_ARRAY_ACCESS:
            return ir_array_access_print_internal(ir_array_access_const_unwrap(ir), indent);
        case IR_FUNCTION_PARAMS:
            return ir_function_params_print_internal(ir_function_params_const_unwrap(ir), indent);
        case IR_RETURN:
            return ir_return_print_internal(ir_return_const_unwrap(ir), indent);
        case IR_GOTO:
            return ir_goto_print_internal(ir_goto_const_unwrap(ir), indent);
        case IR_COND_GOTO:
            return ir_cond_goto_print_internal(ir_cond_goto_const_unwrap(ir), indent);
        case IR_ALLOCA:
            return ir_alloca_temp_internal(ir_alloca_const_unwrap(ir), indent);
        case IR_LOAD_ANOTHER_IR:
            return ir_load_another_ir_print_internal(ir_load_another_ir_const_unwrap(ir), indent);
        case IR_STORE_ANOTHER_IR:
            return ir_store_another_ir_print_internal(ir_store_another_ir_const_unwrap(ir), indent);
        case IR_IMPORT_PATH:
            return ir_import_path_print_internal(ir_import_path_const_unwrap(ir), indent);
        case IR_STRUCT_MEMB_DEF:
            return ir_struct_memb_def_print_internal(ir_struct_memb_def_const_unwrap(ir), indent);
        case IR_REMOVED:
            return ir_removed_print_internal(ir_removed_const_unwrap(ir), indent);
    }
    unreachable("");
}
