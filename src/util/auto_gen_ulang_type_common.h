#ifndef AUTO_GEN_ULANG_TYPE_COMMON_H
#define AUTO_GEN_ULANG_TYPE_COMMON_H

#include <auto_gen_util.h>

static void ulang_type_gen_internal_unwrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* lang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&a_gen, &function, "static inline ");
    extend_uast_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&a_gen, &function, "*");
    }
    string_extend_f(&a_gen, &function, " "FMT"_", strv_lower_print(&a_gen, type.name.type));
    strv_extend_lower(&a_gen, &function, type.name.base);
    if (is_const) {
        string_extend_cstr(&a_gen, &function, "_const");
    }
    string_extend_cstr(&a_gen, &function, "_unwrap");
    string_extend_cstr(&a_gen, &function, "(");
    if (is_const) {
        string_extend_cstr(&a_gen, &function, "const ");
    }

    extend_parent_uast_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&a_gen, &function, "*");
    }
    string_extend_cstr(&a_gen, &function, " ulang_type) {\n");

    //    unwrap(ulang_type->type == upper); 
    if (is_const) {
        string_extend_cstr(&a_gen, &function, "    unwrap(ulang_type.type == ");
        extend_uast_name_upper(&function, type.name);
        string_extend_cstr(&a_gen, &function, ");\n");
    } else {
        string_extend_cstr(&a_gen, &function, "    unwrap(ulang_type->type == ");
        extend_uast_name_upper(&function, type.name);
        string_extend_cstr(&a_gen, &function, ");\n");
    }

    //    return &ulang_type->as._##lower; 
    if (is_const) {
        string_extend_cstr(&a_gen, &function, "    return ulang_type.as.");
        extend_uast_name_lower(&function, type.name);
        string_extend_cstr(&a_gen, &function, ";\n");
    } else {
        string_extend_cstr(&a_gen, &function, "    return &ulang_type->as.");
        extend_uast_name_lower(&function, type.name);
        string_extend_cstr(&a_gen, &function, ";\n");
    }

    //} 
    string_extend_cstr(&a_gen, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ulang_type_gen_internal_wrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        ulang_type_gen_internal_wrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    //static inline Ulang_type_##lower* lang_type__unwrap##lower(Ulang_type* ulang_type) { 
    string_extend_cstr(&a_gen, &function, "static inline ");
    extend_parent_uast_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&a_gen, &function, "* ");
    }
    string_extend_f(&a_gen, &function, " "FMT"_", strv_lower_print(&a_gen, type.name.type));
    strv_extend_lower(&a_gen, &function, type.name.base);
    if (is_const) {
        string_extend_cstr(&a_gen, &function, "_const");
    }
    string_extend_cstr(&a_gen, &function, "_wrap(");
    extend_uast_name_first_upper(&function, type.name);
    if (!is_const) {
        string_extend_cstr(&a_gen, &function, "* ");
    }
    string_extend_cstr(&a_gen, &function, " ulang_type) {\n");

    extend_parent_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&a_gen, &function, " new_ulang_type = {0};\n");
    string_extend_cstr(&a_gen, &function, "    new_ulang_type.type = ");
    extend_uast_name_upper(&function, type.name);
    string_extend_cstr(&a_gen, &function, ";\n");

    string_extend_cstr(&a_gen, &function, "    new_ulang_type.as.");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&a_gen, &function, " = ulang_type;\n");
    if (!is_const) {
        todo();
    }

    string_extend_f(&a_gen, &function, "    #ifndef NDEBUG\n");
    string_extend_f(&a_gen, &function, "        new_ulang_type.loc = ulang_type.loc;\n");
    string_extend_f(&a_gen, &function, "    #endif // NDEBUG\n");

    string_extend_cstr(&a_gen, &function, "    return new_ulang_type;\n");

    string_extend_cstr(&a_gen, &function, "}");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void ulang_type_gen_ulang_type_unwrap(Uast_type ulang_type) {
    ulang_type_gen_internal_unwrap(ulang_type, false);
    ulang_type_gen_internal_unwrap(ulang_type, true);
}

