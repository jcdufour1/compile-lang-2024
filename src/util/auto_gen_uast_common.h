#ifndef AUTO_GEN_UAST_COMMON_H
#define AUTO_GEN_UAST_COMMON_H

#include <auto_gen_util.h>
#include <local_string.h>

static void uast_gen_internal_unwrap(Uast_type type, bool is_const) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_internal_unwrap(vec_at(type.sub_types, idx), is_const);
    }

    if (type.name.base.count < 1) {
        return;
    }

    String function = {0};
    string_extend_cstr(&gen_a, &function, "static inline ");
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "const ");
    }

    String uast_lower = {0};
    strv_extend_lower(&gen_a, &uast_lower, type.name.type);

    String uast_first_upper = {0};
    extend_strv_first_upper(&uast_first_upper, type.name.type);

    extend_uast_name_first_upper(&function, type.name);
    string_extend_f(&gen_a, &function, "* "FMT"_", string_print(uast_lower));
    strv_extend_lower(&gen_a, &function, type.name.base);
    if (is_const) {
        string_extend_cstr(&gen_a, &function, "_const");
    }
    string_extend_cstr(&gen_a, &function, "_unwrap(");
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
    string_extend_f(&gen_a, &function, "* "FMT"_", strv_lower_print(&gen_a, type.name.type));
    strv_extend_lower(&gen_a, &function, type.name.base);
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

    Strv uast_str = vec_at(types, 0).name.type;

    string_extend_f(&gen_a, &function, "#define "FMT"_print(uast) strv_print("FMT"_print_overload(uast, 0))\n", strv_print(uast_str), strv_print(uast_str));

    string_extend_f(&gen_a, &function, "#define "FMT"_print_overload(uast, indent) _Generic ((uast), \\\n", strv_print(uast_str));

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
static void uast_gen_print_forward_decl_internal(const char* file, int line, Uast_type type) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_print_forward_decl_internal(file, line, vec_at(type.sub_types, idx));
    }
 
    String function = {0};
 
    if (type.name.is_topmost) {
        string_extend_f(&gen_a, &function, "Strv "FMT, strv_lower_print(&gen_a, type.name.type));
        string_extend_cstr(&gen_a, &function, "_print_internal(const ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* uast, Indent indent);");
    } else {
        string_extend_cstr(&gen_a, &function, "Strv ");
        extend_uast_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_print_internal(const ");
        extend_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* uast, Indent indent);");
    }
 
    gen_gen_internal(global_output, file, line, FMT"\n", string_print(function));

    //gen_gen("#define uast_print(uast) strv_print(uast_print_internal(uast, 0))\n");
}

#define uast_gen_print_forward_decl(type) uast_gen_print_forward_decl_internal(__FILE__, __LINE__, type)

