
#ifndef AUTO_GEN_IR_H
#define AUTO_GEN_IR_H

#include <auto_gen_util.h>
#include <auto_gen_vecs.h>

struct Ir_type_;
typedef struct Ir_type_ Ir_type;

typedef struct {
    Vec_base info;
    Ir_type* buf;
} Ir_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent;
    Strv base;
} Ir_name;

typedef struct Ir_type_ {
    Ir_name name;
    Members members;
    Ir_type_vec sub_types;
} Ir_type;

static void extend_ir_name_upper(String* output, Ir_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "IR");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_ir_name_lower(String* output, Ir_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ir");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_ir_name_first_upper(String* output, Ir_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Ir");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_ir_name_upper(String* output, Ir_name name) {
    todo();
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "IR");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_ir_name_lower(String* output, Ir_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        string_extend_cstr(&gen_a, output, "ir");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "ir");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_ir_name_first_upper(String* output, Ir_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir"))) {
        string_extend_cstr(&gen_a, output, "Ir");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Ir");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Ir_name ir_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Ir_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static Ir_type ir_gen_block(void) {
    Ir_type block = {.name = ir_name_new("ir", "block", false)};

    append_member(&block.members, "Ir_name", "name");
    append_member(&block.members, "Ir_vec", "children");
    append_member(&block.members, "Pos", "pos_end");
    append_member(&block.members, "Scope_id", "scope_id");
    append_member(&block.members, "Cfg_node_vec", "cfg");

    return block;
}

static Ir_type ir_gen_unary(void) {
    Ir_type unary = {.name = ir_name_new("operator", "unary", false)};

    append_member(&unary.members, "Ir_name", "child");
    append_member(&unary.members, "IR_UNARY_TYPE", "token_type");
    append_member(&unary.members, "Ir_lang_type", "lang_type");
    append_member(&unary.members, "Ir_name", "name");

    return unary;
}

static Ir_type ir_gen_binary(void) {
    Ir_type binary = {.name = ir_name_new("operator", "binary", false)};

    append_member(&binary.members, "Ir_name", "lhs");
    append_member(&binary.members, "Ir_name", "rhs");
    append_member(&binary.members, "IR_BINARY_TYPE", "token_type");
    append_member(&binary.members, "Ir_lang_type", "lang_type");
    append_member(&binary.members, "Ir_name", "name");

    return binary;
}

static Ir_type ir_gen_operator(void) {
    Ir_type operator = {.name = ir_name_new("expr", "operator", false)};

    vec_append(&gen_a, &operator.sub_types, ir_gen_unary());
    vec_append(&gen_a, &operator.sub_types, ir_gen_binary());

    return operator;
}

static Ir_type ir_gen_int(void) {
    Ir_type number = {.name = ir_name_new("literal", "int", false)};

    append_member(&number.members, "int64_t", "data");
    append_member(&number.members, "Ir_lang_type", "lang_type");
    append_member(&number.members, "Ir_name", "name");

    return number;
}

static Ir_type ir_gen_float(void) {
    Ir_type number = {.name = ir_name_new("literal", "float", false)};

    append_member(&number.members, "double", "data");
    append_member(&number.members, "Ir_lang_type", "lang_type");
    append_member(&number.members, "Ir_name", "name");

    return number;
}

static Ir_type ir_gen_string(void) {
    Ir_type string = {.name = ir_name_new("literal", "string", false)};

    append_member(&string.members, "Strv", "data");
    append_member(&string.members, "Ir_name", "name");

    return string;
}

static Ir_type ir_gen_function_name(void) {
    Ir_type lang_char = {.name = ir_name_new("literal", "function_name", false)};

    append_member(&lang_char.members, "Ir_name", "fun_name");
    append_member(&lang_char.members, "Ir_name", "name_self");

    return lang_char;
}

static Ir_type ir_gen_void(void) {
    Ir_type lang_void = {.name = ir_name_new("literal", "void", false)};

    append_member(&lang_void.members, "Ir_name", "name");

    return lang_void;
}

