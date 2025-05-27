#include <llvm_print_graphvis.h>
#include <parser_utils.h>
#include <symbol_iter.h>

// NOTE: arrow from parent to child created in parent corresponding function, not child

static Alloca_table already_visited = {0};

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void llvm_graphvis_internal(String* buf, const Llvm* llvm);

static void llvm_variable_def_graphvis_internal(String* buf, const Llvm_variable_def* def);

static void extend_source_loc_internal(const char* file, int line, String* buf) {
    string_extend_cstr(&a_print, buf, "// ");
    string_extend_cstr(&a_print, buf, file);
    string_extend_cstr(&a_print, buf, ":");
    string_extend_int64_t(&a_print, buf, line);
    string_extend_cstr(&a_print, buf, "\n");
}

static void extend_name_graphvis(String* buf, Name name) {
    extend_name_log_internal(false, buf, name);
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
    string_extend_cstr(&a_print, buf, "\"];\n");
}

#define label_ex(buf, name, label, actual_name) label_ex_internal(__FILE__, __LINE__, buf, name, label, actual_name)

static void label_ex_internal(const char* file, int line, String* buf, Name name, Str_view label, Name actual_name) {
    extend_source_loc_internal(file, line, buf);

    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, " ");
    extend_name_graphvis(buf, actual_name);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void llvm_block_graphvis_internal(String* buf, const Llvm_block* block) {
    extend_source_loc(buf);

    String scope_buf = {0};
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr("block (scope "));
    string_extend_size_t(&a_print, &scope_buf, block->scope_id);
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr(")"));

    label(buf, block->name, string_to_strv(scope_buf));

    Name prev = block->name;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        llvm_graphvis_internal(buf, vec_at(&block->children, idx));
        arrow_names_label(buf, prev, llvm_tast_get_name(vec_at(&block->children, idx)), string_to_strv(idx_buf));
        unwrap(all_tbl_add_ex(&already_visited, vec_at(&block->children, idx)));
        prev = llvm_tast_get_name(vec_at(&block->children, idx));
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
        llvm_variable_def_graphvis_internal(buf, vec_at(&params->params, idx));
    }
}

static void llvm_variable_def_graphvis_internal(String* buf, const Llvm_variable_def* def) {
    label_ex(buf, def->name_self, str_view_from_cstr("variable_def"), def->name_corr_param);
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

static void llvm_def_graphvis_internal(String* buf, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            llvm_function_def_graphvis_internal(buf, llvm_function_def_const_unwrap(def));
            return;
        case LLVM_VARIABLE_DEF:
            todo();
        case LLVM_STRUCT_DEF:
            todo();
        case LLVM_PRIMITIVE_DEF:
            todo();
        case LLVM_FUNCTION_DECL:
            todo();
        case LLVM_LABEL:
            todo();
        case LLVM_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static void llvm_int_graphvis_internal(String* buf, const Llvm_int* lit) {
    String num_buf = {0};

    string_extend_int64_t(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void llvm_void_graphvis_internal(String* buf, const Llvm_void* lit) {
    label(buf, lit->name, str_view_from_cstr("void"));
}

static void llvm_literal_graphvis_internal(String* buf, const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_INT:
            llvm_int_graphvis_internal(buf, llvm_int_const_unwrap(lit));
            return;
        case LLVM_FLOAT:
            //return llvm_float_graphvis_floaternal(lit_name, llvm_float_const_unwrap(lit));
            todo();
        case LLVM_STRING:
            //return llvm_string_graphvis_stringernal(lit_name, llvm_string_const_unwrap(lit));
            todo();
        case LLVM_VOID:
            llvm_void_graphvis_internal(buf, llvm_void_const_unwrap(lit));
            return;
        case LLVM_FUNCTION_NAME:
            todo();
    }
    unreachable("");
}

static void llvm_expr_graphvis_internal(String* buf, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            todo();
        case LLVM_LITERAL:
            llvm_literal_graphvis_internal(buf, llvm_literal_const_unwrap(expr));
            return;
        case LLVM_FUNCTION_CALL:
            todo();
    }
    unreachable("");
}

static void llvm_return_graphvis_internal(String* buf, const Llvm_return* rtn) {
    // TODO: possible abstraction 1
    label(buf, rtn->name_self, str_view_from_cstr("return"));

    arrow_names(buf, rtn->name_self, rtn->child);
    llvm_graphvis_internal(buf, llvm_from_get_name(rtn->child));
}

static void llvm_alloca_graphvis_internal(String* buf, const Llvm_alloca* alloca) {
    label_ex(buf, alloca->name, str_view_from_cstr("alloca"), alloca->name);

    // TODO
}

static void llvm_store_another_llvm_graphvis_internal(String* buf, const Llvm_store_another_llvm* store) {
    label(buf, store->name, str_view_from_cstr("store"));

    arrow_names_label(buf, store->name, store->llvm_src, str_view_from_cstr("src"));
    arrow_names_label(buf, store->name, store->llvm_dest, str_view_from_cstr("dest"));
}

static void llvm_graphvis_internal(String* buf, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            todo();
        case LLVM_EXPR:
            llvm_expr_graphvis_internal(buf, llvm_expr_const_unwrap(llvm));
            return;
        case LLVM_DEF:
            llvm_def_graphvis_internal(buf, llvm_def_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            todo();
        case LLVM_ARRAY_ACCESS:
            todo();
        case LLVM_FUNCTION_PARAMS:
            todo();
        case LLVM_RETURN:
            llvm_return_graphvis_internal(buf, llvm_return_const_unwrap(llvm));
            return;
        case LLVM_GOTO:
            todo();
        case LLVM_COND_GOTO:
            todo();
        case LLVM_ALLOCA:
            llvm_alloca_graphvis_internal(buf, llvm_alloca_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            todo();
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_store_another_llvm_graphvis_internal(buf, llvm_store_another_llvm_const_unwrap(llvm));
            return;
    }
    unreachable("");
}

Str_view llvm_graphvis(const Llvm_block* block) {
    (void) block;
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_cstr(&a_print, &buf, "bgcolor=\"black\"\n");
    string_extend_cstr(&a_print, &buf, "node [style=filled, fillcolor=\"black\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_print, &buf, "edge [color=\"white\", fontcolor=\"white\"];");

    Alloca_iter iter = all_tbl_iter_new(SCOPE_BUILTIN);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        llvm_graphvis_internal(&buf, curr);
    }

    //llvm_block_graphvis_internal(&buf, block);

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
