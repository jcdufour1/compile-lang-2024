
#ifndef AUTO_GEN_IR_H
#define AUTO_GEN_IR_H

#include <auto_gen_util.h>
#include <auto_gen_darrs.h>
#include <auto_gen_uast_common.h>

static Uast_type ir_gen_block(void) {
    Uast_type block = {.name = uast_name_new("ir", "block", false, "ir")};

    append_member(&block.members, "Ir_name", "name");
    append_member(&block.members, "Ir_darr", "children");
    append_member(&block.members, "Pos", "pos_end");
    append_member(&block.members, "Scope_id", "scope_id");
    append_member(&block.members, "Cfg_node_darr", "cfg");

    return block;
}

static Uast_type ir_gen_unary(void) {
    Uast_type unary = {.name = uast_name_new("operator", "unary", false, "ir")};

    append_member(&unary.members, "Ir_name", "child");
    append_member(&unary.members, "IR_UNARY_TYPE", "token_type");
    append_member(&unary.members, "Ir_lang_type", "lang_type");
    append_member(&unary.members, "Ir_name", "name");

    return unary;
}

static Uast_type ir_gen_binary(void) {
    Uast_type binary = {.name = uast_name_new("operator", "binary", false, "ir")};

    append_member(&binary.members, "Ir_name", "lhs");
    append_member(&binary.members, "Ir_name", "rhs");
    append_member(&binary.members, "IR_BINARY_TYPE", "token_type");
    append_member(&binary.members, "Ir_lang_type", "lang_type");
    append_member(&binary.members, "Ir_name", "name");

    return binary;
}

static Uast_type ir_gen_operator(void) {
    Uast_type operator = {.name = uast_name_new("expr", "operator", false, "ir")};

    darr_append(&a_gen, &operator.sub_types, ir_gen_unary());
    darr_append(&a_gen, &operator.sub_types, ir_gen_binary());

    return operator;
}

static Uast_type ir_gen_int(void) {
    Uast_type number = {.name = uast_name_new("literal", "int", false, "ir")};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Ir_lang_type", "lang_type");
    append_member(&number.members, "Ir_name", "name");

    return number;
}

static Uast_type ir_gen_float(void) {
    Uast_type number = {.name = uast_name_new("literal", "float", false, "ir")};

    append_member(&number.members, "double", "data");
    append_member(&number.members, "Ir_lang_type", "lang_type");
    append_member(&number.members, "Ir_name", "name");

    return number;
}

static Uast_type ir_gen_string(void) {
    Uast_type string = {.name = uast_name_new("literal", "string", false, "ir")};

    append_member(&string.members, "Strv", "data");
    append_member(&string.members, "Ir_name", "name");

    return string;
}

static Uast_type ir_gen_function_name(void) {
    Uast_type lang_char = {.name = uast_name_new("literal", "function_name", false, "ir")};

    append_member(&lang_char.members, "Ir_name", "fun_name");
    append_member(&lang_char.members, "Ir_name", "name_self");

    return lang_char;
}

static Uast_type ir_gen_void(void) {
    Uast_type lang_void = {.name = uast_name_new("literal", "void", false, "ir")};

    append_member(&lang_void.members, "Ir_name", "name");

    return lang_void;
}

static Uast_type ir_gen_literal(void) {
    Uast_type lit = {.name = uast_name_new("expr", "literal", false, "ir")};

    darr_append(&a_gen, &lit.sub_types, ir_gen_int());
    darr_append(&a_gen, &lit.sub_types, ir_gen_float());
    darr_append(&a_gen, &lit.sub_types, ir_gen_string());
    darr_append(&a_gen, &lit.sub_types, ir_gen_void());
    darr_append(&a_gen, &lit.sub_types, ir_gen_function_name());

    return lit;
}

static Uast_type ir_gen_function_call(void) {
    Uast_type call = {.name = uast_name_new("expr", "function_call", false, "ir")};

    append_member(&call.members, "Ir_name_darr", "args");
    append_member(&call.members, "Ir_name", "name_self");
    append_member(&call.members, "Ir_name", "callee");
    append_member(&call.members, "Ir_lang_type", "lang_type");

    return call;
}

static Uast_type ir_gen_expr(void) {
    Uast_type expr = {.name = uast_name_new("ir", "expr", false, "ir")};

    darr_append(&a_gen, &expr.sub_types, ir_gen_operator());
    darr_append(&a_gen, &expr.sub_types, ir_gen_literal());
    darr_append(&a_gen, &expr.sub_types, ir_gen_function_call());

    return expr;
}

