#ifndef SYMBOL_ITER_H
#define SYMBOL_ITER_H

#include <util.h>
#include <symbol_table.h>

typedef struct {
    size_t bucket_idx;
    Symbol_table tbl;
} Symbol_iter;

static inline Symbol_iter sym_tbl_iter_new_table(Symbol_table tbl) {
    return (Symbol_iter) {.bucket_idx = 0, .tbl = tbl};
}

static inline Symbol_iter sym_tbl_iter_new(Scope_id scope_id) {
    return sym_tbl_iter_new_table(darr_at_ref(&env.symbol_tables, scope_id)->symbol_table);
}

static inline bool sym_tbl_iter_next(Tast_def** result, Symbol_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

typedef struct {
    size_t bucket_idx;
    Usymbol_table tbl;
} Usymbol_iter;

static inline Usymbol_iter usym_tbl_iter_new_table(Usymbol_table tbl) {
    return (Usymbol_iter) {.bucket_idx = 0, .tbl = tbl};
}

static inline Usymbol_iter usym_tbl_iter_new(Scope_id scope_id) {
    return usym_tbl_iter_new_table(darr_at_ref(&env.symbol_tables, scope_id)->usymbol_table);
}


static inline bool usym_tbl_iter_next(Uast_def** result, Usymbol_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

typedef struct {
    size_t bucket_idx;
    Ir_table tbl;
} Ir_iter;

// TODO: rename Ir_table to Ir_table, etc.
static inline Ir_iter ir_tbl_iter_new(Scope_id scope_id) {
    return (Ir_iter) {.bucket_idx = 0, .tbl = darr_at_ref(&env.symbol_tables, scope_id)->ir_table};
}

static inline bool ir_tbl_iter_next(Ir** result, Ir_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}

typedef struct {
    size_t bucket_idx;
    Init_table tbl;
} Init_table_iter;

static inline Init_table_iter init_tbl_iter_new_table(Init_table tbl) {
    return (Init_table_iter) {.bucket_idx = 0, .tbl = tbl};
}

static inline bool init_tbl_iter_next(Init_table_node* result, Init_table_iter* iter) {
    bool was_found = false;
    while (!was_found && iter->bucket_idx < iter->tbl.capacity) {
        if (iter->tbl.table_tasts[iter->bucket_idx].status == SYM_TBL_OCCUPIED) {
            *result = *iter->tbl.table_tasts[iter->bucket_idx].tast;
            was_found = true;
        }
        iter->bucket_idx++;
    }

    return was_found;
}
#endif // SYMBOL_ITER_H
