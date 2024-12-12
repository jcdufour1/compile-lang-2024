#ifndef NODE_HAND_WRITTEN_H
#define NODE_HAND_WRITTEN_H

#include <node_ptr_vec.h>
#include <node_if_ptr_vec.h>
#include <symbol_table_struct.h>

typedef size_t Llvm_id;

typedef struct {
    Str_view str;
    int16_t pointer_depth; // for function parameter: 2 means that function argument must also have 2 for this field
                           // and that in function, variable is already referenced twice
                           //
                           // for function argument: 2 means to reference the variable twice
} Lang_type;

typedef struct {
    Lang_type lang_type;
    struct Node_* node;
} Llvm_register_sym;

#define LLVM_REGISTER_SYM_FMT LANG_TYPE_FMT"    "NODE_FMT

#define llvm_register_sym_print(reg_sym) \
    lang_type_print((reg_sym).lang_type), node_print((reg_sym).node)

typedef struct {
    Node_ptr_vec members;
    Llvm_id llvm_id;
    Str_view name;
} Struct_def_base;

typedef struct {
    Lang_type lang_type;
    Str_view name;
    Llvm_id llvm_id;
} Sym_typed_base;

#endif // NODE_HAND_WRITTEN_H
