#include <ir.h>
#include <tast.h>
#include <util.h>
#include <ir_utils.h>
#include <lang_type_print.h>
#include <ir_print_graphvis.h>
#include <symbol_iter.h>
#include <util.h>
#include <parser_utils.h>

static Strv ir_expr_graphvis_internal(const Ir_expr* expr);

// idea: edge from return_stmt -> child should be drawn in the ir_return_graphvis_internal, not ir_expr_graphvis_internal or whatever

#define extend_source_loc(buf) extend_source_loc_internal(__FILE__, __LINE__, buf)

static void extend_source_loc_internal(const char* file, int line, String* buf) {
    string_extend_cstr(&a_temp, buf, "// ");
    string_extend_cstr(&a_temp, buf, file);
    string_extend_cstr(&a_temp, buf, ":");
    string_extend_int64_t(&a_temp, buf, line);
    string_extend_cstr(&a_temp, buf, "\n");
}

static Strv ir_graphvis_internal(Name ir_name, const Ir* ir);

static void extend_name_graphvis(String* buf, Name name) {
    extend_name_log_internal(false, buf, name);
}

static void arrow_names(String* buf, Name parent, Name child) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_temp, buf, " -> ");
    extend_name_graphvis(buf, child);
    string_extend_cstr(&a_temp, buf, ";\n");
}

static void arrow_names_label(String* buf, Name lhs, Name rhs, Strv label) {
    extend_name_graphvis(buf, lhs);
    string_extend_cstr(&a_temp, buf, " -> ");
    extend_name_graphvis(buf, rhs);
    string_extend_cstr(&a_temp, buf, " [label = \"");
    string_extend_strv(&a_temp, buf, label);
    string_extend_cstr(&a_temp, buf, "\"];\n");
}

// TODO: rename or remove these two functions
// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun)
#define child_with_arrow2(buf, parent_name, child, child_fun) \
    do { \
        Name child_name = util_literal_name_new(); \
        arrow_names(buf, parent_name, child_name); \
        string_extend_strv(&a_temp, buf, child_fun(child_name, child)); \
    } while (0)

// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun)
#define child_with_arrow1(buf, parent_name, child, child_fun) \
    do { \
        Name child_name = util_literal_name_new(); \
        arrow_names(buf, parent_name, child_name); \
        string_extend_strv(&a_temp, buf, child_fun(child)); \
    } while (0)

// draw child and draw arrow to that child
// (String* buf, Name parent_name, void* child, <Fun*> child_fun, Strv label)
#define child_with_arrow_label(buf, parent_name, child, child_fun, label) \
    do { \
        Name child_name = util_literal_name_new(); \
        arrow_names_label(buf, parent_name, child_name, label); \
        string_extend_strv(&a_temp, buf, child_fun(child_name, child)); \
    } while (0)

// draw lang_type and draw arrow to that lang_type
static void lang_type_with_arrow(String* buf, Name parent, Lang_type child) {
    extend_name_graphvis(buf, parent);
    string_extend_cstr(&a_temp, buf, " -> ");
    string_extend_strv(&a_temp, buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, child));
    string_extend_cstr(&a_temp, buf, ";\n");
}

static void label(String* buf, Name name, Strv label) {
    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_temp, buf, " [label = \"");
    string_extend_strv(&a_temp, buf, label);
    string_extend_cstr(&a_temp, buf, "\"];\n");
}

static void label_ex(String* buf, Name name, Strv label, Name actual_name) {
    extend_name_graphvis(buf, name);
    string_extend_cstr(&a_temp, buf, " [label = \"");
    string_extend_strv(&a_temp, buf, label);
    string_extend_cstr(&a_temp, buf, " ");
    extend_name_graphvis(buf, actual_name);
    string_extend_cstr(&a_temp, buf, "\"];\n");
}

//static void arrow(Name lhs, Name rhs) {
//    todo();
//}
//
//static void arrow_name_lang_type(Name lhs, Lang_type rhs) {
//    todo();
//}