static Ir_type ir_gen_literal(void) {
    Ir_type lit = {.name = ir_name_new("expr", "literal", false)};

    vec_append(&gen_a, &lit.sub_types, ir_gen_int());
    vec_append(&gen_a, &lit.sub_types, ir_gen_float());
    vec_append(&gen_a, &lit.sub_types, ir_gen_string());
    vec_append(&gen_a, &lit.sub_types, ir_gen_void());
    vec_append(&gen_a, &lit.sub_types, ir_gen_function_name());

    return lit;
}

static Ir_type ir_gen_function_call(void) {
    Ir_type call = {.name = ir_name_new("expr", "function_call", false)};

    append_member(&call.members, "Ir_name_vec", "args");
    append_member(&call.members, "Ir_name", "name_self");
    append_member(&call.members, "Ir_name", "callee");
    append_member(&call.members, "Ir_lang_type", "lang_type");

    return call;
}

static Ir_type ir_gen_expr(void) {
    Ir_type expr = {.name = ir_name_new("ir", "expr", false)};

    vec_append(&gen_a, &expr.sub_types, ir_gen_operator());
    vec_append(&gen_a, &expr.sub_types, ir_gen_literal());
    vec_append(&gen_a, &expr.sub_types, ir_gen_function_call());

    return expr;
}

static Ir_type ir_gen_struct_def(void) {
    Ir_type def = {.name = ir_name_new("def", "struct_def", false)};

    append_member(&def.members, "Ir_struct_def_base", "base");

    return def;
}

static Ir_type ir_gen_function_decl(void) {
    Ir_type def = {.name = ir_name_new("def", "function_decl", false)};

    append_member(&def.members, "Ir_function_params*", "params");
    append_member(&def.members, "Ir_lang_type", "return_type");
    append_member(&def.members, "Ir_name", "name");

    return def;
}

static Ir_type ir_gen_function_def(void) {
    Ir_type def = {.name = ir_name_new("def", "function_def", false)};

    append_member(&def.members, "Ir_name", "name_self");
    append_member(&def.members, "Ir_function_decl*", "decl");
    append_member(&def.members, "Ir_block*", "body");

    return def;
}

static Ir_type ir_gen_variable_def(void) {
    Ir_type def = {.name = ir_name_new("def", "variable_def", false)};

    append_member(&def.members, "Ir_lang_type", "lang_type");
    append_member(&def.members, "bool", "is_variadic");
    append_member(&def.members, "Ir_name", "name_self"); // for loading from variable_def param
    append_member(&def.members, "Ir_name", "name_corr_param"); // for loading from lang_alloca

    return def;
}

static Ir_type ir_gen_primitive_def(void) {
    Ir_type def = {.name = ir_name_new("def", "primitive_def", false)};

    append_member(&def.members, "Ir_lang_type", "lang_type");

    return def;
}

static Ir_type ir_gen_label(void) {
    Ir_type def = {.name = ir_name_new("def", "label", false)};

    append_member(&def.members, "Ir_name", "name");

    return def;
}

static Ir_type ir_gen_string_def(void) {
    Ir_type def = {.name = ir_name_new("literal_def", "string_def", false)};

    append_member(&def.members, "Ir_name", "name");
    append_member(&def.members, "Strv", "data");

    return def;
}

static Ir_type ir_gen_struct_lit_def(void) {
    Ir_type def = {.name = ir_name_new("literal_def", "struct_lit_def", false)};

    append_member(&def.members, "Ir_expr_vec", "members");
    append_member(&def.members, "Ir_name", "name");
    append_member(&def.members, "Ir_lang_type", "lang_type");

    return def;
}

static Ir_type ir_gen_literal_def(void) {
    Ir_type def = {.name = ir_name_new("def", "literal_def", false)};

    vec_append(&gen_a, &def.sub_types, ir_gen_string_def());
    vec_append(&gen_a, &def.sub_types, ir_gen_struct_lit_def());

    return def;
}

