#ifndef IR_UTIL_H
#define IR_UTIL_H

#include <ir.h>

Strv lang_type_vec_print_internal(Ir_lang_type_vec types);

#define lang_type_vec_print(types) strv_print(lang_type_vec_print_internal((types), false))

#define ir_printf(ir) \
    do { \
        log(LOG_NOTE, FMT"\n", ir_print(ir)); \
    } while (0);

Ir_lang_type ir_operator_get_lang_type(const Ir_operator* operator);

Ir_lang_type* ir_get_operator_lang_type_ref(Ir_operator* operator);

Ir_lang_type ir_literal_get_lang_type(const Ir_literal* lit);

Ir_lang_type ir_expr_get_lang_type(const Ir_expr* expr);

Ir_lang_type ir_def_get_lang_type(const Ir_def* def);

Ir_lang_type* ir_expr_ref_get_lang_type(Ir_expr* expr);

Ir_lang_type ir_get_lang_type(const Ir* ir);

Ir_lang_type* ir_def_ref_get_lang_type(Ir_def* def);

Ir_lang_type* ir_ref_get_lang_type(Ir* ir);

Ir* ir_get_expr_src(Ir_expr* expr);

Ir* get_ir_src(Ir* ir);

Ir* ir_get_expr_dest(Ir_expr* expr);

Ir* get_ir_dest(Ir* ir);

Name ir_literal_get_name(const Ir_literal* lit);

Name ir_operator_get_name(const Ir_operator* operator);

Name ir_expr_get_name(const Ir_expr* expr);

Name ir_literal_def_get_name(const Ir_literal_def* lit_def);

Name ir_def_get_name(const Ir_def* def);

Name ir_ir_expr_get_name(const Ir_expr* expr);

Ir_name ir_tast_get_name(const Ir* ir);

const Ir* get_ir_src_const(const Ir* ir);

const Ir* get_ir_dest_const(const Ir* ir);

Ir_lang_type* ir_literal_ref_get_lang_type(Ir_literal* lit);

Ir_lang_type lang_type_from_get_name(Ir_name name);

Ir* ir_from_get_name(Ir_name name);

size_t struct_def_get_idx_matching_member(Ir_struct_def* base, Name memb_name);

static inline bool ir_is_label(const Ir* ir) {
    return ir->type == IR_DEF && ir_def_const_unwrap(ir)->type == IR_LABEL;
}

#endif // IR_UTIL_H
