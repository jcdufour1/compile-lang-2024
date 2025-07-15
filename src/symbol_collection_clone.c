#include <symbol_collection_clone.h>
#include <uast_clone.h>
#include <symbol_iter.h>
#include <symbol_log.h>

void usymbol_table_clone(Scope_id old_scope, Scope_id new_scope) {
    Usymbol_iter iter = usym_tbl_iter_new(old_scope);
    Uast_def* curr = NULL;
    
    while (usym_tbl_iter_next(&curr, &iter)) {
        Uast_def* new_def = uast_def_clone(curr, new_scope);
        unwrap(usym_tbl_add(new_def));
    }
}

void symbol_table_clone(Scope_id old_scope, Scope_id new_scope) {
    (void) new_scope;
    unwrap(vec_at_ref(&env.symbol_tables, old_scope)->symbol_table.count == 0 && "not implemented; possibly should never be");
}

void alloca_table_clone(Scope_id old_scope, Scope_id new_scope) {
    (void) new_scope;
    unwrap(vec_at_ref(&env.symbol_tables, old_scope)->alloca_table.count == 0 && "not implemented; possibly should never be");
}


Scope_id scope_id_clone(Scope_id old_scope, Scope_id new_parent) {
    Scope_id new_scope = symbol_collection_new(new_parent);
    old_block_scope_to_new_add(old_scope, new_scope);
    usymbol_table_clone(old_scope, new_scope);
    symbol_table_clone(old_scope, new_scope);
    alloca_table_clone(old_scope, new_scope);
    return new_scope;
}

