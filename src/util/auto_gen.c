#include <auto_gen_vecs.h>
#include <auto_gen_util.h>
#include <auto_gen_tast.h>
#include <auto_gen_uast.h>
#include <auto_gen_llvm.h>

// TODO: expected failure test for 
// type struct {
//

// TODO: expected failure test for 
// type Token {
//

// TODO: remove dummy members

// TODO: test for
//struct Token {
//    num i32
//}
//
//fn do_thing(num i32) {
//}

// TODO: test for
//struct Token {
//    num i32
//}
//
//fn do_thing(token Token) {
//    let num i32 = deref(token).num
//}

// TODO: expected failure test with more than one non-variadic parameter in function decl or def
// TODO: tast_block_new, etc. should have arguments for every item
// TODO: actually use newline to end statement depending on last token of line of line
// TODO: expected failure case for invalid type in extern "c" function
// TODO: char literal with escape
// TODO: lang_type subtypes for number, etc.
//  lang_type_number should also have subtypes for signed, unsigned, etc.
//
//

static void gen_symbol_table_c_file_symbol_update(String* text, Symbol_tbl_type type) {
    string_extend_cstr(&gen_a, text, "void ");
    extend_strv_lower(text, type.normal_prefix);
    string_extend_cstr(&gen_a, text, "_update(Env* env, ");
    extend_strv_first_upper(text, type.type_name);
    string_extend_cstr(&gen_a, text, "* tast_of_symbol) {\n");
    string_extend_cstr(&gen_a, text, "    if (");
    extend_strv_lower(text, type.normal_prefix);
    string_extend_cstr(&gen_a, text, "_add(env, ");
    string_extend_cstr(&gen_a, text, "tast_of_symbol)) {\n");
    string_extend_cstr(&gen_a, text, "        return;\n");
    string_extend_cstr(&gen_a, text, "    }\n");
    string_extend_cstr(&gen_a, text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, text, "        unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, text, "    }\n");
    string_extend_cstr(&gen_a, text, "\n");
    string_extend_cstr(&gen_a, text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, text, "        Symbol_collection* sym_coll = vec_at(&env->ancesters, idx);\n");
    string_extend_cstr(&gen_a, text, "        ");
    extend_strv_first_upper(text, type.normal_prefix);
    string_extend_cstr(&gen_a, text, "_table_tast* curr_tast = NULL;\n");
    string_extend_cstr(&gen_a, text, "        if (");
    extend_strv_lower(text, type.internal_prefix);
    string_extend_cstr(&gen_a, text, "_tbl_lookup_internal(&curr_tast, ");
    string_extend_cstr(&gen_a, text, "&sym_coll->");
    extend_strv_lower(text, type.symbol_table_name);
    string_extend_cstr(&gen_a, text, ", ");
    extend_strv_lower(text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, text, "(tast_of_symbol))) {\n");
    string_extend_cstr(&gen_a, text, "            curr_tast->tast = tast_of_symbol;\n");
    string_extend_cstr(&gen_a, text, "            return;\n");
    string_extend_cstr(&gen_a, text, "        }\n");
    string_extend_cstr(&gen_a, text, "\n");
    string_extend_cstr(&gen_a, text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, text, "            unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, text, "        }\n");
    string_extend_cstr(&gen_a, text, "    }\n");
    string_extend_cstr(&gen_a, text, "}\n");
    string_extend_cstr(&gen_a, text, "\n");
}

