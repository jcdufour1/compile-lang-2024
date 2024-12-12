#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include "symbol_table.h"

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f)

static size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {
    assert(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

void symbol_log_table_internal(int log_level, const Symbol_table sym_table, int recursion_depth, const char* file_path, int line) {
    String padding = {0};
    int indent_amt = 2*(recursion_depth + 4);
    vec_reserve(&print_arena, &padding, indent_amt);
    for (int idx = 0; idx < indent_amt; idx++) {
        vec_append(&print_arena, &padding, ' ');
    }

    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Symbol_table_node* sym_node = &sym_table.table_nodes[idx];
        if (sym_node->status == SYM_TBL_OCCUPIED) {
            log_file_new(log_level, file_path, line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print(node_wrap_def_const(sym_node->node)));
        }
    }
}

// returns false if symbol is already added to the table
bool sym_tbl_add_internal(Symbol_table_node* sym_tbl_nodes, size_t capacity, Node_def* node_of_symbol) {
    assert(node_of_symbol);
    Str_view symbol_name = get_def_name(node_of_symbol);
    assert(symbol_name.count > 0 && "invalid node_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_nodes[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(get_def_name(sym_tbl_nodes[curr_table_idx].node), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Symbol_table_node node = {.key = symbol_name, .node = node_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_nodes[curr_table_idx] = node;
    return true;
}

static void sym_tbl_cpy(
    Symbol_table_node* dest,
    const Symbol_table_node* src,
    size_t capacity,
    size_t count_nodes_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_nodes_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
            sym_tbl_add_internal(dest, capacity, src[bucket_src].node);
        }
    }
}

static void sym_tbl_expand_if_nessessary(Symbol_table* sym_table) {
    size_t old_capacity_node_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t node_size = sizeof(sym_table->table_nodes[0]);

    bool should_move_elements = false;
    Symbol_table_node* new_table_nodes;

    if (sym_table->capacity < 1) {
        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {
        sym_table->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_nodes = arena_alloc(&a_main, sym_table->capacity*node_size);
        sym_tbl_cpy(new_table_nodes, sym_table->table_nodes, sym_table->capacity, old_capacity_node_count);
        if (sym_table->table_nodes) {
            // not freeing here currently
        }
        sym_table->table_nodes = new_table_nodes;
    }
}

// return false if symbol is not found
bool sym_tbl_lookup_internal(Symbol_table_node** result, const Symbol_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Symbol_table_node* curr_node = &sym_table->table_nodes[curr_table_idx];

        if (curr_node->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(curr_node->key, key)) {
                *result = curr_node;
                return true;
            }
        }

        if (curr_node->status == SYM_TBL_NEVER_OCCUPIED) {
            return false;
        }

        curr_table_idx = (curr_table_idx + 1) % sym_table->capacity;
        if (curr_table_idx == init_table_idx) {
            return false;
        }
    }

    unreachable("");
}

// returns false if symbol has already been added to the table
bool sym_tbl_add(Symbol_table* sym_table, Node_def* node_of_symbol) {
    sym_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!sym_tbl_add_internal(sym_table->table_nodes, sym_table->capacity, node_of_symbol)) {
        return false;
    }
    Node_def* dummy;
    (void) dummy;
    assert(sym_tbl_lookup(&dummy, sym_table, get_def_name(node_of_symbol)));
    sym_table->count++;
    return true;
}

void sym_tbl_update(Symbol_table* sym_table, Node_def* node_of_symbol) {
    Symbol_table_node* sym_node;
    if (sym_tbl_lookup_internal(&sym_node, sym_table, get_def_name(node_of_symbol))) {
        sym_node->node = node_of_symbol;
        return;
    }
    try(sym_tbl_add(sym_table, node_of_symbol));
}

const char* sym_tbl_status_print(SYM_TBL_STATUS status) {
    switch (status) {
        case SYM_TBL_NEVER_OCCUPIED:
            return "never occupied";
        case SYM_TBL_PREVIOUSLY_OCCUPIED:
            return "prev occupied";
        case SYM_TBL_OCCUPIED:
            return "currently occupied";
        default:
            unreachable("");
    }
}

void symbol_log_internal(int log_level, const Env* env, const char* file_path, int line) {
    if (env->ancesters.info.count < 1) {
        return;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&env->ancesters, idx);
        if (curr_node->type == NODE_BLOCK) {
            log_indent_file(log_level, file_path, line, 0, "at index: %zu (curr_node = %p)\n", idx, (void*)curr_node);
            log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
            symbol_log_table_internal(log_level, node_unwrap_block(curr_node)->symbol_table, 4, file_path, line);
        }

        if (idx < 1) {
            return;
        }
    }
}

bool symbol_lookup(Node_def** result, const Env* env, Str_view key) {
    if (sym_tbl_lookup(result, &env->primitives, key)) {
        return true;
    }

    //log(LOG_DEBUG, "entering symbol_lookup\n");
    if (env->ancesters.info.count < 1) {
        //log(LOG_DEBUG, "symbol_lookup thing 1\n");
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            const Node_block* block = node_unwrap_block_const(vec_at(&env->ancesters, idx));
            if (sym_tbl_lookup(result, &block->symbol_table, key)) {
                //log(LOG_DEBUG, "symbol_lookup thing true\n");
                return true;
            }
        }

        if (idx < 1) {
            //log(LOG_DEBUG, "symbol_lookup thing 3\n");
            return false;
        }
    }
}

bool symbol_add(Env* env, Node_def* node_of_symbol) {
    if (str_view_is_equal(str_view_from_cstr("str8"), get_def_name(node_of_symbol))) {
    }
    Node_def* dummy;
    if (symbol_lookup(&dummy, env, get_def_name(node_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            Node_block* block = node_unwrap_block(vec_at(&env->ancesters, idx));
            try(sym_tbl_add(&block->symbol_table, node_of_symbol));
            return true;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

bool symbol_do_add_defered(Node_def** redefined_sym, Env* env) {
    for (size_t idx = 0; idx < env->defered_symbols_to_add.info.count; idx++) {
        log(LOG_DEBUG, NODE_FMT"\n", node_print(node_wrap_def(vec_at(&env->defered_symbols_to_add, idx))));
        if (!symbol_add(env, vec_at(&env->defered_symbols_to_add, idx))) {
            *redefined_sym = vec_at(&env->defered_symbols_to_add, idx);
            vec_reset(&env->defered_symbols_to_add);
            return false;
        }
    }

    vec_reset(&env->defered_symbols_to_add);
    return true;
}

Symbol_table* symbol_get_block(Env* env) {
    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            return &node_unwrap_block(vec_at(&env->ancesters, idx))->symbol_table;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

void log_symbol_table_if_block(Env* env, const char* file_path, int line) {
    Node* curr_node = vec_top(&env->ancesters);
    if (curr_node->type != NODE_BLOCK)  {
        return;
    }

    Node_block* block = node_unwrap_block(curr_node);
    //log_indent_file(LOG_DEBUG, __FILE__, __LINE__, env->padding, "at recursion_depth: %d\n", env->recursion_depth);
    symbol_log_table_internal(LOG_DEBUG, block->symbol_table, env->recursion_depth, file_path, line);
}

