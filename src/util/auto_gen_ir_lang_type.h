
#ifndef AUTO_GEN_IR_LANG_TYPE_H
#define AUTO_GEN_IR_LANG_TYPE_H

#include <auto_gen_util.h>

struct Ir_lang_type_type_;
typedef struct Ir_lang_type_type_ Ir_lang_type_type;

typedef struct {
    Vec_base info;
    Ir_lang_type_type* buf;
} Ir_lang_type_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent;
    Strv base;
} Ir_lang_type_name;

typedef struct Ir_lang_type_type_ {
    Ir_lang_type_name name;
    Members members;
    Ir_lang_type_type_vec sub_types;
} Ir_lang_type_type;

static void extend_ir_lang_type_name_upper(String* output, Ir_lang_type_name name) {
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "IR_LANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_ir_lang_type_name_lower(String* output, Ir_lang_type_name name) {
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ir_lang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_ir_lang_type_name_first_upper(String* output, Ir_lang_type_name name) {
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Ir_lang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_ir_lang_type_name_upper(String* output, Ir_lang_type_name name) {
    todo();
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "IR_LANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_ir_lang_type_name_lower(String* output, Ir_lang_type_name name) {
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        string_extend_cstr(&gen_a, output, "ir_lang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "ir_lang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_ir_lang_type_name_first_upper(String* output, Ir_lang_type_name name) {
    assert(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ir_lang_type"))) {
        string_extend_cstr(&gen_a, output, "Ir_lang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Ir_lang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Ir_lang_type_name ir_lang_type_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Ir_lang_type_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static Ir_lang_type_type ir_lang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_opaque(const char* prefix) {
    const char* base_name = "opaque";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    // TODO: get rid of these unneeded atoms
    append_member(&sym.members, "Ir_lang_type_atom", "atom");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Ir_lang_type_type ir_lang_type = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_signed_int(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_unsigned_int(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_float(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_opaque(base_name));

    return ir_lang_type;
}

static Ir_lang_type_type ir_lang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ir_lang_type_tuple", "params");
    append_member(&sym.members, "Ir_lang_type*", "return_type");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ir_lang_type_atom", "atom");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ir_lang_type_vec", "ir_lang_types");

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Ir_lang_type_type sym = {.name = ir_lang_type_name_new(prefix, base_name, false)};

    return sym;
}

static Ir_lang_type_type ir_lang_type_gen_ir_lang_type(void) {
    const char* base_name = "ir_lang_type";
    Ir_lang_type_type ir_lang_type = {.name = ir_lang_type_name_new(base_name, "", true)};

    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_primitive(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_struct(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_tuple(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_void(base_name));
    vec_append(&gen_a, &ir_lang_type.sub_types, ir_lang_type_gen_fn(base_name));

    return ir_lang_type;
}

static void ir_lang_type_gen_ir_lang_type_forward_decl(Ir_lang_type_type ir_lang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ir_lang_type.sub_types.info.count; idx++) {
        ir_lang_type_gen_ir_lang_type_forward_decl(vec_at(&ir_lang_type.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_ir_lang_type_name_first_upper(&output, ir_lang_type.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ir_lang_type_name_first_upper(&output, ir_lang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_ir_lang_type_name_first_upper(&output, ir_lang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ir_lang_type_gen_ir_lang_type_struct_as(String* output, Ir_lang_type_type ir_lang_type) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_ir_lang_type_name_first_upper(output, ir_lang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ir_lang_type.sub_types.info.count; idx++) {
        Ir_lang_type_type curr = vec_at(&ir_lang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ir_lang_type_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_ir_lang_type_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ir_lang_type_name_first_upper(output, ir_lang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ir_lang_type_gen_ir_lang_type_struct_enum(String* output, Ir_lang_type_type ir_lang_type) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_ir_lang_type_name_upper(output, ir_lang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ir_lang_type.sub_types.info.count; idx++) {
        Ir_lang_type_type curr = vec_at(&ir_lang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ir_lang_type_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ir_lang_type_name_upper(output, ir_lang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ir_lang_type_gen_ir_lang_type_struct(Ir_lang_type_type ir_lang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ir_lang_type.sub_types.info.count; idx++) {
        ir_lang_type_gen_ir_lang_type_struct(vec_at(&ir_lang_type.sub_types, idx));
    }

    if (ir_lang_type.sub_types.info.count > 0) {
        ir_lang_type_gen_ir_lang_type_struct_as(&output, ir_lang_type);
        ir_lang_type_gen_ir_lang_type_struct_enum(&output, ir_lang_type);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ir_lang_type_name_first_upper(&output, ir_lang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (ir_lang_type.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_ir_lang_type_name_first_upper(&as_member_type, ir_lang_type.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_ir_lang_type_name_upper(&enum_member_type, ir_lang_type.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < ir_lang_type.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&ir_lang_type.members, idx));
    }

    if (ir_lang_type.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_ir_lang_type_name_first_upper(&output, ir_lang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ir_lang_type_gen_internal_unwrap(Ir_lang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_lang_type_gen_internal_unwrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ir_lang_type_##lower* ir_lang_type__unwrap##lower(Ir_lang_type* ir_lang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ir_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ir_lang_type_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_ir_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ir_lang_type) {\n");

    //    unwrap(ir_lang_type->type == upper); 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    unwrap(ir_lang_type.type == ");
        extend_ir_lang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    unwrap(ir_lang_type->type == ");
        extend_ir_lang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    }

    //    return &ir_lang_type->as._##lower; 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    return ir_lang_type.as.");
        extend_ir_lang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    return &ir_lang_type->as.");
        extend_ir_lang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    }

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ir_lang_type_gen_internal_wrap(Ir_lang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_lang_type_gen_internal_wrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ir_lang_type_##lower* ir_lang_type__unwrap##lower(Ir_lang_type* ir_lang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_parent_ir_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " ir_lang_type_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    extend_ir_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " ir_lang_type) {\n");

    //    return &ir_lang_type->as._##lower; 
    extend_parent_ir_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " new_ir_lang_type = {0};\n");
    string_extend_cstr(&gen_a, &function, "    new_ir_lang_type.type = ");
    extend_ir_lang_type_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    string_extend_cstr(&gen_a, &function, "    new_ir_lang_type.as.");
    extend_ir_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, " = ir_lang_type;\n");
    if (!is_const) {
        todo();
    }
    string_extend_cstr(&gen_a, &function, "    return new_ir_lang_type;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

void ir_lang_type_gen_ir_lang_type_unwrap(Ir_lang_type_type ir_lang_type) {
    ir_lang_type_gen_internal_unwrap(ir_lang_type, false);
    ir_lang_type_gen_internal_unwrap(ir_lang_type, true);
}

void ir_lang_type_gen_ir_lang_type_wrap(Ir_lang_type_type ir_lang_type) {
    //ir_lang_type_gen_internal_wrap(ir_lang_type, false);
    ir_lang_type_gen_internal_wrap(ir_lang_type, true);
}

// TODO: deduplicate these functions (use same function for Ir and Ir_lang_type)
static void ir_lang_type_gen_print_forward_decl(Ir_lang_type_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_lang_type_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_ir_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(ir_lang_type) strv_print(");
    extend_ir_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(ir_lang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "Strv ");
    extend_ir_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_ir_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ir_lang_type, int recursion_depth);");

    gen_gen(FMT"\n", string_print(function));
}

static void ir_lang_type_gen_new_internal(Ir_lang_type_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ir_lang_type_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }
    if (type.sub_types.info.count > 0) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ir_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " ");
    extend_ir_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(Pos pos ");

    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        string_extend_cstr(&gen_a, &function, ", ");

        Member curr = vec_at(&type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    return (");
        extend_ir_lang_type_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ") {.pos = pos");

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            string_extend_cstr(&gen_a, &function, ", ");

            Member curr = vec_at(&type.members, idx);

            string_extend_cstr(&gen_a, &function, " .");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
        }

        string_extend_cstr(&gen_a, &function, "};\n");
        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_ir_lang_type_new_forward_decl(Ir_lang_type_type ir_lang_type) {
    ir_lang_type_gen_new_internal(ir_lang_type, false);
}

static void gen_ir_lang_type_new_define(Ir_lang_type_type ir_lang_type) {
    ir_lang_type_gen_new_internal(ir_lang_type, true);
}

static void gen_ir_lang_type_vecs(Ir_lang_type_type ir_lang_type) {
    for (size_t idx = 0; idx < ir_lang_type.sub_types.info.count; idx++) {
        gen_ir_lang_type_vecs(vec_at(&ir_lang_type.sub_types, idx));
    }

    String vec_name = {0};
    extend_ir_lang_type_name_first_upper(&vec_name, ir_lang_type.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_ir_lang_type_name_first_upper(&item_name, ir_lang_type.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_ir_lang_type(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Ir_lang_type_type ir_lang_type = ir_lang_type_gen_ir_lang_type();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef IR_LANG_TYPE_H\n");
        gen_gen("#define IR_LANG_TYPE_H\n");

        gen_gen("#include <ir_lang_type_hand_written.h>\n");
    } else {
        gen_gen("#ifndef IR_LANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#define IR_LANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    ir_lang_type_gen_ir_lang_type_forward_decl(ir_lang_type);

    if (!implementation) {
        gen_ir_lang_type_vecs(ir_lang_type);
    }

    if (implementation) {
        ir_lang_type_gen_ir_lang_type_struct(ir_lang_type);

        ir_lang_type_gen_ir_lang_type_unwrap(ir_lang_type);
        ir_lang_type_gen_ir_lang_type_wrap(ir_lang_type);

        gen_gen("%s\n", "static inline Ir_lang_type* ir_lang_type_new(void) {");
        gen_gen("%s\n", "    Ir_lang_type* new_ir_lang_type = arena_alloc(&a_main, sizeof(*new_ir_lang_type));");
        gen_gen("%s\n", "    return new_ir_lang_type;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_ir_lang_type_new_forward_decl(ir_lang_type);
    }
    ir_lang_type_gen_print_forward_decl(ir_lang_type);
    if (implementation) {
        gen_ir_lang_type_new_define(ir_lang_type);
    }

    if (implementation) {
        gen_gen("#endif // IR_LANG_TYPE_H\n");
    } else {
        gen_gen("#endif // IR_LANG_TYPE_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_IR_LANG_TYPE_H
