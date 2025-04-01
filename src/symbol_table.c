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

//
// util
//
static size_t sym_tbl_calculate_idx(Str_view key, size_t capacity) {
    assert(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

//
// generics
//

typedef bool(*Tbl_add_fn)(Env* env, void* tast_to_add);

typedef Str_view(*Get_key_fn)(const void* tast);

typedef bool(*Tbl_add_internal_fn)(Generic_symbol_table_tast* usym_tbl_tasts, size_t capacity, void* tast_of_symbol);

typedef bool(*Tbl_lookup_fn)(void** result, const void* sym_table, Str_view key);

typedef bool(*Symbol_lookup_fn)(void** result, Env* env, Str_view key);

typedef void*(*Get_tbl_from_collection_fn)(Symbol_collection* collection);

bool generic_symbol_table_add_internal(Generic_symbol_table_tast* sym_tbl_tasts, size_t capacity, void* tast_of_symbol, Get_key_fn get_key_fn) {
    assert(tast_of_symbol);
    Str_view symbol_name = get_key_fn(tast_of_symbol);
    assert(symbol_name.count > 0 && "invalid tast_of_symbol");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(symbol_name, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(get_key_fn(sym_tbl_tasts[curr_table_idx].tast), symbol_name)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Generic_symbol_table_tast tast = {.tast = tast_of_symbol, .status = SYM_TBL_OCCUPIED};
    sym_tbl_tasts[curr_table_idx] = tast;
    return true;
}

bool generic_do_add_defered(void** redefined_sym, Env* env, void* defered_tasts_to_add, Tbl_add_fn add_fn) {
    for (size_t idx = 0; idx < ((Generic_vec*)defered_tasts_to_add)->info.count; idx++) {
        if (!add_fn(env, vec_at((Generic_vec*)defered_tasts_to_add, idx))) {
            *redefined_sym = vec_at((Generic_vec*)defered_tasts_to_add, idx);
            vec_reset((Generic_vec*)defered_tasts_to_add);
            return false;
        }
    }

    vec_reset((Generic_vec*)defered_tasts_to_add);
    return true;
}

static void generic_tbl_cpy(
    void* dest,
    const void* src,
    size_t capacity,
    size_t count_tasts_to_cpy,
    Tbl_add_internal_fn add_internal_fn
) {
    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {
        if (((Usymbol_table_tast*)src)[bucket_src].status == SYM_TBL_OCCUPIED) {
            add_internal_fn(dest, capacity, ((Usymbol_table_tast*)src)[bucket_src].tast);
        }
    }
}

static void generic_tbl_expand_if_nessessary(void* sym_table, Tbl_add_internal_fn add_cpy_internal_fn) {
    size_t old_capacity_tast_count = ((Generic_symbol_table*)sym_table)->capacity;
    size_t minimum_count_to_reserve = 1;
    size_t new_count = ((Generic_symbol_table*)sym_table)->count + minimum_count_to_reserve;
    size_t tast_size = sizeof(Generic_symbol_table_tast);

    bool should_move_elements = false;
    Usymbol_table_tast* new_table_tasts;

    if (((Generic_symbol_table*)sym_table)->capacity < 1) {
        ((Generic_symbol_table*)sym_table)->capacity = SYM_TBL_DEFAULT_CAPACITY;
        should_move_elements = true;
    }
    while (((float)new_count / ((Generic_symbol_table*)sym_table)->capacity) >= SYM_TBL_MAX_DENSITY) {
        ((Generic_symbol_table*)sym_table)->capacity *= 2;
        should_move_elements = true;
    }

    if (should_move_elements) {
        new_table_tasts = arena_alloc(&a_main, ((Generic_symbol_table*)sym_table)->capacity*tast_size);
        generic_tbl_cpy(new_table_tasts, ((Generic_symbol_table*)sym_table)->table_tasts, ((Generic_symbol_table*)sym_table)->capacity, old_capacity_tast_count, add_cpy_internal_fn);
        ((Generic_symbol_table*)sym_table)->table_tasts = new_table_tasts;
    }
}

// return false if symbol is not found
bool generic_tbl_lookup_internal(Generic_symbol_table_tast** result, const void* sym_table, Str_view query, Get_key_fn get_key_fn) {
    if (((Generic_symbol_table*)sym_table)->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(query, ((Generic_symbol_table*)sym_table)->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Generic_symbol_table_tast* curr_tast = &((Generic_symbol_table_tast*)(((Generic_symbol_table*)sym_table)->table_tasts))[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(get_key_fn(curr_tast->tast), query)) {
                *result = curr_tast;
                return true;
            }
        }

        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {
            return false;
        }

        curr_table_idx = (curr_table_idx + 1) % ((Generic_symbol_table*)sym_table)->capacity;
        if (curr_table_idx == init_table_idx) {
            return false;
        }
    }

    unreachable("");
}

// returns false if symbol has already been added to the table
bool generic_tbl_add(Generic_symbol_table* sym_table, void* tast_of_symbol, Tbl_add_internal_fn add_internal_fn, Tbl_lookup_fn tbl_lookup_fn, Get_key_fn get_key_fn) {
    generic_tbl_expand_if_nessessary(sym_table, add_internal_fn);
    assert(((Generic_symbol_table*)sym_table)->capacity > 0);
    if (!generic_symbol_table_add_internal(sym_table->table_tasts, sym_table->capacity, tast_of_symbol, get_key_fn)) {
        return false;
    }
    Llvm* dummy;
    (void) dummy;
    assert(tbl_lookup_fn((void**)&dummy, sym_table, get_key_fn(tast_of_symbol)));
    sym_table->count++;
    return true;
}

bool generic_symbol_add(
    Env* env,
    void* tast_of_symbol,
    Symbol_lookup_fn symbol_lookup_fn,
    Tbl_add_internal_fn add_internal_fn,
    Tbl_lookup_fn tbl_lookup_fn,
    Get_key_fn get_key_fn,
    Get_tbl_from_collection_fn get_tbl_from_collection_fn
) {
    void* dummy;
    if (symbol_lookup_fn((void**)&dummy, env, get_key_fn(tast_of_symbol))) {
        return false;
    }
    unwrap(env->ancesters.info.count > 0 && "no block ancester found");
    Symbol_collection* curr_tast = vec_at(&env->ancesters, env->ancesters.info.count - 1);
    unwrap(generic_tbl_add((Generic_symbol_table*)get_tbl_from_collection_fn(curr_tast), tast_of_symbol, add_internal_fn, tbl_lookup_fn, get_key_fn));
    return true;
}

void generic_tbl_update(Generic_symbol_table* sym_table, void* tast_of_symbol, Tbl_add_internal_fn add_internal_fn, Tbl_lookup_fn tbl_lookup_fn, Get_key_fn get_key_fn) {
    Generic_symbol_table_tast* sym_tast;
    if (generic_tbl_lookup_internal(&sym_tast, sym_table, get_key_fn(tast_of_symbol), get_key_fn)) {
        sym_tast->tast = tast_of_symbol;
        return;
    }
    unwrap(generic_tbl_add(sym_table, tast_of_symbol, add_internal_fn, tbl_lookup_fn, get_key_fn));
}

//
// implementations
//

// returns false if symbol is already added to the table
bool usym_tbl_add_internal(Usymbol_table_tast* usym_tbl_tasts, size_t capacity, Uast_def* tast_of_symbol) {
    return generic_symbol_table_add_internal((Generic_symbol_table_tast*)usym_tbl_tasts, capacity, tast_of_symbol, (Get_key_fn)uast_def_get_name);
}

// returns false if symbol is already added to the table
bool sym_tbl_add_internal(Symbol_table_tast* sym_tbl_tasts, size_t capacity, Tast_def* tast_of_symbol) {
    return generic_symbol_table_add_internal((Generic_symbol_table_tast*)sym_tbl_tasts, capacity, tast_of_symbol, (Get_key_fn)tast_def_get_name);
}

// returns false if symbol is already added to the table
bool all_tbl_add_internal(Alloca_table_tast* all_tbl_tasts, size_t capacity, Llvm* tast_of_symbol) {
    return generic_symbol_table_add_internal((Generic_symbol_table_tast*)all_tbl_tasts, capacity, tast_of_symbol, (Get_key_fn)llvm_tast_get_name);
}

bool alloca_do_add_defered(Llvm** redefined_sym, Env* env) {
    return generic_do_add_defered((void**)redefined_sym, env, &env->defered_allocas_to_add, (Tbl_add_fn)alloca_add);
}

bool symbol_do_add_defered(Tast_def** redefined_sym, Env* env) {
    return generic_do_add_defered((void**)redefined_sym, env, &env->defered_symbols_to_add, (Tbl_add_fn)symbol_add);
}

bool usymbol_do_add_defered(Uast_def** redefined_sym, Env* env) {
    return generic_do_add_defered((void**)redefined_sym, env, &env->udefered_symbols_to_add, (Tbl_add_fn)usymbol_add);
}

static void usym_tbl_expand_if_nessessary(Usymbol_table* sym_table) {
    generic_tbl_expand_if_nessessary(sym_table, (Tbl_add_internal_fn)usym_tbl_add_internal);
}

static void sym_tbl_expand_if_nessessary(Symbol_table* sym_table) {
    generic_tbl_expand_if_nessessary(sym_table, (Tbl_add_internal_fn)sym_tbl_add_internal);
}

static void all_tbl_expand_if_nessessary(Alloca_table* sym_table) {
    generic_tbl_expand_if_nessessary(sym_table, (Tbl_add_internal_fn)all_tbl_add_internal);
}

bool usym_tbl_lookup_internal(Usymbol_table_tast** result, const Usymbol_table* sym_table, Str_view key) {
    return generic_tbl_lookup_internal((Generic_symbol_table_tast**)result, sym_table, key, (Get_key_fn)uast_def_get_name);
}

bool sym_tbl_lookup_internal(Symbol_table_tast** result, const Symbol_table* sym_table, Str_view key) {
    return generic_tbl_lookup_internal((Generic_symbol_table_tast**)result, sym_table, key, (Get_key_fn)tast_def_get_name);
}

bool all_tbl_lookup_internal(Alloca_table_tast** result, const Alloca_table* sym_table, Str_view key) {
    return generic_tbl_lookup_internal((Generic_symbol_table_tast**)result, sym_table, key, (Get_key_fn)llvm_tast_get_name);
}

// returns false if symbol has already been added to the table
bool all_tbl_add(Alloca_table* sym_table, Llvm* tast_of_symbol) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, tast_of_symbol, (Tbl_add_internal_fn)all_tbl_add_internal, (Tbl_lookup_fn)all_tbl_lookup, (Get_key_fn)llvm_tast_get_name);
}

// returns false if symbol has already been added to the table
bool sym_tbl_add(Symbol_table* sym_table, Tast_def* tast_of_symbol) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, tast_of_symbol, (Tbl_add_internal_fn)sym_tbl_add_internal, (Tbl_lookup_fn)sym_tbl_lookup, (Get_key_fn)tast_def_get_name);
}

