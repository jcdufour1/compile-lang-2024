#include <util.h>
WIMPLICIT_FALLTHROUGH_IGNORE_START
WSIGN_CONVERSION_IGNORE_START
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>
WIMPLICIT_FALLTHROUGH_IGNORE_END
WSIGN_CONVERSION_IGNORE_END

#include "symbol_table.h"
#include "symbol_table_struct.h"
#include <uast_utils.h>
#include <tast_utils.h>
#include <ir_utils.h>
#include <symbol_log.h>

#define SYM_TBL_DEFAULT_CAPACITY 1
#define SYM_TBL_MAX_DENSITY (0.6f) // TODO: change this to an integer

//
// util
//
static size_t sym_tbl_calculate_idx(Strv key, size_t capacity) {
    unwrap(capacity > 0);
    return stbds_hash_bytes(key.str, key.count, 0)%capacity;
}

//
// generics
//

typedef bool(*Symbol_add_fn)(void* tast_to_add);

typedef void*(*Get_tbl_from_collection_fn)(Symbol_collection* collection);

bool generic_symbol_lookup(void** result, Strv key, Get_tbl_from_collection_fn get_tbl_from_collection_fn, Scope_id scope_id);

static Strv sym_tbl_status_print_internal(SYM_TBL_STATUS status) {
    switch (status) {
        case SYM_TBL_NEVER_OCCUPIED:
            return sv("never_occupied");
        case SYM_TBL_PREVIOUSLY_OCCUPIED:
            return sv("previously_occupied");
        case SYM_TBL_OCCUPIED:
            return sv("occupied");
        default:
            unreachable("");
    }
    unreachable("");
}

#define sym_tbl_status_print(status) \
    strv_print(sym_tbl_status_print_internal(status))

// return false if symbol is not found
bool generic_tbl_lookup_internal(Generic_symbol_table_tast** result, const void* sym_table, Strv query) {
    const Generic_symbol_table* sym_tbl = sym_table;

    if (sym_tbl->capacity < 1) {
        return false;
    }
    size_t curr_table_idx = sym_tbl_calculate_idx(query, sym_tbl->capacity);
    size_t init_table_idx = curr_table_idx; 

    while (1) {
        Generic_symbol_table_tast* curr_tast = &((Generic_symbol_table_tast*)(sym_tbl->table_tasts))[curr_table_idx];

        if (curr_tast->status == SYM_TBL_OCCUPIED) {
            if (strv_is_equal(curr_tast->key, query)) {
                *result = curr_tast;
                return true;
            }
        }

        if (curr_tast->status == SYM_TBL_NEVER_OCCUPIED) {
            return false;
        }

        curr_table_idx = (curr_table_idx + 1) % sym_tbl->capacity;
        if (curr_table_idx == init_table_idx) {
            return false;
        }
    }

    unreachable("");
}

//
// Uast_def implementation
//

void* sym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->symbol_table;
}

void* expand_get_tbl_from_collection(Symbol_collection* collection) {
    todo();
    //return &collection->expand_again_table;
}

//
// usymbol implementation
//

void* usym_get_tbl_from_collection(Symbol_collection* collection) {
    return &collection->usymbol_table;
}

//
// Ir implementation
//

//
// C_forward_struct_tbl implementation
//

//
// Scope_id_to_next_table implementation
//

//void scope_get_parent_tbl_add(Scope_id key, Scope_id parent) {
//    while (scope_id_to_parent.info.count <= key) {
//        darr_append(&a_main, &scope_id_to_parent, 0);
//    }
//    *darr_at_ref(&scope_id_to_parent, key) = parent;
//}
//
//void scope_get_parent_tbl_update(Scope_id key, Scope_id parent) {
//    *darr_at_ref(&scope_id_to_parent, key) = parent;
//}
//
//void scope_id_to_parent_dump(LOG_LEVEL log_level) {
//    String buf = {0};
//
//    darr_foreach(idx, Scope_id, scope, scope_id_to_parent) {
//        string_extend_f(&a_main, &buf, "%zu, ", scope);
//    }
//
//    log(log_level, FMT"\n", string_print(buf));
//}

bool scope_id_is_top_level(Scope_id scope) {
    return scope_get_parent_tbl_lookup(scope) == SCOPE_BUILTIN;
}

//
// Scope_id_to_name implementation
//

//
// not generic
//

