#ifndef IR_UTIL_H
#define IR_UTIL_H

#include <ir.h>

#ifdef NDEBUG
#   define ir_get_loc(node) ((Loc) {.file = "", .line = 0})
#else
#   define ir_get_loc(node) ((node)->loc)
#endif // NDEBUG

#define lang_type_darr_print(types) strv_print(lang_type_darr_print_internal((types), false))

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

Name ir_def_get_name(LANG_TYPE_MODE mode, const Ir_def* def);

Name ir_ir_expr_get_name(const Ir_expr* expr);

Name ir_get_name(LANG_TYPE_MODE mode, const Ir* ir);

Ir_lang_type* ir_literal_ref_get_lang_type(Ir_literal* lit);

Ir_lang_type lang_type_from_ir_name(Name name);

size_t struct_def_get_idx_matching_member(Ir_struct_def* base, Name memb_name);

static inline bool ir_is_label(const Ir* ir) {
    return ir->type == IR_DEF && ir_def_const_unwrap(ir)->type == IR_LABEL;
}

Ir* ir_from_name(Name name);

#endif // IR_UTIL_H
