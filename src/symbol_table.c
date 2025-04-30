#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include "symbol_table.h"
#include "symbol_table_struct.h"
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

typedef bool(*Symbol_add_fn)(void* tast_to_add);

typedef void*(*Get_tbl_from_collection_fn)(Symbol_collection* collection);

bool generic_symbol_lookup(void** result, Str_view key, Get_tbl_from_collection_fn get_tbl_from_collection_fn);

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Str_view key);

bool generic_symbol_table_add_internal(Generic_symbol_table_tast* sym_tbl_tasts, size_t capacity, Str_view key, void* item) {
    assert(item);
    assert(key.count > 0 && "invalid item");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(key, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (str_view_is_equal(sym_tbl_tasts[curr_table_idx].key, key)) {
            return false;
        }
        curr_table_idx = (curr_table_idx + 1) % capacity;
        assert(init_table_idx != curr_table_idx && "hash table is full here, and it should not be");
        (void) init_table_idx;
    }

    Generic_symbol_table_tast tast = {.tast = item, .status = SYM_TBL_OCCUPIED, .key = key};
    sym_tbl_tasts[curr_table_idx] = tast;
    return true;
}

bool generic_do_add_defered(void** redefined_sym, void* defered_tasts_to_add, Symbol_add_fn add_fn) {
    for (size_t idx = 0; idx < ((Generic_vec*)defered_tasts_to_add)->info.count; idx++) {
        if (!add_fn(vec_at((Generic_vec*)defered_tasts_to_add, idx))) {
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
    size_t count_tasts_to_cpy
) {
    for (size_t bucket_src = 0; bucket_src < count_tasts_to_cpy; bucket_src++) {
        // TODO: do not use Usymbol_table_tast verbatim here
        if (((Usymbol_table_tast*)src)[bucket_src].status == SYM_TBL_OCCUPIED) {
            generic_symbol_table_add_internal(dest, capacity, ((Usymbol_table_tast*)src)[bucket_src].key, ((Usymbol_table_tast*)src)[bucket_src].tast);
        }
    }
}

static void generic_tbl_expand_if_nessessary(void* sym_table) {
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
        generic_tbl_cpy(new_table_tasts, ((Generic_symbol_table*)sym_table)->table_tasts, ((Generic_symbol_table*)sym_table)->capacity, old_capacity_tast_count);
        ((Generic_symbol_table*)sym_table)->table_tasts = new_table_tasts;
    }
}

// return false if symbol is not found
bool generic_tbl_lookup_internal(Generic_symbol_table_tast** result, const void* sym_table, Str_view query) {
    if (((Generic_symbol_table*)sym_table)->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(query, ((Generic_symbol_table*)sym_table)->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Generic_symbol_table_tast* curr_tast = &((Generic_symbol_table_tast*)(((Generic_symbol_table*)sym_table)->table_tasts))[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (str_view_is_equal(curr_tast->key, query)) {
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
bool generic_tbl_add(Generic_symbol_table* sym_table, Str_view key, void* item) {
    generic_tbl_expand_if_nessessary(sym_table);
    assert(((Generic_symbol_table*)sym_table)->capacity > 0);
    if (!generic_symbol_table_add_internal(sym_table->table_tasts, sym_table->capacity, key, item)) {
        return false;
    }
    Llvm* dummy;
    (void) dummy;
    assert(generic_tbl_lookup((void**)&dummy, sym_table, key));
    sym_table->count++;
    return true;
}

bool generic_symbol_add(
    Str_view key,
    void* item,
    Get_tbl_from_collection_fn get_tbl_from_collection_fn
) {
    void* dummy;
    if (generic_symbol_lookup((void**)&dummy,  key, get_tbl_from_collection_fn)) {
        return false;
    }
    unwrap(env.ancesters.info.count > 0 && "no block ancester found");
    Symbol_collection* curr_tast = vec_at(&env.ancesters, env.ancesters.info.count - 1);
    unwrap(generic_tbl_add((Generic_symbol_table*)get_tbl_from_collection_fn(curr_tast), key, item));
    return true;
}

void generic_tbl_update(Generic_symbol_table* sym_table, Str_view key, void* item) {
    Generic_symbol_table_tast* sym_tast;
    if (generic_tbl_lookup_internal(&sym_tast, sym_table, key)) {
        sym_tast->tast = item;
        return;
    }
    unwrap(generic_tbl_add(sym_table, key, item));
}

void generic_symbol_update(Str_view key, void* item, Get_tbl_from_collection_fn get_tbl_from_collection_fn) {
    if (generic_symbol_add(key, item, get_tbl_from_collection_fn)) {
        return;
    }
    if (env.ancesters.info.count < 1) {
        unreachable("no block ancester found");
    }

    for (size_t idx = env.ancesters.info.count - 1;; idx--) {
        Symbol_collection* sym_coll = vec_at(&env.ancesters, idx);
        Generic_symbol_table_tast* curr_tast = NULL;
        if (generic_tbl_lookup_internal(&curr_tast, get_tbl_from_collection_fn(sym_coll), key)) {
            curr_tast->tast = item;
            return;
        }

        if (idx < 1) {
            unreachable("no block ancester found");
        }
    }
}

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Str_view key) {
    Generic_symbol_table_tast* sym_tast;
    if (!generic_tbl_lookup_internal(&sym_tast, sym_table, key)) {
        return false;
    }
    *result = sym_tast->tast;
    return true;
}

bool generic_symbol_lookup(void** result, Str_view key, Get_tbl_from_collection_fn get_tbl_from_collection_fn) {
    if (env.ancesters.info.count < 1) {
        return false;
    }

    for (size_t idx = env.ancesters.info.count - 1;; idx--) {
        Symbol_collection* curr_tast = vec_at(&env.ancesters, idx);
        if (generic_tbl_lookup(result, get_tbl_from_collection_fn(curr_tast), key)) {
             return true;
         }

        if (idx < 1) {
            return false;
        }
    }
}

//
// Uast_def implementation
//

bool symbol_do_add_defered(Tast_def** redefined_sym) {
    return generic_do_add_defered((void**)redefined_sym,  &env.defered_symbols_to_add, (Symbol_add_fn)symbol_add);
}

// returns false if symbol has already been added to the table
bool sym_tbl_add(Symbol_table* sym_table, Tast_def* item) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, serialize_name_symbol_table(tast_def_get_name(item)), item);
}

void* sym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->symbol_table;
}

bool symbol_add(Tast_def* item) {
    return generic_symbol_add(
        serialize_name_symbol_table(tast_def_get_name(item)),
        item,
        (Get_tbl_from_collection_fn)sym_get_tbl_from_collection
    );
}

void sym_tbl_update(Symbol_table* sym_table, Tast_def* item) {
    generic_tbl_update((Generic_symbol_table*)sym_table, serialize_name_symbol_table(tast_def_get_name(item)), item);
}

void symbol_update(Tast_def* item) {
    generic_symbol_update(serialize_name_symbol_table(tast_def_get_name(item)), item, (Get_tbl_from_collection_fn)sym_get_tbl_from_collection);
}

bool symbol_lookup(Tast_def** result, Name key) {
    return generic_symbol_lookup((void**)result, serialize_name_symbol_table(key), (Get_tbl_from_collection_fn)sym_get_tbl_from_collection);
}

void symbol_extend_table_internal(String* buf, const Symbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Symbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, tast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

void* usym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->usymbol_table;
}

bool usymbol_add(Uast_def* item) {
    return generic_symbol_add(
        serialize_name_symbol_table(uast_def_get_name(item)),
        item,
        (Get_tbl_from_collection_fn)usym_get_tbl_from_collection
    );
}

bool sym_tbl_lookup(Tast_def** result, const Symbol_table* sym_table, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)sym_table, serialize_name_symbol_table(key));
}

