#ifndef SYMBOL_COLLECTION_CLONE_H
#define SYMBOL_COLLECTION_CLONE_H

#include <env.h>
#include <util.h>
#include <uast.h>

void usymbol_table_clone(Scope_id old_scope, Scope_id new_scope);

void symbol_table_clone(Scope_id old_scope, Scope_id new_scope);

void alloca_table_clone(Scope_id old_scope, Scope_id new_scope);

Scope_id scope_id_clone(Scope_id old_scope, Scope_id new_parent);

#endif // SYMBOL_COLLECTION_CLONE_H
