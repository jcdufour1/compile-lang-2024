#include <offsetof.h>

// TODO: move to sizeof.c?
// TODO: document include_curr_memb parameter?
Bytes offsetof_ir_lang_type_struct(Ir_lang_type_struct base, size_t memb_idx, bool include_curr_memb) {
    Ir* def_ = NULL;
    unwrap(ir_lookup(&def_, base.name));
    Ir_struct_def* def = ir_struct_def_unwrap(ir_def_unwrap(def_));
    Bytes base_size = sizeof_ir_struct_def_base_internal(&def->base, memb_idx);
    if (include_curr_memb) {
        base_size = bytes_add(base_size, sizeof_ir_lang_type(darr_at(def->base.members, memb_idx)->lang_type));
    }
    return base_size;
}
