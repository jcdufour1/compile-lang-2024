#ifndef TAST_HAND_WRITTEN_H
#define TAST_HAND_WRITTEN_H

#include <vecs.h>
#include <symbol_table_struct.h>
#include <tast_forward_decl.h>
#include <lang_type.h>

#define ir_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), tast_print((reg_sym).tast)

typedef struct {
    Tast_variable_def_vec members;
    Name name;
} Struct_def_base;

typedef struct {
    Ir_struct_memb_def_vec members;
    Ir_name name;
} Ir_struct_def_base;

typedef struct {
    Lang_type lang_type;
    Name name;
} Sym_typed_base;

typedef struct {
    Ir_lang_type lang_type;
    Ir_name name;
} Llvm_sym_typed_base;

struct Tast_expr_;
typedef struct Tast_expr_ Tast_expr;

#endif // TAST_HAND_WRITTEN_H