static void gen_symbol_table_c_file_internal(Symbol_tbl_type type) {
    String text = {0};

    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_extend_table_internal(String* buf, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth) {\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tast = &sym_table.table_tasts[idx];\n");
    string_extend_cstr(&gen_a, &text, "        if (sym_tast->status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            string_extend_strv(&print_arena, buf, ");
    extend_strv_lower(&text, type.print_fn);
    string_extend_cstr(&gen_a, &text, "_internal(sym_tast->tast, recursion_depth));\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");

    // TODO: callback, Str_view or other solution for getting tast_name?
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(int log_level, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth, const char* file_path, int line) {\n");
    string_extend_cstr(&gen_a, &text, "    String padding = {0};\n");
    string_extend_cstr(&gen_a, &text, "    int indent_amt = 2*(recursion_depth + 4);\n");
    string_extend_cstr(&gen_a, &text, "    vec_reserve(&print_arena, &padding, indent_amt);\n");
    string_extend_cstr(&gen_a, &text, "    for (int idx = 0; idx < indent_amt; idx++) {\n");
    string_extend_cstr(&gen_a, &text, "        vec_append(&print_arena, &padding, ' ');\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = 0; idx < sym_table.capacity; idx++) {\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tast = &sym_table.table_tasts[idx];\n");
    string_extend_cstr(&gen_a, &text, "        if (sym_tast->status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            log_file_new(log_level, file_path, line, STRING_FMT TAST_FMT\"\\n\", string_print(padding), ");
    extend_strv_lower(&text, type.print_fn);
    string_extend_cstr(&gen_a, &text, "((sym_tast->tast)));\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");

    string_extend_cstr(&gen_a, &text, "// returns false if symbol is already added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tbl_tasts, size_t capacity, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    assert(tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "    Str_view symbol_name = ");
    string_extend_strv(&gen_a, &text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, &text, "(tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "    assert(symbol_name.count > 0 && \"invalid tast_of_symbol\");\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    assert(capacity > 0);\n");
    string_extend_cstr(&gen_a, &text, "    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);\n");
    string_extend_cstr(&gen_a, &text, "    size_t init_table_idx = curr_table_idx; \n");
    string_extend_cstr(&gen_a, &text, "    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "        if (str_view_is_equal(");
    string_extend_strv(&gen_a, &text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, &text, "(sym_tbl_tasts[curr_table_idx].tast), symbol_name)) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "        curr_table_idx = (curr_table_idx + 1) % capacity;\n");
    string_extend_cstr(&gen_a, &text, "        assert(init_table_idx != curr_table_idx && \"hash table is full here, and it should not be\");\n");
    string_extend_cstr(&gen_a, &text, "        (void) init_table_idx;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast tast = {.key = symbol_name, .tast = tast_of_symbol, .status = SYM_TBL_OCCUPIED};\n");
    string_extend_cstr(&gen_a, &text, "    sym_tbl_tasts[curr_table_idx] = tast;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_cpy(\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* dest,\n");
    string_extend_cstr(&gen_a, &text, "    const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* src,\n");
    string_extend_cstr(&gen_a, &text, "    size_t capacity,\n");
    string_extend_cstr(&gen_a, &text, "    size_t count_tasts_to_cpy\n");
    string_extend_cstr(&gen_a, &text, ") {\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {\n");
    string_extend_cstr(&gen_a, &text, "        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {\n");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(dest, capacity, src[bucket_src].tast);\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_expand_if_nessessary(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table) {\n");
    string_extend_cstr(&gen_a, &text, "    size_t old_capacity_tast_count = sym_table->capacity;\n");
    string_extend_cstr(&gen_a, &text, "    size_t minimum_count_to_reserve = 1;\n");
    string_extend_cstr(&gen_a, &text, "    size_t new_count = sym_table->count + minimum_count_to_reserve;\n");
    string_extend_cstr(&gen_a, &text, "    size_t tast_size = sizeof(sym_table->table_tasts[0]);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    bool should_move_elements = false;\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* new_table_tasts;\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    if (sym_table->capacity < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;\n");
    string_extend_cstr(&gen_a, &text, "        should_move_elements = true;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->capacity *= 2;\n");
    string_extend_cstr(&gen_a, &text, "        should_move_elements = true;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    if (should_move_elements) {\n");
    string_extend_cstr(&gen_a, &text, "        new_table_tasts = arena_alloc(&a_main, sym_table->capacity*tast_size);\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_cpy(new_table_tasts, sym_table->table_tasts, sym_table->capacity, old_capacity_tast_count);\n");
    string_extend_cstr(&gen_a, &text, "        sym_table->table_tasts = new_table_tasts;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// return false if symbol is not found\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key) {\n");
    string_extend_cstr(&gen_a, &text, "    if (sym_table->capacity < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);\n");
    string_extend_cstr(&gen_a, &text, "    size_t init_table_idx = curr_table_idx; \n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    while (1) {\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* curr_tast = &sym_table->table_tasts[curr_table_idx];\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_tast->status == SYM_TBL_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            if (str_view_is_equal(curr_tast->key, key)) {\n");
    string_extend_cstr(&gen_a, &text, "                *result = curr_tast;\n");
    string_extend_cstr(&gen_a, &text, "                return true;\n");
    string_extend_cstr(&gen_a, &text, "            }\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        curr_table_idx = (curr_table_idx + 1) % sym_table->capacity;\n");
    string_extend_cstr(&gen_a, &text, "        if (curr_table_idx == init_table_idx) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    unreachable(\"\");\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol has already been added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* ");
    string_extend_cstr(&gen_a, &text, "sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_expand_if_nessessary(sym_table);\n");
    string_extend_cstr(&gen_a, &text, "    assert(sym_table->capacity > 0);\n");
    string_extend_cstr(&gen_a, &text, "    if (!");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(sym_table->table_tasts, sym_table->capacity, tast_of_symbol)) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* dummy;\n");
    string_extend_cstr(&gen_a, &text, "    (void) dummy;\n");
    string_extend_cstr(&gen_a, &text, "    assert(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(&dummy, sym_table, ");
    extend_strv_lower(&text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, &text, "(tast_of_symbol)));\n");
    string_extend_cstr(&gen_a, &text, "    sym_table->count++;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_update(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tast;\n");
    string_extend_cstr(&gen_a, &text, "    if (");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(&sym_tast, sym_table, ");
    extend_strv_lower(&text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, &text, "(tast_of_symbol))) {\n");
    string_extend_cstr(&gen_a, &text, "        sym_tast->tast = tast_of_symbol;\n");
    string_extend_cstr(&gen_a, &text, "        return;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    try(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(sym_table, tast_of_symbol));\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(int log_level, const Env* env, const char* file_path, int line) {\n");
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(log_level, curr_tast->");
    extend_strv_lower(&text, type.symbol_table_name);
    string_extend_cstr(&gen_a, &text, ", 4, file_path, line);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            return;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const Env* env, Str_view key) {\n");
    if (type.do_primitives) {
        string_extend_cstr(&gen_a, &text, "    if (");
        extend_strv_lower(&text, type.internal_prefix);
        string_extend_cstr(&gen_a, &text, "_tbl_lookup(result, &env->primitives, key)) {\n");
        string_extend_cstr(&gen_a, &text, "        return true;\n");
        string_extend_cstr(&gen_a, &text, "    }\n");
        string_extend_cstr(&gen_a, &text, "\n");
    }
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        const Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);\n");
    string_extend_cstr(&gen_a, &text, "        if (");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(result, &curr_tast->");
    extend_strv_lower(&text, type.symbol_table_name);
    string_extend_cstr(&gen_a, &text, ", key)) {\n");
    string_extend_cstr(&gen_a, &text, "             return true;\n");
    string_extend_cstr(&gen_a, &text, "         }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            return false;\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");

    // symbol_add
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_add(Env* env, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* dummy;\n");
    string_extend_cstr(&gen_a, &text, "    if (");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(&dummy, env, ");
    extend_strv_lower(&text, type.get_key_fn_name);
    string_extend_cstr(&gen_a, &text, "(tast_of_symbol))) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    if (env->ancesters.info.count < 1) {\n");
    string_extend_cstr(&gen_a, &text, "        unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "    for (size_t idx = env->ancesters.info.count - 1;; idx--) {\n");
    string_extend_cstr(&gen_a, &text, "        Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);\n");
    string_extend_cstr(&gen_a, &text, "        try(");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(&curr_tast->");
    extend_strv_lower(&text, type.symbol_table_name);
    string_extend_cstr(&gen_a, &text, ", tast_of_symbol));\n");
    string_extend_cstr(&gen_a, &text, "            return true;\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "        if (idx < 1) {\n");
    string_extend_cstr(&gen_a, &text, "            unreachable(\"no block ancester found\");\n");
    string_extend_cstr(&gen_a, &text, "        }\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");

    gen_symbol_table_c_file_symbol_update(&text, type);


    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_c_file(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "#define STB_DS_IMPLEMENTATION");
    gen_gen("%s\n", "#include <stb_ds.h>");
    gen_gen("%s\n", "#include \"symbol_table.h\"");
    gen_gen("%s\n", "#include <uast_utils.h>");
    gen_gen("%s\n", "#include <tast_utils.h>");
    gen_gen("%s\n", "#include <llvm_utils.h>");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#define SYM_TBL_DEFAULT_CAPACITY 1");
    gen_gen("%s\n", "#define SYM_TBL_MAX_DENSITY (0.6f)");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {");
    gen_gen("%s\n", "    assert(capacity > 0);");
    gen_gen("%s\n", "    return stbds_hash_bytes(key.str, key.count, 0)%capacity;");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    
    gen_gen("%s\n", "bool alloca_do_add_defered(Llvm** redefined_sym, Env* env) {\n");
    gen_gen("%s\n", "    for (size_t idx = 0; idx < env->defered_allocas_to_add.info.count; idx++) {\n");
    gen_gen("%s\n", "        if (!alloca_add(env, vec_at(&env->defered_allocas_to_add, idx))) {\n");
    gen_gen("%s\n", "            *redefined_sym = vec_at(&env->defered_allocas_to_add, idx);\n");
    gen_gen("%s\n", "            vec_reset(&env->defered_allocas_to_add);\n");
    gen_gen("%s\n", "            return false;\n");
    gen_gen("%s\n", "        }\n");
    gen_gen("%s\n", "    }\n");
    gen_gen("%s\n", "\n");
    gen_gen("%s\n", "    vec_reset(&env->defered_allocas_to_add);\n");
    gen_gen("%s\n", "    return true;\n");
    gen_gen("%s\n", "}\n");

    gen_gen("%s\n", "bool symbol_do_add_defered(Tast_def** redefined_sym, Env* env) {\n");
    gen_gen("%s\n", "    for (size_t idx = 0; idx < env->defered_symbols_to_add.info.count; idx++) {\n");
    gen_gen("%s\n", "        if (!symbol_add(env, vec_at(&env->defered_symbols_to_add, idx))) {\n");
    gen_gen("%s\n", "            *redefined_sym = vec_at(&env->defered_symbols_to_add, idx);\n");
    gen_gen("%s\n", "            vec_reset(&env->defered_symbols_to_add);\n");
    gen_gen("%s\n", "            return false;\n");
    gen_gen("%s\n", "        }\n");
    gen_gen("%s\n", "    }\n");
    gen_gen("%s\n", "\n");
    gen_gen("%s\n", "    vec_reset(&env->defered_symbols_to_add);\n");
    gen_gen("%s\n", "    return true;\n");
    gen_gen("%s\n", "}\n");
    gen_gen("%s\n", "\n");
   
    gen_gen("%s\n", "bool usymbol_do_add_defered(Uast_def** redefined_sym, Env* env) {\n");
    gen_gen("%s\n", "    for (size_t idx = 0; idx < env->udefered_symbols_to_add.info.count; idx++) {\n");
    gen_gen("%s\n", "        if (!usymbol_add(env, vec_at(&env->udefered_symbols_to_add, idx))) {\n");
    gen_gen("%s\n", "            *redefined_sym = vec_at(&env->udefered_symbols_to_add, idx);\n");
    gen_gen("%s\n", "            vec_reset(&env->udefered_symbols_to_add);\n");
    gen_gen("%s\n", "            return false;\n");
    gen_gen("%s\n", "        }\n");
    gen_gen("%s\n", "    }\n");
    gen_gen("%s\n", "\n");
    gen_gen("%s\n", "    vec_reset(&env->udefered_symbols_to_add);\n");
    gen_gen("%s\n", "    return true;\n");
    gen_gen("%s\n", "}\n");
    gen_gen("%s\n", "\n");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_c_file_internal(vec_at(&types, idx));
    }

    CLOSE_FILE(global_output);
}

static void gen_symbol_table_header_internal(Symbol_tbl_type type) {
    String text = {0};

    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_extend_table_internal(String* buf, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth);\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(int log_level, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table sym_table, int recursion_depth, const char* file_path, int line);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "#define ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table(log_level, sym_table) \\\n");
    string_extend_cstr(&gen_a, &text, "    do { \\\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_table_internal(log_level, sym_table, 0, __FILE__, __LINE__); \\\n");
    string_extend_cstr(&gen_a, &text, "    } while(0)\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol is already added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tbl_tasts, size_t capacity, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "static inline bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, Str_view key) {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* sym_tast;\n");
    string_extend_cstr(&gen_a, &text, "    if (!");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_lookup_internal(&sym_tast, sym_table, key)) {\n");
    string_extend_cstr(&gen_a, &text, "        return false;\n");
    string_extend_cstr(&gen_a, &text, "    }\n");
    string_extend_cstr(&gen_a, &text, "    *result = sym_tast->tast;\n");
    string_extend_cstr(&gen_a, &text, "    return true;\n");
    string_extend_cstr(&gen_a, &text, "}\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "// returns false if symbol has already been added to the table\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_add(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.internal_prefix);
    string_extend_cstr(&gen_a, &text, "_tbl_update(");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table* sym_table, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(int log_level, const Env* env, const char* file_path, int line);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "#define ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log(log_level, env) \\\n");
    string_extend_cstr(&gen_a, &text, "    do { \\\n");
    string_extend_cstr(&gen_a, &text, "        ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_log_internal(log_level, env, __FILE__, __LINE__); \\\n");
    string_extend_cstr(&gen_a, &text, "    } while(0)\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_lookup(");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "** result, const Env* env, Str_view key);\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "bool ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_add(Env* env, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol);\n");
    string_extend_cstr(&gen_a, &text, "\n");

    string_extend_cstr(&gen_a, &text, "void ");
    extend_strv_lower(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_update(Env* env, ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast_of_symbol);\n");

    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_header(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "/* autogenerated */");
    gen_gen("%s\n", "#ifndef SYMBOL_TABLE_H");
    gen_gen("%s\n", "#define SYMBOL_TABLE_H");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#include \"str_view.h\"");
    gen_gen("%s\n", "#include \"string.h\"");
    gen_gen("%s\n", "#include \"env.h\"");
    gen_gen("%s\n", "#include \"symbol_table_struct.h\"");
    gen_gen("%s\n", "#include \"do_passes.h\"");
    gen_gen("%s\n", "#include <uast_forward_decl.h>");
    gen_gen("%s\n", "#include <tast_forward_decl.h>");
    gen_gen("%s\n", "#include <llvm_forward_decl.h>");
    gen_gen("%s\n", "");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_header_internal(vec_at(&types, idx));
    }

    gen_gen("%s\n", "// these tasts will be actually added to a symbol table when `symbol_do_add_defered` is called");
    gen_gen("%s\n", "static inline void alloca_add_defer(Env* env, Llvm* tast_of_alloca) {");
    gen_gen("%s\n", "    assert(tast_of_alloca);");
    gen_gen("%s\n", "    vec_append(&a_main, &env->defered_allocas_to_add, tast_of_alloca);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "bool alloca_do_add_defered(Llvm** redefined_sym, Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static inline void alloca_ignore_defered(Env* env) {");
    gen_gen("%s\n", "    vec_reset(&env->defered_allocas_to_add);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");

    gen_gen("%s\n", "// these tasts will be actually added to a symbol table when `symbol_do_add_defered` is called");
    gen_gen("%s\n", "static inline void symbol_add_defer(Env* env, Tast_def* tast_of_symbol) {");
    gen_gen("%s\n", "    assert(tast_of_symbol);");
    gen_gen("%s\n", "    vec_append(&a_main, &env->defered_symbols_to_add, tast_of_symbol);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "bool symbol_do_add_defered(Tast_def** redefined_sym, Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static inline void symbol_ignore_defered(Env* env) {");
    gen_gen("%s\n", "    vec_reset(&env->defered_symbols_to_add);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");

    gen_gen("%s\n", "// these tasts will be actually added to a symbol table when `symbol_do_add_defered` is called");
    gen_gen("%s\n", "static inline void usymbol_add_defer(Env* env, Uast_def* uast_of_symbol) {");
    gen_gen("%s\n", "    assert(uast_of_symbol);");
    gen_gen("%s\n", "    vec_append(&a_main, &env->udefered_symbols_to_add, uast_of_symbol);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "bool usymbol_do_add_defered(Uast_def** redefined_sym, Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "static inline void usymbol_ignore_defered(Env* env) {");
    gen_gen("%s\n", "    vec_reset(&env->udefered_symbols_to_add);");
    gen_gen("%s\n", "}");
    gen_gen("%s\n", "");

    gen_gen("%s\n", "Symbol_table* symbol_get_block(Env* env);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "void log_symbol_table_if_block(Env* env, const char* file_path, int line);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#define SYM_TBL_STATUS_FMT \"%s\"");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "const char* sym_tbl_status_print(SYM_TBL_STATUS status);");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "#endif // SYMBOL_TABLE_H");

    CLOSE_FILE(global_output);
}

static void gen_symbol_table_struct_internal(Symbol_tbl_type type) {
    String text = {0};

    string_extend_cstr(&gen_a, &text, "typedef struct {\n");
    string_extend_cstr(&gen_a, &text, "    Str_view key;\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.type_name);
    string_extend_cstr(&gen_a, &text, "* tast;\n");
    string_extend_cstr(&gen_a, &text, "    SYM_TBL_STATUS status;\n");
    string_extend_cstr(&gen_a, &text, "} ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast;\n");
    string_extend_cstr(&gen_a, &text, "\n");
    string_extend_cstr(&gen_a, &text, "typedef struct {\n");
    string_extend_cstr(&gen_a, &text, "    ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table_tast* table_tasts;\n");
    string_extend_cstr(&gen_a, &text, "    size_t count; // count elements in symbol_table\n");
    string_extend_cstr(&gen_a, &text, "    size_t capacity; // count buckets in symbol_table\n");
    string_extend_cstr(&gen_a, &text, "} ");
    extend_strv_first_upper(&text, type.normal_prefix);
    string_extend_cstr(&gen_a, &text, "_table;\n");
    string_extend_cstr(&gen_a, &text, "\n");

    gen_gen(STRING_FMT"\n", string_print(text));
}

static void gen_symbol_table_struct(const char* file_path, Sym_tbl_type_vec types) {
    global_output = fopen(file_path, "w");
    if (!global_output) {
        fprintf(stderr, "fatal error: could not open file %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    gen_gen("%s\n", "#ifndef SYMBOL_TABLE_STRUCT_H");
    gen_gen("%s\n", "#define SYMBOL_TABLE_STRUCT_H");
    gen_gen("%s\n", "");
    gen_gen("%s\n", "typedef enum {");
    gen_gen("%s\n", "    SYM_TBL_NEVER_OCCUPIED = 0,");
    gen_gen("%s\n", "    SYM_TBL_PREVIOUSLY_OCCUPIED,");
    gen_gen("%s\n", "    SYM_TBL_OCCUPIED,");
    gen_gen("%s\n", "} SYM_TBL_STATUS;");
    gen_gen("%s\n", "");

    gen_gen("%s\n", "#include <uast_forward_decl.h>");
    gen_gen("%s\n", "#include <tast_forward_decl.h>");
    gen_gen("%s\n", "#include <llvm_forward_decl.h>");

    gen_gen("%s\n", "static inline Str_view get_alloca_name(const Llvm_alloca* llvm);");
    gen_gen("%s\n", "static inline Str_view get_def_name(const Tast_def* def);");
    gen_gen("%s\n", "static inline Str_view get_uast_name_def(const Uast_def* def);");
    gen_gen("%s\n", "static inline Str_view llvm_get_tast_name(const Llvm* llvm);");

    for (size_t idx = 0; idx < types.info.count; idx++) {
        gen_symbol_table_struct_internal(vec_at(&types, idx));
    }

    gen_gen("typedef struct {\n");
    gen_gen("    Usymbol_table usymbol_table;\n");
    gen_gen("    Symbol_table symbol_table;\n");
    gen_gen("    Alloca_table alloca_table;\n");
    gen_gen("} Symbol_collection;\n");
    gen_gen("\n");

    gen_gen("%s\n", "#endif // SYMBOL_TABLE_STRUCT_H");

    CLOSE_FILE(global_output);
}

static Symbol_tbl_type symbol_tbl_type_new(
    const char* type_name,
    const char* normal_prefix,
    const char* internal_prefix,
    const char* get_key_fn_name,
    const char* symbol_table_name,
    const char* ancesters_type,
    const char* print_fn,
    bool do_primitives
) {
    return (Symbol_tbl_type) {
        .type_name = str_view_from_cstr(type_name),
        .normal_prefix = str_view_from_cstr(normal_prefix),
        .internal_prefix = str_view_from_cstr(internal_prefix),
        .get_key_fn_name = str_view_from_cstr(get_key_fn_name),
        .symbol_table_name = str_view_from_cstr(symbol_table_name),
        .ancesters_type = str_view_from_cstr(ancesters_type),
        .print_fn = str_view_from_cstr(print_fn),
        .do_primitives = do_primitives
    };
}

static Sym_tbl_type_vec get_symbol_tbl_types(void) {
    Sym_tbl_type_vec types = {0};

    vec_append(&gen_a, &types, symbol_tbl_type_new(
        "Uast_def", "usymbol", "usym", "get_uast_name_def", "usymbol_table", "uancesters", "uast_def_print", true
    ));
    vec_append(&gen_a, &types, symbol_tbl_type_new(
        "Tast_def", "symbol", "sym", "get_def_name", "symbol_table", "ancesters", "tast_def_print", false
    ));
    vec_append(&gen_a, &types, symbol_tbl_type_new( 
        "Llvm", "alloca", "all", "llvm_get_tast_name", "alloca_table", "llvm_ancesters", "llvm_print", false
    ));
    //vec_append(&gen_a, &types, symbol_tbl_type_new( 
    //    "Llvm", "Llvm", "ll", "llvm_get_tast_name", "llvm_table", "llvm_ancesters", false
    //));

    return types;
}

static const char* get_path(const char* build_dir, const char* file_name_in_dir) {
    String path = {0};

    string_extend_cstr(&gen_a, &path, build_dir);
    string_extend_cstr(&gen_a, &path, "/");
    string_extend_cstr(&gen_a, &path, file_name_in_dir);
    string_extend_cstr(&gen_a, &path, "\0");

    return path.buf;
}

int main(int argc, char** argv) {
    assert(argc == 2 && "invalid count of arguments provided");

    Sym_tbl_type_vec symbol_tbl_types = get_symbol_tbl_types();

    gen_all_tasts(get_path(argv[1], "tast_forward_decl.h"), false);
    assert(!global_output);
    gen_all_uasts(get_path(argv[1], "uast_forward_decl.h"), false);
    assert(!global_output);
    gen_all_llvms(get_path(argv[1], "llvm_forward_decl.h"), false);
    assert(!global_output);

    gen_all_vecs(get_path(argv[1], "vecs.h"));
    assert(!global_output);

    gen_symbol_table_struct(get_path(argv[1], "symbol_table_struct.h"), symbol_tbl_types);
    assert(!global_output);

    gen_all_tasts(get_path(argv[1], "tast.h"), true);
    assert(!global_output);

    gen_all_uasts(get_path(argv[1], "uast.h"), true);
    assert(!global_output);

    gen_all_llvms(get_path(argv[1], "llvm.h"), true);
    assert(!global_output);

    gen_symbol_table_header(get_path(argv[1], "symbol_table.h"), symbol_tbl_types);
    assert(!global_output);

    gen_symbol_table_c_file(get_path(argv[1], "symbol_table.c"), symbol_tbl_types);
    assert(!global_output);
}

