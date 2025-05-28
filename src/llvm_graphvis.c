#include <llvm_print_graphvis.h>
#include <parser_utils.h>
#include <symbol_iter.h>

// NOTE: arrow from parent to child created in parent corresponding function, not child

static Alloca_table already_visited = {0};

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void llvm_graphvis_internal(String* buf, const Llvm* llvm);

static void llvm_variable_def_graphvis_internal(String* buf, const Llvm_variable_def* def);

static bool llvm_graphvis_do_next_arrow(const Llvm* llvm);

static void extend_source_loc_internal(const char* file, int line, String* buf) {
    string_extend_cstr(&a_print, buf, "// ");
    string_extend_cstr(&a_print, buf, file);
    string_extend_cstr(&a_print, buf, ":");
    string_extend_int64_t(&a_print, buf, line);
    string_extend_cstr(&a_print, buf, "\n");
}

static void extend_name_graphvis(String* buf, Name name) {
    string_extend_strv(&a_print, buf, serialize_name(name));
}

#define arrow_names(buf, parent, child) arrow_names_internal(__FILE__, __LINE__, buf, parent, child)

static void arrow_names_internal(const char* file, int line, String* buf, Name parent, Name child) {
    extend_source_loc_internal(file, line, buf);

    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_print, buf, ";\n");
}

#define arrow_names_label(buf, parent, child, label) \
    arrow_names_label_internal(__FILE__, __LINE__, buf, parent, child, label)

static void arrow_names_label_internal(
    const char* file,
    int line,
    String* buf,
    Name parent,
    Name child,
    Str_view label
) {
    extend_source_loc_internal(file, line, buf);

    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

#define label(buf, name, label) label_internal(__FILE__, __LINE__, buf, name, label)

static void label_internal(const char* file, int line, String* buf, Name name, Str_view label) {
    extend_source_loc_internal(file, line, buf);

    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    // TODO: make parameter to enable/disable below block
    {
        //string_extend_cstr(&a_print, buf, " ");
        //string_extend_strv(&a_print, buf, serialize_name(name));

    }
    string_extend_cstr(&a_print, buf, "\"];\n");
}

#define label_ex(buf, name, label, actual_name) label_ex_internal(__FILE__, __LINE__, buf, name, label, actual_name)

static void label_ex_internal(const char* file, int line, String* buf, Name name, Str_view label, Name actual_name) {
    extend_source_loc_internal(file, line, buf);

    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    // TODO: make parameter to enable/disable below block
    {
        //string_extend_cstr(&a_print, buf, " ");
        //string_extend_strv(&a_print, buf, serialize_name(name));
    }
    string_extend_cstr(&a_print, buf, " ");
    extend_name_graphvis(buf, actual_name);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void llvm_block_graphvis_internal(String* buf, const Llvm_block* block) {
    extend_source_loc(buf);

    // TODO: make size_t_print_macro?
    String scope_buf = {0};
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr("block (scope "));
    string_extend_size_t(&a_print, &scope_buf, block->scope_id);
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr(")"));

    label(buf, block->name, string_to_strv(scope_buf));

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        bool is_last = idx + 1 >= block->children.info.count;
        Llvm* curr = vec_at(&block->children, idx);
        Llvm* next = is_last ? NULL : vec_at(&block->children, idx + 1);

        if (idx < 1) {
            arrow_names_label(buf, block->name, llvm_tast_get_name(curr), str_view_from_cstr("next"));
        }

        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        if (all_tbl_add_ex(&already_visited, curr)) {
            Name old_parent_block_next = env.llvm_graphvis_parent_block_next;
            env.llvm_graphvis_parent_block_next = is_last ? (Name) {0} : llvm_tast_get_name(next);
            llvm_graphvis_internal(buf, curr);
            env.llvm_graphvis_parent_block_next = old_parent_block_next;
        }
        if (llvm_graphvis_do_next_arrow(curr)) {
            if (is_last) {
                // this is the last node of this block. If we are nested in a parent block, then
                // the next statement to be executed is in env.llvm_graphvis_parent_block_next (set by the parent block)
                if (env.llvm_graphvis_parent_block_next.base.count > 0) {
                    arrow_names_label(
                        buf,
                        llvm_tast_get_name(curr),
                        env.llvm_graphvis_parent_block_next,
                        str_view_from_cstr("next")
                    );
                }
            } else {
                arrow_names_label(buf, llvm_tast_get_name(curr), llvm_tast_get_name(next), str_view_from_cstr("next"));
            }
        }
    }

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    log(LOG_DEBUG, "%zu\n", vec_at_ref(&env.symbol_tables, block->scope_id)->alloca_table.count);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        if (all_tbl_add_ex(&already_visited, curr)) {
            todo();
            arrow_names(buf, block->name, llvm_tast_get_name(curr));
            llvm_graphvis_internal(buf, curr);
        }
    }
}

