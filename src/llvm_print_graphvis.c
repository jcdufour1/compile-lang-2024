#include <llvm.h>
#include <tast.h>
#include <util.h>
#include <llvm_utils.h>
#include <lang_type_print.h>
#include <llvm_print_graphvis.h>
#include <symbol_iter.h>
#include <util.h>
#include <parser_utils.h>

static Str_view llvm_expr_graphvis_internal(const Llvm_expr* expr);

// idea: edge from return_stmt -> child should be drawn in the llvm_return_graphvis_internal, not llvm_expr_graphvis_internal or whatever

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void extend_source_loc_internal(const char* file, int line, String* buf) {
    string_extend_cstr(&a_print, buf, "// ");
    string_extend_cstr(&a_print, buf, file);
    string_extend_cstr(&a_print, buf, ":");
    string_extend_int64_t(&a_print, buf, line);
    string_extend_cstr(&a_print, buf, "\n");
}

static Str_view llvm_graphvis_internal(Name llvm_name, const Llvm* llvm);

static void extend_name_graphvis(String* buf, Name name) {
    extend_name_log_internal(false, buf, name);
}

static void arrow_names(String* buf, Name parent, Name child) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_print, buf, ";\n");
}

static void arrow_names_label(String* buf, Name lhs, Name rhs, Str_view label) {
    extend_name_graphvis(buf, lhs);
    string_extend_cstr(&a_print, buf, " -> ");
    extend_name_graphvis(buf, rhs);
    string_extend_cstr(&a_print, buf, " [label = \"");
    string_extend_strv(&a_print, buf, label);
    string_extend_cstr(&a_print, buf, "\"];\n");
}

// TODO: rename or remove these two functions
// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun)
#define child_with_arrow2(buf, parent_name, child, child_fun) \
    do { \
        Name child_name = util_literal_name_new2(); \
        arrow_names(buf, parent_name, child_name); \
        string_extend_strv(&a_print, buf, child_fun(child_name, child)); \
    } while (0)

// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun)
#define child_with_arrow1(buf, parent_name, child, child_fun) \
    do { \
        Name child_name = util_literal_name_new2(); \
        arrow_names(buf, parent_name, child_name); \
        string_extend_strv(&a_print, buf, child_fun(child)); \
    } while (0)

// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun, Str_view label)
#define child_with_arrow_label(buf, parent_name, child, child_fun, label) \
    do { \
        Name child_name = util_literal_name_new2(); \
        arrow_names_label(buf, parent_name, child_name, label); \
        string_extend_strv(&a_print, buf, child_fun(child_name, child)); \
    } while (0)

// draw lang_type and draw arrow to that lang_type
static void lang_type_with_arrow(String* buf, Name parent, Lang_type child) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_print, buf, " -> ");
    string_extend_strv(&a_print, buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, child));
    string_extend_cstr(&a_print, buf, ";\n");
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

//static void arrow(Name lhs, Name rhs) {
//    todo();
//}
//
//static void arrow_name_lang_type(Name lhs, Lang_type rhs) {
//    todo();
//}

static Str_view llvm_block_graphvis_internal(Name block_name, const Llvm_block* block) {
    String buf = {0};
    extend_source_loc(&buf);

    Name children_name = util_literal_name_new2();
    label(&buf, block_name, str_view_from_cstr("block"));
    arrow_names(&buf, block_name, children_name);
    label(&buf, children_name, str_view_from_cstr("block children"));

    Alloca_iter iter = all_tbl_iter_new(block->scope_id);
    Llvm* curr = NULL;
    while (all_tbl_iter_next(&curr, &iter)) {
        Name child_name = util_literal_name_new2();
        arrow_names(&buf, children_name, child_name);
        string_extend_strv(&a_print, &buf, llvm_graphvis_internal(child_name, curr));
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        string_extend_strv(&a_print, &buf, llvm_graphvis_internal(children_name, vec_at(&block->children, idx)));
    }
    string_extend_cstr(&a_print, &buf, "\n");

    return string_to_strv(buf);
}

