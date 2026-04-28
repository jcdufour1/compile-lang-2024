#include <offsetof.h>

uint64_t offsetof_ir_lang_type_struct(Ir_lang_type_struct base, size_t memb_idx) {
    Ir* def_ = NULL;
    unwrap(ir_lookup(&def_, base.name));
    Ir_struct_def* def = ir_struct_def_unwrap(ir_def_unwrap(def_));
    return sizeof_ir_struct_def_base_internal(&def->base, memb_idx);
}
