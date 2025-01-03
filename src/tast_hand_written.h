#ifndef TAST_HAND_WRITTEN_H
#define TAST_HAND_WRITTEN_H

#include <vecs.h>
#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <lang_type_struct.h>

typedef size_t Llvm_id;

#define LLVM_REGISTER_SYM_FMT LANG_TYPE_FMT"    "TAST_FMT

#define TAST_FMT STR_VIEW_FMT

#define llvm_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), tast_print((reg_sym).tast)

typedef struct {
    Tast_variable_def_vec members;
    Llvm_id llvm_id;
    Str_view name;
} Struct_def_base;

typedef struct {
    Lang_type lang_type;
    Str_view name;
    Llvm_id llvm_id;
} Sym_typed_base;

struct Tast_expr_;
typedef struct Tast_expr_ Tast_expr;

#endif // TAST_HAND_WRITTEN_H
