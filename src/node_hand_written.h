#ifndef NODE_HAND_WRITTEN_H
#define NODE_HAND_WRITTEN_H

#include <vecs.h>
#include <node_var_def_vec.h>
#include <symbol_table_struct.h>
#include <node_forward_decl.h>
#include <lang_type_struct.h>

typedef size_t Llvm_id;

#define LLVM_REGISTER_SYM_FMT LANG_TYPE_FMT"    "NODE_FMT

#define NODE_FMT STR_VIEW_FMT

#define llvm_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), node_print((reg_sym).node)

typedef struct {
    Node_vec members;
    Llvm_id llvm_id;
    Str_view name;
} Struct_def_base;

typedef struct {
    Lang_type lang_type;
    Str_view name;
    Llvm_id llvm_id;
} Sym_typed_base;

struct Node_expr_;
typedef struct Node_expr_ Node_expr;

#endif // NODE_HAND_WRITTEN_H
