
#ifndef AUTO_GEN_UULANG_TYPE_H
#define AUTO_GEN_UULANG_TYPE_H

#include <auto_gen_util.h>

struct Ulang_type_type_;
typedef struct Ulang_type_type_ Ulang_type_type;

typedef struct {
    Vec_base info;
    Ulang_type_type* buf;
} Ulang_type_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Ulang_type_name;

typedef struct Ulang_type_type_ {
    Ulang_type_name name;
    Members members;
    Ulang_type_type_vec sub_types;
} Ulang_type_type;

static void extend_ulang_type_name_upper(String* output, Ulang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ULANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_ulang_type_name_lower(String* output, Ulang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "ulang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_ulang_type_name_first_upper(String* output, Ulang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Ulang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_ulang_type_name_upper(String* output, Ulang_type_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "ULANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_ulang_type_name_lower(String* output, Ulang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        string_extend_cstr(&gen_a, output, "ulang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "ulang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_ulang_type_name_first_upper(String* output, Ulang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "ulang_type")) {
        string_extend_cstr(&gen_a, output, "Ulang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Ulang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Ulang_type_name ulang_type_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Ulang_type_name) {.parent = str_view_from_cstr(parent), .base = str_view_from_cstr(base), .is_topmost = is_topmost};
}

static Ulang_type_type ulang_type_gen_regular(const char* prefix) {
    const char* base_name = "regular";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_atom", "atom");

    return sym;
}

static Ulang_type_type ulang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_vec", "ulang_types");

    return sym;
}

static Ulang_type_type ulang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Ulang_type_type sym = {.name = ulang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Ulang_type_tuple", "params");
    append_member(&sym.members, "Ulang_type*", "return_type");

    return sym;
}

static Ulang_type_type ulang_type_gen_ulang_type(void) {
    const char* base_name = "ulang_type";
    Ulang_type_type ulang_type = {.name = ulang_type_name_new(base_name, "", true)};

    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_tuple(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_fn(base_name));
    vec_append(&gen_a, &ulang_type.sub_types, ulang_type_gen_regular(base_name));

    return ulang_type;
}

static void ulang_type_gen_ulang_type_forward_decl(Ulang_type_type ulang_type) {
    String output = {0};

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        ulang_type_gen_ulang_type_forward_decl(vec_at(&ulang_type.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void ulang_type_gen_ulang_type_struct_as(String* output, Ulang_type_type ulang_type) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_ulang_type_name_first_upper(output, ulang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        Ulang_type_type curr = vec_at(&ulang_type.sub_types, idx);
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
        Ulang_type_type curr = vec_at(&ulang_type.sub_types, idx);
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
        ulang_type_gen_ulang_type_struct(vec_at(&ulang_type.sub_types, idx));
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
        extend_struct_member(&output, vec_at(&ulang_type.members, idx));
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_ulang_type_name_first_upper(&output, ulang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void ulang_type_gen_internal_unwrap(Ulang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_unwrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* ulang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type) {\n");

    //    try(ulang_type->type == upper); 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    try(ulang_type.type == ");
        extend_ulang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    try(ulang_type->type == ");
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

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void ulang_type_gen_internal_wrap(Ulang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_wrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* ulang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_parent_ulang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " ulang_type_");
    extend_strv_lower(&function, type.name.base);
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
    string_extend_cstr(&gen_a, &function, "    Ulang_type new_ulang_type = {0};\n");
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

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void ulang_type_gen_ulang_type_unwrap(Ulang_type_type ulang_type) {
    ulang_type_gen_internal_unwrap(ulang_type, true);
}

void ulang_type_gen_ulang_type_wrap(Ulang_type_type ulang_type) {
    ulang_type_gen_internal_wrap(ulang_type, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Ulang_type)
static void ulang_type_gen_print_forward_decl(Ulang_type_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(ulang_type) str_view_print(");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(ulang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_ulang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ulang_type, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void ulang_type_gen_new_internal(Ulang_type_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_ulang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " ");
    extend_ulang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(");

    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    }
    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        if (idx > 0 && idx < type.members.info.count) {
            string_extend_cstr(&gen_a, &function, ", ");
        }

        Member curr = vec_at(&type.members, idx);

        string_extend_strv(&gen_a, &function, curr.type);
        string_extend_cstr(&gen_a, &function, " ");
        string_extend_strv(&gen_a, &function, curr.name);
    }

    string_extend_cstr(&gen_a, &function, ")");

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        string_extend_cstr(&gen_a, &function, "    return (");
        extend_ulang_type_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ") {");

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            if (idx > 0 && idx < type.members.info.count) {
                string_extend_cstr(&gen_a, &function, ", ");
            }

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

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_ulang_type_new_forward_decl(Ulang_type_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, false);
}

static void gen_ulang_type_new_define(Ulang_type_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, true);
}

static void gen_ulang_type_vecs(Ulang_type_type ulang_type) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        gen_ulang_type_vecs(vec_at(&ulang_type.sub_types, idx));
    }

    String vec_name = {0};
    extend_ulang_type_name_first_upper(&vec_name, ulang_type.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_ulang_type_name_first_upper(&item_name, ulang_type.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_ulang_type(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Ulang_type_type ulang_type = ulang_type_gen_ulang_type();

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
    }

    if (implementation) {
        gen_ulang_type_new_forward_decl(ulang_type);
    }
    ulang_type_gen_print_forward_decl(ulang_type);
    if (implementation) {
        gen_ulang_type_new_define(ulang_type);
    }

    if (implementation) {
        gen_gen("#endif // ULANG_TYPE_H\n");
    } else {
        gen_gen("#endif // ULANG_TYPE_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_UULANG_TYPE_H
