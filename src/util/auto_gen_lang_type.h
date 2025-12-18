
#ifndef AUTO_GEN_LANG_TYPE_H
#define AUTO_GEN_LANG_TYPE_H

#include <auto_gen_util.h>

static Uast_type lang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_opaque(const char* prefix) {
    const char* base_name = "opaque";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Uast_type lang_type = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_signed_int(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_unsigned_int(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_float(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_opaque(base_name));

    return lang_type;
}

static Uast_type lang_type_gen_int_lit(const char* prefix) {
    const char* base_name = "int_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&lit.members, "int64_t", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type lang_type_gen_float_lit(const char* prefix) {
    const char* base_name = "float_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&lit.members, "double", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type lang_type_gen_string_lit(const char* prefix) {
    const char* base_name = "string_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&lit.members, "Strv", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type lang_type_gen_struct_lit(const char* prefix) {
    const char* base_name = "struct_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&lit.members, "Uast_expr*", "data");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type lang_type_gen_fn_lit(const char* prefix) {
    const char* base_name = "fn_lit";
    Uast_type lit = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&lit.members, "Name", "name");
    append_member(&lit.members, "int16_t", "pointer_depth");

    return lit;
}

static Uast_type lang_type_gen_lit(const char* prefix) {
    const char* base_name = "lit";
    Uast_type lang_type = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_int_lit(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_float_lit(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_string_lit(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_struct_lit(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_fn_lit(base_name));

    return lang_type;
}

static Uast_type lang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Lang_type_tuple", "params");
    append_member(&sym.members, "Lang_type*", "return_type");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_a_genrray(const char* prefix) {
    const char* base_name = "array";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Lang_type*", "item_type");
    append_member(&sym.members, "int64_t", "count");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_removed(const char* prefix) {
    const char* base_name = "removed";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Name", "name");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_raw_union(const char* prefix) {
    const char* base_name = "raw_union";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Name", "name");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_enum(const char* prefix) {
    const char* base_name = "enum";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Name", "name");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "Lang_type_vec", "lang_types");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "lang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type lang_type_gen_lang_type(void) {
    const char* base_name = "lang_type";
    Uast_type lang_type = {.name = uast_name_new(base_name, "", true, "lang_type")};

    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_primitive(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_struct(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_raw_union(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_enum(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_tuple(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_void(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_fn(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_a_genrray(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_lit(base_name));
    vec_append(&a_gen, &lang_type.sub_types, lang_type_gen_removed(base_name));

    return lang_type;
}

static void gen_lang_type(const char* file_path, bool implementation) {
    Uast_type lang_type = lang_type_gen_lang_type();
    unwrap(lang_type.name.type.count > 0);
    gen_ulang_type_common(file_path, implementation, lang_type);
}

#endif // AUTO_GEN_LANG_TYPE_H