static Strv ir_block_graphvis_internal(Name block_name, const Ir_block* block) {
    String buf = {0};
    extend_source_loc(&buf);

    Name children_name = util_literal_name_new();
    label(&buf, block_name, sv("block"));
    arrow_names(&buf, block_name, children_name);
    label(&buf, children_name, sv("block children"));

    Alloca_iter iter = ir_tbl_iter_new(block->scope_id);
    Ir* curr = NULL;
    while (ir_tbl_iter_next(&curr, &iter)) {
        Name child_name = util_literal_name_new();
        arrow_names(&buf, children_name, child_name);
        string_extend_strv(&a_temp, &buf, ir_graphvis_internal(child_name, curr));
    }

    for (size_t idx = 0; idx < block->children.info.count; idx++) {
        string_extend_strv(&a_temp, &buf, ir_graphvis_internal(children_name, vec_at(block->children, idx)));
    }
    string_extend_cstr(&a_temp, &buf, "\n");

    return string_to_strv(buf);
}

static Strv ir_variable_def_graphvis_internal(const Ir_variable_def* def) {
    String buf = {0};
    extend_source_loc(&buf);

    lang_type_with_arrow(&buf, def->name_self, def->lang_type);

    //label(&buf, params_name, string_to_strv(buf));

    return string_to_strv(buf);
}

static Strv ir_function_params_graphvis_internal(Name params_name, const Ir_function_params* params) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, params_name, sv("params"));
    for (size_t idx = 0; idx < params->params.info.count; idx++) {
        child_with_arrow1(&buf, params_name, vec_at(params->params, idx), ir_variable_def_graphvis_internal);
    }

    return string_to_strv(buf);
}

static Strv ir_function_decl_graphvis_internal(const Ir_function_decl* decl) {
    String buf = {0};
    extend_source_loc(&buf);

    label_ex(&buf, decl->name, sv("function_decl"), decl->name);

    extend_name_graphvis(&buf, decl->name);
    string_extend_cstr(&a_temp, &buf, " -> \"");
    string_extend_strv(&a_temp, &buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, decl->return_type));
    string_extend_cstr(&a_temp, &buf, "\"");
    string_extend_cstr(&a_temp, &buf, " [label = \"return type\"];\n");

    child_with_arrow2(&buf, decl->name, decl->params, ir_function_params_graphvis_internal);

    return string_to_strv(buf);
}

static Strv ir_function_def_graphvis_internal(Name def_name, const Ir_function_def* def) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, def_name, sv("function_def"));

    arrow_names(&buf, def_name, def->decl->name);
    string_extend_strv(&a_temp, &buf, ir_function_decl_graphvis_internal(def->decl));

    child_with_arrow2(&buf, def_name, def->body, ir_block_graphvis_internal);

    return string_to_strv(buf);
}

static Strv ir_int_graphvis_internal(const Ir_int* lit) {
    String num_buf = {0};
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_int64_t(&a_temp, &num_buf, lit->data);
    string_extend_cstr(&a_temp, &num_buf, " ");
    string_extend_strv(&a_temp, &num_buf, lang_type_print_internal(LANG_TYPE_MODE_MSG, lit->lang_type));
    label(&buf, lit->name, string_to_strv(num_buf));
    todo();

    return string_to_strv(buf);
}

static Strv ir_function_name_graphvis_internal(const Ir_function_name* lit) {
    String buf = {0};
    String buf_fun_name = {0};

    extend_name_graphvis(&buf_fun_name, lit->fun_name);
    label(&buf, lit->name_self/*TODO*/, string_to_strv(buf_fun_name)); // TODO: print name_self?

    return string_to_strv(buf);
}

static Strv ir_literal_graphvis_internal(const Ir_literal* lit) {
    switch (lit->type) {
        case IR_INT:
            return ir_int_graphvis_internal(ir_int_const_unwrap(lit));
        case IR_FLOAT:
            //return ir_float_graphvis_floaternal(lit_name, ir_float_const_unwrap(lit));
            todo();
        case IR_STRING:
            //return ir_string_graphvis_stringernal(lit_name, ir_string_const_unwrap(lit));
            todo();
        case IR_VOID:
            //return ir_void_graphvis_voidernal(lit_name, ir_void_const_unwrap(lit));
            todo();
        case IR_FUNCTION_NAME:
            return ir_function_name_graphvis_internal(ir_function_name_const_unwrap(lit));
    }
    unreachable("");
}

static Strv ir_function_call_graphvis_internal(const Ir_function_call* call) {
    String buf = {0};

    label(&buf, call->name_self, sv("fun_call"));

    arrow_names_label(&buf, call->name_self, call->callee, sv("callee"));

    Name args_name = util_literal_name_new();

    arrow_names(&buf, call->name_self, args_name);

    for (size_t idx = 0; idx < call->args.info.count; idx++) {
        child_with_arrow2(&buf, args_name, ir_from_ir_name(vec_at(call->args, idx)), ir_graphvis_internal);
    }

    return string_to_strv(buf);
}