static Str_view llvm_variable_def_graphvis_internal(const Llvm_variable_def* def) {
    String buf = {0};
    extend_source_loc(&buf);

    lang_type_with_arrow(&buf, def->name_self, def->lang_type);

    //label(&buf, params_name, string_to_strv(buf));

    return string_to_strv(buf);
}

static Str_view llvm_function_params_graphvis_internal(Name params_name, const Llvm_function_params* params) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, params_name, str_view_from_cstr("params"));
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        child_with_arrow1(&buf, params_name, vec_at(&params->params, idx), llvm_variable_def_graphvis_internal);
    }

    return string_to_strv(buf);
}

static Str_view llvm_function_decl_graphvis_internal(const Llvm_function_decl* decl) {
    String buf = {0};
    extend_source_loc(&buf);

    label_ex(&buf, decl->name, str_view_from_cstr("function_decl"), decl->name);

    extend_name_graphvis(&buf, decl->name);
    string_extend_cstr(&a_print, &buf, " -> \"");
    string_extend_strv(&a_print, &buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, decl->return_type));
    string_extend_cstr(&a_print, &buf, "\"");
    string_extend_cstr(&a_print, &buf, " [label = \"return type\"];\n");

    child_with_arrow2(&buf, decl->name, decl->params, llvm_function_params_graphvis_internal);

    return string_to_strv(buf);
}

static Str_view llvm_function_def_graphvis_internal(Name def_name, const Llvm_function_def* def) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, def_name, str_view_from_cstr("function_def"));

    arrow_names(&buf, def_name, def->decl->name);
    string_extend_strv(&a_print, &buf, llvm_function_decl_graphvis_internal(def->decl));

    child_with_arrow2(&buf, def_name, def->body, llvm_block_graphvis_internal);

    return string_to_strv(buf);
}

static Str_view llvm_int_graphvis_internal(const Llvm_int* lit) {
    String num_buf = {0};
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_int64_t(&a_print, &num_buf, lit->data);
    string_extend_cstr(&a_print, &num_buf, " ");
    string_extend_strv(&a_print, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(&buf, lit->name, string_to_strv(num_buf));
    todo();

    return string_to_strv(buf);
}

static Str_view llvm_function_name_graphvis_internal(const Llvm_function_name* lit) {
    String buf = {0};
    String buf_fun_name = {0};

    extend_name_graphvis(&buf_fun_name, lit->fun_name);
    label(&buf, lit->name_self/*TODO*/, string_to_strv(buf_fun_name)); // TODO: print name_self?

    return string_to_strv(buf);
}

static Str_view llvm_literal_graphvis_internal(const Llvm_literal* lit) {
    switch (lit->type) {
        case LLVM_INT:
            return llvm_int_graphvis_internal(llvm_int_const_unwrap(lit));
        case LLVM_FLOAT:
            //return llvm_float_graphvis_floaternal(lit_name, llvm_float_const_unwrap(lit));
            todo();
        case LLVM_STRING:
            //return llvm_string_graphvis_stringernal(lit_name, llvm_string_const_unwrap(lit));
            todo();
        case LLVM_VOID:
            //return llvm_void_graphvis_voidernal(lit_name, llvm_void_const_unwrap(lit));
            todo();
        case LLVM_FUNCTION_NAME:
            return llvm_function_name_graphvis_internal(llvm_function_name_const_unwrap(lit));
    }
    unreachable("");
}

static Str_view llvm_function_call_graphvis_internal(const Llvm_function_call* call) {
    String buf = {0};

    label(&buf, call->name_self, str_view_from_cstr("fun_call"));

    arrow_names_label(&buf, call->name_self, call->callee, str_view_from_cstr("callee"));

    Name args_name = util_literal_name_new2();

    arrow_names(&buf, call->name_self, args_name);

    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        child_with_arrow2(&buf, args_name, llvm_from_get_name(vec_at(&call->args, idx)), llvm_graphvis_internal);
    }

    return string_to_strv(buf);
}