//
// Uast_def implementation
//
bool usymbol_do_add_defered(Uast_def** redefined_sym) {
    return generic_do_add_defered((void**)redefined_sym,  &env.udefered_symbols_to_add, (Symbol_add_fn)usymbol_add);
}

// returns false if symbol has already been added to the table
bool usym_tbl_add(Usymbol_table* sym_table, Uast_def* item) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, serialize_name_symbol_table(uast_def_get_name(item)), item);
}

void usym_tbl_update(Usymbol_table* sym_table, Uast_def* item) {
    generic_tbl_update((Generic_symbol_table*)sym_table, serialize_name_symbol_table(uast_def_get_name(item)), item);
}

bool usym_tbl_lookup(Uast_def** result, const Usymbol_table* sym_table, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)sym_table, serialize_name_symbol_table(key));
}

bool usymbol_lookup(Uast_def** result, Name key) {
    if (key.mod_path.count < 1) {
        if (usym_tbl_lookup(result, &env.primitives, key)) {
            return true;
        }
        if (lang_type_atom_is_signed(lang_type_atom_new(key, 0))) {
            int32_t bit_width = str_view_to_int64_t(POS_BUILTIN, str_view_slice(key.base, 1, key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(&env.primitives, uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        } else if (lang_type_atom_is_unsigned(lang_type_atom_new(key, 0))) {
            int32_t bit_width = str_view_to_int64_t(POS_BUILTIN, str_view_slice(key.base, 1, key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(&env.primitives, uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        }
    }

    return generic_symbol_lookup((void**)result,  serialize_name_symbol_table(key), (Get_tbl_from_collection_fn)usym_get_tbl_from_collection);
}

//
// Llvm implementation
//

bool alloca_do_add_defered(Llvm** redefined_sym) {
    return generic_do_add_defered((void**)redefined_sym,  &env.defered_allocas_to_add, (Symbol_add_fn)alloca_add);
}

// returns false if symbol has already been added to the table
bool all_tbl_add(Alloca_table* sym_table, Llvm* item) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, serialize_name_symbol_table(llvm_tast_get_name(item)), item);
}

void* all_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->alloca_table;
}

bool alloca_add(Llvm* item) {
    return generic_symbol_add(
        
        serialize_name_symbol_table(llvm_tast_get_name(item)),
        item,
        (Get_tbl_from_collection_fn)all_get_tbl_from_collection
    );
}

void all_tbl_update(Alloca_table* sym_table, Llvm* item) {
    generic_tbl_update((Generic_symbol_table*)sym_table, serialize_name_symbol_table(llvm_tast_get_name(item)), item);
}

void usymbol_update(Uast_def* item) {
    // TODO: do extra stuff similar to usymbol_add?
    generic_symbol_update(serialize_name_symbol_table(uast_def_get_name(item)), item, (Get_tbl_from_collection_fn)usym_get_tbl_from_collection);
}

void alloca_update(Llvm* item) {
    generic_symbol_update(serialize_name_symbol_table(llvm_tast_get_name(item)), item, (Get_tbl_from_collection_fn)all_get_tbl_from_collection);
}

bool all_tbl_lookup(Llvm** result, const Alloca_table* sym_table, Str_view key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)sym_table, key);
}

bool alloca_lookup(Llvm** result, Name key) {
    return generic_symbol_lookup((void**)result, serialize_name_symbol_table(key), (Get_tbl_from_collection_fn)all_get_tbl_from_collection);
}

//
// File_path_to_text implementation
//

bool file_path_to_text_tbl_lookup(Str_view** result, const File_path_to_text* sym_table, Str_view key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)sym_table, key);
}