Scope_id symbol_collection_new(Scope_id parent, Name scope_name) {
    Scope_id new_scope = scope_to_name.info.count;
    log(LOG_DEBUG, "%zu\n", new_scope);

    darr_append(&a_main, &symbol_tables.ir_table, (Hash_table_stable_ir) {0});
    darr_append(&a_main, &symbol_tables.usymbol_table, (Hash_table_stable_uast) {0});
    darr_append(&a_main, &symbol_tables.symbol_table, (Hash_table_stable_tast) {0});

    scope_get_parent_tbl_add(new_scope, parent);
    scope_to_name_tbl_add(new_scope, scope_name);
    assert(scope_to_name.info.count == scope_id_to_parent.info.count);
    return new_scope;
}

    //if (lang_type_name_base_is_signed(prim_key.base)) {
    //    if (usym_tbl_lookup(result, prim_key)) {
    //        return true;
    //    }
    //    Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
    //    Uast_primitive_def* def = uast_primitive_def_new(
    //        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, bit_width, 0)))
    //    );
    //    usym_tbl_add(uast_primitive_def_wrap(def));
    //    *result = uast_primitive_def_wrap(def);
    //    return true;
    //} else if (lang_type_name_base_is_unsigned(prim_key.base)) {
    //    if (usym_tbl_lookup(result, prim_key)) {
    //        return true;
    //    }
    //    Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
    //    Uast_primitive_def* def = uast_primitive_def_new(
    //        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)))
    //    );
    //    usym_tbl_add(uast_primitive_def_wrap(def));
    //    *result = uast_primitive_def_wrap(def);
    //    return true;
    //} else if (lang_type_name_base_is_float(prim_key.base)) {
    //    if (usym_tbl_lookup(result, prim_key)) {
    //        return true;
    //    }
    //    Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(prim_key.base, 1, prim_key.base.count - 1));
    //    Uast_primitive_def* def = uast_primitive_def_new(
    //        POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_float_const_wrap(lang_type_float_new(POS_BUILTIN, bit_width, 0)))
    //    );
    //    usym_tbl_add(uast_primitive_def_wrap(def));
    //    *result = uast_primitive_def_wrap(def);
    //    return true;
    //} else if (strv_is_equal(prim_key.base, sv("opaque"))) {
    //    if (usym_tbl_lookup(result, prim_key)) {
    //        return true;
    //    }
    //    Uast_primitive_def* def = uast_primitive_def_new(
    //        POS_BUILTIN,
    //        lang_type_primitive_const_wrap(lang_type_opaque_const_wrap(lang_type_opaque_new(POS_BUILTIN, 0)))
    //    );
    //    usym_tbl_add(uast_primitive_def_wrap(def));
    //    *result = uast_primitive_def_wrap(def);
    //    return true;
    //} else if (strv_is_equal(prim_key.base, sv("void"))) {
    //    if (usym_tbl_lookup(result, prim_key)) {
    //        return true;
    //    }
    //    Uast_primitive_def* def = uast_primitive_def_new(
    //        POS_BUILTIN,
    //        lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN, 0))
    //    );
    //    usym_tbl_add(uast_primitive_def_wrap(def));
    //    *result = uast_primitive_def_wrap(def);
    //    return true;
    //}




bool usymbol_lookup(Uast_def** result, Name key) {
        // TODO: add static assertion for new primitive type being added?
            if (lang_type_name_base_is_signed(key.base)) {
                if (usym_tbl_lookup(result, key)) {
                    return true;
                }
                Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(key.base, 1, key.base.count - 1));
                Uast_primitive_def* def = uast_primitive_def_new(
                    POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_signed_int_const_wrap(lang_type_signed_int_new(POS_BUILTIN, bit_width, 0)))
                );
                usym_tbl_add(uast_primitive_def_wrap(def));
                *result = uast_primitive_def_wrap(def);
                return true;
            } else if (lang_type_name_base_is_unsigned(key.base)) {
                if (usym_tbl_lookup(result, key)) {
                    return true;
                }
                Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(key.base, 1, key.base.count - 1));
                Uast_primitive_def* def = uast_primitive_def_new(
                    POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_unsigned_int_const_wrap(lang_type_unsigned_int_new(POS_BUILTIN, bit_width, 0)))
                );
                usym_tbl_add(uast_primitive_def_wrap(def));
                *result = uast_primitive_def_wrap(def);
                return true;
            } else if (lang_type_name_base_is_float(key.base)) {
                if (usym_tbl_lookup(result, key)) {
                    return true;
                }
                Bits bit_width = bits_from_strv(POS_BUILTIN, strv_slice(key.base, 1, key.base.count - 1));
                Uast_primitive_def* def = uast_primitive_def_new(
                    POS_BUILTIN, lang_type_primitive_const_wrap(lang_type_float_const_wrap(lang_type_float_new(POS_BUILTIN, bit_width, 0)))
                );
                usym_tbl_add(uast_primitive_def_wrap(def));
                *result = uast_primitive_def_wrap(def);
                return true;
            } else if (strv_is_equal(key.base, sv("opaque"))) {
                if (usym_tbl_lookup(result, key)) {
                    return true;
                }
                Uast_primitive_def* def = uast_primitive_def_new(
                    POS_BUILTIN,
                    lang_type_primitive_const_wrap(lang_type_opaque_const_wrap(lang_type_opaque_new(POS_BUILTIN, 0)))
                );
                usym_tbl_add(uast_primitive_def_wrap(def));
                *result = uast_primitive_def_wrap(def);
                return true;
            } else if (strv_is_equal(key.base, sv("void"))) {
                if (usym_tbl_lookup(result, key)) {
                    return true;
                }
                Uast_primitive_def* def = uast_primitive_def_new(
                    POS_BUILTIN,
                    lang_type_void_const_wrap(lang_type_void_new(POS_BUILTIN, 0))
                );
                usym_tbl_add(uast_primitive_def_wrap(def));
                *result = uast_primitive_def_wrap(def);
                return true;
            } else {
                assert(!lang_type_name_base_is_primitive(key.base) && "add new else if here?");
            } 

            return hash_table_scoped_lookup_uast(result, &symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), key.scope_id);
        }