static Ir_type ir_gen_def(void) {
    Ir_type def = {.name = ir_name_new("ir", "def", false)};

    vec_append(&gen_a, &def.sub_types, ir_gen_function_def());
    vec_append(&gen_a, &def.sub_types, ir_gen_variable_def());
    vec_append(&gen_a, &def.sub_types, ir_gen_struct_def());
    vec_append(&gen_a, &def.sub_types, ir_gen_primitive_def());
    vec_append(&gen_a, &def.sub_types, ir_gen_function_decl());
    vec_append(&gen_a, &def.sub_types, ir_gen_label());
    vec_append(&gen_a, &def.sub_types, ir_gen_literal_def());

    return def;
}

static Ir_type ir_gen_load_element_ptr(void) {
    Ir_type load = {.name = ir_name_new("ir", "load_element_ptr", false)};

    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "size_t", "memb_idx");
    append_member(&load.members, "Ir_name", "ir_src");
    append_member(&load.members, "Ir_name", "name_self");

    return load;
}

static Ir_type ir_gen_array_access(void) {
    Ir_type load = {.name = ir_name_new("ir", "array_access", false)};

    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "Ir_name", "index");
    append_member(&load.members, "Ir_name", "callee");
    append_member(&load.members, "Ir_name", "name_self");

    return load;
}

static Ir_type ir_gen_function_params(void) {
    Ir_type params = {.name = ir_name_new("ir", "function_params", false)};

    append_member(&params.members, "Ir_name", "name");
    append_member(&params.members, "Ir_variable_def_vec", "params");

    return params;
}

static Ir_type ir_gen_return(void) {
    Ir_type rtn = {.name = ir_name_new("ir", "return", false)};

    append_member(&rtn.members, "Ir_name", "name_self");
    append_member(&rtn.members, "Ir_name", "child");
    append_member(&rtn.members, "bool", "is_auto_inserted");

    return rtn;
}

static Ir_type ir_gen_goto(void) {
    Ir_type lang_goto = {.name = ir_name_new("ir", "goto", false)};

    append_member(&lang_goto.members, "Ir_name", "name_self");
    append_member(&lang_goto.members, "Ir_name", "label");

    return lang_goto;
}

static Ir_type ir_gen_cond_goto(void) {
    Ir_type cond_goto = {.name = ir_name_new("ir", "cond_goto", false)};

    append_member(&cond_goto.members, "Ir_name", "name_self");
    append_member(&cond_goto.members, "Ir_name", "condition");
    append_member(&cond_goto.members, "Ir_name", "if_true");
    append_member(&cond_goto.members, "Ir_name", "if_false");

    return cond_goto;
}

static Ir_type ir_gen_alloca(void) {
    Ir_type lang_alloca = {.name = ir_name_new("ir", "alloca", false)};

    append_member(&lang_alloca.members, "Ir_lang_type", "lang_type");
    append_member(&lang_alloca.members, "Ir_name", "name");

    return lang_alloca;
}

static Ir_type ir_gen_load_another_ir(void) {
    Ir_type load = {.name = ir_name_new("ir", "load_another_ir", false)};

    append_member(&load.members, "Ir_name", "ir_src");
    append_member(&load.members, "Ir_lang_type", "lang_type");
    append_member(&load.members, "Ir_name", "name");

    return load;
}

static Ir_type ir_gen_store_another_ir(void) {
    Ir_type store = {.name = ir_name_new("ir", "store_another_ir", false)};

    append_member(&store.members, "Ir_name", "ir_src");
    append_member(&store.members, "Ir_name", "ir_dest");
    append_member(&store.members, "Ir_lang_type", "lang_type");
    append_member(&store.members, "Ir_name", "name");

    return store;
}

static Ir_type ir_gen_removed(void) {
    Ir_type store = {.name = ir_name_new("ir", "removed", false)};

    return store;
}