static void ulang_type_gen_ulang_type_wrap(Uast_type ulang_type) {
    //ulang_type_gen_internal_wrap(ulang_type, false);
    ulang_type_gen_internal_wrap(ulang_type, true);
}

static void ulang_type_gen_new_internal(Uast_type type, bool implementation) {
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

    string_extend_cstr(&a_gen, &function, "static inline ");
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&a_gen, &function, " ");
    extend_uast_name_lower(&function, type.name);
    string_extend_cstr(&a_gen, &function, "_new(Pos pos ");

    for (size_t idx = 0; idx < type.members.info.count; idx++) {
        string_extend_cstr(&a_gen, &function, ", ");

        Member curr = vec_at(type.members, idx);

        string_extend_strv(&a_gen, &function, curr.type);
        string_extend_cstr(&a_gen, &function, " ");
        string_extend_strv(&a_gen, &function, curr.name);
    }

    string_extend_cstr(&a_gen, &function, ")");

    if (implementation) {
        string_extend_cstr(&a_gen, &function, "{\n");

        string_extend_cstr(&a_gen, &function, "    return (");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&a_gen, &function, ") {.pos = pos");

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            string_extend_cstr(&a_gen, &function, ", ");

            Member curr = vec_at(type.members, idx);

            string_extend_cstr(&a_gen, &function, " .");
            strv_extend_lower(&a_gen, &function, curr.name);
            string_extend_cstr(&a_gen, &function, " = ");
            strv_extend_lower(&a_gen, &function, curr.name);
        }

        string_extend_cstr(&a_gen, &function, "};\n");
        string_extend_cstr(&a_gen, &function, "}");

    } else {
        string_extend_cstr(&a_gen, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

// "constructors" for ulang_types
static void gen_ulang_type_new_define(Uast_type ulang_type) {
    ulang_type_gen_new_internal(ulang_type, true);
}

static void gen_ulang_type_set_ptr_depth_common(Uast_type ulang_type, Strv op_str, Strv name_op_str, bool is_get) {
    for (size_t idx = 0; idx < ulang_type.sub_types.info.count; idx++) {
        gen_ulang_type_set_ptr_depth_common(vec_at(ulang_type.sub_types, idx), op_str, name_op_str, is_get);
    }

    String function = {0};

    string_extend_f(&a_gen, &function, "static inline %s ", is_get ? "int16_t" : "void");
    string_extend_cstr(&a_gen, &function, " ");
    extend_uast_name_lower(&function, ulang_type.name);
    string_extend_f(&a_gen, &function, "_"FMT"_pointer_depth(", strv_print(name_op_str));
    extend_uast_name_first_upper(&function, ulang_type.name);
    string_extend_f(&a_gen, &function, "%s ulang_type%s) {\n    ", is_get ? "" : "*", is_get ? "" : ", int16_t new_ptr_depth");

    if (ulang_type.sub_types.info.count > 0) {
        string_extend_f(&a_gen, &function, "switch (ulang_type%stype) {\n", is_get ? "." : "->");
        vec_foreach(idx, Uast_type, sub_type, ulang_type.sub_types) {
            string_extend_cstr(&a_gen, &function, "        case ");
            extend_uast_name_upper(&function, sub_type.name);
            string_extend_cstr(&a_gen, &function, ":\n");
            string_extend_f(&a_gen, &function, "            %s", is_get ? "return " : "");
            extend_uast_name_lower(&function, sub_type.name);
            string_extend_f(&a_gen, &function, "_"FMT"_pointer_depth(", strv_print(name_op_str));
            extend_uast_name_lower(&function, sub_type.name);
            string_extend_f(&a_gen, &function, "%s_unwrap(ulang_type)%s);\n", is_get ? "_const" : "", is_get ? "" : ", new_ptr_depth");
            if (!is_get) {
                string_extend_cstr(&a_gen, &function, "            return;\n");
            }
        }
        // do switch
        string_extend_cstr(&a_gen, &function, "    }\n");
        string_extend_cstr(&a_gen, &function, "    unreachable(\"\");\n");
    } else {
        // TODO: if Lang_type_atom is not removed, put static assert system here
        if (
            strv_is_equal(ulang_type.name.base, sv("regular")) || 
            strv_is_equal(ulang_type.name.base, sv("struct")) || 
            strv_is_equal(ulang_type.name.base, sv("raw_union")) || 
            strv_is_equal(ulang_type.name.base, sv("enum"))
        ) {
            if (is_get) {
                string_extend_f(&a_gen, &function, "return ulang_type.atom.pointer_depth;;\n");
            } else {
                string_extend_f(&a_gen, &function, "ulang_type->atom.pointer_depth "FMT" new_ptr_depth;\n", strv_print(op_str));
            }
        } else {
            if (is_get) {
                string_extend_f(&a_gen, &function, "return ulang_type.pointer_depth;;\n");
            } else {
                string_extend_f(&a_gen, &function, "ulang_type->pointer_depth "FMT"  new_ptr_depth;\n", strv_print(op_str));
            }

        }
    }

    string_extend_cstr(&a_gen, &function, "}\n");

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void gen_ulang_type_get_ptr_depth(Uast_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("="), sv("get"), true);
}

static void gen_ulang_type_set_ptr_depth(Uast_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("="), sv("set"), false);
}

static void gen_ulang_type_add_ptr_depth(Uast_type ulang_type) {
    gen_ulang_type_set_ptr_depth_common(ulang_type, sv("+="), sv("add"), false);
}

static void gen_ulang_type_get_pos(Uast_type type, bool implementation, bool is_ref) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        gen_ulang_type_get_pos(vec_at(type.sub_types, idx), implementation, is_ref);
    }

    String function = {0};

    string_extend_cstr(&a_gen, &function, "static inline Pos ");
    if (is_ref) {
        string_extend_cstr(&a_gen, &function, "*");
    }

    string_extend_f(&a_gen, &function, "    "FMT"_", strv_lower_print(&a_gen, type.name.type));
    strv_extend_lower(&a_gen, &function, type.name.base);
    if (is_ref) {
        string_extend_f(&a_gen, &function, "%sget_pos_ref(", type.name.is_topmost ? "" : "_");
    } else {
        string_extend_f(&a_gen, &function, "%sget_pos(const ", type.name.is_topmost ? "" : "_");
    }
    extend_uast_name_first_upper(&function, type.name);
    string_extend_f(&a_gen, &function, "%s ulang_type)", is_ref ? "*" : "");

    if (implementation) {
        string_extend_cstr(&a_gen, &function, "{\n");

        if (type.sub_types.info.count < 1) {
            if (is_ref) {
                string_extend_cstr(&a_gen, &function, "    return &ulang_type->pos;\n");
            } else {
                string_extend_cstr(&a_gen, &function, "    return ulang_type.pos;\n");
            }
        } else {
            string_extend_f(&a_gen, &function, "    switch (ulang_type%stype) {\n", is_ref ? "->" : ".");

            for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
                Uast_type curr = vec_at(type.sub_types, idx);
                string_extend_cstr(&a_gen, &function, "        case ");
                extend_uast_name_upper(&function, curr.name);
                string_extend_cstr(&a_gen, &function, ":\n");


                string_extend_f(&a_gen, &function, "            return "FMT"_", strv_lower_print(&a_gen, type.name.type));
                strv_extend_lower(&a_gen, &function, curr.name.base);
                if (is_ref) {
                    string_extend_f(&a_gen, &function, "_get_pos_ref("FMT"_", strv_lower_print(&a_gen, type.name.type));
                } else {
                    string_extend_f(&a_gen, &function, "_get_pos("FMT"_", strv_lower_print(&a_gen, type.name.type));
                }
                strv_extend_lower(&a_gen, &function, curr.name.base);
                if (is_ref) {
                    string_extend_cstr(&a_gen, &function, "_unwrap(ulang_type));\n");
                } else {
                    string_extend_cstr(&a_gen, &function, "_const_unwrap(ulang_type));\n");
                }

                string_extend_cstr(&a_gen, &function, "        break;\n");
            }

            string_extend_cstr(&a_gen, &function, "    }\n");
        }

        string_extend_cstr(&a_gen, &function, "unreachable(\"\");\n");
        string_extend_cstr(&a_gen, &function, "}\n");

    } else {
        string_extend_cstr(&a_gen, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

// TODO: remove implementation paramter, or actually call gen_ulang_type_common with implementation == to both true and false
static void gen_ulang_type_common(const char* file_path, bool implementation, Uast_type ulang_type) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    Uast_type_vec type_vec = uast_get_type_vec(ulang_type);
    (void) type_vec;

    gen_gen("/* autogenerated */\n");

    if (implementation) {
        gen_gen("#ifndef "FMT"_H\n", strv_upper_print(&a_gen, ulang_type.name.type));
        gen_gen("#define "FMT"_H\n", strv_upper_print(&a_gen, ulang_type.name.type));

        if (strv_is_equal(ulang_type.name.type, sv("ulang_type"))) {
            gen_gen("#include <ulang_type_hand_written.h>\n");
        } else if (strv_is_equal(ulang_type.name.type, sv("lang_type"))) {
            gen_gen("#include <lang_type_hand_written.h>\n");
        } else if (strv_is_equal(ulang_type.name.type, sv("ir_lang_type"))) {
            gen_gen("#include <ir_lang_type_hand_written.h>\n");
        } else {
            unreachable(FMT" is not covered", strv_print(ulang_type.name.type));
        }
    } else {
        gen_gen("#ifndef "FMT"_FORWARD_DECL_H\n", strv_upper_print(&a_gen, ulang_type.name.type));
        gen_gen("#define "FMT"_FORWARD_DECL_H\n", strv_upper_print(&a_gen, ulang_type.name.type));
        gen_gen("#include <vecs.h>\n");
    }
    gen_gen("#include <loc.h>\n");

    uast_gen_uast_forward_decl(ulang_type);

    if (!implementation) {
        gen_uast_vecs(ulang_type);
    }

    if (implementation) {
        uast_gen_uast_struct(ulang_type);

        ulang_type_gen_ulang_type_unwrap(ulang_type);
        ulang_type_gen_ulang_type_wrap(ulang_type);

        gen_gen(
            "static inline "FMT"* "FMT"_new(void) {\n",
            strv_first_upper_print(&a_gen, ulang_type.name.type),
            strv_lower_print(&a_gen, ulang_type.name.type)
        );
        gen_gen(
            "    "FMT"* new_ulang_type = arena_alloc(&a_main, sizeof(*new_ulang_type));\n",
            strv_first_upper_print(&a_gen, ulang_type.name.type)
        );
        gen_gen("    return new_ulang_type;\n");
        gen_gen("}\n");
    }

    if (implementation) {
        gen_uast_new_forward_decl(ulang_type);
    }
    if (implementation) {
        gen_ulang_type_new_define(ulang_type);
        gen_ulang_type_get_ptr_depth(ulang_type);
        gen_ulang_type_set_ptr_depth(ulang_type);
        gen_ulang_type_add_ptr_depth(ulang_type);
    }
    gen_ulang_type_get_pos(ulang_type, implementation, false);
    gen_ulang_type_get_pos(ulang_type, implementation, true);

    if (implementation) {
        gen_gen("#endif // "FMT"_H\n", strv_upper_print(&a_gen, ulang_type.name.type));
    } else {
        gen_gen("#endif // "FMT"_FORWARD_DECL_H\n", strv_upper_print(&a_gen, ulang_type.name.type));
    }

    close_file(global_output);
}

#endif // AUTO_GEN_ULANG_TYPE_COMMON_H
