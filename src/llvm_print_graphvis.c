#include <llvm.h>
#include <tast.h>
#include <util.h>
#include <llvm_utils.h>
#include <lang_type_print.h>
#include <llvm_print_graphvis.h>
#include <symbol_iter.h>
#include <util.h>
#include <parser_utils.h>

static Str_view llvm_graphvis_internal(const Llvm* llvm, int indent);

static void extend_name_graphvis(String* buf, Name name) {
    string_extend_cstr(&a_print, buf, "\"");
    extend_name_log_internal(false, buf, name);
    string_extend_cstr(&a_print, buf, "\"");
}

static Str_view llvm_block_graphvis_internal(const Llvm_block* block, int indent) {
    String buf = {0};

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        Str_view arg_text = llvm_graphvis_internal(curr, indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        Str_view arg_text = llvm_graphvis_internal(vec_at(&block->children, idx), indent + INDENT_WIDTH);
        string_extend_strv(&a_print, &buf, arg_text);
    }
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

static Str_view llvm_function_params_graphvis_internal(Name params_name, const Llvm_function_params* params) {
    String buf = {0};

    for (size_t idx = 0; idx < params.params.info.count; idx++) {
        string_extend_strv(&a_print, &buf, decl_name);
        string_extend_cstr(&a_print, &buf, " -> ");

        string_extend_strv(&a_print, &buf, "thing params");
        string_extend_strv(&a_print, &buf, llvm_variable_def_graphvis_internal());
        string_extend_cstr(&a_print, &buf, "\n;");
    }

    return string_to_strv(buf);
}

static Str_view llvm_function_decl_graphvis_internal_common(const Llvm_function_decl* decl, bool is_def) {
    String buf = {0};

    extend_name_graphvis(&buf, decl->name);
    string_extend_cstr(&a_print, &buf, " -> ");
    extend_name_graphvis(&buf, decl->name);
    if (is_def) {
        string_extend_cstr(&a_print, &buf, " [label = \"function_def\"];\n");
    } else {
        string_extend_cstr(&a_print, &buf, " [label = \"function_decl\"];\n");
    }

    string_extend_strv(&a_print, &buf, name_print_internal(NAME_LOG, false, decl->name));
    string_extend_cstr(&a_print, &buf, " -> \"");
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_LOG, decl->return_type));
    string_extend_cstr(&a_print, &buf, "\"\n;");

    Name params_name = util_literal_name_new2();
    extend_name(NAME_LOG, &buf, params_name);
    string_extend_cstr(&a_print, &buf, " [label = params]\n;");

    extend_name(NAME_LOG, &buf, decl->name);
    string_extend_cstr(&a_print, &buf, " -> ");
    extend_name(NAME_LOG, &buf, params_name);
    string_extend_strv(&a_print, &buf, llvm_function_params_graphvis_internal(params_name, decl->params));

    return string_to_strv(buf);
}

static Str_view llvm_function_decl_graphvis_internal(const Llvm_function_decl* decl) {
    return llvm_function_decl_graphvis_internal_common(decl, false);
}

static Str_view llvm_function_def_graphvis_internal(const Llvm_function_def* def) {
    String buf = {0};

    string_extend_strv(&a_print, &buf, llvm_function_decl_graphvis_internal_common(def->decl, true));

    return string_to_strv(buf);
}

static Str_view llvm_def_graphvis_internal(const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_graphvis_internal(llvm_function_def_const_unwrap(def));
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

static Str_view llvm_graphvis_internal(const Llvm* llvm, int indent) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return llvm_block_graphvis_internal(llvm_block_const_unwrap(llvm), indent);
        case LLVM_EXPR:
            todo();
        case LLVM_DEF:
            return llvm_def_graphvis_internal(llvm_def_const_unwrap(llvm));
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

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_strv(&a_print, &buf, llvm_block_graphvis_internal(block, 0));
    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