// returns false if symbol has already been added to the table
bool usym_tbl_add(Usymbol_table* sym_table, Uast_def* tast_of_usymbol) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, tast_of_usymbol, (Tbl_add_internal_fn)usym_tbl_add_internal, (Tbl_lookup_fn)usym_tbl_lookup, (Get_key_fn)uast_def_get_name);
}

void* usym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->usymbol_table;
}

void* sym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->symbol_table;
}

void* all_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->alloca_table;
}

bool usymbol_add(Env* env, Uast_def* tast_of_symbol) {
    return generic_symbol_add(
        env,
        tast_of_symbol,
        (Symbol_lookup_fn)usymbol_lookup,
        (Tbl_add_internal_fn)usym_tbl_add_internal,
        (Tbl_lookup_fn)usym_tbl_lookup,
        (Get_key_fn)uast_def_get_name,
        (Get_tbl_from_collection_fn)usym_get_tbl_from_collection
    );
}

bool symbol_add(Env* env, Tast_def* tast_of_symbol) {
    return generic_symbol_add(
        env,
        tast_of_symbol,
        (Symbol_lookup_fn)symbol_lookup,
        (Tbl_add_internal_fn)sym_tbl_add_internal,
        (Tbl_lookup_fn)sym_tbl_lookup,
        (Get_key_fn)tast_def_get_name,
        (Get_tbl_from_collection_fn)sym_get_tbl_from_collection
    );
}

