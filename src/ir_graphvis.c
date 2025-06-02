#include <ir_graphvis.h>
#include <parser_utils.h>
#include <symbol_iter.h>

// NOTE: arrow from parent to child created in parent corresponding function, not child


static Name ir_graphvis_parent_block_next;
static Alloca_table already_visited = {0};

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void ir_graphvis_internal(String* buf, const Ir* ir);

static void ir_variable_def_graphvis_internal(String* buf, const Ir_variable_def* def);

static bool ir_graphvis_do_next_arrow(const Ir* ir);

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
    Strv label
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

static void label_internal(const char* file, int line, String* buf, Name name, Strv label) {
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

static void label_ex_internal(const char* file, int line, String* buf, Name name, Strv label, Name actual_name) {
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

static void ir_block_graphvis_internal(String* buf, const Ir_block* block) {
    extend_source_loc(buf);

    // TODO: make size_t_print_macro?
    String scope_buf = {0};
    string_extend_strv(&a_print, &scope_buf, sv("block (scope "));
    string_extend_size_t(&a_print, &scope_buf, block->scope_id);
    string_extend_strv(&a_print, &scope_buf, sv(")"));

    label(buf, block->name, string_to_strv(scope_buf));

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        bool is_last = idx + 1 >= block->children.info.count;
        Ir* curr = vec_at(&block->children, idx);
        Ir* next = is_last ? NULL : vec_at(&block->children, idx + 1);

        if (idx < 1) {
            arrow_names_label(buf, block->name, ir_tast_get_name(curr), sv("next"));
        }

        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        if (all_tbl_add_ex(&already_visited, curr)) {
            Name old_parent_block_next = ir_graphvis_parent_block_next;
            ir_graphvis_parent_block_next = is_last ? (Name) {0} : ir_tast_get_name(next);
            ir_graphvis_internal(buf, curr);
            ir_graphvis_parent_block_next = old_parent_block_next;
        }
        if (ir_graphvis_do_next_arrow(curr)) {
            if (is_last) {
                // this is the last node of this block. If we are nested in a parent block, then
                // the next statement to be executed is in env.ir_graphvis_parent_block_next (set by the parent block)
                if (ir_graphvis_parent_block_next.base.count > 0) {
                    arrow_names_label(
                        buf,
                        ir_tast_get_name(curr),
                        ir_graphvis_parent_block_next,
                        sv("next")
                    );
                }
            } else {
                arrow_names_label(buf, ir_tast_get_name(curr), ir_tast_get_name(next), sv("next"));
            }
        }
    }

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Ir* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        if (all_tbl_add_ex(&already_visited, curr)) {
            todo();
            arrow_names(buf, block->name, ir_tast_get_name(curr));
            ir_graphvis_internal(buf, curr);
        }
    }
}

static void ir_function_params_graphvis_internal(String* buf, const Ir_function_params* params) {
    label(buf, params->name, sv("params"));

    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        arrow_names_label(buf, params->name, vec_at(&params->params, idx)->name_self, string_to_strv(idx_buf));
    }
}

static void ir_variable_def_graphvis_internal(String* buf, const Ir_variable_def* def) {
    label_ex(buf, def->name_self, sv("variable_def"), def->name_corr_param);
}

static void ir_label_graphvis_internal(String* buf, const Ir_label* label) {
    label_ex(buf, label->name, sv("label"), label->name);
}

static void ir_function_decl_graphvis_internal(String* buf, const Ir_function_decl* decl) {
    label_ex(buf, decl->name, sv("function decl"), decl->name);
    arrow_names(buf, decl->name, decl->params->name);

    ir_function_params_graphvis_internal(buf, decl->params);
    // TODO
    //string_extend_strv(&a_print, buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, decl->return_type));
}

static void ir_function_def_graphvis_internal(String* buf, const Ir_function_def* def) {
    label(buf, def->name_self, sv("function def"));
    arrow_names(buf, def->name_self, def->decl->name);
    arrow_names(buf, def->name_self, def->body->name);

    ir_function_decl_graphvis_internal(buf, def->decl);
    ir_block_graphvis_internal(buf, def->body);
}

static void ir_struct_def_base_graphvis_internal(String* buf, const Ir_struct_def_base* base) {
    for (size_t idx = 0; idx < base->members.info.count; idx++) {
        arrow_names(buf, base->name, vec_at(&base->members, idx)->name_self);
        ir_variable_def_graphvis_internal(buf, vec_at(&base->members, idx));
    }
}

static void ir_struct_def_graphvis_internal(String* buf, const Ir_struct_def* def) {
    ir_struct_def_base_graphvis_internal(buf, &def->base);
}

