#ifndef UAST_HAND_WRITTEN_H
#define UAST_HAND_WRITTEN_H

#include <vecs.h>
#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <lang_type_struct.h>

typedef size_t Llvm_id;

#define UAST_FMT STR_VIEW_FMT

#define llvm_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), tast_print((reg_sym).tast)

typedef struct {
    Uast_stmt_vec members;
    Str_view name;
} Ustruct_def_base;

#endif // UAST_HAND_WRITTEN_H
