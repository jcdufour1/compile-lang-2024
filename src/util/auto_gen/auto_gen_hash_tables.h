#ifndef AUTO_GEN_HASH_TABLES_H
#define AUTO_GEN_HASH_TABLES_H

#include <auto_gen_util.h>
#include <local_string.h>

static void gen_hash_table(Strv suffix, Strv base_type, int16_t pointer_depth, bool do_scoped_lookup, bool implementation) {
    assert(strv_is_equal(suffix, strv_lower_print_internal(&a_gen, suffix)));

    String ptr_depth_str = {0};
    for (uint16_t idx = 0; idx < pointer_depth; idx++) {
        string_append(&a_gen, &ptr_depth_str, '*');
    }

    Strv type_with_ptr = {0};
    if (pointer_depth > 0) {
        type_with_ptr = strv_from_f(&a_gen, FMT FMT, strv_print(base_type), string_print(ptr_depth_str));
    } else {
        type_with_ptr = strv_from_f(&a_gen, FMT, strv_print(base_type));
    }

    if (implementation) {
        gen_gen("#include <hash_table_structs.h>\n");

        gen_gen("static void hash_table_reserve_"FMT"(void) {\n", strv_print(suffix));
        gen_gen("    todo();\n");
        gen_gen("}\n\n");

        gen_gen("static void hash_table_add_"FMT"(void) {\n", strv_print(suffix));
        gen_gen("    todo();\n");
        gen_gen("}\n\n");

        gen_gen("static void hash_table_stable_add_"FMT"(void) {\n", strv_print(suffix));
        gen_gen("    todo();\n");
        gen_gen("}\n\n");
    } else {
        gen_gen("#include <util.h>\n");
        gen_gen("#include <darr.h>\n");
        gen_gen("#include <uast_forward_decl.h>\n");
        gen_gen("#include <tast_forward_decl.h>\n");
        gen_gen("#include <ir_forward_decl.h>\n");

        gen_gen("typedef struct {\n");
        gen_gen("    Vec_base info;\n");
        gen_gen("    "FMT" buf;\n", strv_print(type_with_ptr));
        gen_gen("} Hash_table_"FMT"_darr;\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    "FMT" item;\n", strv_print(type_with_ptr));
        gen_gen("    Strv key;\n");
        gen_gen("    SYM_TBL_STATUS status;\n");
        gen_gen("} Hash_table_node_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_node_"FMT"* nodes;\n", strv_print(suffix));
        gen_gen("    size_t count;\n");
        gen_gen("    size_t capacity;\n");
        gen_gen("} Hash_table_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_"FMT" hash_table;\n\n", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"_darr keys;\n", strv_print(suffix));
        gen_gen("} Hash_table_stable_"FMT";\n\n", strv_print(suffix));
    }
}

static void gen_all_hash_tables(const char* file_path, bool implementation) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    if (implementation) {
        gen_gen("#ifndef HASH_TABLES_H\n");
        gen_gen("#define HASH_TABLES_H\n");
    } else {
        gen_gen("#ifndef HASH_TABLE_STRUCTS_H\n");
        gen_gen("#define HASH_TABLE_STRUCTS_H\n");

        gen_gen("typedef enum {\n");
        gen_gen("    HASH_TABLE_NODE_NEVER_OCCUPIED = 0,\n");
        gen_gen("    HASH_TABLE_NODE_PREVIOUSLY_OCCUPIED,\n");
        gen_gen("    HASH_TABLE_NODE_OCCUPIED,\n");
        gen_gen("} SYM_TBL_STATUS;\n");
    }

    gen_hash_table(sv("uast"), sv("Uast"), 1, true, implementation);
    gen_hash_table(sv("tast"), sv("Tast"), 1, true, implementation);
    gen_hash_table(sv("ir"), sv("Ir"), 1, true, implementation);
    gen_hash_table(sv("function_decl_was_encountered"), sv("Uast_function_decl"), 1, false, implementation);

    if (implementation) {
        //gen_gen("#define Hash_table_stable_uast hash_table");
    } else {
        // TODO: this symbol_collection system is suboptional (come up with a better system):
        //   - Expand_again is only used in one pass, but is stored everywhere
        //   - Hash_table_stable_tast and Hash_table_stable_ir are stored even in Uast
        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_stable_uast usymbol_table;\n");
        gen_gen("    Hash_table_stable_tast symbol_table;\n");
        gen_gen("    Hash_table_stable_ir ir_table;\n");
        //gen_gen("    Hash_table_stable_expand_again expand_again_table;\n");
        gen_gen("} Symbol_collection;\n");
    }

    if (implementation) {
        gen_gen("#endif // HASH_TABLES_H\n");
    } else {
        gen_gen("#endif // HASH_TABLE_STRUCTS_H\n");
    }

    close_file(global_output);
}

#endif // AUTO_GEN_HASH_TABLES_H

