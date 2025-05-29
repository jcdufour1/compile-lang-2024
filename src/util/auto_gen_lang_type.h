
#ifndef AUTO_GEN_LANG_TYPE_H
#define AUTO_GEN_LANG_TYPE_H

#include <auto_gen_util.h>

struct Lang_type_type_;
typedef struct Lang_type_type_ Lang_type_type;

typedef struct {
    Vec_base info;
    Lang_type_type* buf;
} Lang_type_type_vec;

typedef struct {
    bool is_topmost;
    Str_view parent;
    Str_view base;
} Lang_type_name;

typedef struct Lang_type_type_ {
    Lang_type_name name;
    Members members;
    Lang_type_type_vec sub_types;
} Lang_type_type;

static void extend_lang_type_name_upper(String* output, Lang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        extend_strv_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "LANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_lang_type_name_lower(String* output, Lang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        extend_strv_lower(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "lang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_lang_type_name_first_upper(String* output, Lang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        extend_strv_first_upper(output, name.parent);
    } else {
        string_extend_cstr(&gen_a, output, "Lang_type");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_lower(output, name.base);
    }
}

static void extend_parent_lang_type_name_upper(String* output, Lang_type_name name) {
    todo();
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        unreachable("");
    } else {
        string_extend_cstr(&gen_a, output, "LANG_TYPE");
    }
    if (name.base.count > 0) {
        string_extend_cstr(&gen_a, output, "_");
        extend_strv_upper(output, name.base);
    }
}

static void extend_parent_lang_type_name_lower(String* output, Lang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        string_extend_cstr(&gen_a, output, "lang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "lang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static void extend_parent_lang_type_name_first_upper(String* output, Lang_type_name name) {
    assert(name.parent.count > 0);

    if (str_view_cstr_is_equal(name.parent, "lang_type")) {
        string_extend_cstr(&gen_a, output, "Lang_type");
        return;
    }

    assert(name.base.count > 0);

    string_extend_cstr(&gen_a, output, "Lang_type");
    string_extend_cstr(&gen_a, output, "_");
    extend_strv_lower(output, name.parent);
}

static Lang_type_name lang_type_name_new(const char* parent, const char* base, bool is_topmost) {
    return (Lang_type_name) {.parent = sv(parent), .base = sv(base), .is_topmost = is_topmost};
}

static Lang_type_type lang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Lang_type_type lang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Lang_type_type lang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Lang_type_type lang_type_gen_char(const char* prefix) {
    const char* base_name = "char";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_atom", "atom");

    return sym;
}

static Lang_type_type lang_type_gen_any(const char* prefix) {
    const char* base_name = "any";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    // TODO: get rid of these unneeded atoms
    append_member(&sym.members, "Lang_type_atom", "atom");

    return sym;
}

static Lang_type_type lang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Lang_type_type lang_type = {.name = lang_type_name_new(prefix, base_name, false)};

    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_char(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_signed_int(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_unsigned_int(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_float(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_any(base_name));

    return lang_type;
}

static Lang_type_type lang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_tuple", "params");
    append_member(&sym.members, "Lang_type*", "return_type");

    return sym;
}

static Lang_type_type lang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_atom", "atom");

    return sym;
}

static Lang_type_type lang_type_gen_raw_union(const char* prefix) {
    const char* base_name = "raw_union";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_atom", "atom");

    return sym;
}

static Lang_type_type lang_type_gen_enum(const char* prefix) {
    const char* base_name = "enum";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_atom", "atom");

    return sym;
}

static Lang_type_type lang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    append_member(&sym.members, "Lang_type_vec", "lang_types");

    return sym;
}

static Lang_type_type lang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Lang_type_type sym = {.name = lang_type_name_new(prefix, base_name, false)};

    return sym;
}