static Strv ir_expr_graphvis_internal(const Ir_expr* expr) {
    switch (expr->type) {
        case IR_OPERATOR:
            todo();
        case IR_LITERAL:
            return ir_literal_graphvis_internal(ir_literal_const_unwrap(expr));
        case IR_FUNCTION_CALL:
            return ir_function_call_graphvis_internal(ir_function_call_const_unwrap(expr));
    }
    unreachable("");
}

static Strv ir_def_graphvis_internal(Name def_name, const Ir_def* def) {
    switch (def->type) {
        case IR_FUNCTION_DEF:
            return ir_function_def_graphvis_internal(def_name, ir_function_def_const_unwrap(def));
        case IR_VARIABLE_DEF:
            return ir_variable_def_graphvis_internal(ir_variable_def_const_unwrap(def));
        case IR_STRUCT_DEF:
            todo();
        case IR_PRIMITIVE_DEF:
            todo();
        case IR_FUNCTION_DECL:
            todo();
        case IR_LABEL:
            todo();
        case IR_LITERAL_DEF:
            todo();
    }
    unreachable("");
}

static Strv ir_return_graphvis_internal(Name rtn_name, const Ir_return* rtn) {
    String buf = {0};
    extend_source_loc(&buf);

    label(&buf, rtn_name, sv("return"));

    arrow_names(&buf, rtn_name, rtn->child);

    return string_to_strv(buf);
}

static Strv ir_alloca_graphvis_internal(Name all_name, const Ir_alloca* lang_alloca) {
    String buf = {0};

    label(&buf, all_name, sv("return"));

    return string_to_strv(buf);
}

static Strv ir_store_another_ir_graphvis_internal(Name store_name, const Ir_store_another_ir* store) {
    String buf = {0};

    label(&buf, store_name, sv("store_another_ir"));
    // TODO
    //
    return string_to_strv(buf);
}

static Strv ir_load_another_ir_graphvis_internal(Name load_name, const Ir_load_another_ir* load) {
    String buf = {0};

    label(&buf, load_name, sv("load_another_ir"));
    // TODO
    //
    return string_to_strv(buf);
}

static Strv ir_graphvis_internal(Name ir_name, const Ir* ir) {
    switch (ir->type) {
        case IR_BLOCK:
            return ir_block_graphvis_internal(ir_name, ir_block_const_unwrap(ir));
        case IR_EXPR:
            return ir_expr_graphvis_internal(ir_expr_const_unwrap(ir));
        case IR_DEF:
            return ir_def_graphvis_internal(ir_name, ir_def_const_unwrap(ir));
        case IR_LOAD_ELEMENT_PTR:
            todo();
        case IR_ARRAY_ACCESS:
            todo();
        case IR_FUNCTION_PARAMS:
            todo();
        case IR_RETURN:
            return ir_return_graphvis_internal(ir_name, ir_return_const_unwrap(ir));
        case IR_GOTO:
            todo();
        case IR_COND_GOTO:
            todo();
        case IR_ALLOCA:
            return ir_alloca_graphvis_internal(ir_name, ir_alloca_const_unwrap(ir));
        case IR_LOAD_ANOTHER_IR:
            return ir_load_another_ir_graphvis_internal(ir_name, ir_load_another_ir_const_unwrap(ir));
        case IR_STORE_ANOTHER_IR:
            return ir_store_another_ir_graphvis_internal(ir_name, ir_store_another_ir_const_unwrap(ir));
    }
    unreachable("");
}

Strv ir_graphvis(const Ir_block* block) {
    String buf = {0};
    extend_source_loc(&buf);

    string_extend_cstr(&a_temp, &buf, "digraph G {\n");
    string_extend_cstr(&a_temp, &buf, "bgcolor=\"#1e1e1e\"\n");
    string_extend_cstr(&a_temp, &buf, "node [style=filled, fillcolor=\"#2e2e2e\", fontcolor=\"white\", color=\"white\"];\n");
    string_extend_cstr(&a_temp, &buf, "edge [color=\"white\", fontcolor=\"white\"];");

    Name block_name = util_literal_name_new();
    extend_name_graphvis(&buf, block_name);
    string_extend_cstr(&a_temp, &buf, " [label = block];\n");
    string_extend_strv(&a_temp, &buf, ir_block_graphvis_internal(block_name, block));

    string_extend_cstr(&a_temp, &buf, "}\n");

    return string_to_strv(buf);
}