static void ir_def_graphvis_internal(String* buf, const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            ir_function_def_graphvis_internal(buf, ir_function_def_const_unwrap(def));
            return;
        case IR_VARIABLE_DEF:
            ir_variable_def_graphvis_internal(buf, ir_variable_def_const_unwrap(def));
            return;
        case IR_STRUCT_DEF:
            ir_struct_def_graphvis_internal(buf, ir_struct_def_const_unwrap(def));
            return;
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            todo();
        case IR_LABEL:
            ir_label_graphvis_internal(buf, ir_label_const_unwrap(def));
            return;
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void ir_load_element_ptr_graphvis_internal(String* buf, const Ir_load_element_ptr* load) {
    String idx_buf = {0};
    string_extend_strv(&a_print, &idx_buf, sv("load element ptr "));
    string_extend_size_t(&a_print, &idx_buf, load->memb_idx);
    label(buf, load->name_self, string_to_strv(idx_buf));

    arrow_names_label(buf, load->name_self, load->ir_src, sv("src"));
}

static void ir_array_access_graphvis_internal(String* buf, const Ir_array_access* access) {
    label(buf, access->name_self, sv("access"));
    arrow_names_label(buf, access->name_self, access->index, sv("index"));
    arrow_names_label(buf, access->name_self, access->callee, sv("callee"));
}

static void ir_int_graphvis_internal(String* buf, const Ir_int* lit) {
    String num_buf = {0};

    string_extend_int64_t(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void ir_float_graphvis_internal(String* buf, const Ir_float* lit) {
    String num_buf = {0};

    string_extend_double(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void ir_string_graphvis_internal(String* buf, const Ir_string* lit) {
    String str_buf = {0};

    string_extend_cstr(&a_print, &str_buf, "string_lit ");
    string_extend_strv(&a_print, &str_buf, lit->data);
    label(buf, lit->name, string_to_strv(str_buf));
}

static void ir_void_graphvis_internal(String* buf, const Ir_void* lit) {
    label(buf, lit->name, sv("void"));
}

static void ir_function_name_graphvis_internal(String* buf, const Ir_function_name* lit) {
    label_ex(buf, lit->name_self, sv("function_name"), lit->fun_name);
}

static void ir_literal_graphvis_internal(String* buf, const Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            ir_int_graphvis_internal(buf, ir_int_const_unwrap(lit));
            return;
        case IR_FLOAT:
            ir_float_graphvis_internal(buf, ir_float_const_unwrap(lit));
            return;
        case IR_STRING:
            ir_string_graphvis_internal(buf, ir_string_const_unwrap(lit));
            return;
        case IR_VOID:
            ir_void_graphvis_internal(buf, ir_void_const_unwrap(lit));
            return;
        case IR_FUNCTION_NAME:
            ir_function_name_graphvis_internal(buf, ir_function_name_const_unwrap(lit));
            return;
    }
    unreachable("");
}

static void ir_function_call_graphvis_internal(String* buf, const Ir_function_call* call) {
    label(buf, call->name_self, sv("function_call"));
    arrow_names_label(buf, call->name_self, call->callee, sv("callee"));

    Name args_name = util_literal_name_new();
    label(buf, args_name, sv("args"));
    arrow_names_label(buf, call->name_self, args_name, sv("args"));
    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        arrow_names_label(buf, args_name, vec_at(&call->args, idx), string_to_strv(idx_buf));
    }

    // TODO: lang_type
}

static void ir_binary_graphvis_internal(String* buf, const Ir_binary* bin) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "binary ");
    string_extend_strv(&a_print, &type_buf, binary_type_to_strv(bin->token_type));
    string_extend_cstr(&a_print, &type_buf, " ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, bin->lang_type));
    label(buf, bin->name, string_to_strv(type_buf));

    arrow_names_label(buf, bin->name, bin->lhs, sv("lhs"));
    arrow_names_label(buf, bin->name, bin->rhs, sv("rhs"));
}

static void ir_unary_graphvis_internal(String* buf, const Ir_unary* unary) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "unary ");
    string_extend_strv(&a_print, &type_buf, unary_type_to_strv(unary->token_type));
    string_extend_cstr(&a_print, &type_buf, " ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, unary->lang_type));
    label(buf, unary->name, string_to_strv(type_buf));

    arrow_names_label(buf, unary->name, unary->child, sv("child"));
}

static void ir_operator_graphvis_internal(String* buf, const Ir_operator* oper) {
    switch (oper->type) {
        case IR_BINARY:
            ir_binary_graphvis_internal(buf, ir_binary_const_unwrap(oper));
            return;
        case IR_UNARY:
            ir_unary_graphvis_internal(buf, ir_unary_const_unwrap(oper));
            return;
    }
    unreachable("");
}

static void ir_expr_graphvis_internal(String* buf, const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            ir_operator_graphvis_internal(buf, ir_operator_const_unwrap(expr));
            return;
        case IR_LITERAL:
            ir_literal_graphvis_internal(buf, ir_literal_const_unwrap(expr));
            return;
        case IR_FUNCTION_CALL:
            ir_function_call_graphvis_internal(buf, ir_function_call_const_unwrap(expr));
            return;
    }
    unreachable("");
}

