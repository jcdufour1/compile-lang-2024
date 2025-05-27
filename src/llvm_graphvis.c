#include <llvm_print_graphvis.h>
#include <parser_utils.h>
#include <symbol_iter.h>

// NOTE: util_literal_name_new2 to create placeholder name for Llvm_function_call generated in llvm_function_call_graphvis_internal, etc.

static Alloca_table already_visited = {0};

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void llvm_graphvis_internal(String* buf, Name parent, const Llvm* llvm);

static void llvm_variable_def_graphvis_internal(String* buf, Name parent, const Llvm_variable_def* def);

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

static void arrow_names(String* buf, Name parent, Name child) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_print, buf, ";\n");
}

static void arrow_names_label(String* buf, Name parent, Name child, Str_view label) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void label(String* buf, Name name, Str_view label) {
    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void label_ex(String* buf, Name name, Str_view label, Name actual_name) {
    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, " ");
    extend_name_graphvis(buf, actual_name);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void llvm_block_graphvis_internal(String* buf, Name parent, const Llvm_block* block) {
    extend_source_loc(buf);

    String scope_buf = {0};
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr("block (scope "));
    string_extend_size_t(&a_print, &scope_buf, block->scope_id);
    string_extend_strv(&a_print, &scope_buf, str_view_from_cstr(")"));

    Name block_name = util_literal_name_new2();

    String block_buf = {0};
    if (parent.base.count > 0) {
        arrow_names(buf, parent, block_name);
        label(buf, block_name, string_to_strv(scope_buf));
    }

    label(buf, block_name, string_to_strv(scope_buf));

    Name prev = block_name;
    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        String idx_buf = {0};
        string_extend_size_t(&a_print, &idx_buf, idx);
        llvm_graphvis_internal(buf, block_name, vec_at(&block->children, idx));
        // TODO: move extend_source_loc to arrow_names_label, etc.
        arrow_names_label(buf, prev, llvm_tast_get_name(vec_at(&block->children, idx)), string_to_strv(idx_buf));
        unwrap(all_tbl_add_ex(&already_visited, vec_at(&block->children, idx)));
        prev = llvm_tast_get_name(vec_at(&block->children, idx));
    }

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        if (all_tbl_add_ex(&already_visited, curr)) {
            llvm_graphvis_internal(buf, block_name, curr);
        }
    }
}

static void llvm_function_params_graphvis_internal(String* buf, Name parent, const Llvm_function_params* params) {
    extend_source_loc(buf);

    Name params_name = util_literal_name_new2();
    arrow_names(buf, parent, params_name);
    label(buf, params_name, str_view_from_cstr("params"));

    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        llvm_variable_def_graphvis_internal(buf, params_name, vec_at(&params->params, idx));
    }
}

static void llvm_variable_def_graphvis_internal(String* buf, Name parent, const Llvm_variable_def* def) {
    extend_source_loc(buf);

    arrow_names(buf, parent, def->name_self);
    label_ex(buf, def->name_self, str_view_from_cstr("variable_def"), def->name_corr_param);
}

static void llvm_function_decl_graphvis_internal(String* buf, Name parent, const Llvm_function_decl* decl) {
    extend_source_loc(buf);

    // TODO: possible abstraction 2
    arrow_names(buf, parent, decl->name);
    label_ex(buf, decl->name, str_view_from_cstr("function decl"), decl->name);

    llvm_function_params_graphvis_internal(buf, decl->name, decl->params);
    // TODO
    //string_extend_strv(&a_print, buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, decl->return_type));
}

static void llvm_function_def_graphvis_internal(String* buf, Name parent, const Llvm_function_def* def) {
    extend_source_loc(buf);

    Name def_name = util_literal_name_new2();
    arrow_names(buf, parent, def_name);
    label(buf, def_name, str_view_from_cstr("function def"));

    llvm_function_decl_graphvis_internal(buf, def_name, def->decl);
    llvm_block_graphvis_internal(buf, def_name, def->body);
}