static Lang_type_type lang_type_gen_lang_type(void) {
    const char* base_name = "lang_type";
    Lang_type_type lang_type = {.name = lang_type_name_new(base_name, "", true)};

    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_primitive(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_struct(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_raw_union(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_enum(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_tuple(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_void(base_name));
    vec_append(&gen_a, &lang_type.sub_types, lang_type_gen_fn(base_name));

    return lang_type;
}

static void lang_type_gen_lang_type_forward_decl(Lang_type_type lang_type) {
    String output = {0};

    for (size_t idx = 0; idx < lang_type.sub_types.info.count; idx++) {
        lang_type_gen_lang_type_forward_decl(vec_at(&lang_type.sub_types, idx));
    }

    string_extend_cstr(&gen_a, &output, "struct ");
    extend_lang_type_name_first_upper(&output, lang_type.name);
    string_extend_cstr(&gen_a, &output, "_;\n");

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_lang_type_name_first_upper(&output, lang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    extend_lang_type_name_first_upper(&output, lang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void lang_type_gen_lang_type_struct_as(String* output, Lang_type_type lang_type) {
    string_extend_cstr(&gen_a, output, "typedef union ");
    extend_lang_type_name_first_upper(output, lang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < lang_type.sub_types.info.count; idx++) {
        Lang_type_type curr = vec_at(&lang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_lang_type_name_first_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, " ");
        extend_lang_type_name_lower(output, curr.name);
        string_extend_cstr(&gen_a, output, ";\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_lang_type_name_first_upper(output, lang_type.name);
    string_extend_cstr(&gen_a, output, "_as");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void lang_type_gen_lang_type_struct_enum(String* output, Lang_type_type lang_type) {
    string_extend_cstr(&gen_a, output, "typedef enum ");
    extend_lang_type_name_upper(output, lang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, "_ ");
    string_extend_cstr(&gen_a, output, "{\n");

    for (size_t idx = 0; idx < lang_type.sub_types.info.count; idx++) {
        Lang_type_type curr = vec_at(&lang_type.sub_types, idx);
        string_extend_cstr(&gen_a, output, "    ");
        extend_lang_type_name_upper(output, curr.name);
        string_extend_cstr(&gen_a, output, ",\n");
    }

    string_extend_cstr(&gen_a, output, "}");
    extend_lang_type_name_upper(output, lang_type.name);
    string_extend_cstr(&gen_a, output, "_TYPE");
    string_extend_cstr(&gen_a, output, ";\n");

}

static void lang_type_gen_lang_type_struct(Lang_type_type lang_type) {
    String output = {0};

    for (size_t idx = 0; idx < lang_type.sub_types.info.count; idx++) {
        lang_type_gen_lang_type_struct(vec_at(&lang_type.sub_types, idx));
    }

    if (lang_type.sub_types.info.count > 0) {
        lang_type_gen_lang_type_struct_as(&output, lang_type);
        lang_type_gen_lang_type_struct_enum(&output, lang_type);
    }

    string_extend_cstr(&gen_a, &output, "typedef struct ");
    extend_lang_type_name_first_upper(&output, lang_type.name);
    string_extend_cstr(&gen_a, &output, "_ ");
    string_extend_cstr(&gen_a, &output, "{\n");

    if (lang_type.sub_types.info.count > 0) {
        String as_member_type = {0};
        extend_lang_type_name_first_upper(&as_member_type, lang_type.name);
        string_extend_cstr(&gen_a, &as_member_type, "_as");

        String as_member_name = {0};
        string_extend_cstr(&gen_a, &as_member_name, "as");

        extend_struct_member(&output, (Member) {.type = string_to_strv(as_member_type), .name = string_to_strv(as_member_name)});

        String enum_member_type = {0};
        extend_lang_type_name_upper(&enum_member_type, lang_type.name);
        string_extend_cstr(&gen_a, &enum_member_type, "_TYPE");

        String enum_member_name = {0};
        string_extend_cstr(&gen_a, &enum_member_name, "type");

        extend_struct_member(&output, (Member) {.type = string_to_strv(enum_member_type), .name = string_to_strv(enum_member_name)});
    }

    for (size_t idx = 0; idx < lang_type.members.info.count; idx++) {
        extend_struct_member(&output, vec_at(&lang_type.members, idx));
    }

    if (lang_type.sub_types.info.count < 1) {
        extend_struct_member(&output, (Member) {
            .type = sv("Pos"), .name = sv("pos")
        });
    }

    string_extend_cstr(&gen_a, &output, "}");
    extend_lang_type_name_first_upper(&output, lang_type.name);
    string_extend_cstr(&gen_a, &output, ";\n");

    gen_gen(STRING_FMT"\n", string_print(output));
}

static void lang_type_gen_internal_unwrap(Lang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        lang_type_gen_internal_unwrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Lang_type_##lower* lang_type__unwrap##lower(Lang_type* lang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " lang_type_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap");
    string_extend_cstr(&gen_a, &function, "(");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    extend_parent_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "*");
    }
    string_extend_cstr(&gen_a, &function, " lang_type) {\n");

    //    unwrap(lang_type->type == upper); 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    unwrap(lang_type.type == ");
        extend_lang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    unwrap(lang_type->type == ");
        extend_lang_type_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ");\n");
    }

    //    return &lang_type->as._##lower; 
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "    return lang_type.as.");
        extend_lang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    } else {
        string_extend_cstr(&gen_a, &function, "    return &lang_type->as.");
        extend_lang_type_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");
    }

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void lang_type_gen_internal_wrap(Lang_type_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        lang_type_gen_internal_wrap(vec_at(&type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Lang_type_##lower* lang_type__unwrap##lower(Lang_type* lang_type) { 
    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_parent_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " lang_type_");
    extend_strv_lower(&function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_wrap(");
    extend_lang_type_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&gen_a, &function, "* ");
    }
    string_extend_cstr(&gen_a, &function, " lang_type) {\n");

    //    return &lang_type->as._##lower; 
    extend_parent_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " new_lang_type = {0};\n");
    string_extend_cstr(&gen_a, &function, "    new_lang_type.type = ");
    extend_lang_type_name_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, ";\n");

    string_extend_cstr(&gen_a, &function, "    new_lang_type.as.");
    extend_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, " = lang_type;\n");
    if (!is_const) {
        todo();
    }
    string_extend_cstr(&gen_a, &function, "    return new_lang_type;\n");

    //} 
    string_extend_cstr(&gen_a, &function, "}");

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

void lang_type_gen_lang_type_unwrap(Lang_type_type lang_type) {
    lang_type_gen_internal_unwrap(lang_type, false);
    lang_type_gen_internal_unwrap(lang_type, true);
}

void lang_type_gen_lang_type_wrap(Lang_type_type lang_type) {
    //lang_type_gen_internal_wrap(lang_type, false);
    lang_type_gen_internal_wrap(lang_type, true);
}

// TODO: deduplicate these functions (use same function for Llvm and Lang_type)
static void lang_type_gen_print_forward_decl(Lang_type_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        lang_type_gen_print_forward_decl(vec_at(&type.sub_types, idx));
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "#define ");
    extend_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print(lang_type) str_view_print(");
    extend_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(lang_type, 0))\n");

    string_extend_cstr(&gen_a, &function, "Str_view ");
    extend_lang_type_name_lower(&function, type.name);
    string_extend_cstr(&gen_a, &function, "_print_internal(const ");
    extend_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* lang_type, int recursion_depth);");

    gen_gen(STRING_FMT"\n", string_print(function));
}

static void lang_type_gen_new_internal(Lang_type_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        lang_type_gen_new_internal(vec_at(&type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }
    if (type.sub_types.info.count > 0) {
        return;
    }

    String function = {0};

    string_extend_cstr(&gen_a, &function, "static inline ");
    extend_lang_type_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, " ");
    extend_lang_type_name_lower(&function, type.name);
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
        extend_lang_type_name_first_upper(&function, type.name);
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

    gen_gen(STR_VIEW_FMT"\n", str_view_print(string_to_strv(function)));
}

static void gen_lang_type_new_forward_decl(Lang_type_type lang_type) {
    lang_type_gen_new_internal(lang_type, false);
}

static void gen_lang_type_new_define(Lang_type_type lang_type) {
    lang_type_gen_new_internal(lang_type, true);
}

static void gen_lang_type_vecs(Lang_type_type lang_type) {
    for (size_t idx = 0; idx < lang_type.sub_types.info.count; idx++) {
        gen_lang_type_vecs(vec_at(&lang_type.sub_types, idx));
    }

    String vec_name = {0};
    extend_lang_type_name_first_upper(&vec_name, lang_type.name);
    string_extend_cstr(&gen_a, &vec_name, "_vec");

    String item_name = {0};
    extend_lang_type_name_first_upper(&item_name, lang_type.name);
    string_extend_cstr(&gen_a, &item_name, "*");

    gen_vec_from_strv(string_to_strv(vec_name), string_to_strv(item_name));
}

static void gen_lang_type(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Lang_type_type lang_type = lang_type_gen_lang_type();

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef LANG_TYPE_H\n");
        gen_gen("#define LANG_TYPE_H\n");

        gen_gen("#include <lang_type_hand_written.h>\n");
    } else {
        gen_gen("#ifndef LANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#define LANG_TYPE_FORWARD_DECL_H\n");
        gen_gen("#include <vecs.h>\n");
    }

    lang_type_gen_lang_type_forward_decl(lang_type);

    if (!implementation) {
        gen_lang_type_vecs(lang_type);
    }

    if (implementation) {
        lang_type_gen_lang_type_struct(lang_type);

        lang_type_gen_lang_type_unwrap(lang_type);
        lang_type_gen_lang_type_wrap(lang_type);

        gen_gen("%s\n", "static inline Lang_type* lang_type_new(void) {");
        gen_gen("%s\n", "    Lang_type* new_lang_type = arena_alloc(&a_main, sizeof(*new_lang_type));");
        gen_gen("%s\n", "    return new_lang_type;");
        gen_gen("%s\n", "}");
    }

    if (implementation) {
        gen_lang_type_new_forward_decl(lang_type);
    }
    lang_type_gen_print_forward_decl(lang_type);
    if (implementation) {
        gen_lang_type_new_define(lang_type);
    }

    if (implementation) {
        gen_gen("#endif // LANG_TYPE_H\n");
    } else {
        gen_gen("#endif // LANG_TYPE_FORWARD_DECL_H\n");
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_LANG_TYPE_H
