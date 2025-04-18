#ifndef TAST_SERIALIZE_H
#define TAST_SERIALIZE_H

#include <lang_type_serialize.h>

static inline Name serialize_tast_struct_def(const Tast_struct_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "struct def must be in symbol table to be serialized");
    return (Name) {.mod_path = env.curr_mod_path, .base = serialize_lang_type( tast_struct_def_get_lang_type(def))};
}

static inline Name serialize_tast_raw_union_def(const Tast_raw_union_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "raw_union def must be in symbol table to be serialized");
    return (Name) {.mod_path = env.curr_mod_path, .base = serialize_lang_type( tast_raw_union_def_get_lang_type(def))};
}

static inline Name serialize_tast_sum_def(const Tast_sum_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "sum def must be in symbol table to be serialized");
    return (Name) {.mod_path = env.curr_mod_path, .base = serialize_lang_type( tast_sum_def_get_lang_type(def))};
}

Str_view serialize_tast_def(const Tast_def* def);

#endif // TAST_SERIALIZE_H
