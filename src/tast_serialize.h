#ifndef TAST_SERIALIZE_H
#define TAST_SERIALIZE_H

#include <lang_type_serialize.h>

static inline Name serialize_tast_struct_def(const Tast_struct_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "struct def must be in symbol table to be serialized");
    return name_new(env.curr_mod_path, serialize_lang_type(tast_struct_def_get_lang_type(def)), (Ulang_type_vec) {0} /* TODO */, def->base.name.scope_id);
}

static inline Name serialize_tast_raw_union_def(const Tast_raw_union_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "raw_union def must be in symbol table to be serialized");
    return name_new(env.curr_mod_path, serialize_lang_type( tast_raw_union_def_get_lang_type(def)), (Ulang_type_vec) {0} /* TODO */, def->base.name.scope_id);
}

static inline Name serialize_tast_sum_def(const Tast_sum_def* def) {
    Tast_def* dummy = NULL;
    assert(symbol_lookup(&dummy,  def->base.name) && "sum def must be in symbol table to be serialized");
    return name_new(env.curr_mod_path, serialize_lang_type( tast_sum_def_get_lang_type(def)), (Ulang_type_vec) {0} /* TODO */, def->base.name.scope_id);
}

Str_view serialize_tast_def(const Tast_def* def);

#endif // TAST_SERIALIZE_H
