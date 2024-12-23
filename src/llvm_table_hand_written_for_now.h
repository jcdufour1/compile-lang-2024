#ifndef LLVM_TABLE_HAND_WRITTEN_FOR_NOW
#define LLVM_TABLE_HAND_WRITTEN_FOR_NOW

#include <symbol_table.h>
#error "ksdjf"

struct Llvm_def_;
typedef struct Llvm_def_ Llvm_def;

struct Llvm_operator_;
typedef struct Llvm_operator_ Llvm_operator;

static inline Str_view llvm_get_node_name_def(const Llvm_def* def);

size_t sym_tbl_calculate_idx(Str_view key, size_t capacity);

typedef struct {
    Str_view key;
    Llvm_def* node;
    SYM_TBL_STATUS status;
} Llvm_table_node;

typedef struct {
    Llvm_table_node* table_nodes;
    size_t count; // count elements in symbol_table
    size_t capacity; // count buckets in symbol_table
} Llvm_table;

static void llvm_log_table_internal(int log_level, const Llvm_table sym_table, int recursion_depth, const char* file_path, int line) {
    String padding = {0};
    int indent_amt = 2*(recursion_depth + 4);
    vec_reserve(&print_arena, &padding, indent_amt);
    for (int idx = 0; idx < indent_amt; idx++) {
        vec_append(&print_arena, &padding, ' ');
    }

    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Llvm_table_node* sym_node = &sym_table.table_nodes[idx];
        if (sym_node->status == SYM_TBL_OCCUPIED) {
            log_file_new(log_level, file_path, line, STRING_FMT NODE_FMT"\n", string_print(padding), node_print((Node*)(sym_node->node)));
        }
    }
}

// returns false if symbol is already added to the table
static bool ll_tbl_add_internal(Llvm_table_node* sym_tbl_nodes, size_t capacity, Llvm_def* node_of_symbol) {
    assert(node_of_symbol);
    Str_view symbol_name = llvm_get_node_name_def(node_of_symbol);
    assert(symbol_name.count > 0 && "invalid node_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_nodes[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(llvm_get_node_name_def(sym_tbl_nodes[curr_table_idx].node), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Llvm_table_node node = {.key = symbol_name, .node = node_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_nodes[curr_table_idx] = node;
    return true;
}

static void ll_tbl_cpy(
    Llvm_table_node* dest,
    const Llvm_table_node* src,
    size_t capacity,
    size_t count_nodes_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_nodes_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
ll_tbl_add_internal(dest, capacity, src[bucket_src].node);
        }
    }
}

static void ll_tbl_expand_if_nessessary(Llvm_table* sym_table) {
    size_t old_capacity_node_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t node_size = sizeof(sym_table->table_nodes[0]);

    bool should_move_elements = false;
    Llvm_table_node* new_table_nodes;

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
        ll_tbl_cpy(new_table_nodes, sym_table->table_nodes, sym_table->capacity, old_capacity_node_count);
        sym_table->table_nodes = new_table_nodes;
    }
}

// return false if symbol is not found
static bool ll_tbl_lookup_internal(Llvm_table_node** result, const Llvm_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Llvm_table_node* curr_node = &sym_table->table_nodes[curr_table_idx];

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
static bool ll_tbl_add(Llvm_table* sym_table, Llvm* node_of_symbol) {
    ll_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!ll_tbl_add_internal(sym_table->table_nodes, sym_table->capacity, node_of_symbol)) {
        return false;
    }
    Llvm* dummy;
    (void) dummy;
    assert(ll_tbl_lookup(&dummy, sym_table, llvm_get_node_name(node_of_symbol)));
    sym_table->count++;
    return true;
}

static void ll_tbl_update(Llvm_table* sym_table, Llvm* node_of_symbol) {
    Llvm_table_node* sym_node;
    if (ll_tbl_lookup_internal(&sym_node, sym_table, llvm_get_node_name(node_of_symbol))) {
        sym_node->node = node_of_symbol;
        return;
    }
    try(ll_tbl_add(sym_table, node_of_symbol));
}

static void llvm_log_internal(int log_level, const Env* env, const char* file_path, int line) {
    if (env->ancesters.info.count < 1) {
        return;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Node* curr_node = vec_at(&env->ancesters, idx);
        if (curr_node->type == NODE_BLOCK) {
            log(LOG_DEBUG, NODE_FMT"\n", node_print(curr_node));
            llvm_log_table_internal(log_level, node_unwrap_block(curr_node)->llvm_table, 4, file_path, line);
        }

        if (idx < 1) {
            return;
        }
    }
}

static bool llvm_lookup(Llvm** result, const Env* env, Str_view key) {
    if (env->ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            const Node_block* block = node_unwrap_block_const(vec_at(&env->ancesters, idx));
            if (ll_tbl_lookup(result, &block->llvm_table, key)) {
                return true;
            }
        }

        if (idx < 1) {
            return false;
        }
    }
}

static bool llvm_add(Env* env, Llvm* node_of_symbol) {
    Llvm* dummy;
    if (llvm_lookup(&dummy, env, llvm_get_node_name(node_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            Node_block* block = node_unwrap_block(vec_at(&env->ancesters, idx));
            try(ll_tbl_add(&block->llvm_table, node_of_symbol));
            return true;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

static void llvm_update(Env* env, Llvm* node_of_symbol) {
    if (llvm_add(env, node_of_symbol)) {
        return;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        if (vec_at(&env->ancesters, idx)->type == NODE_BLOCK) {
            Node_block* block = node_unwrap_block(vec_at(&env->ancesters, idx));
            Llvm_table_node* curr_node = NULL;
            if (ll_tbl_lookup_internal(&curr_node, &block->llvm_table, llvm_get_node_name(node_of_symbol))) {
                curr_node->node = node_of_symbol;
                return;
            }
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

// these nodes will be actually added to a symbol table when `symbol_do_add_defered` is called
static inline void symbol_add_defer(Env* env, Node_def* node_of_symbol) {
    assert(node_of_symbol);
    vec_append(&a_main, &env->defered_symbols_to_add, node_of_symbol);
}

#endif // LLVM_TABLE_HAND_WRITTEN_FOR_NOW
