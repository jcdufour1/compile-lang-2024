#include <llvm_print_graphvis.h>
#include <parser_utils.h>
#include <symbol_iter.h>

// NOTE: util_literal_name_new2 to create placeholder name for Llvm_function_call generated in llvm_function_call_graphvis_internal, etc.

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void llvm_graphvis_internal(String* buf, Name parent, const Llvm* llvm);

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
    string_extend_cstr(&a_print, buf, "\";\n");
}

static void label(String* buf, Name name, Str_view label) {
    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

static void llvm_block_graphvis_internal(String* buf, const Llvm_block* block) {
    Name block_name = util_literal_name_new2();
    label(buf, block_name, str_view_from_cstr("block"));

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        llvm_graphvis_internal(buf, block_name, curr);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        llvm_graphvis_internal(buf, block_name, vec_at(&block->children, idx));
    }
}

static void llvm_function_def_graphvis_internal(String* buf, Name parent, const Llvm_function_def* def) {
    Name def_name = util_literal_name_new2();
    arrow_names(buf, parent, def_name);
    label(buf, def_name, str_view_from_cstr("function def"));
    // TODO: decl
    // TODO: block
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

static void llvm_graphvis_internal(String* buf, Name parent, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            todo();
        case LLVM_EXPR:
            todo();
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
            todo();
        case LLVM_GOTO:
            todo();
        case LLVM_COND_GOTO:
            todo();
        case LLVM_ALLOCA:
            todo();
        case LLVM_LOAD_ANOTHER_LLVM:
            todo();
        case LLVM_STORE_ANOTHER_LLVM:
            todo();
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

    llvm_block_graphvis_internal(&buf, block);

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