// TODO: remove this function
static void uast_gen_new_internal(Uast_type type, bool implementation) {
    return;
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
    string_extend_cstr(&gen_a, &function, "_new_internal(");

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
            string_extend_f(&gen_a, &function, "    "FMT"_", strv_print(type.name.type));
            strv_extend_lower(&gen_a, &function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->pos = pos;\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(type.members, idx);

            string_extend_f(&gen_a, &function, "    "FMT"_", strv_print(type.name.type));
            strv_extend_lower(&gen_a, &function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap");
            string_extend_cstr(&gen_a, &function, "(base_uast)->");
            strv_extend_lower(&gen_a, &function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            strv_extend_lower(&gen_a, &function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_f(&gen_a, &function, "    return "FMT"_", strv_print(type.name.type));
        strv_extend_lower(&gen_a, &function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap");
        string_extend_cstr(&gen_a, &function, "(base_uast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

static void uast_gen_new_macro(Uast_type type, bool implementation) {
    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
        uast_gen_new_macro(vec_at(type.sub_types, idx), implementation);
    }

    if (type.name.is_topmost) {
        return;
    }

    String function = {0};

    if (!implementation) {
        string_extend_cstr(&gen_a, &function, "#define ");
        extend_uast_name_lower(&function, type.name);
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
        extend_uast_name_lower(&function, type.name);
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
    extend_uast_name_first_upper(&function, type.name);
    string_extend_cstr(&gen_a, &function, "* ");
    extend_uast_name_lower(&function, type.name);
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
        extend_parent_uast_name_first_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, "* base_uast = ");
        extend_parent_uast_name_lower(&function, type.name);
        string_extend_cstr(&gen_a, &function, "_new();\n");

        string_extend_cstr(&gen_a, &function, "    base_uast->type = ");
        extend_uast_name_upper(&function, type.name);
        string_extend_cstr(&gen_a, &function, ";\n");

        if (type.sub_types.info.count < 1) {
            string_extend_f(&gen_a, &function, "    "FMT"_", strv_lower_print(&gen_a, type.name.type));
            strv_extend_lower(&gen_a, &function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_uast)->pos = pos;\n");

            string_extend_cstr(&gen_a, &function, "    (void) loc;\n");
            string_extend_cstr(&gen_a, &function, "#ifndef NDEBUG\n");
            string_extend_f(&gen_a, &function, "    "FMT"_", strv_lower_print(&gen_a, type.name.type));
            strv_extend_lower(&gen_a, &function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_uast)->loc = loc;\n");
            string_extend_cstr(&gen_a, &function, "#endif // NDEBUG\n");
        }

        for (size_t idx = 0; idx < type.members.info.count; idx++) {
            Member curr = vec_at(type.members, idx);

            string_extend_f(&gen_a, &function, "    "FMT"_", strv_lower_print(&gen_a, type.name.type));
            strv_extend_lower(&gen_a, &function, type.name.base);
            string_extend_cstr(&gen_a, &function, "_unwrap(base_uast)->");
            strv_extend_lower(&gen_a, &function, curr.name);
            string_extend_cstr(&gen_a, &function, " = ");
            strv_extend_lower(&gen_a, &function, curr.name);
            string_extend_cstr(&gen_a, &function, ";\n");
        }

        string_extend_f(&gen_a, &function, "    return "FMT"_", strv_lower_print(&gen_a, type.name.type));
        strv_extend_lower(&gen_a, &function, type.name.base);
        string_extend_cstr(&gen_a, &function, "_unwrap(base_uast);\n");

        string_extend_cstr(&gen_a, &function, "}");

    } else {
        string_extend_cstr(&gen_a, &function, ";");
    }

    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
}

//static void uast_gen_new_macro(Uast_type type, bool implementation) {
//    for (size_t idx = 0; idx < type.sub_types.info.count; idx++) {
//        uast_gen_new_macro(vec_at(type.sub_types, idx), implementation);
//    }
//
//    if (type.name.is_topmost) {
//        return;
//    }
//
//    String function = {0};
//
//    string_extend_f(&gen_a, &function, "#define "FMT"_new(", strv_lower_print(&gen_a, type.name.type));
//
//    log(LOG_DEBUG, FMT"\n", string_print(function));
//    todo();
//
//    gen_gen(FMT"\n", strv_print(string_to_strv(function)));
//}

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
        String uast_lower = {0};
        strv_extend_lower(&gen_a, &uast_lower, type.name.type);

        String uast_first_upper = {0};
        extend_strv_first_upper(&uast_first_upper, type.name.type);

        if (is_ref) {
            string_extend_f(&gen_a, &function, "    "FMT"_get_pos_ref("FMT"* uast)", string_print(uast_lower), string_print(uast_first_upper));
        } else {
            string_extend_f(&gen_a, &function, "    "FMT"_get_pos(const "FMT"* uast)", string_print(uast_lower), string_print(uast_first_upper));
        }
    } else {
        string_extend_f(&gen_a, &function, "    "FMT"_", strv_print(type.name.type));
        strv_extend_lower(&gen_a, &function, type.name.base);
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


                string_extend_f(&gen_a, &function, "            return "FMT"_", strv_print(type.name.type));
                strv_extend_lower(&gen_a, &function, curr.name.base);
                string_extend_f(&gen_a, &function, "_get_pos%s(", is_ref ? "_ref" : "");
                string_extend_f(&gen_a, &function, FMT"_", strv_lower_print(&gen_a, type.name.type));
                strv_extend_lower(&gen_a, &function, curr.name.base);
                string_extend_f(&gen_a, &function, "%s_unwrap(uast));\n", is_ref ? "" : "_const");

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

// TODO: rename this function
static void gen_thing(Uast_type uast) {
    Strv sym_text = {0};
    if (strv_is_equal(uast.name.type, sv("uast"))) {
        gen_gen("static void usymbol_extend_table_internal(String* buf, const Usymbol_table sym_table, Indent indent) {\n");
        gen_gen("    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
        gen_gen("        Usymbol_table_tast* sym_tast = &sym_table.table_tasts[idx];\n");
        gen_gen("        if (sym_tast->status == SYM_TBL_OCCUPIED) {\n");
        gen_gen("            string_extend_strv(&a_temp, buf, uast_def_print_internal(sym_tast->tast, indent));\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("}\n");

        sym_text = sv("usymbol");
    } else if (strv_is_equal(uast.name.type, sv("tast"))) {
        gen_gen("static void symbol_extend_table_internal(String* buf, const Symbol_table sym_table, Indent indent) {\n");
        gen_gen("    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
        gen_gen("        Symbol_table_tast* sym_tast = &sym_table.table_tasts[idx];\n");
        gen_gen("        if (sym_tast->status == SYM_TBL_OCCUPIED) {\n");
        gen_gen("            string_extend_strv(&a_temp, buf, tast_def_print_internal(sym_tast->tast, indent));\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("}\n");

        sym_text = sv("symbol");
    } else if (strv_is_equal(uast.name.type, sv("ir"))) {
        gen_gen("static void ir_extend_table_internal(String* buf, const Ir_table sym_table, Indent indent) {\n");
        gen_gen("    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
        gen_gen("        Ir_table_tast* sym_tast = &sym_table.table_tasts[idx];\n");
        gen_gen("        if (sym_tast->status == SYM_TBL_OCCUPIED) {\n");
        gen_gen("            string_extend_strv(&a_temp, buf, ir_print_internal(sym_tast->tast, indent));\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("}\n");

        sym_text = sv("ir");
    } else {
        unreachable(FMT" not covered\n", strv_print(uast.name.type));
    }
    
    gen_gen(
        "#define "FMT"_log(log_level, scope_id) "FMT"_log_internal(log_level, __FILE__, __LINE__,  0, scope_id);\n",
        strv_print(sym_text),
        strv_print(sym_text)
    );
    gen_gen(
        "#define "FMT"_log_level(dest, log_level, scope_id) \\\n"
        "    if (log_level >= MIN_LOG_LEVEL && log_level >= params_log_level) { \\\n"
        "        "FMT"_level_log_internal(dest, log_level, __FILE__, __LINE__, vec_at(env.symbol_tables, scope_id)."FMT"_table, 0); \\\n"
        "    }\n",
        strv_print(sym_text),
        strv_print(sym_text),
        strv_print(sym_text)
    );
    gen_gen(
        "static void "FMT"_level_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, "FMT"_table level, Indent indent) {\n",
        strv_print(sym_text),
        strv_first_upper_print(&gen_a, sym_text)
    );
        gen_gen("    String buf = {0};\n");
        gen_gen("    "FMT"_extend_table_internal(&buf, level, indent);\n", strv_print(sym_text));
        gen_gen("    log_internal_ex(dest, log_level, file, line, indent + INDENT_WIDTH, \"\\n\"FMT\"\\n\", string_print(buf));\n");
    gen_gen("}\n");

    gen_gen("static void "FMT"_log_internal(FILE* dest, LOG_LEVEL log_level, const char* file, int line, Indent indent, Scope_id scope_id) {\n", strv_print(sym_text));
    gen_gen("    log_internal(log_level, file, line, 0, \"----start "FMT" table----\\n\");\n", strv_print(sym_text));
    gen_gen("    Scope_id curr_scope = scope_id;\n");
    gen_gen("    size_t idx = 0;\n");
    gen_gen("    while (true) {\n");
    gen_gen("        "FMT"_table curr = vec_at(env.symbol_tables, curr_scope)."FMT"_table;\n", strv_first_upper_print(&gen_a, sym_text), strv_print(sym_text));
    gen_gen("        log_internal(log_level, file, line, 0, \"level: %%zu\\n\", idx);\n");
    gen_gen("        "FMT"_level_log_internal(dest, log_level, file, line, curr, indent + INDENT_WIDTH);\n", strv_print(sym_text));
    gen_gen("        if (curr_scope == 0) {\n");
    gen_gen("            break;\n");
    gen_gen("        }\n");
    gen_gen("        curr_scope = scope_get_parent_tbl_lookup(curr_scope);\n");
    gen_gen("    }\n");
    gen_gen("    log_internal(log_level, file, line, 0, \"----end "FMT" table------\\n\");\n", strv_print(sym_text));
    gen_gen("}\n");

}


static void gen_uasts_common(const char* file_path, bool implementation, Uast_type uast) {
    unwrap(uast.name.type.count > 0);

    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    String normal_include_guard = {0};
    extend_uast_name_upper(&normal_include_guard, uast.name);
    string_extend_cstr(&gen_a, &normal_include_guard, "_H");

    Uast_type_vec type_vec = uast_get_type_vec(uast);

    gen_gen("/* autogenerated */\n");


    if (implementation) {
        gen_gen("#ifndef "FMT"_H\n", string_print(normal_include_guard));
        gen_gen("#define "FMT"_H\n", string_print(normal_include_guard));

        gen_gen("#include <"FMT"_hand_written.h>\n", uast_lower_print(uast.name));
        gen_gen("#include <"FMT"_forward_decl.h>\n", uast_lower_print(uast.name));
        gen_gen("#include <symbol_table_struct.h>\n");
        gen_gen("#include <symbol_table.h>\n");
        gen_gen("#include <token.h>\n");
        gen_gen("#include <ulang_type.h>\n");
        gen_gen("#include <env.h>\n");
    } else {
        gen_gen("#ifndef "FMT"_FORWARD_DECLARATION_H\n", string_print(normal_include_guard));
        gen_gen("#define "FMT"_FORWARD_DECLARATION_H\n", string_print(normal_include_guard));
        gen_gen("#include <vecs.h>\n");
        gen_gen("#include <operator_type.h>\n");
        gen_gen("#include <ir_operator_type.h>\n");
        gen_gen("#include <cfg.h>\n");
        gen_gen("#include <loc.h>\n");
        gen_gen("#include <lang_type.h>\n");

        gen_gen("struct Tast_enum_tag_lit_;\n");
        gen_gen("typedef struct Tast_enum_tag_lit_ Tast_enum_tag_lit;\n");

        gen_gen("Scope_id scope_get_parent_tbl_lookup(Scope_id key);\n");
    }

    uast_gen_uast_forward_decl(uast);


    if (!implementation) {
        gen_uast_vecs(uast);
        if (strv_is_equal(uast.name.type, sv("uast"))) {
            gen_gen("typedef struct {\n");
                gen_gen("Uast_generic_param_vec generics;\n");
                gen_gen("Uast_variable_def_vec members;\n");
                gen_gen("Name name;\n");
            gen_gen("} Ustruct_def_base;\n");
        } else if (strv_is_equal(uast.name.type, sv("tast"))) {
            gen_gen("typedef struct {\n");
                gen_gen("Lang_type lang_type;\n");
                gen_gen("Name name;\n");
            gen_gen("} Sym_typed_base;\n");

            gen_gen("typedef struct {\n");
                gen_gen("Tast_variable_def_vec members;\n");
                gen_gen("Name name;\n");
            gen_gen("} Struct_def_base;\n");
            gen_gen("#include <ir_lang_type.h>\n");
        } else if (strv_is_equal(uast.name.type, sv("ir"))) {
            gen_gen("typedef struct {\n");
                gen_gen("Ir_struct_memb_def_vec members;\n");
                gen_gen("Ir_name name;\n");
            gen_gen("} Ir_struct_def_base;\n");
        } else {
            unreachable("uncovered type");
        }
    }

    if (implementation) {
        uast_gen_uast_struct(uast);

        uast_gen_uast_unwrap(uast);
        uast_gen_uast_wrap(uast);

        gen_gen("static inline "FMT"* "FMT"_new(void) {\n", strv_first_upper_print(&gen_a, uast.name.type), strv_lower_print(&gen_a, uast.name.type));
        gen_gen("    "FMT"* new_uast = arena_alloc(&a_main, sizeof(*new_uast));\n", strv_first_upper_print(&gen_a, uast.name.type));
        gen_gen("    return new_uast;\n");
        gen_gen("}\n");
    }

    if (implementation) {
        gen_uast_new_forward_decl(uast);
        gen_thing(uast);
    }
    uast_gen_new_macro(uast, implementation);
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
        gen_gen("#endif // "FMT"_H\n", string_print(normal_include_guard));
    } else {
        gen_gen("#endif // "FMT"_FORWARD_DECLARATION_H\n", string_print(normal_include_guard));
    }

    CLOSE_FILE(global_output);
}

#endif // AUTO_GEN_UAST_COMMON_H