static Uast_type ir_gen_struct_def(void) {
    Uast_type def = {.name = uast_name_new("def", "struct_def", false, "ir")};

    append_member(&def.members, "Ir_struct_def_base", "base");

    return def;
}

static Uast_type ir_gen_function_decl(void) {
    Uast_type def = {.name = uast_name_new("def", "function_decl", false, "ir")};

    append_member(&def.members, "Ir_function_params*", "params");
    append_member(&def.members, "Ir_lang_type", "return_type");
    append_member(&def.members, "Ir_name", "name");

    return def;
}

static Uast_type ir_gen_function_def(void) {
    Uast_type def = {.name = uast_name_new("def", "function_def", false, "ir")};

    append_member(&def.members, "Ir_name", "name_self");
    append_member(&def.members, "Ir_function_decl*", "decl");
    append_member(&def.members, "Ir_block*", "body");

    return def;
}

static Uast_type ir_gen_variable_def(void) {
    Uast_type def = {.name = uast_name_new("def", "variable_def", false, "ir")};

    append_member(&def.members, "Ir_lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Ir_name", "name_self"); // for loading from variable_def param
    append_member(&def.members, "Ir_name", "name_corr_param"); // for loading from lang_alloca
    append_member(&def.members, "Attrs", "attrs");

    return def;
}

static Uast_type ir_gen_primitive_def(void) {
    Uast_type def = {.name = uast_name_new("def", "primitive_def", false, "ir")};

    append_member(&def.members, "Ir_lang_type", "lang_type");

    return def;
}

static Uast_type ir_gen_label(void) {
    Uast_type def = {.name = uast_name_new("def", "label", false, "ir")};

    append_member(&def.members, "Ir_name", "name");

    return def;
}

static Uast_type ir_gen_string_def(void) {
    Uast_type def = {.name = uast_name_new("literal_def", "string_def", false, "ir")};

    append_member(&def.members, "Ir_name", "name");
    append_member(&def.members, "Strv", "data");

    return def;
}

static Uast_type ir_gen_struct_lit_def(void) {
    Uast_type def = {.name = uast_name_new("literal_def", "struct_lit_def", false, "ir")};

    append_member(&def.members, "Ir_expr_darr", "members");
    append_member(&def.members, "Ir_name", "name");
    append_member(&def.members, "Ir_lang_type", "lang_type");

    return def;
}

static Uast_type ir_gen_literal_def(void) {
    Uast_type def = {.name = uast_name_new("def", "literal_def", false, "ir")};

    darr_append(&a_gen, &def.sub_types, ir_gen_string_def());
    darr_append(&a_gen, &def.sub_types, ir_gen_struct_lit_def());

    return def;
}

static Uast_type ir_gen_def(void) {
    Uast_type def = {.name = uast_name_new("ir", "def", false, "ir")};

    darr_append(&a_gen, &def.sub_types, ir_gen_function_def());
    darr_append(&a_gen, &def.sub_types, ir_gen_variable_def());
    darr_append(&a_gen, &def.sub_types, ir_gen_struct_def());
    darr_append(&a_gen, &def.sub_types, ir_gen_primitive_def());
    darr_append(&a_gen, &def.sub_types, ir_gen_function_decl());
    darr_append(&a_gen, &def.sub_types, ir_gen_label());
    darr_append(&a_gen, &def.sub_types, ir_gen_literal_def());

    return def;
}

static Uast_type ir_gen_load_element_ptr(void) {
    Uast_type load = {.name = uast_name_new("ir", "load_element_ptr", false, "ir")};

    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "size_t", "memb_idx");
    append_member(&load.members, "Ir_name", "ir_src");
    append_member(&load.members, "Ir_name", "name_self");

    return load;
}

static Uast_type ir_gen_array_access(void) {
    Uast_type load = {.name = uast_name_new("ir", "array_access", false, "ir")};

    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "Ir_name", "index");
    append_member(&load.members, "Ir_name", "callee");
    append_member(&load.members, "Ir_name", "name_self");

    return load;
}

static Uast_type ir_gen_function_params(void) {
    Uast_type params = {.name = uast_name_new("ir", "function_params", false, "ir")};

    append_member(&params.members, "Ir_name", "name");
    append_member(&params.members, "Ir_variable_def_darr", "params");

    return params;
}

