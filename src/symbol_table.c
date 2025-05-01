#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include "symbol_table.h"
#include "symbol_table_struct.h"
#include <uast_utils.h>
#include <tast_utils.h>
#include <llvm_utils.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <symbol_log.h>

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

typedef Generic_symbol_table*(*Get_tbl_from_collection_fn)(Symbol_collection* collection);

bool generic_symbol_lookup(void** result, Str_view key, Get_tbl_from_collection_fn get_tbl_from_collection_fn, Scope_id scope_id);

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Str_view key);

// TODO: symbol_add should call symbol_update to reduce duplication
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

static void generic_tbl_cpy(void* dest, const void* src, size_t capacity, size_t count_tasts_to_cpy) {
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
    Get_tbl_from_collection_fn get_tbl_from_collection_fn,
    Scope_id scope_id
) {
    void* dummy;
    if (generic_symbol_lookup((void**)&dummy, key, get_tbl_from_collection_fn, scope_id)) {
        return false;
    }
    Symbol_collection* curr_tast = vec_at(&env.symbol_tables, scope_id);
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

void generic_symbol_update(Str_view key, void* item, Get_tbl_from_collection_fn get_tbl_from_collection_fn, Scope_id scope_id) {
    if (generic_symbol_add(key, item, get_tbl_from_collection_fn, scope_id)) {
        return;
    }

    Scope_id curr_scope = scope_id;
    while (true) {
        void* tbl = get_tbl_from_collection_fn(vec_at(&env.symbol_tables, curr_scope));
        Generic_symbol_table_tast* curr_tast = NULL;
        if (generic_tbl_lookup_internal(&curr_tast, tbl, key)) {
             curr_tast->tast = item;
             return;
        }
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_tbl_lookup(curr_scope);
    }
    unreachable("if there was no matching symbol found, generic_symbol_add should have worked");
}

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Str_view key) {
    Generic_symbol_table_tast* sym_tast;
    if (!generic_tbl_lookup_internal(&sym_tast, sym_table, key)) {
        return false;
    }
    *result = sym_tast->tast;
    return true;
}

bool generic_symbol_lookup(
    void** result,
    Str_view key,
    Get_tbl_from_collection_fn get_tbl_from_collection_fn,
    Scope_id scope_id
) {
    if (scope_id == SIZE_MAX /* TODO: use different name for this constant? */) {
        return false;
    }

    Scope_id curr_scope = scope_id;
    while (true) {
        void* tbl = get_tbl_from_collection_fn(vec_at(&env.symbol_tables, curr_scope));
        if (generic_tbl_lookup(result, tbl, key)) {
             return true;
        }
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_tbl_lookup(curr_scope);
    }

    return false;
}

//
// Uast_def implementation
//

// returns false if symbol has already been added to the table
bool sym_tbl_add(Tast_def* item) {
    Scope_id scope_id = tast_def_get_name(item).scope_id;
    return generic_tbl_add(
        (Generic_symbol_table*)&vec_at(&env.symbol_tables, scope_id)->symbol_table,
        serialize_name_symbol_table(tast_def_get_name(item)),
        item
    );
}

void* sym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->symbol_table;
}

bool symbol_add(Tast_def* item) {
    Name name = tast_def_get_name(item);
    return generic_symbol_add(
        serialize_name_symbol_table(name),
        item,
        (Get_tbl_from_collection_fn)sym_get_tbl_from_collection,
        name.scope_id
    );
}

void sym_tbl_update(Scope_id scope_id, Tast_def* item) {
    generic_tbl_update((Generic_symbol_table*)&vec_at(&env.symbol_tables, scope_id)->symbol_table, serialize_name_symbol_table(tast_def_get_name(item)), item);
}

void symbol_update(Tast_def* item) {
    (void) item;
    todo();
    //generic_symbol_update(serialize_name_symbol_table(tast_def_get_name(item)), item, (Get_tbl_from_collection_fn)sym_get_tbl_from_collection);
}

bool symbol_lookup(Tast_def** result, Name key) {
    return generic_symbol_lookup((void**)result, serialize_name_symbol_table(key), (Get_tbl_from_collection_fn)sym_get_tbl_from_collection, key.scope_id);
}

void symbol_extend_table_internal(String* buf, const Symbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Symbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&print_arena, buf, tast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

//
// usymbol implementation
//

void* usym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->usymbol_table;
}

bool usymbol_add(Uast_def* item) {
    Name name = uast_def_get_name(item);
    return generic_symbol_add(
        serialize_name_symbol_table(name),
        item,
        (Get_tbl_from_collection_fn)usym_get_tbl_from_collection,
        name.scope_id
    );
}

bool sym_tbl_lookup(Tast_def** result, Name key) {
    return generic_tbl_lookup(
        (void**)result,
        (Generic_symbol_table*)&vec_at(&env.symbol_tables, key.scope_id)->symbol_table,
        serialize_name_symbol_table(key)
    );
}