static void llvm_def_graphvis_internal(String* buf, Name parent, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            llvm_function_def_graphvis_internal(buf, parent, llvm_function_def_const_unwrap(def));
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

static void llvm_int_graphvis_internal(String* buf, Name parent, const Llvm_int* lit) {
    extend_source_loc(buf);

    String num_buf = {0};
    extend_source_loc(buf);

    arrow_names(buf, parent, lit->name);

    string_extend_int64_t(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(buf, lit->name, string_to_strv(num_buf));
}

static void llvm_void_graphvis_internal(String* buf, Name parent, const Llvm_void* lit) {
    extend_source_loc(buf);

    arrow_names(buf, parent, lit->name);
    label(buf, lit->name, str_view_from_cstr("void"));
}

static void llvm_literal_graphvis_internal(String* buf, Name parent, const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_INT:
            llvm_int_graphvis_internal(buf, parent, llvm_int_const_unwrap(lit));
            return;
        case LLVM_FLOAT:
            //return llvm_float_graphvis_floaternal(lit_name, llvm_float_const_unwrap(lit));
            todo();
        case LLVM_STRING:
            //return llvm_string_graphvis_stringernal(lit_name, llvm_string_const_unwrap(lit));
            todo();
        case LLVM_VOID:
            llvm_void_graphvis_internal(buf, parent, llvm_void_const_unwrap(lit));
            return;
        case LLVM_FUNCTION_NAME:
            todo();
    }
    unreachable("");
}

static void llvm_expr_graphvis_internal(String* buf, Name parent, const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            todo();
        case LLVM_LITERAL:
            llvm_literal_graphvis_internal(buf, parent, llvm_literal_const_unwrap(expr));
            return;
        case LLVM_FUNCTION_CALL:
            todo();
    }
    unreachable("");
}

static void llvm_return_graphvis_internal(String* buf, Name parent, const Llvm_return* rtn) {
    extend_source_loc(buf);

    // TODO: possible abstraction 1
    Name rtn_name = util_literal_name_new2();
    arrow_names(buf, parent, rtn_name);
    label(buf, rtn_name, str_view_from_cstr("return"));

    llvm_graphvis_internal(buf, rtn_name, llvm_from_get_name(rtn->child));
}

static void llvm_alloca_graphvis_internal(String* buf, Name parent, const Llvm_alloca* alloca) {
    extend_source_loc(buf);

    arrow_names(buf, parent, alloca->name);
    label_ex(buf, alloca->name, str_view_from_cstr("alloca"), alloca->name);

    // TODO
}

static void llvm_store_another_llvm_graphvis_internal(String* buf, Name parent, const Llvm_store_another_llvm* store) {
    extend_source_loc(buf);

    arrow_names(buf, parent, store->name);
    label(buf, store->name, str_view_from_cstr("store"));

    arrow_names_label(buf, store->name, store->llvm_src, str_view_from_cstr("src"));
    arrow_names_label(buf, store->name, store->llvm_dest, str_view_from_cstr("dest"));
}

static void llvm_graphvis_internal(String* buf, Name parent, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            todo();
        case LLVM_EXPR:
            llvm_expr_graphvis_internal(buf, parent, llvm_expr_const_unwrap(llvm));
            return;
        case LLVM_DEF:
            llvm_def_graphvis_internal(buf, parent, llvm_def_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ELEMENT_PTR:
            todo();
        case LLVM_ARRAY_ACCESS:
            todo();
        case LLVM_FUNCTION_PARAMS:
            todo();
        case LLVM_RETURN:
            llvm_return_graphvis_internal(buf, parent, llvm_return_const_unwrap(llvm));
            return;
        case LLVM_GOTO:
            todo();
        case LLVM_COND_GOTO:
            todo();
        case LLVM_ALLOCA:
            llvm_alloca_graphvis_internal(buf, parent, llvm_alloca_const_unwrap(llvm));
            return;
        case LLVM_LOAD_ANOTHER_LLVM:
            todo();
        case LLVM_STORE_ANOTHER_LLVM:
            llvm_store_another_llvm_graphvis_internal(buf, parent, llvm_store_another_llvm_const_unwrap(llvm));
            return;
    }
    unreachable("");
}

Str_view llvm_graphvis(const Llvm_block* block) {
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_cstr(&a_print, &buf, "bgcolor=\"black\"\n");
    string_extend_cstr(&a_print, &buf, "node [style=filled, fillcolor=\"black\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_print, &buf, "edge [color=\"white\", fontcolor=\"white\"];");

    llvm_block_graphvis_internal(&buf, (Name) {0}, block);

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
