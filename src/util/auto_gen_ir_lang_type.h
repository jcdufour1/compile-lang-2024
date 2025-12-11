
#ifndef AUTO_GEN_IR_LANG_TYPE_H
#define AUTO_GEN_IR_LANG_TYPE_H

#include <auto_gen_util.h>

static Uast_type ir_lang_type_gen_signed_int(const char* prefix) {
    const char* base_name = "signed_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_unsigned_int(const char* prefix) {
    const char* base_name = "unsigned_int";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_float(const char* prefix) {
    const char* base_name = "float";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "uint32_t", "bit_width");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_opaque(const char* prefix) {
    const char* base_name = "opaque";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_primitive(const char* prefix) {
    const char* base_name = "primitive";
    Uast_type ir_lang_type = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_signed_int(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_unsigned_int(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_float(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_opaque(base_name));

    return ir_lang_type;
}

static Uast_type ir_lang_type_gen_fn(const char* prefix) {
    const char* base_name = "fn";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "Ir_lang_type_tuple", "params");
    append_member(&sym.members, "Ir_lang_type*", "return_type");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_struct(const char* prefix) {
    const char* base_name = "struct";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "Ir_lang_type_atom", "atom");

    return sym;
}

static Uast_type ir_lang_type_gen_tuple(const char* prefix) {
    const char* base_name = "tuple";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "Ir_lang_type_vec", "ir_lang_types");
    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_void(const char* prefix) {
    const char* base_name = "void";
    Uast_type sym = {.name = uast_name_new(prefix, base_name, false, "ir_lang_type")};

    append_member(&sym.members, "int16_t", "pointer_depth");

    return sym;
}

static Uast_type ir_lang_type_gen_ir_lang_type(void) {
    const char* base_name = "ir_lang_type";
    Uast_type ir_lang_type = {.name = uast_name_new(base_name, "", true, "ir_lang_type")};

    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_primitive(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_struct(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_tuple(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_void(base_name));
    vec_append(&a_gen, &ir_lang_type.sub_types, ir_lang_type_gen_fn(base_name));

    return ir_lang_type;
}

static void gen_ir_lang_type(const char* file_path, bool implementation) {
    Uast_type ir_lang_type = ir_lang_type_gen_ir_lang_type();
    unwrap(ir_lang_type.name.type.count > 0);
    gen_ulang_type_common(file_path, implementation, ir_lang_type);
}

#endif // AUTO_GEN_IR_LANG_TYPE_H
