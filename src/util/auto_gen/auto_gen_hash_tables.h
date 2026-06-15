#ifndef AUTO_GEN_HASH_TABLES_H
#define AUTO_GEN_HASH_TABLES_H

#include <auto_gen_util.h>
#include <local_string.h>

static void gen_hash_table(
    Strv suffix,
    Strv base_type,
    int16_t pointer_depth,
    bool do_scoped_lookup,
    bool implementation
) {
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

        gen_gen("static bool hash_table_iter_node_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Hash_table_node_"FMT"** curr_node,\n", strv_print(suffix));
        gen_gen("    Hash_table_iter_node_"FMT"* iter\n", strv_print(suffix));
        gen_gen(");\n");
        gen_gen("static bool hash_table_iter_node_any_status_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Hash_table_node_"FMT"** curr_node,\n", strv_print(suffix));
        gen_gen("    Hash_table_iter_node_"FMT"* iter,\n", strv_print(suffix));
        gen_gen("    bool wrap_around\n");
        gen_gen(");\n");
        gen_gen("static bool hash_table_add_internal_"FMT"(Hash_table_"FMT"* hash_table, Strv key, "FMT" item);\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));

        gen_gen("static void hash_table_reserve_"FMT"(Hash_table_"FMT"* hash_table, size_t count_to_add) {\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    assert(count_to_add == 1 && \"TODO\");\n");
        gen_gen("    Hash_table_"FMT" old_hash_table = *hash_table;\n", strv_print(suffix));
        gen_gen("    bool resize_is_nessessary = false;\n");
        gen_gen("    while (hash_table->count + count_to_add >= (size_t)((uint64_t /*TODO*/)hash_table->capacity*70/100) /* TODO: do not hardcode percent */) {\n");
        gen_gen("        resize_is_nessessary = true;\n");
        gen_gen("        if (hash_table->capacity < 1) {\n");
        gen_gen("            hash_table->capacity = 1;\n");
        gen_gen("        } else {\n");
        gen_gen("            hash_table->capacity *= 2;\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("    if (resize_is_nessessary) {\n");
        gen_gen("        hash_table->count = 0;\n");
        gen_gen("        hash_table->nodes = arena_alloc(&a_leak/*TODO*/, sizeof(hash_table->nodes[0])*hash_table->capacity);\n");
        gen_gen("        Hash_table_node_"FMT"* curr_node = NULL;\n", strv_print(suffix));
        gen_gen("        Hash_table_iter_node_"FMT" iter = (Hash_table_iter_node_"FMT") {0};\n", strv_print(suffix), strv_print(suffix));
        gen_gen("        if (old_hash_table.capacity > 0) {\n");
        gen_gen("            while (hash_table_iter_node_"FMT"(&old_hash_table, &curr_node, &iter)) {\n", strv_print(suffix));
        gen_gen("                unwrap(hash_table_add_internal_"FMT"(hash_table, curr_node->key, curr_node->item));\n", strv_print(suffix));
        gen_gen("            }\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_iter_node_any_status_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Hash_table_node_"FMT"** curr_node,\n", strv_print(suffix));
        gen_gen("    Hash_table_iter_node_"FMT"* iter,\n", strv_print(suffix));
        gen_gen("    bool wrap_around\n");
        gen_gen(") {\n");
        gen_gen("    if (wrap_around || iter->index < hash_table->capacity) {\n");
        gen_gen("        if (wrap_around) {\n");
        gen_gen("            iter->index %%= hash_table->capacity;\n");
        gen_gen("        }\n");
        gen_gen("        size_t curr_idx = iter->index;\n");
        gen_gen("        iter->index++;\n");
        gen_gen("        *curr_node = &hash_table->nodes[curr_idx];\n");
        gen_gen("        return true;\n");
        gen_gen("    }\n");
        gen_gen("    return false;\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_iter_node_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Hash_table_node_"FMT"** curr_node,\n", strv_print(suffix));
        gen_gen("    Hash_table_iter_node_"FMT"* iter\n", strv_print(suffix));
        gen_gen(") {\n");
        gen_gen("    while (iter->index < hash_table->capacity) {\n");
        gen_gen("        size_t curr_idx = iter->index;\n");
        gen_gen("        iter->index++;\n");
        gen_gen("        if (hash_table->nodes[curr_idx].status == HASH_TABLE_NODE_OCCUPIED) {\n");
        gen_gen("            *curr_node = &hash_table->nodes[curr_idx];\n");
        gen_gen("            return true;\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("    return false;\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_iter_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Strv* curr_key,\n");
        gen_gen("    "FMT"* curr_item,\n", strv_print(type_with_ptr));
        gen_gen("    Hash_table_iter_node_"FMT"* iter\n", strv_print(suffix));
        gen_gen(") {\n");
        gen_gen("    Hash_table_node_"FMT"* curr_node = NULL;\n", strv_print(suffix));
        gen_gen("    if (!hash_table_iter_node_"FMT"(hash_table, &curr_node, iter)) {\n", strv_print(suffix));
        gen_gen("        return false;\n");
        gen_gen("    }\n");
        gen_gen("    *curr_key = curr_node->key;\n");
        gen_gen("    *curr_item = curr_node->item;\n");
        gen_gen("    return true;\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_add_internal_"FMT"(Hash_table_"FMT"* hash_table, Strv key, "FMT" item) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
        gen_gen("    Hash_table_iter_node_"FMT" iter = (Hash_table_iter_node_"FMT") {0};\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    iter.index = hash_table_calculate_idx(key, hash_table->capacity);\n");
        gen_gen("    Hash_table_node_"FMT"* curr_node = NULL;\n", strv_print(suffix));
        gen_gen("    size_t original_iter_idx = iter.index;\n");
        gen_gen("    while (1) {\n");
        gen_gen("        iter.index = iter.index%%hash_table->capacity;\n");
        gen_gen("        size_t curr_idx = iter.index;\n");
        gen_gen("        iter.index++;\n");
        gen_gen("        assert(iter.index != original_iter_idx && \"every hash node is occupied, which should not be the case\");\n");
        gen_gen("        assert(hash_table);\n");
        gen_gen("        assert(hash_table->nodes);\n");
        gen_gen("        curr_node = &hash_table->nodes[curr_idx];\n");
        gen_gen("        assert(curr_node);\n");
        gen_gen("        if (curr_node->status == HASH_TABLE_NODE_OCCUPIED) {\n");
        gen_gen("            if (strv_is_equal(curr_node->key, key)) {\n");
        gen_gen("                return false;\n");
        gen_gen("            }\n");
        gen_gen("        } else {");
        gen_gen("            curr_node->status = HASH_TABLE_NODE_OCCUPIED;\n");
        gen_gen("            curr_node->key = key;\n");
        gen_gen("            curr_node->item = item;\n");
        gen_gen("            hash_table->count++;\n");
        gen_gen("            return true;\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("    unreachable(\"above while loop should return\");\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_add_"FMT"(Hash_table_"FMT"* hash_table, Strv key, "FMT" item) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
        gen_gen("    hash_table_reserve_"FMT"(hash_table, 1);\n", strv_print(suffix));
        gen_gen("    return hash_table_add_internal_"FMT"(hash_table, key, item);\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_lookup_node_"FMT"(Hash_table_node_"FMT"** result, Hash_table_"FMT"* hash_table, Strv key) {\n", strv_print(suffix), strv_print(suffix), strv_print(suffix));
        gen_gen("    if (hash_table->capacity < 1) {\n");
        gen_gen("        return false;\n");
        gen_gen("    }\n");
        gen_gen("    Hash_table_iter_node_"FMT" iter = (Hash_table_iter_node_"FMT") {0};\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    iter.index = hash_table_calculate_idx(key, hash_table->capacity);\n");
        gen_gen("    Hash_table_node_"FMT"* curr_node = NULL;\n", strv_print(suffix));
        gen_gen("    size_t original_iter_idx = iter.index;\n");
        gen_gen("    while (hash_table_iter_node_any_status_"FMT"(hash_table, &curr_node, &iter, true)) {\n", strv_print(suffix));
        gen_gen("        if (iter.index == original_iter_idx) {\n");
        gen_gen("            assert(false && \"not implementation\");\n");
        gen_gen("        }\n");
        gen_gen("        switch (curr_node->status) {\n");
        gen_gen("            case HASH_TABLE_NODE_NEVER_OCCUPIED: {\n");
        gen_gen("                return false;\n");
        gen_gen("            }\n");
        gen_gen("            case HASH_TABLE_NODE_PREVIOUSLY_OCCUPIED: {\n");
        gen_gen("                break;\n");
        gen_gen("            }\n");
        gen_gen("            case HASH_TABLE_NODE_OCCUPIED: {\n");
        gen_gen("                if (strv_is_equal(curr_node->key, key)) {\n");
        gen_gen("                    *result = curr_node;\n");
        gen_gen("                    return true;\n");
        gen_gen("                }\n");
        gen_gen("                break;\n");
        gen_gen("            }\n");
        gen_gen("            default: {\n");
        gen_gen("               unreachable(\"\");\n");
        gen_gen("            }\n");
        gen_gen("        }\n");
        gen_gen("    }\n");
        gen_gen("    unreachable(\"\");\n");
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_lookup_"FMT"("FMT"* result, Hash_table_"FMT"* hash_table, Strv key) {\n", strv_print(suffix), strv_print(type_with_ptr), strv_print(suffix));
        gen_gen("    Hash_table_node_"FMT"* node = NULL;\n", strv_print(suffix));
        gen_gen("    if (!hash_table_lookup_node_"FMT"(&node, hash_table, key)) {\n", strv_print(suffix));
        gen_gen("        return false;\n");
        gen_gen("    }\n");
        gen_gen("    *result = node->item;\n");
        gen_gen("    return true;\n");
        gen_gen("}\n");

        gen_gen("static void hash_table_update_"FMT"(Hash_table_"FMT"* hash_table, Strv key, "FMT" item) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
        gen_gen("    Hash_table_node_"FMT"* node = NULL;\n", strv_print(suffix));
        gen_gen("    if (hash_table_lookup_node_"FMT"(&node, hash_table, key)) {\n", strv_print(suffix));
        gen_gen("        assert(strv_is_equal(node->key, key));\n");
        gen_gen("        assert(node->status == HASH_TABLE_NODE_OCCUPIED);\n");
        gen_gen("        node->item = item;\n");
        gen_gen("        return;\n");
        gen_gen("    }\n");
        gen_gen("    unwrap(hash_table_add_"FMT"(hash_table, key, item));\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_stable_lookup_"FMT"("FMT"* result, Hash_table_stable_"FMT"* hash_table, Strv key) {\n", strv_print(suffix), strv_print(type_with_ptr), strv_print(suffix));
        gen_gen("    return hash_table_lookup_"FMT"(result, &hash_table->hash_table, key);\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_stable_lookup_node_"FMT"(Hash_table_node_"FMT"** result, Hash_table_stable_"FMT"* hash_table, Strv key) {\n", strv_print(suffix), strv_print(suffix), strv_print(suffix));
        gen_gen("    return hash_table_lookup_node_"FMT"(result, &hash_table->hash_table, key);\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_stable_add_"FMT"(Hash_table_stable_"FMT"* hash_table_stable, Strv key, "FMT" item) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
        gen_gen("    if (!hash_table_add_"FMT"(&hash_table_stable->hash_table, key, item)) {\n", strv_print(suffix));
        gen_gen("        return false;\n");
        gen_gen("    }\n");
        gen_gen("    darr_append(&a_leak/*TODO*/, &hash_table_stable->keys, key);\n");
        gen_gen("    return true;\n");
        gen_gen("}\n\n");

        gen_gen("static void hash_table_stable_update_"FMT"(Hash_table_stable_"FMT"* hash_table, Strv key, "FMT" item) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
        gen_gen("    Hash_table_node_"FMT"* node = NULL;\n", strv_print(suffix));
        gen_gen("    if (hash_table_stable_lookup_node_"FMT"(&node, hash_table, key)) {\n", strv_print(suffix));
        gen_gen("        assert(strv_is_equal(node->key, key));\n");
        gen_gen("        assert(node->status == HASH_TABLE_NODE_OCCUPIED);\n");
        gen_gen("        node->item = item;\n");
        gen_gen("        return;\n");
        gen_gen("    }\n");
        gen_gen("    unwrap(hash_table_stable_add_"FMT"(hash_table, key, item));\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static bool hash_table_stable_iter_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_stable_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Strv* curr_key,\n");
        gen_gen("    "FMT"* curr_item,\n", strv_print(type_with_ptr));
        gen_gen("    Hash_table_iter_node_"FMT"* iter\n", strv_print(suffix));
        gen_gen(") {\n");
        gen_gen("    size_t curr_idx = iter->index;\n");
        gen_gen("    if (curr_idx >= hash_table->keys.info.count) {\n");
        gen_gen("        return false;\n");
        gen_gen("    }\n");
        gen_gen("    iter->index++;\n");
        gen_gen("    *curr_key = darr_at(hash_table->keys, curr_idx);\n");
        gen_gen("    unwrap(hash_table_stable_lookup_"FMT"(curr_item, hash_table, *curr_key));\n", strv_print(suffix));
        gen_gen("    return true;\n");
        gen_gen("}\n\n");

        gen_gen("static Strv hash_table_stable_print_internal_"FMT"(", strv_print(suffix));
        gen_gen("    Hash_table_stable_"FMT"* hash_table,\n", strv_print(suffix));
        gen_gen("    Strv(*item_print_internal_fn)(const "FMT" item, Indent indent)\n", strv_print(type_with_ptr));
        gen_gen(") {\n");
        gen_gen("    String buf = {0};\n");
        gen_gen("    Hash_table_iter_node_"FMT" iter = (Hash_table_iter_node_"FMT") {0};\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    "FMT" curr_item = {0};\n", strv_print(type_with_ptr));
        gen_gen("    Strv curr_key = {0};\n");
        gen_gen("    while (hash_table_stable_iter_"FMT"(hash_table, &curr_key, &curr_item, &iter)) {\n", strv_print(suffix));
        gen_gen("        string_extend_f(&a_temp, &buf, FMT\"\\n\", strv_print(item_print_internal_fn(curr_item, 0)));\n");
        gen_gen("    }\n");
        gen_gen("    return string_to_strv(buf);\n");
        gen_gen("}\n\n");
        gen_gen("#define hash_table_stable_print_"FMT"(hash_table, item_print_internal_fn) strv_print(hash_table_stable_print_internal_"FMT"(hash_table, item_print_internal_fn))\n\n", strv_print(suffix), strv_print(suffix));

        gen_gen("static Hash_table_iter_node_"FMT" hash_table_iter_new_"FMT"(\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    void\n");
        gen_gen(") {\n");
        gen_gen("    return (Hash_table_iter_node_"FMT") {.index = 0};\n", strv_print(suffix));
        gen_gen("}\n\n");

        gen_gen("static Hash_table_stable_iter_node_"FMT" hash_table_stable_iter_new_"FMT"(\n", strv_print(suffix), strv_print(suffix));
        gen_gen("    void\n");
        gen_gen(") {\n");
        gen_gen("    return (Hash_table_stable_iter_node_"FMT") {.index = 0};\n", strv_print(suffix));
        gen_gen("}\n\n");

        if (do_scoped_lookup) {
            gen_gen("static bool hash_table_scoped_lookup_node_"FMT"(\n", strv_print(suffix));
            gen_gen("    Hash_table_node_"FMT"** result,\n", strv_print(suffix));
            gen_gen("    Hash_table_stable_"FMT"_darr* collection,\n", strv_print(suffix));
            gen_gen("    Strv key,\n");
            gen_gen("    Scope_id scope_id\n");
            gen_gen(") {\n");
            //gen_gen("    if (scope_id >= collection->info.count) {\n");
            //gen_gen("        return false;\n");
            //gen_gen("    }\n");
            //gen_gen("\n");
            gen_gen("    Scope_id curr_scope = scope_id;\n");
            gen_gen("    while (1) {\n");
            gen_gen("        if (curr_scope < collection->info.count && hash_table_stable_lookup_node_"FMT"(result, darr_at_ref(collection, curr_scope), key)) {\n", strv_print(suffix));
            gen_gen("            return true;\n");
            gen_gen("        }\n");
            gen_gen("");
            gen_gen("        if (curr_scope == SCOPE_BUILTIN) {\n");
            gen_gen("            return false;\n");
            gen_gen("        }\n");
            gen_gen("        curr_scope = scope_get_parent_tbl_lookup(curr_scope);\n");
            gen_gen("    }\n");
            gen_gen("    unreachable(\"\");\n");
            gen_gen("}\n");

            gen_gen("static bool hash_table_scoped_lookup_"FMT"("FMT"* result, Hash_table_stable_"FMT"_darr* collection, Strv key, Scope_id scope_id) {\n", strv_print(suffix), strv_print(type_with_ptr), strv_print(suffix));
            //gen_gen("    if (scope_id >= collection->info.count) {\n");
            //gen_gen("        return false;\n");
            //gen_gen("    }\n");
            //gen_gen("\n");
            gen_gen("    Scope_id curr_scope = scope_id;\n");
            gen_gen("    if (curr_scope == SCOPE_NOT) {\n");
            gen_gen("        return false;\n");
            gen_gen("    }\n");
            gen_gen("    while (1) {\n");
            gen_gen("        assert(curr_scope != SCOPE_NOT);\n");
            gen_gen("        if (curr_scope < collection->info.count && hash_table_stable_lookup_"FMT"(result, darr_at_ref(collection, curr_scope), key)) {\n", strv_print(suffix));
            gen_gen("            return true;\n");
            gen_gen("        }\n");
            gen_gen("");
            gen_gen("        if (curr_scope == SCOPE_BUILTIN) {\n");
            gen_gen("            return false;\n");
            gen_gen("        }\n");
            gen_gen("        curr_scope = scope_get_parent_tbl_lookup(curr_scope);\n");
            gen_gen("    }\n");
            gen_gen("    unreachable(\"\");\n");
            gen_gen("}\n");

            gen_gen("static bool hash_table_scoped_add_"FMT"(Hash_table_stable_"FMT"_darr* collection, Strv key, "FMT" item, Scope_id scope_id) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
            gen_gen("    "FMT" dummy = {0};\n", strv_print(type_with_ptr));
            gen_gen("    if (hash_table_scoped_lookup_"FMT"(&dummy, collection, key, scope_id)) {\n", strv_print(suffix));
            gen_gen("        return false;\n");
            gen_gen("    }\n");
            gen_gen("    while (collection->info.count <= scope_id) {\n");
            gen_gen("        darr_append(&a_leak/*TODO*/, collection, (Hash_table_stable_"FMT") {0});\n", strv_print(suffix));
            gen_gen("    }\n");
            gen_gen("    return hash_table_stable_add_"FMT"(darr_at_ref(collection, scope_id), key, item);\n", strv_print(suffix));
            gen_gen("}\n");
            
            gen_gen("static void hash_table_scoped_update_"FMT"(Hash_table_stable_"FMT"_darr* collection, Strv key, "FMT" item, Scope_id scope_id) {\n", strv_print(suffix), strv_print(suffix), strv_print(type_with_ptr));
            gen_gen("    if (scope_id == SCOPE_NOT) {\n");
            gen_gen("        return; // TODO: returning for SCOPE_NOT seems like a hack\n");
            gen_gen("    }\n");
            gen_gen("\n");
            gen_gen("    Hash_table_node_"FMT"* node = NULL;\n", strv_print(suffix));
            gen_gen("    if (hash_table_scoped_lookup_node_"FMT"(&node, collection, key, scope_id)) {\n", strv_print(suffix));
            gen_gen("        assert(strv_is_equal(node->key, key));\n");
            gen_gen("        assert(node->status == HASH_TABLE_NODE_OCCUPIED);\n");
            gen_gen("        node->item = item;\n");
            gen_gen("        return;\n");
            gen_gen("    }\n");
            gen_gen("    unwrap(hash_table_scoped_add_"FMT"(collection, key, item, scope_id));\n", strv_print(suffix));
            gen_gen("}\n\n");

            gen_gen("static bool hash_table_scoped_iter_"FMT"(", strv_print(suffix));
            gen_gen("    Hash_table_stable_"FMT"_darr* collection,\n", strv_print(suffix));
            gen_gen("    Strv* curr_key,\n");
            gen_gen("    "FMT"* curr_item,\n", strv_print(type_with_ptr));
            gen_gen("    Hash_table_scoped_iter_node_"FMT"* iter\n", strv_print(suffix));
            gen_gen(") {\n");
            gen_gen("    return hash_table_stable_iter_"FMT"(darr_at_ref(collection, iter->scope_id), curr_key, curr_item, &iter->iter);\n", strv_print(suffix));
            gen_gen("}\n\n");

            gen_gen("static Hash_table_scoped_iter_node_"FMT" hash_table_scoped_iter_new_"FMT"(\n", strv_print(suffix), strv_print(suffix));
            gen_gen("    Scope_id scope_id\n");
            gen_gen(") {\n");
            gen_gen("    return (Hash_table_scoped_iter_node_"FMT") {.scope_id = scope_id, .iter = (Hash_table_iter_node_"FMT") {0}};\n", strv_print(suffix), strv_print(suffix));
            gen_gen("}\n\n");

            gen_gen("static Strv hash_table_scoped_print_internal_"FMT"(", strv_print(suffix));
            gen_gen("    Hash_table_scoped_"FMT"* hash_table\n", strv_print(suffix));
            gen_gen(") {\n");
            gen_gen("    todo();\n");
            gen_gen("}\n\n");
            gen_gen("#define hash_table_scoped_print_"FMT"(hash_table) strv_print(hash_table_scoped_print_internal_"FMT"(hash_table))\n\n", strv_print(suffix), strv_print(suffix));
        }
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
        gen_gen("    HASH_TABLE_STATUS status;\n");
        gen_gen("} Hash_table_node_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_node_"FMT"* nodes;\n", strv_print(suffix));
        gen_gen("    size_t count;\n");
        gen_gen("    size_t capacity;\n");
        gen_gen("} Hash_table_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_"FMT" hash_table;\n\n", strv_print(suffix));
        gen_gen("    Strv_darr keys;\n");
        gen_gen("} Hash_table_stable_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    size_t index;\n");
        gen_gen("} Hash_table_iter_node_"FMT";\n\n", strv_print(suffix));
        gen_gen("typedef Hash_table_iter_node_"FMT" Hash_table_iter_"FMT";\n", strv_print(suffix), strv_print(suffix));

        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_iter_node_"FMT" iter;\n", strv_print(suffix));
        gen_gen("    Scope_id scope_id;\n");
        gen_gen("} Hash_table_scoped_iter_node_"FMT";\n\n", strv_print(suffix));
        gen_gen("typedef Hash_table_scoped_iter_node_"FMT" Hash_table_scoped_iter_"FMT";\n", strv_print(suffix), strv_print(suffix));

        //gen_gen("#define Hash_table_stable_iter_"FMT" Hash_table_iter_node_"FMT"\n", strv_print(suffix), strv_print(suffix));
        //gen_gen("typedef struct {\n");
        //gen_gen("    size_t index;\n");
        //gen_gen("} Hash_table_stable_iter_"FMT";\n\n", strv_print(suffix));

        gen_gen("typedef Hash_table_iter_node_"FMT" Hash_table_stable_iter_node_"FMT";\n", strv_print(suffix), strv_print(suffix));
        //gen_gen("typedef Hash_table_stable_iter_node_"FMT" Hash_table_stable_iter_node_"FMT"");

        if (do_scoped_lookup) {
            gen_gen("typedef struct {\n");
            gen_gen("    Vec_base info;\n");
            gen_gen("    Hash_table_stable_"FMT"* buf;\n", strv_print(suffix));
            gen_gen("} Hash_table_stable_"FMT"_darr;\n\n", strv_print(suffix));

            gen_gen("typedef Hash_table_stable_"FMT"_darr Hash_table_scoped_"FMT";\n", strv_print(suffix), strv_print(suffix));
        }

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
    }

    if (implementation) {
        gen_gen("#include <darr.h>\n");
        gen_gen("#include <util.h>\n");
        gen_gen("WIMPLICIT_FALLTHROUGH_IGNORE_START\n");
        gen_gen("WSIGN_CONVERSION_IGNORE_START\n");
        gen_gen("//#define STB_DS_IMPLEMENTATION\n");
        gen_gen("#include <stb_ds.h>\n");
        gen_gen("WIMPLICIT_FALLTHROUGH_IGNORE_END\n");
        gen_gen("WSIGN_CONVERSION_IGNORE_END\n");
        gen_gen("\n");
        //gen_gen("#include <uast_utils.h>\n");
        //gen_gen("#include <tast_utils.h>\n");
        //gen_gen("#include <ir_utils.h>\n");
        //gen_gen("#include <symbol_log.h>\n");
        gen_gen("\n");
        gen_gen("#define SYM_TBL_DEFAULT_CAPACITY 1\n");
        gen_gen("#define SYM_TBL_MAX_DENSITY (0.6f) // TODO: change this to an integer\n");
        gen_gen("\n");
        gen_gen("//\n");
        gen_gen("// util\n");
        gen_gen("//\n");
        gen_gen("static size_t hash_table_calculate_idx(Strv key, size_t capacity) {\n");
        gen_gen("    assert(capacity > 0);\n");
        gen_gen("    return stbds_hash_bytes(key.str, key.count, 0)%%capacity;\n");
        gen_gen("}\n");
        gen_gen("//\n");

        gen_gen("typedef struct {\n");
        gen_gen("    Vec_base info;\n");
        gen_gen("    Scope_id* buf;\n");
        gen_gen("} Scope_id_darr;\n");

        gen_gen("extern Scope_id_darr scope_id_to_parent;\n");
        gen_gen("\n");
        gen_gen("// returns parent of key\n");
        gen_gen("static Scope_id scope_get_parent_tbl_lookup(Scope_id key) {\n");
        gen_gen("    assert(key != SCOPE_BUILTIN);\n");
        gen_gen("    return darr_at(scope_id_to_parent, key);\n");
        gen_gen("}\n");

        gen_gen("static void scope_get_parent_tbl_add(Scope_id key, Scope_id parent) {\n");
        gen_gen("    while (scope_id_to_parent.info.count <= key) {\n");
        gen_gen("        darr_append(&a_leak/*TODO*/, &scope_id_to_parent, 0);\n");
        gen_gen("    }\n");
        gen_gen("    *darr_at_ref(&scope_id_to_parent, key) = parent;\n");
        gen_gen("}\n");
    } else {
        gen_gen("typedef enum {\n");
        gen_gen("    HASH_TABLE_NODE_NEVER_OCCUPIED = 0,\n");
        gen_gen("    HASH_TABLE_NODE_PREVIOUSLY_OCCUPIED,\n");
        gen_gen("    HASH_TABLE_NODE_OCCUPIED,\n");
        gen_gen("} HASH_TABLE_STATUS;\n");

        // TODO: remove
        gen_gen("typedef enum {\n");
        gen_gen("    SYM_TBL_NEVER_OCCUPIED = 0,\n");
        gen_gen("    SYM_TBL_PREVIOUSLY_OCCUPIED,\n");
        gen_gen("    SYM_TBL_OCCUPIED,\n");
        gen_gen("} SYM_TBL_STATUS;\n");
    }

    gen_hash_table(sv("uast"), sv("Uast_def"), 1, true, implementation);
    gen_hash_table(sv("tast"), sv("Tast_def"), 1, true, implementation);
    gen_hash_table(sv("ir"), sv("Ir"), 1, true, implementation);
    gen_hash_table(sv("function_decl_was_encountered"), sv("Uast_function_decl"), 1, false, implementation);
    gen_hash_table(sv("int"), sv("int"), 0, true, implementation);
    gen_hash_table(sv("c_forward_struct"), sv("Name"), 1/*TODO: could be changed to 0?*/, true, implementation);
    gen_hash_table(sv("expand_again"), sv("Uast_def"), 1, true, implementation);
    gen_hash_table(sv("raw_union_of_enum"), sv("Tast_raw_union_def"), 1, false, implementation);
    gen_hash_table(sv("struct_to_struct"), sv("Tast_struct_def"), 1, false, implementation);
    gen_hash_table(sv("file_path_to_text"), sv("Strv"), 0, false, implementation);
    gen_hash_table(sv("function_decls"), sv("Uast_function_decl"), 1, false, implementation);

    if (implementation) {
        //gen_gen("#define Hash_table_stable_uast hash_table");
        
        gen_gen("extern Hash_table_stable_c_forward_struct c_forward_struct_tbl;\n");

        gen_gen("static bool c_forward_struct_tbl_lookup(Name** result, Name key) {\n");
        //gen_gen("    bool status = hash_table_scoped_lookup_ir(result, &symbol_tables.ir_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);\n");
        gen_gen("    return hash_table_stable_lookup_c_forward_struct(result, &c_forward_struct_tbl, serialize_name_symbol_table(&a_leak/*TODO*/, key));\n");
        gen_gen("}\n");

        gen_gen("static bool c_forward_struct_tbl_add(Name* struct_to_use, Name key) {\n");
        gen_gen("    return hash_table_stable_add_c_forward_struct(&c_forward_struct_tbl, serialize_name_symbol_table(&a_leak/*TODO*/, key), struct_to_use);\n");
        gen_gen("    todo();\n");
        gen_gen("}\n");


        gen_gen("extern Name_darr scope_to_name;\n");
        gen_gen("\n");
        gen_gen("static Name scope_to_name_tbl_lookup(Scope_id key) {\n");
        gen_gen("    return darr_at(scope_to_name, key);\n");
        gen_gen("}\n");
        gen_gen("\n");
        gen_gen("static void scope_to_name_tbl_add(Scope_id key, Name scope_name) {\n");
        gen_gen("    while (scope_to_name.info.count <= key) {\n");
        gen_gen("        darr_append(&a_main, &scope_to_name, (Name) {0});\n");
        gen_gen("    }\n");
        gen_gen("    *darr_at_ref(&scope_to_name, key) = scope_name;\n");
        gen_gen("}\n");
        gen_gen("\n");
        gen_gen("static void scope_to_name_tbl_update(Scope_id key, Name scope_name) {\n");
        gen_gen("    *darr_at_ref(&scope_to_name, key) = scope_name;\n");
        gen_gen("}\n");
        gen_gen("\n");

        gen_gen("#define Usymbol_iter Hash_table_scoped_iter_node_uast");
        gen_gen("#define Symbol_iter Hash_table_scoped_iter_node_tast");
        gen_gen("#define Ir_iter Hash_table_scoped_iter_node_ir");

        gen_gen("static inline Usymbol_iter usym_tbl_iter_new(Scope_id scope_id) {\n");
        gen_gen("    return hash_table_scoped_iter_new_uast(scope_id);\n");
        gen_gen("}\n");

        gen_gen("static inline Symbol_iter sym_tbl_iter_new(Scope_id scope_id) {\n");
        gen_gen("    return hash_table_scoped_iter_new_tast(scope_id);\n");
        gen_gen("}\n");

        gen_gen("static inline Ir_iter ir_tbl_iter_new(Scope_id scope_id) {\n");
        gen_gen("    return hash_table_scoped_iter_new_ir(scope_id);\n");
        gen_gen("}\n");

        gen_gen("static bool usym_tbl_iter_next(Uast_def** curr_def, Usymbol_iter* iter) {\n");
        gen_gen("    Strv curr_key = (Strv) {0};\n");
        gen_gen("    return hash_table_scoped_iter_uast(&symbol_tables.usymbol_table, &curr_key, curr_def, iter);\n");
        gen_gen("}\n");

        gen_gen("static bool sym_tbl_iter_next(Tast_def** curr_def, Symbol_iter* iter) {\n");
        gen_gen("    Strv curr_key = (Strv) {0};\n");
        gen_gen("    return hash_table_scoped_iter_tast(&symbol_tables.symbol_table, &curr_key, curr_def, iter);\n");
        gen_gen("}\n");

        gen_gen("static bool ir_tbl_iter_next(Ir** curr_def, Ir_iter* iter) {\n");
        gen_gen("    Strv curr_key = (Strv) {0};\n");
        gen_gen("    return hash_table_scoped_iter_ir(&symbol_tables.ir_table, &curr_key, curr_def, iter);\n");
        gen_gen("}\n");

        gen_gen("static bool symbol_lookup(Tast_def** result, Name key) {\n");
        gen_gen("    return hash_table_scoped_lookup_tast(result, &symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static bool ir_lookup(Ir** result, Name key) {\n");
        gen_gen("    return hash_table_scoped_lookup_ir(result, &symbol_tables.ir_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);\n");
        gen_gen("}\n");

        //gen_gen("static bool usymbol_add(Uast_def** result, Name key) {\n");
        //gen_gen("    return hash_table_scoped_add_uast(result, &symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);\n");
        //gen_gen("}\n");

        //gen_gen("static bool symbol_add(Tast_def** result, Name key) {\n");
        //gen_gen("    return hash_table_scoped_add_tast(result, &symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);\n");
        //gen_gen("}\n");

        gen_gen("Name ir_get_name(LANG_TYPE_MODE mode, const Ir* ir);\n");

        gen_gen("static bool ir_add(Ir* def_to_add) {\n");
        gen_gen("    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_add);\n");
        gen_gen("    return hash_table_scoped_add_ir(&symbol_tables.ir_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static inline Name uast_def_get_name(const Uast_def* def);\n");

        gen_gen("static bool usymbol_add(Uast_def* def_to_add) {\n");
        gen_gen("    Name key = uast_def_get_name(def_to_add);\n");
        gen_gen("    return hash_table_scoped_add_uast(&symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static void usymbol_update(Uast_def* def_to_update) {\n");
        gen_gen("    Name key = uast_def_get_name(def_to_update);\n");
        gen_gen("    hash_table_scoped_update_uast(&symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update, key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static inline Name tast_def_get_name(const Tast_def* def);\n");

        gen_gen("static bool symbol_add(Tast_def* def_to_add) {\n");
        gen_gen("    Name key = tast_def_get_name(def_to_add);\n");
        gen_gen("    return hash_table_scoped_add_tast(&symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static void symbol_update(Tast_def* def_to_add) {\n");
        gen_gen("    Name key = tast_def_get_name(def_to_add);\n");
        gen_gen("    hash_table_scoped_update_tast(&symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);\n");
        gen_gen("}\n");

        gen_gen("static bool usym_tbl_add(Uast_def* def_to_add) {\n");
        gen_gen("    Name key = uast_def_get_name(def_to_add);\n");
        gen_gen("    return hash_table_stable_add_uast(darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);\n");
        gen_gen("}\n");

        gen_gen("static bool sym_tbl_add(Tast_def* def_to_add) {\n");
        gen_gen("    Name key = tast_def_get_name(def_to_add);\n");
        gen_gen("    return hash_table_stable_add_tast(darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);\n");
        gen_gen("}\n");

        gen_gen("static bool ir_tbl_add(Ir* def_to_add) {\n");
        gen_gen("    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_add);\n");
        gen_gen("    return hash_table_stable_add_ir(darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);\n");
        gen_gen("}\n");

        gen_gen("static bool usym_tbl_lookup(Uast_def** result, Name key) {\n");
        gen_gen("    return hash_table_stable_lookup_uast(result, darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));\n");
        gen_gen("}\n");

        gen_gen("static bool sym_tbl_lookup(Tast_def** result, Name key) {\n");
        gen_gen("    return hash_table_stable_lookup_tast(result, darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));\n");
        gen_gen("}\n");

        gen_gen("static bool ir_tbl_lookup(Ir** result, Name key) {\n");
        gen_gen("    return hash_table_stable_lookup_ir(result, darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));\n");
        gen_gen("}\n");


        gen_gen("static void ir_tbl_update(Ir* def_to_update) {\n");
        gen_gen("    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_update);\n");
        gen_gen("    hash_table_stable_update_ir(darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);\n");
        gen_gen("}\n");

        gen_gen("static void usym_tbl_update(Uast_def* def_to_update) {\n");
        gen_gen("    Name key = uast_def_get_name(def_to_update);\n");
        gen_gen("    hash_table_stable_update_uast(darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);\n");
        gen_gen("}\n");

        gen_gen("static void sym_tbl_update(Tast_def* def_to_update) {\n");
        gen_gen("    Name key = tast_def_get_name(def_to_update);\n");
        gen_gen("    hash_table_stable_update_tast(darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);\n");
        gen_gen("}\n");

        //gen_gen("#define usym_tbl_iter_new ((Hash_table_stable_iter_uast) {0})");
    } else {
        // TODO: this symbol_collection system is suboptional (come up with a better system):
        //   - Expand_again is only used in one pass, but is stored everywhere
        //   - Hash_table_stable_tast and Hash_table_stable_ir are stored even in Uast
        gen_gen("typedef struct {\n");
        gen_gen("    Hash_table_stable_uast_darr usymbol_table;\n");
        gen_gen("    Hash_table_stable_tast_darr symbol_table;\n");
        gen_gen("    Hash_table_stable_ir_darr ir_table;\n");
        //gen_gen("    Hash_table_stable_expand_again expand_again_table;\n");
        gen_gen("} Symbol_collection;\n");

        gen_gen("extern Symbol_collection symbol_tables;\n");


        gen_gen("Scope_id symbol_collection_new(Scope_id parent, Name scope_name);\n");
    }

    if (implementation) {
        gen_gen("#endif // HASH_TABLES_H\n");
    } else {
        gen_gen("#endif // HASH_TABLE_STRUCTS_H\n");
    }

    close_file(global_output);
}

#endif // AUTO_GEN_HASH_TABLES_H

