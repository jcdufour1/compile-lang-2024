#ifndef AUTO_GEN_UAST_COMMON_H
#define AUTO_GEN_UAST_COMMON_H

#include <auto_gen_util.h>

struct Uast_type_;
typedef struct Uast_type_ Uast_type;

typedef struct {
    Vec_base info;
    Uast_type* buf;
} Uast_type_vec;

typedef struct {
    bool is_topmost;
    Strv parent;
    Strv base;
} Uast_name;

typedef struct Uast_type_ {
    Uast_name name;
    Members members;
    Uast_type_vec sub_types;
} Uast_type;

static void extend_uast_name_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "UAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "uast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Uast");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_uast_name_upper(String* output, Uast_name name) {
    todo();
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "UAST");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_uast_name_lower(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        string_extend_cstr(&gen_a, output, "uast");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_uast_name_first_upper(String* output, Uast_name name) {
    unwrap(name.parent.count > 0);

    if (strv_is_equal(name.parent, sv("uast"))) {
        string_extend_cstr(&gen_a, output, "Uast");
        return;
    }

    unwrap(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Uast");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Uast_name uast_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Uast_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static void uast_gen_uast_forward_decl(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_forward_decl(vec_at(uast.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void uast_gen_uast_struct_as(String* output, Uast_type uast) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_uast_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_first_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void uast_gen_uast_struct_enum(String* output, Uast_type uast) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        Uast_type curr = vec_at(uast.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_uast_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_uast_name_upper(output, uast.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void uast_gen_uast_struct(Uast_type uast) {
    String output = {0};

    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_gen_uast_struct(vec_at(uast.sub_types, idx));
    }

    if (uast.sub_types.info.count > 0) {
        uast_gen_uast_struct_as(&output, uast);
        uast_gen_uast_struct_enum(&output, uast);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (uast.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_uast_name_first_upper(&as_member_type, uast.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_uast_name_upper(&enum_member_type, uast.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < uast.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(uast.members, idx));
    }

    if (uast.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_uast_name_first_upper(&output, uast.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(FMT"\n", string_print(output));
}

static void uast_gen_internal_unwrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast__unwrap##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast) {\n");

    //    unwrap(uast->type == upper); 
    string_extend_cstr(&gen_a, &function, "    unwrap(uast->type == ");
    extend_uast_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ");\n");

    //    return &uast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return &uast->as.");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void uast_gen_internal_wrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_wrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Uast_##lower* uast__unwrap##lower(Uast* uast) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast) {\n");

    //    return &uast->as._##lower; 
    string_extend_cstr(&gen_a, &function, "    return (");
    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "*) uast;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

void uast_gen_uast_unwrap(Uast_type uast) {
    uast_gen_internal_unwrap(uast, false);
    uast_gen_internal_unwrap(uast, true);
}

void uast_gen_uast_wrap(Uast_type uast) {
    uast_gen_internal_wrap(uast, false);
    uast_gen_internal_wrap(uast, true);
}

static void uast_gen_print_overloading(Uast_type_vec types) {
    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define uast_print(uast) strv_print(uast_print_overload(uast, 0))\n");

    string_extend_cstr(&gen_a, &function, "#define uast_print_overload(uast, indent) _Generic ((uast), \\\n");

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

    string_extend_cstr(&gen_a, &function, ") (uast, indent)\n");

    gen_gen(FMT"\n", string_print(function));
}

// TODO: deduplicate these functions (use same function for Ir and Uast)
static void uast_gen_print_forward_decl(Uast_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_print_forward_decl(vec_at(type.sub_types, idx));
    }
 
    if (type.name.is_topmost) {
        return;
    }
 
    String function = {0};
 
    string_extend_cstr(&gen_a, &function, "Strv ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* uast, Indent indent);");
 
    gen_gen(FMT"\n", string_print(function));

    //gen_gen("#define uast_print(uast) strv_print(uast_print_internal(uast, 0))\n");
}

static void uast_gen_new_internal(Uast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_new_internal(vec_at(type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_new(");

    if (type.sub_types.info.count > 0) {
        string_extend_cstr(&gen_a, &function, "void");
    } else {
        string_extend_cstr(&gen_a, &function, "Pos pos");
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
        extend_parent_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_uast = ");
        extend_parent_uast_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_uast->type = ");
        extend_uast_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_cstr(&gen_a, &function, "    uast_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&gen_a, &function, "    uast_");
            extend_strv_lower(&function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            extend_strv_lower(&function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_cstr(&gen_a, &function, "    return uast_");
        extend_strv_lower(&function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap");
        string_extend_cstr(&gen_a, &function, "(base_uast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void uast_gen_internal_get_pos(Uast_type type, bool implementation, bool is_ref) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_get_pos(vec_at(type.sub_types, idx), implementation, is_ref);
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline Pos ");
    if (is_ref) {
        string_extend_cstr(&gen_a, &function, "*");
    }

    if (type.name.is_topmost) {
        if (is_ref) {
            string_extend_cstr(&gen_a, &function, "    uast_get_pos_ref(Uast* uast)");
        } else {
            string_extend_cstr(&gen_a, &function, "    uast_get_pos(const Uast* uast)");
        }
    } else {
        string_extend_cstr(&gen_a, &function, "    uast_");
        extend_strv_lower(&function, type.name.base);
        if (is_ref) {
            string_extend_cstr(&gen_a, &function, "_get_pos_ref(");
        } else {
            string_extend_cstr(&gen_a, &function, "_get_pos(const ");
        }
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* uast)");
    }

    if (implementation) {
        string_extend_cstr(&gen_a, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            if (is_ref) {
                string_extend_cstr(&gen_a, &function, "    return &uast->pos;\n");
            } else {
                string_extend_cstr(&gen_a, &function, "    return uast->pos;\n");
            }
        } else {
            string_extend_cstr(&gen_a, &function, "    switch (uast->type) {\n");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Uast_type curr = vec_at(type.sub_types, idx);
                string_extend_cstr(&gen_a, &function, "        case ");
                extend_uast_name_upper(&function, curr.name);
                string_extend_cstr(&gen_a, &function, ":\n");


                string_extend_cstr(&gen_a, &function, "            return uast_");
                extend_strv_lower(&function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_get_pos_ref(uast_");
                } else {
                    string_extend_cstr(&gen_a, &function, "_get_pos(uast_");
                }
                extend_strv_lower(&function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&gen_a, &function, "_unwrap(uast));\n");
                } else {
                    string_extend_cstr(&gen_a, &function, "_const_unwrap(uast));\n");
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

static void gen_uast_new_forward_decl(Uast_type uast) {
    uast_gen_new_internal(uast, false);
}

static void gen_uast_new_define(Uast_type uast) {
    uast_gen_new_internal(uast, true);
}

static void gen_uast_forward_decl_get_pos(Uast_type uast) {
    uast_gen_internal_get_pos(uast, false, false);
    uast_gen_internal_get_pos(uast, false, true);
}

static void gen_uast_define_get_pos(Uast_type uast) {
    uast_gen_internal_get_pos(uast, true, false);
    uast_gen_internal_get_pos(uast, true, true);
}

static void gen_uast_vecs(Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        gen_uast_vecs(vec_at(uast.sub_types, idx));
    }

    String vec_name = {0};
    extend_uast_name_first_upper(&vec_name, uast.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_uast_name_first_upper(&item_name, uast.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void uast_get_type_vec_internal(Uast_type_vec* type_vec, Uast_type uast) {
    for (size_t idx = 0; idx < uast.sub_types.info.count; idx++) {
        uast_get_type_vec_internal(type_vec, vec_at(uast.sub_types, idx));
    }

    vec_append(&gen_a, type_vec, uast);
}

static Uast_type_vec uast_get_type_vec(Uast_type uast) {
    Uast_type_vec type_vec = {0};
    uast_get_type_vec_internal(&type_vec, uast);
    return type_vec;
}

static void gen_uasts_common(const char* file_path, bool implementation, Uast_type uast) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Uast_type_vec type_vec = uast_get_type_vec(uast);

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef UAST_H\n");
        gen_gen("#define UAST_H\n");

        gen_gen("#include <uast_hand_written.h>\n");
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
        gen_gen("#include <ulang_type.h>\n");
    } else {
        gen_gen("#ifndef UAST_FORWARD_DECL_H\n");
        gen_gen("#define UAST_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    uast_gen_uast_forward_decl(uast);

    if (!implementation) {
        gen_uast_vecs(uast);
    }

    if (implementation) {
        uast_gen_uast_struct(uast);

        uast_gen_uast_unwrap(uast);
        uast_gen_uast_wrap(uast);

        gen_gen("%s\n", "static inline Uast* uast_new(void) {");
        gen_gen("%s\n", "    Uast* new_uast = arena_alloc(&a_main, sizeof(*new_uast));");
        gen_gen("%s\n", "    return new_uast;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_uast_new_forward_decl(uast);
    }
    uast_gen_print_forward_decl(uast);
    if (!implementation) {
        uast_gen_print_overloading(type_vec);
    }
    if (implementation) {
        gen_uast_new_define(uast);
    }

    gen_uast_forward_decl_get_pos(uast);
    if (implementation) {
        gen_uast_define_get_pos(uast);
    }

    if (implementation) {
        gen_gen("#endif // UAST_H\n");
    } else {
        gen_gen("#endif // UAST_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_UAST_COMMON_H