bool alloca_add(Env* env, Llvm* tast_of_symbol) {
    return generic_symbol_add(
        env,
        tast_of_symbol,
        (Symbol_lookup_fn)alloca_lookup,
        (Tbl_add_internal_fn)all_tbl_add_internal,
        (Tbl_lookup_fn)all_tbl_lookup,
        (Get_key_fn)llvm_tast_get_name,
        (Get_tbl_from_collection_fn)all_get_tbl_from_collection
    );
}

void usym_tbl_update(Usymbol_table* sym_table, Uast_def* tast_of_symbol) {
    generic_tbl_update((Generic_symbol_table*)sym_table, tast_of_symbol, (Tbl_add_internal_fn)usym_tbl_add_internal, (Tbl_lookup_fn)usym_tbl_lookup, (Get_key_fn)uast_def_get_name);
}

void sym_tbl_update(Symbol_table* sym_table, Tast_def* tast_of_symbol) {
    generic_tbl_update((Generic_symbol_table*)sym_table, tast_of_symbol, (Tbl_add_internal_fn)sym_tbl_add_internal, (Tbl_lookup_fn)sym_tbl_lookup, (Get_key_fn)tast_def_get_name);
}

void all_tbl_update(Alloca_table* sym_table, Llvm* tast_of_symbol) {
    generic_tbl_update((Generic_symbol_table*)sym_table, tast_of_symbol, (Tbl_add_internal_fn)all_tbl_add_internal, (Tbl_lookup_fn)all_tbl_lookup, (Get_key_fn)llvm_tast_get_name);
}

//
// not generic
//

void usymbol_extend_table_internal(String* buf, const Usymbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Usymbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, uast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
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



