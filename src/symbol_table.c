#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include "symbol_table.h"
#include <uast_utils.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f)

static size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {
    assert(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

bool alloca_do_add_defered(Llvm** redefined_sym, Env* env) {

    for (size_t idx = 0; idx < env->defered_allocas_to_add.info.count; idx++) {

        if (!alloca_add(env, vec_at(&env->defered_allocas_to_add, idx))) {

            *redefined_sym = vec_at(&env->defered_allocas_to_add, idx);

            vec_reset(&env->defered_allocas_to_add);

            return false;

        }

    }



    vec_reset(&env->defered_allocas_to_add);

    return true;

}

bool symbol_do_add_defered(Tast_def** redefined_sym, Env* env) {

    for (size_t idx = 0; idx < env->defered_symbols_to_add.info.count; idx++) {

        if (!symbol_add(env, vec_at(&env->defered_symbols_to_add, idx))) {

            *redefined_sym = vec_at(&env->defered_symbols_to_add, idx);

            vec_reset(&env->defered_symbols_to_add);

            return false;

        }

    }



    vec_reset(&env->defered_symbols_to_add);

    return true;

}



bool usymbol_do_add_defered(Uast_def** redefined_sym, Env* env) {

    for (size_t idx = 0; idx < env->udefered_symbols_to_add.info.count; idx++) {

        if (!usymbol_add(env, vec_at(&env->udefered_symbols_to_add, idx))) {

            *redefined_sym = vec_at(&env->udefered_symbols_to_add, idx);

            vec_reset(&env->udefered_symbols_to_add);

            return false;

        }

    }



    vec_reset(&env->udefered_symbols_to_add);

    return true;

}



void usymbol_extend_table_internal(String* buf, const Usymbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Usymbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, uast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

// returns false if symbol is already added to the table
bool usym_tbl_add_internal(Usymbol_table_tast* sym_tbl_tasts, size_t capacity, Uast_def* tast_of_symbol) {
    assert(tast_of_symbol);
    Str_view symbol_name = uast_def_get_name(tast_of_symbol);
    assert(symbol_name.count > 0 && "invalid tast_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(uast_def_get_name(sym_tbl_tasts[curr_table_idx].tast), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Usymbol_table_tast tast = {.tast = tast_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_tasts[curr_table_idx] = tast;
    return true;
}

static void usym_tbl_cpy(
    Usymbol_table_tast* dest,
    const Usymbol_table_tast* src,
    size_t capacity,
    size_t count_tasts_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
usym_tbl_add_internal(dest, capacity, src[bucket_src].tast);
        }
    }
}

static void usym_tbl_expand_if_nessessary(Usymbol_table* sym_table) {
    size_t old_capacity_tast_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t tast_size = sizeof(sym_table->table_tasts[0]);

    bool should_move_elements = false;
    Usymbol_table_tast* new_table_tasts;

    if (sym_table->capacity < 1) {
        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {
        sym_table->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_tasts = arena_alloc(&a_main, sym_table->capacity*tast_size);
        usym_tbl_cpy(new_table_tasts, sym_table->table_tasts, sym_table->capacity, old_capacity_tast_count);
        sym_table->table_tasts = new_table_tasts;
    }
}

// return false if symbol is not found
bool usym_tbl_lookup_internal(Usymbol_table_tast** result, const Usymbol_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Usymbol_table_tast* curr_tast = &sym_table->table_tasts[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(uast_def_get_name(curr_tast->tast), key)) {
                *result = curr_tast;
                return true;
            }
        }

        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {
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
bool usym_tbl_add(Usymbol_table* sym_table, Uast_def* tast_of_symbol) {
    usym_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!usym_tbl_add_internal(sym_table->table_tasts, sym_table->capacity, tast_of_symbol)) {
        return false;
    }
    Uast_def* dummy;
    (void) dummy;
    assert(usym_tbl_lookup(&dummy, sym_table, uast_def_get_name(tast_of_symbol)));
    sym_table->count++;
    return true;
}

void usym_tbl_update(Usymbol_table* sym_table, Uast_def* tast_of_symbol) {
    Usymbol_table_tast* sym_tast;
    if (usym_tbl_lookup_internal(&sym_tast, sym_table, uast_def_get_name(tast_of_symbol))) {
        sym_tast->tast = tast_of_symbol;
        return;
    }
    unwrap(usym_tbl_add(sym_table, tast_of_symbol));
}

bool usymbol_lookup(Uast_def** result, Env* env, Str_view key) {
    if (usym_tbl_lookup(result, &env->primitives, key)) {
        return true;
    }
    if (lang_type_atom_is_signed(lang_type_atom_new(key, 0))) {
        int32_t bit_width = str_view_to_int64_t(env, POS_BUILTIN, str_view_slice(key, 1, key.count - 1));
        Uast_primitive_def* def = uast_primitive_def_new(
            POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new((Pos) {0}, bit_width, 0)))
        );
        unwrap(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
        *result = uast_primitive_def_wrap(def);
        return true;
    } else if (lang_type_atom_is_unsigned(lang_type_atom_new(key, 0))) {
        int32_t bit_width = str_view_to_int64_t(env, POS_BUILTIN, str_view_slice(key, 1, key.count - 1));
        Uast_primitive_def* def = uast_primitive_def_new(
            POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new((Pos) {0}, bit_width, 0)))
        );
        unwrap(usym_tbl_add(&env->primitives, uast_primitive_def_wrap(def)));
        *result = uast_primitive_def_wrap(def);
        return true;
    }

    if (env->ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        const Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        if (usym_tbl_lookup(result, &curr_tast->usymbol_table, key)) {
             return true;
         }

        if (idx < 1) {
            return false;
        }
    }
}

