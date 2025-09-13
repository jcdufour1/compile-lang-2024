#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
#include "symbol_table.h"
#include "symbol_table_struct.h"
#include <uast_utils.h>
#include <tast_utils.h>
#include <ir_utils.h>
#include <tast_serialize.h>
#include <lang_type_serialize.h>
#include <symbol_log.h>

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f)

//
// util
//
static size_t sym_tbl_calculate_idx(Strv key, size_t capacity) {
    assert(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

//
// generics
//

typedef bool(*Symbol_add_fn)(void* tast_to_add);

typedef Generic_symbol_table*(*Get_tbl_from_collection_fn)(Symbol_collection* collection);

bool generic_symbol_lookup(void** result, Strv key, Get_tbl_from_collection_fn get_tbl_from_collection_fn, Scope_id scope_id);

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Strv key);

// TODO: symbol_add should call symbol_update to reduce duplication
bool generic_symbol_table_add_internal(Generic_symbol_table_tast* sym_tbl_tasts, size_t capacity, Strv key, void* item) {
    assert(key.count > 0 && "invalid item");

    assert(capacity > 0);
    size_t curr_table_idx = sym_tbl_calculate_idx(key, capacity);
    size_t init_table_idx = curr_table_idx; 
    while (sym_tbl_tasts[curr_table_idx].status == SYM_TBL_OCCUPIED) {
        if (strv_is_equal(sym_tbl_tasts[curr_table_idx].key, key)) {
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
        if (((Generic_symbol_table_tast*)src)[bucket_src].status == SYM_TBL_OCCUPIED) {
            generic_symbol_table_add_internal(dest, capacity, ((Generic_symbol_table_tast*)src)[bucket_src].key, ((Generic_symbol_table_tast*)src)[bucket_src].tast);
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
bool generic_tbl_lookup_internal(Generic_symbol_table_tast** result, const void* sym_table, Strv query) {
    if (((Generic_symbol_table*)sym_table)->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(query, ((Generic_symbol_table*)sym_table)->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Generic_symbol_table_tast* curr_tast = &((Generic_symbol_table_tast*)(((Generic_symbol_table*)sym_table)->table_tasts))[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (strv_is_equal(curr_tast->key, query)) {
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
bool generic_tbl_add(Generic_symbol_table* sym_table, Strv key, void* item) {
    generic_tbl_expand_if_nessessary(sym_table);
    assert(((Generic_symbol_table*)sym_table)->capacity > 0);
    if (!generic_symbol_table_add_internal(sym_table->table_tasts, sym_table->capacity, key, item)) {
        return false;
    }
    Ir* dummy;
    (void) dummy;
    assert(generic_tbl_lookup((void**)&dummy, sym_table, key));
    sym_table->count++;
    return true;
}

bool generic_symbol_add(
    Strv key,
    void* item,
    Get_tbl_from_collection_fn get_tbl_from_collection_fn,
    Scope_id scope_id
) {
    void* dummy;
    if (generic_symbol_lookup((void**)&dummy, key, get_tbl_from_collection_fn, scope_id)) {
        return false;
    }
    Symbol_collection* curr_tast = vec_at_ref(&env.symbol_tables, scope_id);
    unwrap(generic_tbl_add((Generic_symbol_table*)get_tbl_from_collection_fn(curr_tast), key, item));
    return true;
}

void generic_tbl_update(Generic_symbol_table* sym_table, Strv key, void* item) {
    Generic_symbol_table_tast* sym_tast;
    if (generic_tbl_lookup_internal(&sym_tast, sym_table, key)) {
        sym_tast->tast = item;
        return;
    }
    unwrap(generic_tbl_add(sym_table, key, item));
}

void generic_symbol_update(Strv key, void* item, Get_tbl_from_collection_fn get_tbl_from_collection_fn, Scope_id scope_id) {
    if (generic_symbol_add(key, item, get_tbl_from_collection_fn, scope_id)) {
        return;
    }

    Scope_id curr_scope = scope_id;
    while (true) {
        void* tbl = get_tbl_from_collection_fn(vec_at_ref(&env.symbol_tables, curr_scope));
        Generic_symbol_table_tast* curr_tast = NULL;
        if (generic_tbl_lookup_internal(&curr_tast, tbl, key)) {
             curr_tast->tast = item;
             return;
        }
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
    }
    unreachable("if there was no matching symbol found, generic_symbol_add should have worked");
}

bool generic_tbl_lookup(void** result, const Generic_symbol_table* sym_table, Strv key) {
    Generic_symbol_table_tast* sym_tast;
    if (!generic_tbl_lookup_internal(&sym_tast, sym_table, key)) {
        return false;
    }
    *result = sym_tast->tast;
    return true;
}

bool generic_symbol_lookup(
    void** result,
    Strv key,
    Get_tbl_from_collection_fn get_tbl_from_collection_fn,
    Scope_id scope_id
) {
    if (scope_id == SCOPE_NOT) {
        return false;
    }

    Scope_id curr_scope = scope_id;
    while (true) {
        void* tbl = get_tbl_from_collection_fn(vec_at_ref(&env.symbol_tables, curr_scope));
        if (generic_tbl_lookup(result, tbl, key)) {
             return true;
        }
        if (curr_scope == 0) {
            break;
        }
        curr_scope = scope_get_parent_tbl_lookup(curr_scope);
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
        (Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, scope_id)->symbol_table,
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
    generic_tbl_update((Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, scope_id)->symbol_table, serialize_name_symbol_table(tast_def_get_name(item)), item);
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
            string_extend_strv(&a_print, buf, tast_def_print_internal(sym_tast->tast, recursion_depth));
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
        (Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, key.scope_id)->symbol_table,
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
        (Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, name.scope_id)->usymbol_table,
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
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, key.scope_id)->usymbol_table, serialize_name_symbol_table(key));
}

bool usymbol_lookup(Uast_def** result, Name key) {
    if (key.mod_path.count < 1) {
        Name prim_key = key;
        prim_key.scope_id = 0;
        if (usym_tbl_lookup(result, prim_key)) {
            return true;
        }
        if (lang_type_atom_is_signed(lang_type_atom_new(prim_key, 0))) {
            int32_t bit_width = strv_to_int64_t(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        } else if (lang_type_atom_is_unsigned(lang_type_atom_new(prim_key, 0))) {
            int32_t bit_width = strv_to_int64_t(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new((Pos) {0}, bit_width, 0)))
            );
            unwrap(usym_tbl_add(uast_primitive_def_wrap(def)));
            *result = uast_primitive_def_wrap(def);
            return true;
        } else if (lang_type_atom_is_float(lang_type_atom_new(prim_key, 0))) {
            int32_t bit_width = strv_to_int64_t(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
            Uast_primitive_def* def = uast_primitive_def_new(
                POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_float_const_wrap(lang_type_float_new((Pos) {0}, bit_width, 0)))
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
// Ir implementation
//

// returns false if symbol has already been added to the table
bool ir_tbl_add_ex(Ir_table* tbl, Ir* item) {
    Name name = ir_tast_get_name(item);
    return generic_tbl_add((Generic_symbol_table*)tbl, serialize_name_symbol_table(name), item);
}

// returns false if symbol has already been added to the table
bool ir_tbl_add(Ir* item) {
    Name name = ir_tast_get_name(item);
    return ir_tbl_add_ex(&vec_at_ref(&env.symbol_tables, name.scope_id)->alloca_table, item);
}

void* ir_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->alloca_table;
}

bool ir_add(Ir* item) {
    Name name = ir_tast_get_name(item);
    return generic_symbol_add(
        serialize_name_symbol_table(name),
        item,
        (Get_tbl_from_collection_fn)ir_get_tbl_from_collection,
        name.scope_id
    );
}

void ir_tbl_update(Ir* item) {
    Name name = ir_tast_get_name(item);
    generic_tbl_update((Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, name.scope_id)->usymbol_table, serialize_name_symbol_table(name), item);
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

void ir_update(Ir* item) {
    (void) item;
    todo();
    //generic_symbol_update(serialize_name_symbol_table(ir_tast_get_name(item)), item, (Get_tbl_from_collection_fn)ir_get_tbl_from_collection);
}

bool ir_tbl_lookup(Ir** result, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&vec_at_ref(&env.symbol_tables, key.scope_id)->usymbol_table, serialize_name_symbol_table(key));
}

bool ir_lookup(Ir** result, Name key) {
    return generic_symbol_lookup(
        (void**)result,
        serialize_name_symbol_table(key),
        (Get_tbl_from_collection_fn)ir_get_tbl_from_collection,
        key.scope_id
    );
}

//
// File_path_to_text implementation
//

static File_path_to_text file_path_to_text;

bool file_path_to_text_tbl_lookup(Strv** result, Strv key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&file_path_to_text, key);
}

// returns false if file_path_to_text has already been added to the table
bool file_path_to_text_tbl_add(Strv* file_text, Strv key) {
    return generic_tbl_add((Generic_symbol_table*)&file_path_to_text, key, file_text);
}

//
// C_forward_struct_tbl implementation
//

// this is used to define additional structs to get around the requirement of in order definitions in c
static C_forward_struct_tbl c_forward_struct_tbl;

bool c_forward_struct_tbl_lookup(Name** result, Name key) {
    return generic_tbl_lookup((void**)result, (Generic_symbol_table*)&c_forward_struct_tbl, serialize_name_symbol_table(key));
}

// returns false if value has already been added to the table
bool c_forward_struct_tbl_add(Name* value, Name key) {
    return generic_tbl_add((Generic_symbol_table*)&c_forward_struct_tbl, serialize_name_symbol_table(key), value);
}

//
// Function_decl_tbl implementation
//

bool function_decl_tbl_add(Uast_function_decl* decl) {
    return generic_tbl_add((Generic_symbol_table*)&env.function_decl_tbl, serialize_name_symbol_table(decl->name), decl);
}

bool function_decl_tbl_lookup(Uast_function_decl** decl, Name key) {
    return generic_tbl_lookup((void**)decl, (Generic_symbol_table*)&env.function_decl_tbl, serialize_name_symbol_table(key));
}

//
// Struct_like_tbl implementation
//

bool struct_like_tbl_add(Uast_def* def) {
    if (def->type == UAST_IMPORT_PATH) {
        todo();
    }
    return generic_tbl_add((Generic_symbol_table*)&env.struct_like_tbl, serialize_name_symbol_table(uast_def_get_name(def)), def);
}

bool struct_like_tbl_lookup(Uast_def** def, Name key) {
    return generic_tbl_lookup((void**)def, (Generic_symbol_table*)&env.struct_like_tbl, serialize_name_symbol_table(key));
}

//
// Raw_union_of_enum implementation
//

static Raw_union_of_enum raw_union_of_enum;

bool raw_union_of_enum_add(Tast_raw_union_def* def, Name enum_name) {
    return generic_tbl_add((Generic_symbol_table*)&raw_union_of_enum, serialize_name_symbol_table(enum_name), def);
}

bool raw_union_of_enum_lookup(Tast_raw_union_def** def, Name enum_name) {
    return generic_tbl_lookup((void**)def, (Generic_symbol_table*)&raw_union_of_enum, serialize_name_symbol_table(enum_name));
}

//
// Struct_to_struct implementation
//

static Struct_to_struct struct_to_struct;

bool struct_to_struct_add(Tast_struct_def* def, Name enum_name) {
    return generic_tbl_add((Generic_symbol_table*)&struct_to_struct, serialize_name_symbol_table(enum_name), def);
}

bool struct_to_struct_lookup(Tast_struct_def** def, Name enum_name) {
    return generic_tbl_lookup((void**)def, (Generic_symbol_table*)&struct_to_struct, serialize_name_symbol_table(enum_name));
}

//
// Scope_id_to_next_table implementation
//

static Scope_id_vec scope_id_to_parent;

// returns parent of key
Scope_id scope_get_parent_tbl_lookup(Scope_id key) {
    return vec_at(&scope_id_to_parent, key);
}

void scope_get_parent_tbl_add(Scope_id key, Scope_id parent) {
    while (scope_id_to_parent.info.count <= key) {
        vec_append(&a_main, &scope_id_to_parent, 0);
    }
    *vec_at_ref(&scope_id_to_parent, key) = parent;
}

void scope_get_parent_tbl_update(Scope_id key, Scope_id parent) {
    *vec_at_ref(&scope_id_to_parent, key) = parent;
}

//
// not generic
//

void usymbol_extend_table_internal(String* buf, const Usymbol_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Usymbol_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            string_extend_strv(&a_print, buf, uast_def_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

void alloca_extend_table_internal(String* buf, const Ir_table sym_table, int recursion_depth) {
    for (size_t idx = 0; idx < sym_table.capacity; idx++) {
        Ir_table_tast* sym_tast = &sym_table.table_tasts[idx];
        if (sym_tast->status == SYM_TBL_OCCUPIED) {
            //log(LOG_DEBUG, FMT"\n", string_print(*buf));
            string_extend_strv(&a_print, buf, ir_print_internal(sym_tast->tast, recursion_depth));
        }
    }
}

Scope_id symbol_collection_new(Scope_id parent) {
    Scope_id new_scope = env.symbol_tables.info.count;
    vec_append(&a_main, &env.symbol_tables, (Symbol_collection) {0});

    scope_get_parent_tbl_add(new_scope, parent);
    return new_scope;
}