static void llvm_function_params_graphvis_internal(String* buf, const Llvm_function_params* params) {
    label(buf, params->name, str_view_from_cstr("params"));

    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        arrow_names_label(buf, params->name, vec_at(&params->params, idx)->name_self, string_to_strv(idx_buf));
    }
}

static void llvm_variable_def_graphvis_internal(String* buf, const Llvm_variable_def* def) {
    label_ex(buf, def->name_self, str_view_from_cstr("variable_def"), def->name_corr_param);
}

static void llvm_label_graphvis_internal(String* buf, const Llvm_label* label) {
    label_ex(buf, label->name, str_view_from_cstr("label"), label->name);
}

static void llvm_function_decl_graphvis_internal(String* buf, const Llvm_function_decl* decl) {
    label_ex(buf, decl->name, str_view_from_cstr("function decl"), decl->name);
    arrow_names(buf, decl->name, decl->params->name);

    llvm_function_params_graphvis_internal(buf, decl->params);
    // TODO
    //string_extend_strv(&a_print, buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, decl->return_type));
}

static void llvm_function_def_graphvis_internal(String* buf, const Llvm_function_def* def) {
    label(buf, def->name_self, str_view_from_cstr("function def"));
    arrow_names(buf, def->name_self, def->decl->name);
    arrow_names(buf, def->name_self, def->body->name);

    llvm_function_decl_graphvis_internal(buf, def->decl);
    llvm_block_graphvis_internal(buf, def->body);
}

static void llvm_struct_def_base_graphvis_internal(String* buf, const Llvm_struct_def_base* base) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        arrow_names(buf, base->name, vec_at(&base->members, idx)->name_self);
        llvm_variable_def_graphvis_internal(buf, vec_at(&base->members, idx));
    }
}

static void llvm_struct_def_graphvis_internal(String* buf, const Llvm_struct_def* def) {
    llvm_struct_def_base_graphvis_internal(buf, &def->base);
}