static Str_view llvm_expr_graphvis_internal(const Llvm_expr* expr) {
    switch (expr->type) {
        case LLVM_OPERATOR:
            todo();
        case LLVM_LITERAL:
            return llvm_literal_graphvis_internal(llvm_literal_const_unwrap(expr));
        case LLVM_FUNCTION_CALL:
            return llvm_function_call_graphvis_internal(llvm_function_call_const_unwrap(expr));
    }
    unreachable("");
}

static Str_view llvm_def_graphvis_internal(Name def_name, const Llvm_def* def) {
    switch (def->type) {
        case LLVM_FUNCTION_DEF:
            return llvm_function_def_graphvis_internal(def_name, llvm_function_def_const_unwrap(def));
        case LLVM_VARIABLE_DEF:
            return llvm_variable_def_graphvis_internal(llvm_variable_def_const_unwrap(def));
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

static Str_view llvm_return_graphvis_internal(Name rtn_name, const Llvm_return* rtn) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, rtn_name, str_view_from_cstr("return"));

    arrow_names(&buf, rtn_name, rtn->child);

    return string_to_strv(buf);
}

static Str_view llvm_alloca_graphvis_internal(Name all_name, const Llvm_alloca* alloca) {
    String buf = {0};

    label(&buf, all_name, str_view_from_cstr("return"));

    return string_to_strv(buf);
}

static Str_view llvm_store_another_llvm_graphvis_internal(Name store_name, const Llvm_store_another_llvm* store) {
    String buf = {0};

    label(&buf, store_name, str_view_from_cstr("store_another_llvm"));
    // TODO
    //
    return string_to_strv(buf);
}

static Str_view llvm_load_another_llvm_graphvis_internal(Name load_name, const Llvm_load_another_llvm* load) {
    String buf = {0};

    label(&buf, load_name, str_view_from_cstr("load_another_llvm"));
    // TODO
    //
    return string_to_strv(buf);
}

static Str_view llvm_graphvis_internal(Name llvm_name, const Llvm* llvm) {
    switch (llvm->type) {
        case LLVM_BLOCK:
            return llvm_block_graphvis_internal(llvm_name, llvm_block_const_unwrap(llvm));
        case LLVM_EXPR:
            return llvm_expr_graphvis_internal(llvm_expr_const_unwrap(llvm));
        case LLVM_DEF:
            return llvm_def_graphvis_internal(llvm_name, llvm_def_const_unwrap(llvm));
        case LLVM_LOAD_ELEMENT_PTR:
            todo();
        case LLVM_ARRAY_ACCESS:
            todo();
        case LLVM_FUNCTION_PARAMS:
            todo();
        case LLVM_RETURN:
            return llvm_return_graphvis_internal(llvm_name, llvm_return_const_unwrap(llvm));
        case LLVM_GOTO:
            todo();
        case LLVM_COND_GOTO:
            todo();
        case LLVM_ALLOCA:
            return llvm_alloca_graphvis_internal(llvm_name, llvm_alloca_const_unwrap(llvm));
        case LLVM_LOAD_ANOTHER_LLVM:
            return llvm_load_another_llvm_graphvis_internal(llvm_name, llvm_load_another_llvm_const_unwrap(llvm));
        case LLVM_STORE_ANOTHER_LLVM:
            return llvm_store_another_llvm_graphvis_internal(llvm_name, llvm_store_another_llvm_const_unwrap(llvm));
    }
    unreachable("");
}

Str_view llvm_graphvis(const Llvm_block* block) {
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_print, &buf, "digraph G {\n");
    string_extend_cstr(&a_print, &buf, "bgcolor=\"#1e1e1e\"\n");
    string_extend_cstr(&a_print, &buf, "node [style=filled, fillcolor=\"#2e2e2e\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_print, &buf, "edge [color=\"white\", fontcolor=\"white\"];");

    Name block_name = util_literal_name_new2();
    extend_name_graphvis(&buf, block_name);
    string_extend_cstr(&a_print, &buf, " [label = block];\n");
    string_extend_strv(&a_print, &buf, llvm_block_graphvis_internal(block_name, block));

    string_extend_cstr(&a_print, &buf, "}\n");

    return string_to_strv(buf);
}
