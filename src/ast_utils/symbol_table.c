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

bool scope_id_is_top_level(Scope_id scope) {
    return scope_get_parent_tbl_lookup(scope) == SCOPE_BUILTIN;
}

Scope_id symbol_collection_new(Scope_id parent, Name scope_name) {
    Scope_id new_scope = scope_to_name.info.count;

    darr_append(&a_main, &symbol_tables.ir_table, (Hash_table_stable_ir) {0});
    darr_append(&a_main, &symbol_tables.usymbol_table, (Hash_table_stable_uast) {0});
    darr_append(&a_main, &symbol_tables.symbol_table, (Hash_table_stable_tast) {0});

    scope_get_parent_tbl_add(new_scope, parent);
    scope_to_name_tbl_add(new_scope, scope_name);
    assert(scope_to_name.info.count == scope_id_to_parent.info.count);
    return new_scope;
}

bool ir_add(Ir* def_to_add) {
    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_add);
    return hash_table_scoped_add_ir(&symbol_tables.ir_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);
}

bool usymbol_add(Uast_def* def_to_add) {
    Name key = uast_def_get_name(def_to_add);
    return hash_table_scoped_add_uast(&symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);
}

void usymbol_update(Uast_def* def_to_update) {
    Name key = uast_def_get_name(def_to_update);
    hash_table_scoped_update_uast(&symbol_tables.usymbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update, key.scope_id);
}

bool symbol_add(Tast_def* def_to_add) {
    Name key = tast_def_get_name(def_to_add);
    return hash_table_scoped_add_tast(&symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);
}

void symbol_update(Tast_def* def_to_add) {
    Name key = tast_def_get_name(def_to_add);
    hash_table_scoped_update_tast(&symbol_tables.symbol_table, serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add, key.scope_id);
}

bool usym_tbl_add(Uast_def* def_to_add) {
    Name key = uast_def_get_name(def_to_add);
    return hash_table_stable_add_uast(darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);
}

bool sym_tbl_add(Tast_def* def_to_add) {
    Name key = tast_def_get_name(def_to_add);
    return hash_table_stable_add_tast(darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);
}

bool ir_tbl_add(Ir* def_to_add) {
    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_add);
    return hash_table_stable_add_ir(darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_add);
}

bool usym_tbl_lookup(Uast_def** result, Name key) {
    return hash_table_stable_lookup_uast(result, darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));
}

bool sym_tbl_lookup(Tast_def** result, Name key) {
    return hash_table_stable_lookup_tast(result, darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));
}

bool ir_tbl_lookup(Ir** result, Name key) {
    return hash_table_stable_lookup_ir(result, darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key));
}


void ir_tbl_update(Ir* def_to_update) {
    Name key = ir_get_name(LANG_TYPE_MODE_LOG, def_to_update);
    hash_table_stable_update_ir(darr_at_ref(&symbol_tables.ir_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);
}

void usym_tbl_update(Uast_def* def_to_update) {
    Name key = uast_def_get_name(def_to_update);
    hash_table_stable_update_uast(darr_at_ref(&symbol_tables.usymbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);
}

void sym_tbl_update(Tast_def* def_to_update) {
    Name key = tast_def_get_name(def_to_update);
    hash_table_stable_update_tast(darr_at_ref(&symbol_tables.symbol_table, key.scope_id), serialize_name_symbol_table(&a_leak/*TODO*/, key), def_to_update);
}

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

static Strv symbol_table_uast_print_internal(const Uast_def* item, Indent indent) {
    return uast_def_print_internal(UAST_LOG, item, indent);
}

static Strv symbol_table_tast_print_internal(const Tast_def* item, Indent indent) {
    return tast_def_print_internal(item, indent);
}

static Strv symbol_table_ir_print_internal(const Ir* item, Indent indent) {
    return ir_print_internal(item, indent);
}

void usymbol_log(LOG_LEVEL log_level, Scope_id scope_id) {
    log(log_level, FMT"\n", hash_table_scoped_print_uast(&symbol_tables.usymbol_table, scope_id, symbol_table_uast_print_internal));
}

void symbol_log(LOG_LEVEL log_level, Scope_id scope_id) {
    log(log_level, FMT"\n", hash_table_scoped_print_tast(&symbol_tables.symbol_table, scope_id, symbol_table_tast_print_internal));
}

void ir_log(LOG_LEVEL log_level, Scope_id scope_id) {
    log(log_level, FMT"\n", hash_table_scoped_print_ir(&symbol_tables.ir_table, scope_id, symbol_table_ir_print_internal));
}
