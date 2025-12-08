
#ifndef AUTO_GEN_ULANG_TYPE_H
#define AUTO_GEN_ULANG_TYPE_H

#include <auto_gen_util.h>

struct Ulang_type_type_;
typedef struct Ulang_type_type_ Ulang_type_type;

typedef struct {
    Vec_base info;
    Ulang_type_type* buf;
} Ulang_type_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent;
    Strv base;
} Ulang_type_name;

typedef struct Ulang_type_type_ {
    Ulang_type_name name;
    Members members;
    Ulang_type_type_vec sub_types;
} Ulang_type_type;

static void extend_ulang_type_name_upper(String* output, Ulang_type_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        strv_extend_upper(&gen_a, output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ULANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        strv_extend_upper(&gen_a, output, name.base);
    }
}

static void extend_ulang_type_name_lower(String* output, Ulang_type_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        strv_extend_lower(&gen_a, output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ulang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        strv_extend_lower(&gen_a, output, name.base);
    }
}

static void extend_ulang_type_name_first_upper(String* output, Ulang_type_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Ulang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        strv_extend_lower(&gen_a, output, name.base);
    }
}

static void extend_parent_ulang_type_name_upper(String* output, Ulang_type_name name) {
    todo();
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "ULANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        strv_extend_upper(&gen_a, output, name.base);
    }
}

static void extend_parent_ulang_type_name_lower(String* output, Ulang_type_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        string_extend_cstr(&gen_a, output, "ulang_type");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "ulang_type");
    string_extend_cstr(&gen_a, output, "_");
    strv_extend_lower(&gen_a, output, name.parent);
}

static void extend_parent_ulang_type_name_first_upper(String* output, Ulang_type_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("ulang_type"))) {
        string_extend_cstr(&gen_a, output, "Ulang_type");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Ulang_type");
    string_extend_cstr(&gen_a, output, "_");
    strv_extend_lower(&gen_a, output, name.parent);
}