bool usymbol_add(Env* env, Uast_def* tast_of_symbol) {
    Uast_def* dummy;
    if (usymbol_lookup(&dummy, env, uast_def_get_name(tast_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        unwrap(usym_tbl_add(&curr_tast->usymbol_table, tast_of_symbol));
            return true;

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

void usymbol_update(Env* env, Uast_def* tast_of_symbol) {
    if (usymbol_add(env, tast_of_symbol)) {
        return;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* sym_coll = vec_at(&env->ancesters, idx);
        Usymbol_table_tast* curr_tast = NULL;
        if (usym_tbl_lookup_internal(&curr_tast, &sym_coll->usymbol_table, uast_def_get_name(tast_of_symbol))) {
            curr_tast->tast = tast_of_symbol;
            return;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}


void symbol_extend_table_internal(String* buf, const Symbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Symbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, tast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

// returns false if symbol is already added to the table
bool sym_tbl_add_internal(Symbol_table_tast* sym_tbl_tasts, size_t capacity, Tast_def* tast_of_symbol) {
    assert(tast_of_symbol);
    Str_view symbol_name = tast_def_get_name(tast_of_symbol);
    assert(symbol_name.count > 0 && "invalid tast_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(tast_def_get_name(sym_tbl_tasts[curr_table_idx].tast), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Symbol_table_tast tast = {.tast = tast_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_tasts[curr_table_idx] = tast;
    return true;
}

static void sym_tbl_cpy(
    Symbol_table_tast* dest,
    const Symbol_table_tast* src,
    size_t capacity,
    size_t count_tasts_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
sym_tbl_add_internal(dest, capacity, src[bucket_src].tast);
        }
    }
}

static void sym_tbl_expand_if_nessessary(Symbol_table* sym_table) {
    size_t old_capacity_tast_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t tast_size = sizeof(sym_table->table_tasts[0]);

    bool should_move_elements = false;
    Symbol_table_tast* new_table_tasts;

    if (sym_table->capacity < 1) {
        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {
        sym_table->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_tasts = arena_alloc(&a_main, sym_table->capacity*tast_size);
        sym_tbl_cpy(new_table_tasts, sym_table->table_tasts, sym_table->capacity, old_capacity_tast_count);
        sym_table->table_tasts = new_table_tasts;
    }
}

// return false if symbol is not found
bool sym_tbl_lookup_internal(Symbol_table_tast** result, const Symbol_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Symbol_table_tast* curr_tast = &sym_table->table_tasts[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(tast_def_get_name(curr_tast->tast), key)) {
                *result = curr_tast;
                return true;
            }
        }

        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {
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
bool sym_tbl_add(Symbol_table* sym_table, Tast_def* tast_of_symbol) {
    sym_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!sym_tbl_add_internal(sym_table->table_tasts, sym_table->capacity, tast_of_symbol)) {
        return false;
    }
    Tast_def* dummy;
    (void) dummy;
    assert(sym_tbl_lookup(&dummy, sym_table, tast_def_get_name(tast_of_symbol)));
    sym_table->count++;
    return true;
}

void sym_tbl_update(Symbol_table* sym_table, Tast_def* tast_of_symbol) {
    Symbol_table_tast* sym_tast;
    if (sym_tbl_lookup_internal(&sym_tast, sym_table, tast_def_get_name(tast_of_symbol))) {
        sym_tast->tast = tast_of_symbol;
        return;
    }
    unwrap(sym_tbl_add(sym_table, tast_of_symbol));
}

bool symbol_lookup(Tast_def** result, Env* env, Str_view key) {
    if (env->ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        const Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        if (sym_tbl_lookup(result, &curr_tast->symbol_table, key)) {
             return true;
         }

        if (idx < 1) {
            return false;
        }
    }
}

bool symbol_add(Env* env, Tast_def* tast_of_symbol) {
    Tast_def* dummy;
    if (symbol_lookup(&dummy, env, tast_def_get_name(tast_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        unwrap(sym_tbl_add(&curr_tast->symbol_table, tast_of_symbol));
            return true;

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

void symbol_update(Env* env, Tast_def* tast_of_symbol) {
    if (symbol_add(env, tast_of_symbol)) {
        return;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* sym_coll = vec_at(&env->ancesters, idx);
        Symbol_table_tast* curr_tast = NULL;
        if (sym_tbl_lookup_internal(&curr_tast, &sym_coll->symbol_table, tast_def_get_name(tast_of_symbol))) {
            curr_tast->tast = tast_of_symbol;
            return;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}


void alloca_extend_table_internal(String* buf, const Alloca_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Alloca_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, llvm_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

// returns false if symbol is already added to the table
bool all_tbl_add_internal(Alloca_table_tast* sym_tbl_tasts, size_t capacity, Llvm* tast_of_symbol) {
    assert(tast_of_symbol);
    Str_view symbol_name = llvm_tast_get_name(tast_of_symbol);
    assert(symbol_name.count > 0 && "invalid tast_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(llvm_tast_get_name(sym_tbl_tasts[curr_table_idx].tast), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Alloca_table_tast tast = {.tast = tast_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_tasts[curr_table_idx] = tast;
    return true;
}

static void all_tbl_cpy(
    Alloca_table_tast* dest,
    const Alloca_table_tast* src,
    size_t capacity,
    size_t count_tasts_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {
        if (src[bucket_src].status == SYM_TBL_OCCUPIED) {
all_tbl_add_internal(dest, capacity, src[bucket_src].tast);
        }
    }
}

static void all_tbl_expand_if_nessessary(Alloca_table* sym_table) {
    size_t old_capacity_tast_count = sym_table->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = sym_table->count + minimum_count_to_reserve;
    size_t tast_size = sizeof(sym_table->table_tasts[0]);

    bool should_move_elements = false;
    Alloca_table_tast* new_table_tasts;

    if (sym_table->capacity < 1) {
        sym_table->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / sym_table->capacity) >= SYM_TBL_MAX_DENSITY) {
        sym_table->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_tasts = arena_alloc(&a_main, sym_table->capacity*tast_size);
        all_tbl_cpy(new_table_tasts, sym_table->table_tasts, sym_table->capacity, old_capacity_tast_count);
        sym_table->table_tasts = new_table_tasts;
    }
}

// return false if symbol is not found
bool all_tbl_lookup_internal(Alloca_table_tast** result, const Alloca_table* sym_table, Str_view key) {
    if (sym_table->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(key, sym_table->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Alloca_table_tast* curr_tast = &sym_table->table_tasts[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(llvm_tast_get_name(curr_tast->tast), key)) {
                *result = curr_tast;
                return true;
            }
        }

        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {
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
bool all_tbl_add(Alloca_table* sym_table, Llvm* tast_of_symbol) {
    all_tbl_expand_if_nessessary(sym_table);
    assert(sym_table->capacity > 0);
    if (!all_tbl_add_internal(sym_table->table_tasts, sym_table->capacity, tast_of_symbol)) {
        return false;
    }
    Llvm* dummy;
    (void) dummy;
    assert(all_tbl_lookup(&dummy, sym_table, llvm_tast_get_name(tast_of_symbol)));
    sym_table->count++;
    return true;
}

void all_tbl_update(Alloca_table* sym_table, Llvm* tast_of_symbol) {
    Alloca_table_tast* sym_tast;
    if (all_tbl_lookup_internal(&sym_tast, sym_table, llvm_tast_get_name(tast_of_symbol))) {
        sym_tast->tast = tast_of_symbol;
        return;
    }
    unwrap(all_tbl_add(sym_table, tast_of_symbol));
}

bool alloca_lookup(Llvm** result, Env* env, Str_view key) {
    if (env->ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        const Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        if (all_tbl_lookup(result, &curr_tast->alloca_table, key)) {
             return true;
         }

        if (idx < 1) {
            return false;
        }
    }
}

bool alloca_add(Env* env, Llvm* tast_of_symbol) {
    Llvm* dummy;
    if (alloca_lookup(&dummy, env, llvm_tast_get_name(tast_of_symbol))) {
        return false;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* curr_tast = vec_at(&env->ancesters, idx);
        unwrap(all_tbl_add(&curr_tast->alloca_table, tast_of_symbol));
            return true;

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

void alloca_update(Env* env, Llvm* tast_of_symbol) {
    if (alloca_add(env, tast_of_symbol)) {
        return;
    }
    if (env->ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env->ancesters.info.count - 1;; idx--) {
        Symbol_collection* sym_coll = vec_at(&env->ancesters, idx);
        Alloca_table_tast* curr_tast = NULL;
        if (all_tbl_lookup_internal(&curr_tast, &sym_coll->alloca_table, llvm_tast_get_name(tast_of_symbol))) {
            curr_tast->tast = tast_of_symbol;
            return;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}



