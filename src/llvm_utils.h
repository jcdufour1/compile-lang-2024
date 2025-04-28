#ifndef LLVM_UTIL_H
#define LLVM_UTIL_H

#include <llvm.h>

#define LANG_TYPE_FMT STR_VIEW_FMT

Str_view lang_type_vec_print_internal(Lang_type_vec types);

#define lang_type_vec_print(types) str_view_print(lang_type_vec_print_internal((types), false))

#define LLVM_FMT STR_VIEW_FMT

#define llvm_printf(llvm) \
    do { \
        log(LOG_NOTE, LLVM_FMT"\n", llvm_print(llvm)); \
    } while (0);

Lang_type llvm_operator_get_lang_type(const Llvm_operator* operator);

Lang_type* llvm_get_operator_lang_type_ref(Llvm_operator* operator);

Lang_type llvm_literal_get_lang_type(const Llvm_literal* lit);

Lang_type llvm_expr_get_lang_type(const Llvm_expr* expr);

Lang_type llvm_def_get_lang_type(const Llvm_def* def);

Lang_type* llvm_expr_ref_get_lang_type(Llvm_expr* expr);

Lang_type llvm_get_lang_type(const Llvm* llvm);

Lang_type* llvm_def_ref_get_lang_type(Llvm_def* def);

Lang_type* llvm_ref_get_lang_type(Llvm* llvm);

Llvm* llvm_get_expr_src(Llvm_expr* expr);

Llvm* get_llvm_src(Llvm* llvm);

Llvm* llvm_get_expr_dest(Llvm_expr* expr);

Llvm* get_llvm_dest(Llvm* llvm);

Name llvm_literal_get_name(const Llvm_literal* lit);

Name llvm_operator_get_name(const Llvm_operator* operator);

Name llvm_expr_get_name(const Llvm_expr* expr);

Name llvm_literal_def_get_name(const Llvm_literal_def* lit_def);

Name llvm_def_get_name(const Llvm_def* def);

Name llvm_llvm_expr_get_name(const Llvm_expr* expr);

Name llvm_tast_get_name(const Llvm* llvm);

const Llvm* get_llvm_src_const(const Llvm* llvm);

const Llvm* get_llvm_dest_const(const Llvm* llvm);

Lang_type* llvm_literal_ref_get_lang_type(Llvm_literal* lit);

Lang_type lang_type_from_get_name(Name name);

#endif // LLVM_UTIL_H