static Ir_type ir_gen_import_path(void) {
    Ir_type mod = {.name = ir_name_new("ir", "import_path", false)};

    append_member(&mod.members, "Ir_block*", "block");
    append_member(&mod.members, "Strv", "mod_path");

    return mod;
}

static Ir_type ir_gen_struct_memb_def(void) {
    Ir_type def = {.name = ir_name_new("ir", "struct_memb_def", false)};

    append_member(&def.members, "Ir_lang_type", "lang_type");
    append_member(&def.members, "Ir_name", "name_self"); // for loading from variable_def param
    append_member(&def.members, "size_t", "count");


    return def;
}

static Ir_type ir_gen_ir(void) {
    Ir_type ir = {.name = ir_name_new("ir", "", true)};

    vec_append(&gen_a, &ir.sub_types, ir_gen_block());
    vec_append(&gen_a, &ir.sub_types, ir_gen_expr());
    vec_append(&gen_a, &ir.sub_types, ir_gen_load_element_ptr());
    vec_append(&gen_a, &ir.sub_types, ir_gen_array_access());
    vec_append(&gen_a, &ir.sub_types, ir_gen_function_params());
    vec_append(&gen_a, &ir.sub_types, ir_gen_def());
    vec_append(&gen_a, &ir.sub_types, ir_gen_return());
    vec_append(&gen_a, &ir.sub_types, ir_gen_goto());
    vec_append(&gen_a, &ir.sub_types, ir_gen_cond_goto());
    vec_append(&gen_a, &ir.sub_types, ir_gen_alloca());
    vec_append(&gen_a, &ir.sub_types, ir_gen_load_another_ir());
    vec_append(&gen_a, &ir.sub_types, ir_gen_store_another_ir());
    vec_append(&gen_a, &ir.sub_types, ir_gen_import_path());
    vec_append(&gen_a, &ir.sub_types, ir_gen_struct_memb_def());
    vec_append(&gen_a, &ir.sub_types, ir_gen_removed());

    return ir;
}