static void llvm_def_graphvis_internal(String* buf, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            llvm_function_def_graphvis_internal(buf, llvm_function_def_const_unwrap(def));
            return;
        case LLVM_VARIABLE_DEF:
            llvm_variable_def_graphvis_internal(buf, llvm_variable_def_const_unwrap(def));
            return;
        case LLVM_STRUCT_DEF:
            llvm_struct_def_graphvis_internal(buf, llvm_struct_def_const_unwrap(def));
            return;
        case LLVM_PRIMITIVE_DEF:
            todo();
        case LLVM_FUNCTION_DECL:
            todo();
        case LLVM_LABEL:
            llvm_label_graphvis_internal(buf, llvm_label_const_unwrap(def));
            return;
        case LLVM_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void llvm_load_element_ptr_graphvis_internal(String* buf, const Llvm_load_element_ptr* load) {
    String idx_buf = {0};
    string_extend_strv(&a_print, &idx_buf, str_view_from_cstr("load element ptr "));
    string_extend_size_t(&a_print, &idx_buf, load->memb_idx);
    label(buf, load->name_self, string_to_strv(idx_buf));

    arrow_names_label(buf, load->name_self, load->llvm_src, str_view_from_cstr("src"));
}

static void llvm_array_access_graphvis_internal(String* buf, const Llvm_array_access* access) {
    label(buf, access->name_self, str_view_from_cstr("access"));
    arrow_names_label(buf, access->name_self, access->index, str_view_from_cstr("index"));
    arrow_names_label(buf, access->name_self, access->callee, str_view_from_cstr("callee"));
}

static void llvm_int_graphvis_internal(String* buf, const Llvm_int* lit) {
    String num_buf = {0};

    string_extend_int64_t(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void llvm_float_graphvis_internal(String* buf, const Llvm_float* lit) {
    String num_buf = {0};

    string_extend_double(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void llvm_string_graphvis_internal(String* buf, const Llvm_string* lit) {
    String str_buf = {0};

    string_extend_cstr(&a_print, &str_buf, "string_lit ");
    string_extend_strv(&a_print, &str_buf, lit->data);
    label(buf, lit->name, string_to_strv(str_buf));
}

static void llvm_void_graphvis_internal(String* buf, const Llvm_void* lit) {
    label(buf, lit->name, str_view_from_cstr("void"));
}

static void llvm_function_name_graphvis_internal(String* buf, const Llvm_function_name* lit) {
    label_ex(buf, lit->name_self, str_view_from_cstr("function_name"), lit->fun_name);
}

static void llvm_literal_graphvis_internal(String* buf, const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_INT:
            llvm_int_graphvis_internal(buf, llvm_int_const_unwrap(lit));
            return;
        case LLVM_FLOAT:
            llvm_float_graphvis_internal(buf, llvm_float_const_unwrap(lit));
            return;
        case LLVM_STRING:
            llvm_string_graphvis_internal(buf, llvm_string_const_unwrap(lit));
            return;
        case LLVM_VOID:
            llvm_void_graphvis_internal(buf, llvm_void_const_unwrap(lit));
            return;
        case LLVM_FUNCTION_NAME:
            llvm_function_name_graphvis_internal(buf, llvm_function_name_const_unwrap(lit));
            return;
    }
    unreachable("");
}

static void llvm_function_call_graphvis_internal(String* buf, const Llvm_function_call* call) {
    label(buf, call->name_self, str_view_from_cstr("function_call"));
    arrow_names_label(buf, call->name_self, call->callee, str_view_from_cstr("callee"));

    Name args_name = util_literal_name_new2();
    label(buf, args_name, str_view_from_cstr("args"));
    arrow_names_label(buf, call->name_self, args_name, str_view_from_cstr("args"));
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        arrow_names_label(buf, args_name, vec_at(&call->args, idx), string_to_strv(idx_buf));
    }

    // TODO: lang_type
}

static void llvm_binary_graphvis_internal(String* buf, const Llvm_binary* bin) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "binary ");
    string_extend_strv(&a_print, &type_buf, binary_type_to_str_view(bin->token_type));
    string_extend_cstr(&a_print, &type_buf, " ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, bin->lang_type));
    label(buf, bin->name, string_to_strv(type_buf));

    arrow_names_label(buf, bin->name, bin->lhs, str_view_from_cstr("lhs"));
    arrow_names_label(buf, bin->name, bin->rhs, str_view_from_cstr("rhs"));
}

static void llvm_unary_graphvis_internal(String* buf, const Llvm_unary* unary) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "unary ");
    string_extend_strv(&a_print, &type_buf, unary_type_to_str_view(unary->token_type));
    string_extend_cstr(&a_print, &type_buf, " ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, unary->lang_type));
    label(buf, unary->name, string_to_strv(type_buf));

    arrow_names_label(buf, unary->name, unary->child, str_view_from_cstr("child"));
}

static void llvm_operator_graphvis_internal(String* buf, const Llvm_operator* oper) {
    switch (oper->type) {
        case LLVM_BINARY:
            llvm_binary_graphvis_internal(buf, llvm_binary_const_unwrap(oper));
            return;
        case LLVM_UNARY:
            llvm_unary_graphvis_internal(buf, llvm_unary_const_unwrap(oper));
            return;
    }
    unreachable("");
}

static void llvm_expr_graphvis_internal(String* buf, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            llvm_operator_graphvis_internal(buf, llvm_operator_const_unwrap(expr));
            return;
        case LLVM_LITERAL:
            llvm_literal_graphvis_internal(buf, llvm_literal_const_unwrap(expr));
            return;
        case LLVM_FUNCTION_CALL:
            llvm_function_call_graphvis_internal(buf, llvm_function_call_const_unwrap(expr));
            return;
    }
    unreachable("");
}

static void llvm_return_graphvis_internal(String* buf, const Llvm_return* rtn) {
    label(buf, rtn->name_self, str_view_from_cstr("return"));

    arrow_names_label(buf, rtn->name_self, rtn->child, str_view_from_cstr("src"));
}

static void llvm_alloca_graphvis_internal(String* buf, const Llvm_alloca* alloca) {
    label_ex(buf, alloca->name, str_view_from_cstr("alloca"), alloca->name);

    // TODO
}