static void ir_return_graphvis_internal(String* buf, const Ir_return* rtn) {
    label(buf, rtn->name_self, sv("return"));

    arrow_names_label(buf, rtn->name_self, rtn->child, sv("src"));
}

static void ir_alloca_graphvis_internal(String* buf, const Ir_alloca* alloca) {
    label_ex(buf, alloca->name, sv("alloca"), alloca->name);

    // TODO
}

static void ir_cond_goto_graphvis_internal(String* buf, const Ir_cond_goto* cond_goto) {
    label(buf, cond_goto->name_self, sv("cond_goto"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->if_true, sv("if true"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->if_false, sv("if false"));
    arrow_names_label(buf, cond_goto->name_self, cond_goto->condition, sv("condition"));
}

static void ir_goto_graphvis_internal(String* buf, const Ir_goto* lang_goto) {
    label(buf, lang_goto->name_self, sv("goto"));
    arrow_names_label(buf, lang_goto->name_self, lang_goto->label, sv("label"));
}

static void ir_store_another_ir_graphvis_internal(String* buf, const Ir_store_another_ir* store) {
    // TODO: name_self and name inconsistent
    label(buf, store->name, sv("store"));

    arrow_names_label(buf, store->name, store->ir_src, sv("src"));
    arrow_names_label(buf, store->name, store->ir_dest, sv("dest"));
    // TODO: lang_type
}

static void ir_load_another_ir_graphvis_internal(String* buf, const Ir_load_another_ir* load) {
    String type_buf = {0};
    string_extend_cstr(&a_print, &type_buf, "load ");
    string_extend_strv(&a_print, &type_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, load->lang_type));
    label(buf, load->name, string_to_strv(type_buf));

    arrow_names_label(buf, load->name, load->ir_src, sv("src"));
}

// this should return false for statement types that never return or have custom code paths, etc.
static bool ir_graphvis_do_next_arrow(const Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            return false;
        case IR_EXPR:
            return true;
        case IR_DEF:
            return true;
        case IR_LOAD_ELEMENT_PTR:
            return true;
        case IR_ARRAY_ACCESS:
            return true;
        case IR_FUNCTION_PARAMS:
            unreachable("");
        case IR_RETURN:
            return false;
        case IR_GOTO:
            return false;
        case IR_COND_GOTO:
            return false;
        case IR_ALLOCA:
            return true;
        case IR_LOAD_ANOTHER_IR:
            return true;
        case IR_STORE_ANOTHER_IR:
            return true;
    }
    unreachable("");
}

static void ir_graphvis_internal(String* buf, const Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            ir_block_graphvis_internal(buf, ir_block_const_unwrap(ir));
            return;
        case IR_EXPR:
            ir_expr_graphvis_internal(buf, ir_expr_const_unwrap(ir));
            return;
        case IR_DEF:
            ir_def_graphvis_internal(buf, ir_def_const_unwrap(ir));
            return;
        case IR_LOAD_ELEMENT_PTR:
            ir_load_element_ptr_graphvis_internal(buf, ir_load_element_ptr_const_unwrap(ir));
            return;
        case IR_ARRAY_ACCESS:
            ir_array_access_graphvis_internal(buf, ir_array_access_const_unwrap(ir));
            return;
        case IR_FUNCTION_PARAMS:
            ir_function_params_graphvis_internal(buf, ir_function_params_const_unwrap(ir));
            return;
        case IR_RETURN:
            ir_return_graphvis_internal(buf, ir_return_const_unwrap(ir));
            return;
        case IR_GOTO:
            ir_goto_graphvis_internal(buf, ir_goto_const_unwrap(ir));
            return;
        case IR_COND_GOTO:
            ir_cond_goto_graphvis_internal(buf, ir_cond_goto_const_unwrap(ir));
            return;
        case IR_ALLOCA:
            ir_alloca_graphvis_internal(buf, ir_alloca_const_unwrap(ir));
            return;
        case IR_LOAD_ANOTHER_IR:
            ir_load_another_ir_graphvis_internal(buf, ir_load_another_ir_const_unwrap(ir));
            return;
        case IR_STORE_ANOTHER_IR:
            ir_store_another_ir_graphvis_internal(buf, ir_store_another_ir_const_unwrap(ir));
            return;
    }
    unreachable("");
}

Strv ir_graphvis(const Ir_block* block) {
    // TODO: remove parameter block?
    (void) block;
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_cstr(&a_print, &buf, "bgcolor=\"black\"\n");
    string_extend_cstr(&a_print, &buf, "node [style=filled, fillcolor=\"black\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_print, &buf, "edge [color=\"white\", fontcolor=\"white\"];\n");

    Alloca_iter iter = all_tbl_iter_new(SCOPE_BUILTIN);
    Ir* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        // TODO: do scopes correctly (make alloca_add_ex)
        if (all_tbl_add_ex(&already_visited, curr)) {
            ir_graphvis_internal(&buf, curr);
        }
    }

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