static void ir_gen_ir_forward_decl(Ir_type ir) {
    String output = {0};

    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        ir_gen_ir_forward_decl(vec_at(ir.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_ir_name_first_upper(&output, ir.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ir_name_first_upper(&output, ir.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_ir_name_first_upper(&output, ir.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ir_gen_ir_struct_as(String* output, Ir_type ir) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_ir_name_first_upper(output, ir.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        Ir_type curr = vec_at(ir.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ir_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_ir_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ir_name_first_upper(output, ir.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ir_gen_ir_struct_enum(String* output, Ir_type ir) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_ir_name_upper(output, ir.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        Ir_type curr = vec_at(ir.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ir_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ir_name_upper(output, ir.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ir_gen_ir_struct(Ir_type ir) {
    String output = {0};

    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        ir_gen_ir_struct(vec_at(ir.sub_types, idx));
    }

    if (ir.sub_types.info.count > 0) {
        ir_gen_ir_struct_as(&output, ir);
        ir_gen_ir_struct_enum(&output, ir);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ir_name_first_upper(&output, ir.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (ir.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_ir_name_first_upper(&as_member_type, ir.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_ir_name_upper(&enum_member_type, ir.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < ir.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(ir.members, idx));
    }

    if (ir.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });

        string_extend_cstr(&gen_a, &output, "#ifndef NDEBUG\n");
        extend_struct_member(&output, (Member) {
            .type = sv("Loc"), .name = sv("loc")
        });
        string_extend_cstr(&gen_a, &output, "#endif // NDEBUG\n");
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_ir_name_first_upper(&output, ir.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ir_gen_internal_unwrap(Ir_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ir_##lower* ir__unwrap##lower(Ir* ir) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ir_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ir) {\n");

    //    unwrap(ir->type == upper); 
    string_extend_cstr(&gen_a, &function, "    unwrap(ir->type == ");
    extend_ir_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &ir->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &ir->as.");
    extend_ir_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ir_gen_internal_wrap(Ir_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_gen_internal_wrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ir_##lower* ir__unwrap##lower(Ir* ir) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ir_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ir) {\n");

    //    return &ir->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) ir;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

void ir_gen_ir_unwrap(Ir_type ir) {
    ir_gen_internal_unwrap(ir, false);
    ir_gen_internal_unwrap(ir, true);
}

void ir_gen_ir_wrap(Ir_type ir) {
    ir_gen_internal_wrap(ir, false);
    ir_gen_internal_wrap(ir, true);
}

static void ir_gen_print_forward_decl(Ir_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_gen_print_forward_decl(vec_at(type.sub_types, idx));
    }

    String function = {0};

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "Strv ");
        string_extend_cstr(&gen_a, &function, "ir_print_internal(const Ir* ir, Indent recursion_depth);\n");
    } else {
        string_extend_cstr(&gen_a, &function, "#define ");
        extend_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_print(ir) strv_print(");
        extend_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_print_internal(ir, 0))\n");

        string_extend_cstr(&gen_a, &function, "Strv ");
        extend_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_print_internal(const ");
        extend_ir_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* ir, Indent indent);");
    }

    gen_gen(FMT"\n", string_print(function));
}

static void ir_gen_new_internal(Ir_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_gen_new_internal(vec_at(type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    if (!implementation) {
        string_extend_cstr(&gen_a, &function, "#define ");
        extend_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new(");
        if (type.sub_types.info.count > 0) {
            string_extend_cstr(&gen_a, &function, "void");
        } else {
            string_extend_cstr(&gen_a, &function, "pos");
        }
        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            if (idx < type.members.info.count) {
                string_extend_cstr(&gen_a, &function, ", ");
            }

            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, " ");
            string_extend_strv(&gen_a, &function, curr.name);
        }
        string_extend_cstr(&gen_a, &function, ") ");
        extend_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new_internal(");
        if (type.sub_types.info.count > 0) {
            string_extend_cstr(&gen_a, &function, "void");
        } else {
            string_extend_cstr(&gen_a, &function, "pos, loc_new()");
        }
        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            if (idx < type.members.info.count) {
                string_extend_cstr(&gen_a, &function, ", ");
            }

            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, " ");
            string_extend_strv(&gen_a, &function, curr.name);
        }
        string_extend_cstr(&gen_a, &function, ")\n");
    }

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ir_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_ir_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new_internal(");

    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    } else {
        string_extend_cstr(&gen_a, &function, "Pos pos, Loc loc");
    }
    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        if (idx < type.members.info.count) {
            string_extend_cstr(&gen_a, &function, ", ");
        }

        Member curr = vec_at(type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    ");
        extend_parent_ir_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_ir = ");
        extend_parent_ir_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_ir->type = ");
        extend_ir_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    ir_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_ir)->pos = pos;\n");

            string_extend_cstr(&gen_a, &function, "    (void) loc;\n");
            string_extend_cstr(&gen_a, &function, "#ifndef NDEBUG\n");
            string_extend_cstr(&gen_a, &function, "    ir_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_ir)->loc = loc;\n");
            string_extend_cstr(&gen_a, &function, "#endif // NDEBUG\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, "    ir_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_ir)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return ir_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap(base_ir);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ir_gen_internal_get_pos(Ir_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_gen_internal_get_pos(vec_at(type.sub_types, idx), implementation);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");

    if (type.name.is_topmost) {
        string_extend_cstr(&gen_a, &function, "    ir_get_pos(const Ir* ir)");
    } else {
        string_extend_cstr(&gen_a, &function, "    ir_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_get_pos(const ");
        extend_ir_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* ir)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    return ir->pos;\n");
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (ir->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Ir_type curr = vec_at(type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_ir_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return ir_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_get_pos(ir_");
                extend_strv_lower(&function, curr.name.base);
                string_extend_cstr(&gen_a, &function, "_const_unwrap(ir));\n");

                string_extend_cstr(&gen_a, &function, "        break;\n");
            }

            string_extend_cstr(&gen_a, &function, "    }\n");
        }

        string_extend_cstr(&gen_a, &function, "unreachable(\"\");\n");
        string_extend_cstr(&gen_a, &function, "}\n");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_ir_new_forward_decl(Ir_type ir) {
    ir_gen_new_internal(ir, false);
}

static void gen_ir_new_define(Ir_type ir) {
    ir_gen_new_internal(ir, true);
}

static void gen_ir_forward_decl_get_pos(Ir_type ir) {
    ir_gen_internal_get_pos(ir, false);
}

static void gen_ir_define_get_pos(Ir_type ir) {
    ir_gen_internal_get_pos(ir, true);
}

static void gen_ir_vecs(Ir_type ir) {
    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        gen_ir_vecs(vec_at(ir.sub_types, idx));
    }

    String vec_name = {0};
    extend_ir_name_first_upper(&vec_name, ir.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_ir_name_first_upper(&item_name, ir.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void ir_gen_print_overloading(Ir_type_vec types) {
    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ir_print(ir) strv_print(ir_print_overload(ir, 0))\n");

    string_extend_cstr(&gen_a, &function, "#define ir_print_overload(ir, indent) _Generic ((ir), \\\n");

    vec_foreach(idx, Ir_type, type, types) {
        string_extend_cstr(&gen_a, &function, "    ");
        extend_ir_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_ir_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal, \\\n");

        string_extend_cstr(&gen_a, &function, "    const ");
        extend_ir_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_ir_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal%s \\\n", idx + 1 < types.info.count ? "," : "");
    }

    string_extend_cstr(&gen_a, &function, ") (ir, indent)\n");

    gen_gen(FMT"\n", string_print(function));
}

static void ir_get_type_vec_internal(Ir_type_vec* type_vec, Ir_type ir) {
    for (size_t idx = 0; idx < ir.sub_types.info.count; idx++) {
        ir_get_type_vec_internal(type_vec, vec_at(ir.sub_types, idx));
    }

    vec_append(&gen_a, type_vec, ir);
}

static Ir_type_vec ir_get_type_vec(Ir_type ir) {
    Ir_type_vec type_vec = {0};
    ir_get_type_vec_internal(&type_vec, ir);
    return type_vec;
}

static void gen_all_irs(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Ir_type ir = ir_gen_ir();
    Ir_type_vec type_vec = ir_get_type_vec(ir);

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef IR_H\n");
        gen_gen("#define IR_H\n");

        gen_gen("#include <ir_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
        gen_gen("#include <vecs.h>\n");
        gen_gen("#include <cfg.h>\n");
    } else {
        gen_gen("#ifndef IR_FORWARD_DECL_H\n");
        gen_gen("#define IR_FORWARD_DECL_H\n");
        gen_gen("#include <ir_lang_type.h>\n");
    }
    gen_gen("#include <ir_operator_type.h>\n");
    gen_gen("#include <operator_type.h>\n");

    if (!implementation) {
        ir_gen_ir_forward_decl(ir);
        gen_ir_vecs(ir);
    }

    if (implementation) {
        ir_gen_ir_struct(ir);

        ir_gen_ir_unwrap(ir);
        ir_gen_ir_wrap(ir);

        gen_gen("%s\n", "static inline Ir* ir_new(void) {");
        gen_gen("%s\n", "    Ir* new_ir = arena_alloc(&a_main, sizeof(*new_ir));");
        gen_gen("%s\n", "    return new_ir;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_ir_new_forward_decl(ir);
    }
    ir_gen_print_forward_decl(ir);
    if (implementation) {
        gen_ir_new_define(ir);
    } else {
        ir_gen_print_overloading(type_vec);
    }

    gen_ir_forward_decl_get_pos(ir);
    if (implementation) {
        gen_ir_define_get_pos(ir);
    }

    if (implementation) {
        gen_gen("#endif // IR_H\n");
    } else {
        gen_gen("#endif // IR_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}


#endif // AUTO_GEN_IR_H