static void llvm_cond_goto_graphvis_internal(String* buf, const Llvm_cond_goto* cond_goto) {
    label(buf, cond_goto->name_self, str_view_from_cstr("cond_goto"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->if_true, str_view_from_cstr("if true"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->if_false, str_view_from_cstr("if false"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->condition, str_view_from_cstr("condition"));
}

static void llvm_goto_graphvis_internal(String* buf, const Llvm_goto* lang_goto) {
    label(buf, lang_goto->name_self, str_view_from_cstr("goto"));
    arrow_names_label(buf, lang_goto->name_self, lang_goto->label, str_view_from_cstr("label"));
}

static void llvm_store_another_llvm_graphvis_internal(String* buf, const Llvm_store_another_llvm* store) {
    // TODO: name_self and name inconsistent
    label(buf, store->name, str_view_from_cstr("store"));

    arrow_names_label(buf, store->name, store->llvm_src, str_view_from_cstr("src"));
    arrow_names_label(buf, store->name, store->llvm_dest, str_view_from_cstr("dest"));
    // TODO: lang_type
}

static void llvm_load_another_llvm_graphvis_internal(String* buf, const Llvm_load_another_llvm* load) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "load ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, load->lang_type));
    label(buf, load->name, string_to_strv(type_buf));

    arrow_names_label(buf, load->name, load->llvm_src, str_view_from_cstr("src"));
}

// this should return false for statement types that never return or have custom code paths, etc.
static bool llvm_graphvis_do_next_arrow(const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return false;
        case LLVM_EXPR:
            return true;
        case LLVM_DEF:
            return true;
        case LLVM_LOAD_ELEMENT_PTR:
            return true;
        case LLVM_ARRAY_ACCESS:
            return true;
        case LLVM_FUNCTION_PARAMS:
            unreachable("");
        case LLVM_RETURN:
            return false;
        case LLVM_GOTO:
            return false;
        case LLVM_COND_GOTO:
            return false;
        case LLVM_ALLOCA:
            return true;
        case LLVM_LOAD_ANOTHER_LLVM:
            return true;
        case LLVM_STORE_ANOTHER_LLVM:
            return true;
    }
    unreachable("");
}

static void llvm_graphvis_internal(String* buf, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            llvm_block_graphvis_internal(buf, llvm_block_const_unwrap(llvm));
            return;
        case LLVM_EXPR:
            llvm_expr_graphvis_internal(buf, llvm_expr_const_unwrap(llvm));
            return;
        case LLVM_DEF:
            llvm_def_graphvis_internal(buf, llvm_def_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            llvm_load_element_ptr_graphvis_internal(buf, llvm_load_element_ptr_const_unwrap(llvm));
            return;
        case LLVM_ARRAY_ACCESS:
            llvm_array_access_graphvis_internal(buf, llvm_array_access_const_unwrap(llvm));
            return;
        case LLVM_FUNCTION_PARAMS:
            llvm_function_params_graphvis_internal(buf, llvm_function_params_const_unwrap(llvm));
            return;
        case LLVM_RETURN:
            llvm_return_graphvis_internal(buf, llvm_return_const_unwrap(llvm));
            return;
        case LLVM_GOTO:
            llvm_goto_graphvis_internal(buf, llvm_goto_const_unwrap(llvm));
            return;
        case LLVM_COND_GOTO:
            llvm_cond_goto_graphvis_internal(buf, llvm_cond_goto_const_unwrap(llvm));
            return;
        case LLVM_ALLOCA:
            llvm_alloca_graphvis_internal(buf, llvm_alloca_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            llvm_load_another_llvm_graphvis_internal(buf, llvm_load_another_llvm_const_unwrap(llvm));
            return;
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_store_another_llvm_graphvis_internal(buf, llvm_store_another_llvm_const_unwrap(llvm));
            return;
    }
    unreachable("");
}

Str_view llvm_graphvis(const Llvm_block* block) {
    // TODO: remove parameter block?
    (void) block;
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_cstr(&a_print, &buf, "bgcolor=\"black\"\n");
    string_extend_cstr(&a_print, &buf, "node [style=filled, fillcolor=\"black\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_print, &buf, "edge [color=\"white\", fontcolor=\"white\"];\n");

    Alloca_iter iter = all_tbl_iter_new(SCOPE_BUILTIN);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        // TODO: do scopes correctly (make alloca_add_ex)
        if (all_tbl_add_ex(&already_visited, curr)) {
            log(LOG_DEBUG, TAST_FMT"\n", llvm_print(curr));
            llvm_graphvis_internal(&buf, curr);
        }
    }

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
