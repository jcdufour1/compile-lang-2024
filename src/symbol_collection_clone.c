#include <symbol_collection_clone.h>
#include <uast_clone.h>

Usymbol_table usymbol_table_clone(Usymbol_table tbl) {
    Usymbol_table new_tbl = {0};
    if (tbl.count < 1) {
        todo();
        return new_tbl;
    }

    for (size_t idx = 0; idx < tbl.capacity; idx++) {
        if (tbl.table_tasts[idx].status != SYM_TBL_OCCUPIED) {
            continue;
        }

        usym_tbl_add(&new_tbl, uast_def_clone(tbl.table_tasts[idx].tast));
        assert(new_tbl.table_tasts[idx].tast != tbl.table_tasts[idx].tast);
    }

    return new_tbl;
}

Symbol_table symbol_table_clone(Symbol_table tbl) {
    if (tbl.count < 1) {
        return (Symbol_table) {0};
    }
    todo();
}

Alloca_table alloca_table_clone(Alloca_table tbl) {
    if (tbl.count < 1) {
        return (Alloca_table) {0};
    }
    todo();
}

Symbol_collection symbol_collection_clone(Symbol_collection coll) {
    return (Symbol_collection) {
        .usymbol_table = usymbol_table_clone(coll.usymbol_table),
        .symbol_table = symbol_table_clone(coll.symbol_table),
        .alloca_table = alloca_table_clone(coll.alloca_table)
    };
}