static Uast_type ir_gen_return(void) {
    Uast_type rtn = {.name = uast_name_new("ir", "return", false, "ir")};

    append_member(&rtn.members, "Ir_name", "name_self");
    append_member(&rtn.members, "Ir_name", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted");

    return rtn;
}

static Uast_type ir_gen_goto(void) {
    Uast_type lang_goto = {.name = uast_name_new("ir", "goto", false, "ir")};

    append_member(&lang_goto.members, "Ir_name", "name_self");
    append_member(&lang_goto.members, "Ir_name", "label");

    return lang_goto;
}

static Uast_type ir_gen_cond_goto(void) {
    Uast_type cond_goto = {.name = uast_name_new("ir", "cond_goto", false, "ir")};

    append_member(&cond_goto.members, "Ir_name", "name_self");
    append_member(&cond_goto.members, "Ir_name", "condition");
    append_member(&cond_goto.members, "Ir_name", "if_true");
    append_member(&cond_goto.members, "Ir_name", "if_false");

    return cond_goto;
}

static Uast_type ir_gen_alloca(void) {
    Uast_type lang_alloca = {.name = uast_name_new("ir", "alloca", false, "ir")};

    append_member(&lang_alloca.members, "Ir_lang_type", "lang_type");
    append_member(&lang_alloca.members, "Ir_name", "name");
    append_member(&lang_alloca.members, "Attrs", "attrs");

    return lang_alloca;
}

static Uast_type ir_gen_load_another_ir(void) {
    Uast_type load = {.name = uast_name_new("ir", "load_another_ir", false, "ir")};

    append_member(&load.members, "Ir_name", "ir_src");
    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "Ir_name", "name");

    return load;
}

static Uast_type ir_gen_store_another_ir(void) {
    Uast_type store = {.name = uast_name_new("ir", "store_another_ir", false, "ir")};

    append_member(&store.members, "Ir_name", "ir_src");
    append_member(&store.members, "Ir_name", "ir_dest");
    append_member(&store.members, "Ir_lang_type", "lang_type");
    append_member(&store.members, "Ir_name", "name");

    return store;
}

static Uast_type ir_gen_removed(void) {
    Uast_type store = {.name = uast_name_new("ir", "removed", false, "ir")};

    return store;
}

static Uast_type ir_gen_import_path(void) {
    Uast_type mod = {.name = uast_name_new("ir", "import_path", false, "ir")};

    append_member(&mod.members, "Ir_block*", "block");
    append_member(&mod.members, "Strv", "mod_path");

    return mod;
}

static Uast_type ir_gen_struct_memb_def(void) {
    Uast_type def = {.name = uast_name_new("ir", "struct_memb_def", false, "ir")};

    append_member(&def.members, "Ir_lang_type", "lang_type");
    append_member(&def.members, "Ir_name", "name_self"); // for loading from variable_def param
    append_member(&def.members, "size_t", "count"); // TODO: change size_t to int64_t or uint64_t 
                                                    // (becuause array count size is not always based on computer compile happens)

    return def;
}

static Uast_type ir_gen_ir(void) {
    Uast_type ir = {.name = uast_name_new("ir", "", true, "ir")};

    darr_append(&a_gen, &ir.sub_types, ir_gen_block());
    darr_append(&a_gen, &ir.sub_types, ir_gen_expr());
    darr_append(&a_gen, &ir.sub_types, ir_gen_load_element_ptr());
    darr_append(&a_gen, &ir.sub_types, ir_gen_array_access());
    darr_append(&a_gen, &ir.sub_types, ir_gen_function_params());
    darr_append(&a_gen, &ir.sub_types, ir_gen_def());
    darr_append(&a_gen, &ir.sub_types, ir_gen_return());
    darr_append(&a_gen, &ir.sub_types, ir_gen_goto());
    darr_append(&a_gen, &ir.sub_types, ir_gen_cond_goto());
    darr_append(&a_gen, &ir.sub_types, ir_gen_alloca());
    darr_append(&a_gen, &ir.sub_types, ir_gen_load_another_ir());
    darr_append(&a_gen, &ir.sub_types, ir_gen_store_another_ir());
    darr_append(&a_gen, &ir.sub_types, ir_gen_import_path());
    darr_append(&a_gen, &ir.sub_types, ir_gen_struct_memb_def());
    darr_append(&a_gen, &ir.sub_types, ir_gen_removed());

    return ir;
}

static void gen_all_irs(const char* file_path, bool implementation) {
    Uast_type tast = ir_gen_ir();
    unwrap(tast.name.type.count > 0);
    gen_uasts_common(file_path, implementation, tast);
}

#endif // AUTO_GEN_IR_H
