#ifndef AUTO_GEN_ULANG_TYPE_H
#define AUTO_GEN_ULANG_TYPE_H

#include <auto_gen_util.h>
#include <auto_gen_ulang_type_common.h>

static Uast_type ulang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_opaque(const char* prefix) {
    const char* base_name = "opaque";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Uast_type ulang_type = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_signed_int(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_unsigned_int(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_float(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_opaque(base_name));

    return ulang_type;
}

static Uast_type ulang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_tuple", "params");
    append_member(&sym.members, "Ulang_type*", "return_type");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_array(const char* prefix) {
    const char* base_name = "array";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type*", "item_type");
    append_member(&sym.members, "Uast_expr*", "count");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_removed(const char* prefix) {
    const char* base_name = "removed";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Uast_type ulang_type_gen_raw_union(const char* prefix) {
    const char* base_name = "raw_union";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Uast_type ulang_type_gen_enum(const char* prefix) {
    const char* base_name = "enum";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Uast_type ulang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_vec", "ulang_types");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    return sym;
}

static Uast_type ulang_type_gen_int_lit(const char* prefix) {
    const char* base_name = "int_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&lit.members, "int64_t", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type ulang_type_gen_float_lit(const char* prefix) {
    const char* base_name = "float_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&lit.members, "double", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type ulang_type_gen_string_lit(const char* prefix) {
    const char* base_name = "string_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&lit.members, "Strv", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type ulang_type_gen_struct_lit(const char* prefix) {
    const char* base_name = "struct_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&lit.members, "Uast_expr*", "expr");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type ulang_type_gen_fn_lit(const char* prefix) {
    const char* base_name = "fn_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&lit.members, "Name", "name");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type ulang_type_gen_regular(const char* prefix) {
    const char* base_name = "regular";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Uast_type ulang_type_gen_expr(const char* prefix) {
    const char* base_name = "expr";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    append_member(&sym.members, "Uast_expr*", "expr");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ulang_type_gen_lit(const char* prefix) {
    const char* base_name = "lit";
    Uast_type ulang_type = {.name = uast_name_new(prefix, base_name, false, "ulang_type")};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_int_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_float_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_string_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_struct_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_fn_lit(base_name));

    return ulang_type;
}

static Uast_type ulang_type_gen_ulang_type(void) {
    const char* base_name = "ulang_type";
    Uast_type ulang_type = {.name = uast_name_new(base_name, "", true, "ulang_type")};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_tuple(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_fn(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_regular(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_array(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_expr(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_lit(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_removed(base_name));

    return ulang_type;
}

static void ulang_type_gen_ulang_type_forward_decl(Uast_type ulang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_gen_ulang_type_forward_decl(vec_at(ulang_type.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_uast_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_uast_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void ulang_type_gen_ulang_type_struct_as(String* output, Uast_type ulang_type) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_uast_name_first_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(ulang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_uast_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_first_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ulang_type_gen_ulang_type_struct_enum(String* output, Uast_type ulang_type) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_uast_name_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(ulang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void ulang_type_gen_ulang_type_struct(Uast_type ulang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_gen_ulang_type_struct(vec_at(ulang_type.sub_types, idx));
    }

    if (ulang_type.sub_types.info.count > 0) {
        ulang_type_gen_ulang_type_struct_as(&output, ulang_type);
        ulang_type_gen_ulang_type_struct_enum(&output, ulang_type);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (ulang_type.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_uast_name_first_upper(&as_member_type, ulang_type.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_uast_name_upper(&enum_member_type, ulang_type.name);
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
    extend_uast_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void gen_ulang_type_new_forward_decl(Uast_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, false);
}

static void gen_ulang_type_vecs(Uast_type ulang_type) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        gen_ulang_type_vecs(vec_at(ulang_type.sub_types, idx));
    }

    String vec_name = {0};
    extend_uast_name_first_upper(&vec_name, ulang_type.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_uast_name_first_upper(&item_name, ulang_type.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void ulang_type_gen_print_overloading(Uast_type_vec types) {
    return;
    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ulang_type_print(ulang_type) strv_print(ulang_type_print_overload(ulang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "#define ulang_type_print_overload(ulang_type, indent) _Generic ((ulang_type), \\\n");

    vec_foreach(idx, Uast_type, type, types) {
        string_extend_cstr(&gen_a, &function, "    ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_uast_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal, \\\n");

        string_extend_cstr(&gen_a, &function, "    const ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "*: ");
        extend_uast_name_lower(&function, type.name);
        string_extend_f(&gen_a, &function, "_print_internal%s \\\n", idx + 1 < types.info.count ? "," : "");
    }

    string_extend_cstr(&gen_a, &function, ") (ulang_type, indent)\n");

    gen_gen(FMT"\n", string_print(function));
}

static void ulang_type_get_type_vec_internal(Uast_type_vec* type_vec, Uast_type ulang_type) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_get_type_vec_internal(type_vec, vec_at(ulang_type.sub_types, idx));
    }

    vec_append(&gen_a, type_vec, ulang_type);
}

static Uast_type_vec ulang_type_get_type_vec(Uast_type ulang_type) {
    Uast_type_vec type_vec = {0};
    ulang_type_get_type_vec_internal(&type_vec, ulang_type);
    return type_vec;
}

static void gen_ulang_type(const char* file_path, bool implementation) {
    Uast_type ulang_type = ulang_type_gen_ulang_type();
    unwrap(ulang_type.name.type.count > 0);
    gen_ulang_type_common(file_path, implementation, ulang_type);
}


#endif // AUTO_GEN_ULANG_TYPE_H