static Ulang_type_name ulang_type_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Ulang_type_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static Ulang_type_type ulang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_opaque(const char* prefix) {
    const char* base_name = "opaque";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Ulang_type_type ulang_type = {.name = ulang_type_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_signed_int(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_unsigned_int(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_float(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_opaque(base_name));

    return ulang_type;
}

static Ulang_type_type ulang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_tuple", "params");
    append_member(&sym.members, "Ulang_type*", "return_type");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_array(const char* prefix) {
    const char* base_name = "array";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type*", "item_type");
    append_member(&sym.members, "Uast_expr*", "count");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_removed(const char* prefix) {
    const char* base_name = "removed";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Ulang_type_type ulang_type_gen_raw_union(const char* prefix) {
    const char* base_name = "raw_union";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Ulang_type_type ulang_type_gen_enum(const char* prefix) {
    const char* base_name = "enum";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Ulang_type_type ulang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_vec", "ulang_types");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    return sym;
}

static Ulang_type_type ulang_type_gen_int_lit(const char* prefix) {
    const char* base_name = "int_lit";
    Ulang_type_type lit = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&lit.members, "int64_t", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Ulang_type_type ulang_type_gen_float_lit(const char* prefix) {
    const char* base_name = "float_lit";
    Ulang_type_type lit = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&lit.members, "double", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Ulang_type_type ulang_type_gen_string_lit(const char* prefix) {
    const char* base_name = "string_lit";
    Ulang_type_type lit = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Strv", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Ulang_type_type ulang_type_gen_struct_lit(const char* prefix) {
    const char* base_name = "struct_lit";
    Ulang_type_type lit = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Uast_expr*", "expr");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Ulang_type_type ulang_type_gen_fn_lit(const char* prefix) {
    const char* base_name = "fn_lit";
    Ulang_type_type lit = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&lit.members, "Name", "name");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Ulang_type_type ulang_type_gen_regular(const char* prefix) {
    const char* base_name = "regular";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Ulang_type_type ulang_type_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Uast_expr*", "expr");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Ulang_type_type ulang_type_gen_lit(const char* prefix) {
    const char* base_name = "lit";
    Ulang_type_type ulang_type = {.name = ulang_type_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_int_lit(base_name)); // TODO: rename int to int_lit for consistancy?
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_float_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_string_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_struct_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_fn_lit(base_name));

    return ulang_type;
}

static Ulang_type_type ulang_type_gen_ulang_type(void) {
    const char* base_name = "ulang_type";
    Ulang_type_type ulang_type = {.name = ulang_type_name_new(base_name, "", true)};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_tuple(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_fn(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_regular(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_array(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_expr(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_removed(base_name));

    return ulang_type;
}

static void ulang_type_gen_ulang_type_forward_decl(Ulang_type_type ulang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_gen_ulang_type_forward_decl(vec_at(ulang_type.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ulang_type_gen_ulang_type_struct_as(String* output, Ulang_type_type ulang_type) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_ulang_type_name_first_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        Ulang_type_type curr = vec_at(ulang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ulang_type_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_ulang_type_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ulang_type_name_first_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ulang_type_gen_ulang_type_struct_enum(String* output, Ulang_type_type ulang_type) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_ulang_type_name_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        Ulang_type_type curr = vec_at(ulang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_ulang_type_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_ulang_type_name_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ulang_type_gen_ulang_type_struct(Ulang_type_type ulang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_gen_ulang_type_struct(vec_at(ulang_type.sub_types, idx));
    }

    if (ulang_type.sub_types.info.count > 0) {
        ulang_type_gen_ulang_type_struct_as(&output, ulang_type);
        ulang_type_gen_ulang_type_struct_enum(&output, ulang_type);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (ulang_type.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_ulang_type_name_first_upper(&as_member_type, ulang_type.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_ulang_type_name_upper(&enum_member_type, ulang_type.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < ulang_type.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(ulang_type.members, idx));
    }

    if (ulang_type.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ulang_type_gen_internal_unwrap(Ulang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* lang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type_");
    strv_extend_lower(&gen_a, &function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type) {\n");

    //    unwrap(ulang_type->type == upper); 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    unwrap(ulang_type.type == ");
        extend_ulang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    unwrap(ulang_type->type == ");
        extend_ulang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    }

    //    return &ulang_type->as._##lower; 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    return ulang_type.as.");
        extend_ulang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    return &ulang_type->as.");
        extend_ulang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    }

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ulang_type_gen_internal_wrap(Ulang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_wrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* lang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_parent_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type_");
    strv_extend_lower(&gen_a, &function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    extend_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type) {\n");

    //    return &ulang_type->as._##lower; 
    extend_parent_ulang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " new_ulang_type = {0};\n");
    string_extend_cstr(&gen_a, &function, "    new_ulang_type.type = ");
    extend_ulang_type_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    string_extend_cstr(&gen_a, &function, "    new_ulang_type.as.");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, " = ulang_type;\n");
    if (!is_const) {
        todo();
    }
    string_extend_cstr(&gen_a, &function, "    return new_ulang_type;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

void ulang_type_gen_ulang_type_unwrap(Ulang_type_type ulang_type) {
    ulang_type_gen_internal_unwrap(ulang_type, false);
    ulang_type_gen_internal_unwrap(ulang_type, true);
}

void ulang_type_gen_ulang_type_wrap(Ulang_type_type ulang_type) {
    //ulang_type_gen_internal_wrap(ulang_type, false);
    ulang_type_gen_internal_wrap(ulang_type, true);
}

// TODO: deduplicate these functions (use same function for Ir and Ulang_type)
static void ulang_type_gen_print_forward_decl(Ulang_type_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_print_forward_decl(vec_at(type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(ulang_type) strv_print(");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(ulang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "Strv ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_ulang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ulang_type, Indent indent);");

    gen_gen(FMT"\n", string_print(function));
}

static void ulang_type_gen_new_internal(Ulang_type_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_new_internal(vec_at(type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }
    if (type.sub_types.info.count > 0) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ulang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(Pos pos ");

    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        string_extend_cstr(&gen_a, &function, ", ");

        Member curr = vec_at(type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    return (");
        extend_ulang_type_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ") {.pos = pos");

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            string_extend_cstr(&gen_a, &function, ", ");

            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, " .");
            strv_extend_lower(&gen_a, &function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            strv_extend_lower(&gen_a, &function, curr.name);
        }

        string_extend_cstr(&gen_a, &function, "};\n");
        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_ulang_type_new_forward_decl(Ulang_type_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, false);
}

// "constructors" for ulang_types
static void gen_ulang_type_new_define(Ulang_type_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, true);
}

static void gen_ulang_type_get_pos(Ulang_type_type type, bool implementation, bool is_ref) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_ulang_type_get_pos(vec_at(type.sub_types, idx), implementation, is_ref);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");
    if (is_ref) {
        string_extend_cstr(&gen_a, &function, "*");
    }

    string_extend_cstr(&gen_a, &function, "    ulang_type_");
    strv_extend_lower(&gen_a, &function, type.name.base);
    if (is_ref) {
        string_extend_f(&gen_a, &function, "%sget_pos_ref(", type.name.is_topmost ? "" : "_");
    } else {
        string_extend_f(&gen_a, &function, "%sget_pos(const ", type.name.is_topmost ? "" : "_");
    }
    extend_ulang_type_name_first_upper(&function, type.name);
    string_extend_f(&gen_a, &function, "%s ulang_type)", is_ref ? "*" : "");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            if (is_ref) {
                string_extend_cstr(&gen_a, &function, "    return &ulang_type->pos;\n");
            } else {
                string_extend_cstr(&gen_a, &function, "    return ulang_type.pos;\n");
            }
        } else {
            string_extend_f(&gen_a, &function, "    switch (ulang_type%stype) {\n", is_ref ? "->" : ".");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Ulang_type_type curr = vec_at(type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_ulang_type_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return ulang_type_");
                strv_extend_lower(&gen_a, &function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_get_pos_ref(ulang_type_");
                } else {
                    string_extend_cstr(&gen_a, &function, "_get_pos(ulang_type_");
                }
                strv_extend_lower(&gen_a, &function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_unwrap(ulang_type));\n");
                } else {
                    string_extend_cstr(&gen_a, &function, "_const_unwrap(ulang_type));\n");
                }

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

static void gen_ulang_type_set_ptr_depth_common(Ulang_type_type ulang_type, Strv op_str, Strv name_op_str, bool is_get) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        gen_ulang_type_set_ptr_depth_common(vec_at(ulang_type.sub_types, idx), op_str, name_op_str, is_get);
    }

    String function = {0};

    string_extend_f(&gen_a, &function, "static inline %s ", is_get ? "int16_t" : "void");
    string_extend_cstr(&gen_a, &function, " ");
    extend_ulang_type_name_lower(&function, ulang_type.name);
    string_extend_f(&gen_a, &function, "_"FMT"_pointer_depth(", strv_print(name_op_str));
    extend_ulang_type_name_first_upper(&function, ulang_type.name);
    string_extend_f(&gen_a, &function, "%s ulang_type%s) {\n    ", is_get ? "" : "*", is_get ? "" : ", int16_t new_ptr_depth");

    if (ulang_type.sub_types.info.count > 0) {
        string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "switch (ulang_type%stype) {\n", is_get ? "." : "->");
        vec_foreach(idx, Ulang_type_type, sub_type, ulang_type.sub_types) {
            string_extend_cstr(&gen_a/*TODO: rename to a_gen?*/, &function, "        case ");
            extend_ulang_type_name_upper(&function, sub_type.name);
            string_extend_cstr(&gen_a/*TODO: rename to a_gen?*/, &function, ":\n");
            string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "            %s", is_get ? "return " : "");
            extend_ulang_type_name_lower(&function, sub_type.name);
            string_extend_f(&gen_a, &function, "_"FMT"_pointer_depth(", strv_print(name_op_str));
            extend_ulang_type_name_lower(&function, sub_type.name);
            string_extend_f(&gen_a, &function, "%s_unwrap(ulang_type)%s);\n", is_get ? "_const" : "", is_get ? "" : ", new_ptr_depth");
            if (!is_get) {
                string_extend_cstr(&gen_a, &function, "            return;\n");
            }
        }
        // do switch
        string_extend_cstr(&gen_a/*TODO: rename to a_gen?*/, &function, "    }\n");
        string_extend_cstr(&gen_a, &function, "    unreachable(\"\");\n");
    } else {
        if (strv_is_equal(ulang_type.name.base, sv("regular"))) {
            if (is_get) {
                string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "return ulang_type.atom.pointer_depth;;\n");
            } else {
                string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "ulang_type->atom.pointer_depth "FMT" new_ptr_depth;\n", strv_print(op_str));
            }
        } else {
            if (is_get) {
                string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "return ulang_type.pointer_depth;;\n");
            } else {
                string_extend_f(&gen_a/*TODO: rename to a_gen?*/, &function, "ulang_type->pointer_depth "FMT"  new_ptr_depth;\n", strv_print(op_str));
            }

        }
    }

    string_extend_cstr(&gen_a, &function, "}\n");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_ulang_type_get_ptr_depth(Ulang_type_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("="), sv("get"), true);
}

static void gen_ulang_type_set_ptr_depth(Ulang_type_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("="), sv("set"), false);
}

static void gen_ulang_type_add_ptr_depth(Ulang_type_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("+="), sv("add"), false);
}

static void gen_ulang_type_vecs(Ulang_type_type ulang_type) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        gen_ulang_type_vecs(vec_at(ulang_type.sub_types, idx));
    }

    String vec_name = {0};
    extend_ulang_type_name_first_upper(&vec_name, ulang_type.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_ulang_type_name_first_upper(&item_name, ulang_type.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void ulang_type_gen_print_overloading(Ulang_type_type_vec types) {
    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ulang_type_print(ulang_type) strv_print(ulang_type_print_overload(ulang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "#define ulang_type_print_overload(ulang_type, indent) _Generic ((ulang_type), \\\n");

    vec_foreach(idx, Ulang_type_type, type, types) {
        string_extend_cstr(&gen_a, &function, "    ");
        extend_ulang_type_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_ulang_type_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal, \\\n");

        string_extend_cstr(&gen_a, &function, "    const ");
        extend_ulang_type_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_ulang_type_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal%s \\\n", idx + 1 < types.info.count ? "," : "");
    }

    string_extend_cstr(&gen_a, &function, ") (ulang_type, indent)\n");

    gen_gen(FMT"\n", string_print(function));
}

static void ulang_type_get_type_vec_internal(Ulang_type_type_vec* type_vec, Ulang_type_type ulang_type) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_get_type_vec_internal(type_vec, vec_at(ulang_type.sub_types, idx));
    }

    vec_append(&gen_a, type_vec, ulang_type);
}

static Ulang_type_type_vec ulang_type_get_type_vec(Ulang_type_type ulang_type) {
    Ulang_type_type_vec type_vec = {0};
    ulang_type_get_type_vec_internal(&type_vec, ulang_type);
    return type_vec;
}

static void gen_ulang_type(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Ulang_type_type ulang_type = ulang_type_gen_ulang_type();
    Ulang_type_type_vec type_vec = ulang_type_get_type_vec(ulang_type);
    (void) type_vec;

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef ULANG_TYPE_H\n");
        gen_gen("#define ULANG_TYPE_H\n");

        gen_gen("#include <ulang_type_hand_written.h>\n");
    } else {
        gen_gen("#ifndef ULANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#define ULANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    ulang_type_gen_ulang_type_forward_decl(ulang_type);

    if (!implementation) {
        gen_ulang_type_vecs(ulang_type);
    }

    if (implementation) {
        ulang_type_gen_ulang_type_struct(ulang_type);

        ulang_type_gen_ulang_type_unwrap(ulang_type);
        ulang_type_gen_ulang_type_wrap(ulang_type);

        gen_gen("%s\n", "static inline Ulang_type* ulang_type_new(void) {");
        gen_gen("%s\n", "    Ulang_type* new_ulang_type = arena_alloc(&a_main, sizeof(*new_ulang_type));");
        gen_gen("%s\n", "    return new_ulang_type;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_ulang_type_new_forward_decl(ulang_type);
    }
    ulang_type_gen_print_forward_decl(ulang_type);
    if (implementation) {
        gen_ulang_type_new_define(ulang_type);
        gen_ulang_type_get_ptr_depth(ulang_type);
        gen_ulang_type_set_ptr_depth(ulang_type);
        gen_ulang_type_add_ptr_depth(ulang_type);
        //ulang_type_gen_print_overloading(type_vec);
    }
    gen_ulang_type_get_pos(ulang_type, implementation, false);
    gen_ulang_type_get_pos(ulang_type, implementation, true);

    if (implementation) {
        gen_gen("#endif // ULANG_TYPE_H\n");
    } else {
        gen_gen("#endif // ULANG_TYPE_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_ULANG_TYPE_H