// returns false if file_path_to_text has already been added to the table
bool file_path_to_text_tbl_add(File_path_to_text* sym_table, Str_view* file_text, Str_view key) {
    return generic_tbl_add((Generic_symbol_table*)sym_table, key, file_text);
}

//
// Scope_id_to_next_table implementation
//

bool scope_tbl_lookup(Scope_id* result, Scope_id key) {
    char buf[32] = {0};
    sprintf(buf, "%zu", key);
    Scope_id temp_stack = 0;
    Scope_id* temp = &temp_stack;
    bool status = generic_tbl_lookup((void**)&temp, (Generic_symbol_table*)&env.scope_id_to_next, str_view_from_cstr(buf));
    *result = *temp;
    arena_reset(&env.a_temp);
    return status;
}

bool scope_tbl_add(Scope_id key, Scope_id next) {
    char buf[32] = {0};
    sprintf(buf, "%zu", key);
    String serialized = {0};
    string_extend_cstr(&a_main, &serialized, buf);
    Scope_id* next_alloced = arena_alloc(&a_main, sizeof(*next_alloced));
    *next_alloced = next;
    return generic_tbl_add((Generic_symbol_table*)&env.scope_id_to_next, string_to_strv(serialized), next_alloced);
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

void alloca_extend_table_internal(String* buf, const Alloca_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Alloca_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, llvm_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