//
// Uast_def implementation
//

// returns false if symbol has already been added to the table
bool usym_tbl_add(Uast_def* item) {
    Name name = uast_def_get_name(item);
    return generic_tbl_add(
        (Generic_symbol_table*)&vec_at(&env.symbol_tables, name.scope_id)->usymbol_table,
        serialize_name_symbol_table(name),
        item
    );
}

void usym_tbl_update(Uast_def* item) {
    (void) item;
    todo();
    //generic_tbl_update((Generic_symbol_table*)sym_table, serialize_name_symbol_table(uast_def_get_name(item)), item);
}

bool usym_tbl_lookup(Uast_def** result, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&vec_at(&env.symbol_tables, key.scope_id)->usymbol_table, serialize_name_symbol_table(key));
}

bool usymbol_lookup(Uast_def** result, Name key) {
    if (key.mod_path.count < 1) {
        Name prim_key = key;
        prim_key.scope_id = 0;
        if (usym_tbl_lookup(result, prim_key)) {
            return true;
        }
        if (lang_type_atom_is_signed(lang_type_atom_new(prim_key, 0))) {
            int32_t bit_width = str_view_to_int64_t(POS_BUILTIN, str_view_slice(prim_key.base, 1, prim_key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        } else if (lang_type_atom_is_unsigned(lang_type_atom_new(prim_key, 0))) {
            int32_t bit_width = str_view_to_int64_t(POS_BUILTIN, str_view_slice(prim_key.base, 1, prim_key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        }
    }

    return generic_symbol_lookup(
        (void**)result,
        serialize_name_symbol_table(key),
        (Get_tbl_from_collection_fn)usym_get_tbl_from_collection,
        key.scope_id
    );
}

//
// Llvm implementation
//

// returns false if symbol has already been added to the table
bool all_tbl_add(Llvm* item) {
    Name name = llvm_tast_get_name(item);
    return generic_tbl_add((Generic_symbol_table*)&vec_at(&env.symbol_tables, name.scope_id)->alloca_table, serialize_name_symbol_table(name), item);
}

void* all_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->alloca_table;
}

bool alloca_add(Llvm* item) {
    Name name = llvm_tast_get_name(item);
    return generic_symbol_add(
        serialize_name_symbol_table(name),
        item,
        (Get_tbl_from_collection_fn)all_get_tbl_from_collection,
        name.scope_id
    );
}

void all_tbl_update(Llvm* item) {
    Name name = llvm_tast_get_name(item);
    generic_tbl_update((Generic_symbol_table*)&vec_at(&env.symbol_tables, name.scope_id)->usymbol_table, serialize_name_symbol_table(name), item);
}

void usymbol_update(Uast_def* item) {
    Name name = uast_def_get_name(item);
    generic_symbol_update(
        serialize_name_symbol_table(name),
        item,
        (Get_tbl_from_collection_fn)usym_get_tbl_from_collection,
        name.scope_id
    );
}

void alloca_update(Llvm* item) {
    (void) item;
    todo();
    //generic_symbol_update(serialize_name_symbol_table(llvm_tast_get_name(item)), item, (Get_tbl_from_collection_fn)all_get_tbl_from_collection);
}

bool all_tbl_lookup(Llvm** result, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&vec_at(&env.symbol_tables, key.scope_id)->usymbol_table, serialize_name_symbol_table(key));
}

bool alloca_lookup(Llvm** result, Name key) {
    return generic_symbol_lookup(
        (void**)result,
        serialize_name_symbol_table(key),
        (Get_tbl_from_collection_fn)all_get_tbl_from_collection,
        key.scope_id
    );
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

// returns parent of key
Scope_id scope_tbl_lookup(Scope_id key) {
    char buf[32] = {0};
    sprintf(buf, "%zu", key);
    Scope_id parent = 0;
    Scope_id* temp = &parent;
    generic_tbl_lookup((void**)&temp, (Generic_symbol_table*)&env.scope_id_to_parent, str_view_from_cstr(buf));
    parent = *temp;
    return parent;
}

bool scope_tbl_add(Scope_id key, Scope_id parent) {
    char buf[32] = {0};
    sprintf(buf, "%zu", key);
    String serialized = {0};
    string_extend_cstr(&a_main, &serialized, buf);
    Scope_id* next_alloced = arena_alloc(&a_main, sizeof(*next_alloced));
    *next_alloced = parent;
    return generic_tbl_add((Generic_symbol_table*)&env.scope_id_to_parent, string_to_strv(serialized), next_alloced);
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

Scope_id symbol_collection_new(Scope_id parent) {
    Symbol_collection* new_tbl = arena_alloc(&a_main, sizeof(*new_tbl));
    Scope_id new_scope = env.symbol_tables.info.count;
    vec_append(&a_main, &env.symbol_tables, new_tbl);

    unwrap(scope_tbl_add(new_scope, parent));
    return new_scope;
}

